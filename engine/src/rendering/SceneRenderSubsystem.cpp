#include <engine/rendering/SceneRenderSubsystem.hpp>

#include <algorithm>
#include <cmath>

#include <spdlog/spdlog.h>

#include <engine/assets/AssetSystem.hpp>
#include <engine/engine.hpp>
#include <engine/rendering/RenderComponents.hpp>
#include <engine/rendering/RenderingSubsystem.hpp>

namespace engine {

std::string_view SceneRenderSubsystem::GetName() const {
    return "SceneRenderSubsystem";
}

bool SceneRenderSubsystem::Initialize() {
    rendering_ = GetEngine().GetSubsystem<RenderingSubsystem>();
    if (rendering_ == nullptr) {
        spdlog::error("SceneRenderSubsystem: needs RenderingSubsystem registered before it");
        return false;
    }
    defaultMaterial_ = rendering_->GetResourceManager().CreateMaterial({});
    if (!defaultMaterial_) {
        spdlog::error("SceneRenderSubsystem: could not create the default material");
        return false;
    }
    whiteTexture_ = GetEngine().GetAssets().RequestTexture(BuiltinAssets::WhiteTexture);
    // Creates the pools here, so their code lives in Engine.dll rather than in whichever
    // (editor) module happens to emplace the first component.
    auto& registry = GetEngine().GetScene().GetRegistry();
    static_cast<void>(registry.storage<MeshRenderer>());
    static_cast<void>(registry.storage<Camera>());
    static_cast<void>(registry.storage<DirectionalLight>());
    return true;
}

void SceneRenderSubsystem::Shutdown() {
    // Runs before RenderingSubsystem shuts down (registered after it), so the manager is alive.
    RenderingResourceManager& resources = rendering_->GetResourceManager();
    rendering_->GetWorld().renderItems.clear();
    for (const auto& [entity, instance] : instances_) {
        resources.DestroyMaterialInstance(instance);
    }
    instances_.clear();
    resources.DestroyMaterial(defaultMaterial_);
    defaultMaterial_ = {};
}

void SceneRenderSubsystem::Update(const FrameContext&) {
    const Scene& scene = GetEngine().GetScene();
    AssetSystem& assets = GetEngine().GetAssets();
    RenderWorld& world = rendering_->GetWorld();
    RenderingResourceManager& resources = rendering_->GetResourceManager();

    UpdateCamera(scene, world);
    UpdateDirectionalLight(scene, world);
    ReleaseStaleInstances(scene);

    world.renderItems.clear();
    for (const auto [entity, renderer] : scene.GetRegistry().view<MeshRenderer>().each()) {
        // Not loaded yet (or failed): skip it this frame. The passes bind both
        // unconditionally, an empty GPU handle would assert.
        const auto mesh = assets.Peek(renderer.mesh);
        const auto texture = assets.Peek(renderer.texture ? renderer.texture : whiteTexture_);
        if (!mesh || !texture) {
            continue;
        }
        // The draw list skips items without a valid instance, so every drawn entity gets one.
        auto [it, inserted] = instances_.try_emplace(entity);
        if (inserted) {
            it->second = resources.CreateMaterialInstance(defaultMaterial_);
        }
        resources.SetMaterialInstanceParameters(it->second, MaterialParameters{
            .albedoTexture = texture->texture,
            .parameters = renderer.material,
        });
        world.renderItems.push_back({
            .worldMatrix = scene.GetWorldMatrix(entity),
            .mesh = mesh->mesh,
            .materialInstance = it->second,
        });
    }
}

void SceneRenderSubsystem::ReleaseStaleInstances(const Scene& scene) {
    const auto& registry = scene.GetRegistry();
    RenderingResourceManager& resources = rendering_->GetResourceManager();
    for (auto it = instances_.begin(); it != instances_.end();) {
        if (registry.valid(it->first) && registry.all_of<MeshRenderer>(it->first)) {
            ++it;
            continue;
        }
        resources.DestroyMaterialInstance(it->second);
        it = instances_.erase(it);
    }
}

void SceneRenderSubsystem::UpdateCamera(const Scene& scene, RenderWorld& world) const {
    using DirectX::SimpleMath::Vector3;

    // Follows the surface, so hosts only report resizes.
    if (const NativeSurface& surface = GetEngine().GetSurface(); surface.width > 0 && surface.height > 0) {
        world.camera.aspectRatio = static_cast<float>(surface.width) / static_cast<float>(surface.height);
    }

    const entt::entity entity = scene.GetRegistry().view<Camera>().front();
    if (entity == entt::null) {
        return;  // keep whatever the host set
    }
    const Camera& camera = scene.GetRegistry().get<Camera>(entity);
    const auto worldMatrix = scene.GetWorldMatrix(entity);

    // FPSCamera looks along Forward rotated by yaw, then pitch:
    // forward = (-cos(pitch) sin(yaw), sin(pitch), -cos(pitch) cos(yaw)).
    Vector3 forward = Vector3::TransformNormal(Vector3::Forward, worldMatrix);
    forward.Normalize();
    world.camera.position = worldMatrix.Translation();
    world.camera.pitch = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    world.camera.yaw = std::atan2(-forward.x, -forward.z);
    world.camera.fov = camera.fovY;
    world.camera.nearPlane = camera.nearPlane;
    world.camera.farPlane = camera.farPlane;
}

void SceneRenderSubsystem::UpdateDirectionalLight(const Scene& scene, RenderWorld& world) {
    using DirectX::SimpleMath::Vector3;

    const entt::entity entity = scene.GetRegistry().view<DirectionalLight>().front();
    if (entity == entt::null) {
        world.directionalLight = {};  // zero intensity: no sun
        world.directionalLight.type = LightInfo::Directional;
        return;
    }
    const DirectionalLight& light = scene.GetRegistry().get<DirectionalLight>(entity);

    Vector3 direction = Vector3::TransformNormal(Vector3::Forward, scene.GetWorldMatrix(entity));
    direction.Normalize();
    world.directionalLight = {};
    world.directionalLight.direction = direction;
    world.directionalLight.color = light.color;
    world.directionalLight.intensity = light.intensity;
    world.directionalLight.type = LightInfo::Directional;
}

}  // namespace engine
