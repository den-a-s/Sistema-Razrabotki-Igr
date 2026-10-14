#include <engine/assets/AssetSystem.hpp>

#include <cassert>
#include <chrono>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include "AssetPipeline.hpp"

#include <engine/rendering/RenderingContext.hpp>
#include <engine/rendering/RenderingSubsystem.hpp>
#include <engine/assets/AssetEvents.hpp>
#include <engine/assets/AssetPathResolver.hpp>
#include <engine/core/FrameContext.hpp>
#include <engine/engine.hpp>

namespace engine {
namespace {

template <typename T>
bool IsSatisfied(const typename AssetTable<T>::Record& record) {
    if (record.state != AssetState::Ready) {
        return false;
    }
    if constexpr (std::is_same_v<T, ShaderAsset>) {
        switch (record.desc.shaderStage) {
        case ShaderType::Vertex:
            return static_cast<bool>(record.value.vertex);
        case ShaderType::Pixel:
            return static_cast<bool>(record.value.pixel);
        case ShaderType::Geometry:
            return static_cast<bool>(record.value.geometry);
        }
        return false;
    } else {
        return true;
    }
}

template <typename T>
T MakeValue(const T& existing, const GpuPayload& gpu) {
    if constexpr (std::is_same_v<T, MeshAsset>) {
        return MeshAsset{gpu.mesh};
    } else if constexpr (std::is_same_v<T, TextureAsset>) {
        return TextureAsset{gpu.texture};
    } else {
        ShaderAsset value = existing;
        if (gpu.vertex) {
            value.vertex = gpu.vertex;
        }
        if (gpu.pixel) {
            value.pixel = gpu.pixel;
        }
        if (gpu.geometry) {
            value.geometry = gpu.geometry;
        }
        return value;
    }
}

template <typename T>
T* FailLoad(AssetTable<T>& table, const AssetHandle<T> handle, const std::string& error) {
    const typename AssetTable<T>::Record* record = table.Get(handle);
    spdlog::error("AssetSystem: failed to load '{}': {}",
                  record != nullptr ? record->desc.name : std::string{}, error);
    table.Fail(handle, error);
    return nullptr;
}

constexpr std::string_view ToString(const AssetKind kind) {
    switch (kind) {
    case AssetKind::Mesh:
        return "mesh";
    case AssetKind::Texture:
        return "texture";
    case AssetKind::Shader:
        return "shader";
    case AssetKind::Count:
        break;
    }
    return "unknown";
}

}  // namespace

AssetSystem::AssetSystem() = default;

AssetSystem::~AssetSystem() = default;

std::string_view AssetSystem::GetName() const {
    return "AssetSystem";
}

bool AssetSystem::Initialize() {
    rendering_ = GetEngine().GetSubsystem<RenderingSubsystem>();
    if (rendering_ == nullptr) {
        spdlog::error("AssetSystem: RenderingSubsystem must be registered before AssetSystem");
        return false;
    }

    importers_[static_cast<size_t>(AssetKind::Mesh)] = std::make_unique<AssimpMeshImporter>();
    importers_[static_cast<size_t>(AssetKind::Texture)] = std::make_unique<TextureImporter>();
    importers_[static_cast<size_t>(AssetKind::Shader)] = std::make_unique<ShaderImporter>();
    return true;
}

void AssetSystem::Update(const FrameContext& frame) {
    // The worker only fills its payload; device calls and table writes stay on this thread.
    // inFlight_ itself is touched from here and LoadSync only, so it needs no lock.
    for (auto it = inFlight_.begin(); it != inFlight_.end();) {
        if (!it->get()->GetIsComplete()) {
            ++it;
            continue;
        }
        std::unique_ptr<LoadJob> completed = std::move(*it);
        it = inFlight_.erase(it);
        FinishJob(*completed, frame.frameIndex);  // ~ICompletable asserts completion, so destroy only now
    }
}

void AssetSystem::Shutdown() {
    // Engine::Shutdown() stops subsystems before TaskScheduler::WaitforAllAndShutdown(), so
    // the scheduler is still usable here. Waiting for each job is what ~ICompletable
    // requires; their results are dropped, nobody consumes assets during kernel shutdown.
    for (std::unique_ptr<LoadJob>& job : inFlight_) {
        GetEngine().GetTaskScheduler().WaitforTask(job.get());
    }
    inFlight_.clear();
}

MeshAsset* AssetSystem::LoadMesh(const std::string_view name, const bool centerOnLoad) {
    return LoadSync(RequestMesh(name, centerOnLoad));
}

TextureAsset* AssetSystem::LoadTexture(const std::string_view name) {
    return LoadSync(RequestTexture(name));
}

ShaderAsset* AssetSystem::LoadShader(const std::string_view name, const ShaderType stage) {
    AssetDesc desc;
    desc.name = NormalizeName(name);
    desc.path = AssetPathResolver::DefaultPath(desc.name, AssetKind::Shader);
    desc.shaderStage = stage;

    const AssetHandle<ShaderAsset> handle = Request<ShaderAsset>(std::move(desc));
    if (typename AssetTable<ShaderAsset>::Record* record = Table<ShaderAsset>().Get(handle)) {
        if (record->state == AssetState::Ready) {
            // A reused record keeps the stage it was first requested with; patching it is
            // only safe once no job can read the description.
            record->desc.shaderStage = stage;
        }
    }
    return LoadSync(handle);
}

AssetHandle<MeshAsset> AssetSystem::RequestMesh(const std::string_view name, const bool centerOnLoad) {
    AssetDesc desc;
    desc.name = NormalizeName(name);
    desc.path = AssetPathResolver::DefaultPath(desc.name, AssetKind::Mesh);
    desc.centerOnLoad = centerOnLoad;
    return Request<MeshAsset>(std::move(desc));
}

AssetHandle<TextureAsset> AssetSystem::RequestTexture(const std::string_view name) {
    AssetDesc desc;
    desc.name = NormalizeName(name);
    desc.path = AssetPathResolver::DefaultPath(desc.name, AssetKind::Texture);
    return Request<TextureAsset>(std::move(desc));
}

template <typename T>
AssetHandle<T> AssetSystem::Request(AssetDesc&& desc) {
    AssetTable<T>& table = Table<T>();
    const std::string name = desc.name;

    AssetHandle<T> handle = table.Acquire(std::move(desc));
    if (!handle) {
        const AssetTable<T>::Record* collision = table.FindCollision(name);
        spdlog::error("AssetSystem: name '{}' hashes to the same id as already registered '{}'",
                      name, collision != nullptr ? collision->desc.name : std::string{});
        return AssetHandle<T>::Invalid();
    }

    if (typename AssetTable<T>::Record* record = table.Get(handle);
        record != nullptr && record->state == AssetState::Unloaded) {
        table.MarkLoading(handle);
        DispatchJob(AssetValueKind<T>::value, record->desc, handle.slot, handle.generation);
    }
    return handle;
}

template <typename T>
T* AssetSystem::LoadSync(const AssetHandle<T> handle) {
    if (!handle) {
        return nullptr;
    }
    AssetTable<T>& table = Table<T>();
    typename AssetTable<T>::Record* record = table.Get(handle);
    if (record == nullptr) {
        return nullptr;
    }
    if (IsSatisfied<T>(*record)) {
        return &record->value;
    }

    const AssetKind kind = AssetValueKind<T>::value;
    if (record->state != AssetState::Loading) {
        // Adding a stage to an already Ready shader record: the slot is taken, no job runs.
        table.MarkLoading(handle);
        DispatchJob(kind, record->desc, handle.slot, handle.generation);
    }

    std::unique_ptr<LoadJob> job = TakeJob(kind, handle.slot);
    if (job == nullptr) {
        return FailLoad(table, handle, "load job disappeared from the scheduler");
    }
    GetEngine().GetTaskScheduler().WaitforTask(job.get());
    return FinishTyped(*job, table, handle, GetEngine().GetFrame().frameIndex);
}

void AssetSystem::DispatchJob(const AssetKind kind, const AssetDesc& desc, const uint32_t slot,
                              const uint32_t generation) {
    std::unique_ptr<LoadJob> job = std::make_unique<LoadJob>();
    job->kind = kind;
    job->id = ToResourceID(desc.name);
    job->name = desc.name;
    job->slot = slot;
    job->generation = generation;
    job->desc = &desc;
    job->importer = importers_[static_cast<size_t>(kind)].get();
    // Resolved here, on the main thread: the worker only uses the free-threaded device.
    job->device = MakeTransfer().device;
    job->queuedAt = std::chrono::steady_clock::now();
    spdlog::info("AssetSystem: queued '{}' ({}) on frame {}", desc.name, ToString(kind),
                 GetEngine().GetFrame().frameIndex);
    LoadJob* scheduled = job.get();
    inFlight_.push_back(std::move(job));
    GetEngine().GetTaskScheduler().AddTaskSetToPipe(scheduled);
}

std::unique_ptr<LoadJob> AssetSystem::TakeJob(const AssetKind kind, const uint32_t slot) {
    for (auto it = inFlight_.begin(); it != inFlight_.end(); ++it) {
        if ((*it)->kind == kind && (*it)->slot == slot) {
            std::unique_ptr<LoadJob> job = std::move(*it);
            inFlight_.erase(it);
            return job;
        }
    }
    return nullptr;
}

void AssetSystem::FinishJob(const LoadJob& job, const uint64_t frameIndex) {
    switch (job.kind) {
    case AssetKind::Mesh:
        FinishTyped(job, meshes_, AssetHandle<MeshAsset>{job.slot, job.generation}, frameIndex);
        break;
    case AssetKind::Texture:
        FinishTyped(job, textures_, AssetHandle<TextureAsset>{job.slot, job.generation}, frameIndex);
        break;
    case AssetKind::Shader:
        FinishTyped(job, shaders_, AssetHandle<ShaderAsset>{job.slot, job.generation}, frameIndex);
        break;
    case AssetKind::Count:
        break;
    }
}

template <typename T>
T* AssetSystem::FinishTyped(const LoadJob& job, AssetTable<T>& table, const AssetHandle<T> handle,
                            const uint64_t frameIndex) {
    typename AssetTable<T>::Record* record = table.Get(handle);
    if (record == nullptr) {
        return nullptr;  // the record was recycled while the job ran; the result is stale
    }
    if (!job.cpuOk) {
        T* failed = FailLoad(table, handle, job.error);
        GetEngine().GetEvents().trigger(AssetFailedEvent{job.id, job.name, job.kind, job.error});
        return failed;
    }

    GpuPayload gpu;
    std::string error;
    if (!ImporterFor(job.kind).UploadGpu(MakeTransfer(), job.payload, gpu, error)) {
        T* failed = FailLoad(table, handle, error);
        GetEngine().GetEvents().trigger(AssetFailedEvent{job.id, job.name, job.kind, error});
        return failed;
    }

    const auto totalSeconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - job.queuedAt).count();
    spdlog::info("AssetSystem: '{}' ({}) published on frame {} ({} s after queueing)",
                 job.name, ToString(job.kind), frameIndex, totalSeconds);

    T* value = table.Publish(handle, MakeValue<T>(record->value, gpu));
    GetEngine().GetEvents().trigger(AssetLoadedEvent{job.id, job.name, job.kind});
    return value;
}

UploadTransfer AssetSystem::MakeTransfer() const {
    RenderingContext& context = rendering_->GetContext();
    UploadTransfer transfer;
    transfer.device = context.GetDevice();
    transfer.context = context.GetDeviceContext();
    transfer.resources = &context.GetResourceManager();
    return transfer;
}

const IAssetImporter& AssetSystem::ImporterFor(const AssetKind kind) const {
    assert(importers_[static_cast<size_t>(kind)] != nullptr && "AssetSystem::Initialize() did not run");
    return *importers_[static_cast<size_t>(kind)];
}

RenderingSubsystem& AssetSystem::GetRendering() const noexcept {
    assert(rendering_ != nullptr && "AssetSystem::Initialize() did not run");
    return *rendering_;
}

}  // namespace engine
