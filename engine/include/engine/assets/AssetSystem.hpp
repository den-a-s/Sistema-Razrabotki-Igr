#pragma once

#include <array>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#include <engine/assets/AssetCommon.hpp>
#include <engine/assets/AssetHandle.hpp>
#include <engine/assets/AssetTable.hpp>
#include <engine/assets/AssetTypes.hpp>
#include <engine/core/Subsystem.hpp>
#include <engine/engine_export.hpp>

namespace engine {

struct FrameContext;
class RenderingSubsystem;
class IAssetImporter;
struct UploadTransfer;
struct LoadJob;

/// Name-addressed registry of meshes, textures and shaders.
///
/// Adds identity, deduplication, state and background loading on top of
/// RenderingResourceManager, which stays the owner of all GPU memory. Assets are
/// addressed by normalized names ("models/table" == "Models\\Table"), each table
/// entry goes Unloaded -> Loading -> Ready | Failed.
///
/// Registration order matters: RenderingSubsystem must be added first, this finds
/// it in Initialize().
class ENGINE_API AssetSystem : public Subsystem {
public:
    /// Both defined in the .cpp: constructing and destroying the importer pointers needs
    /// their complete types, which live in an engine-private header.
    AssetSystem();
    ~AssetSystem() override;

    [[nodiscard]] std::string_view GetName() const override;

    [[nodiscard]] bool Initialize() override;
    void Update(const FrameContext& frame) override;
    void Shutdown() override;

    /// Blocking loads, for boot-time assets: the CPU phase may already run on a worker
    /// (started by the Request* inside), the call waits for it and uploads on this thread.
    /// The returned pointer lives until a failed re-load recycles the slot; re-taking it
    /// via Peek() after any such recycle is the safe pattern. nullptr on failure.
    MeshAsset* LoadMesh(std::string_view name, bool centerOnLoad = false);
    TextureAsset* LoadTexture(std::string_view name);
    /// Compiles one file for one stage and merges it into the ShaderAsset of that name;
    /// call again with another stage to add it to the same record.
    ShaderAsset* LoadShader(std::string_view name, ShaderType stage);

    /// Starts loading without blocking. Deduplicated: repeated requests for a Loading or
    /// Ready name return the same handle. Invalid() when the name hashes onto a different
    /// asset (logged) ; a Failed attempt must be re-requested explicitly.
    AssetHandle<MeshAsset> RequestMesh(std::string_view name, bool centerOnLoad = false);
    AssetHandle<TextureAsset> RequestTexture(std::string_view name);

    /// Current state of a name; Unloaded when the name was never requested.
    template <typename T>
    [[nodiscard]] AssetState GetState(std::string_view name) const {
        const AssetHandle<T> handle = Table<T>().Find(NormalizeName(name));
        const typename AssetTable<T>::Record* record = Table<T>().Get(handle);
        return record != nullptr ? record->state : AssetState::Unloaded;
    }

    /// The loaded value, or nullptr while the asset is not Ready.
    template <typename T>
    [[nodiscard]] T* Peek(std::string_view name) {
        return Peek<T>(Table<T>().Find(NormalizeName(name)));
    }

    /// Generation-checked: a handle whose slot was recycled for another asset reads as
    /// nullptr instead of returning the wrong value.
    template <typename T>
    [[nodiscard]] T* Peek(const AssetHandle<T> handle) {
        const typename AssetTable<T>::Record* record = Table<T>().Get(handle);
        if (record == nullptr || record->state != AssetState::Ready) {
            return nullptr;
        }
        return const_cast<T*>(&record->value);  // NOLINT(readability-const-cast): this isn't const
    }

    template <typename T>
    [[nodiscard]] bool IsReady(const AssetHandle<T> handle) const {
        const typename AssetTable<T>::Record* record = Table<T>().Get(handle);
        return record != nullptr && record->state == AssetState::Ready;
    }

    [[nodiscard]] RenderingSubsystem& GetRendering() const noexcept;

private:
    template <typename T>
    AssetHandle<T> Request(AssetDesc&& desc);

    /// Waits for the background CPU phase if one is running, uploads on the calling thread.
    template <typename T>
    T* LoadSync(AssetHandle<T> handle);

    /// Queues the CPU phase of one record on the scheduler. `desc` must point into the
    /// stable record storage: workers read it while the main thread keeps streaming frames.
    void DispatchJob(AssetKind kind, const AssetDesc& desc, uint32_t slot, uint32_t generation);
    /// Detaches the scheduled job for one record; main thread only, nullptr if it is gone.
    /// Slot numbers repeat across the tables, so the kind is part of the match.
    std::unique_ptr<LoadJob> TakeJob(AssetKind kind, uint32_t slot);
    /// Uploads a completed job's payload and publishes or fails the record; returns the
    /// value pointer or nullptr. Fires AssetLoadedEvent / AssetFailedEvent.
    void FinishJob(const LoadJob& job, uint64_t frameIndex);
    template <typename T>
    T* FinishTyped(const LoadJob& job, AssetTable<T>& table, AssetHandle<T> handle, uint64_t frameIndex);

    [[nodiscard]] UploadTransfer MakeTransfer() const;
    [[nodiscard]] const IAssetImporter& ImporterFor(AssetKind kind) const;

    template <typename T>
    [[nodiscard]] AssetTable<T>& Table() {
        if constexpr (std::is_same_v<T, MeshAsset>) {
            return meshes_;
        } else if constexpr (std::is_same_v<T, TextureAsset>) {
            return textures_;
        } else if constexpr (std::is_same_v<T, ShaderAsset>) {
            return shaders_;
        } else {
            static_assert(sizeof(T) == 0, "not an asset value type");
        }
    }

    template <typename T>
    [[nodiscard]] const AssetTable<T>& Table() const {
        return const_cast<AssetSystem*>(this)->Table<T>();
    }

    AssetTable<MeshAsset> meshes_;
    AssetTable<TextureAsset> textures_;
    AssetTable<ShaderAsset> shaders_;
    std::array<std::unique_ptr<IAssetImporter>, static_cast<size_t>(AssetKind::Count)> importers_{};
    /// Queued CPU-phase jobs. Touched only from the main thread, so no lock is needed:
    /// workers see just each job's own fields.
    std::vector<std::unique_ptr<LoadJob>> inFlight_;
    RenderingSubsystem* rendering_ = nullptr;
};

}  // namespace engine
