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
    whiteTexture_ = GetEngine().GetAssets().RequestTexture(BuiltinAssets::WhiteTexture);
    // Creates the pools here, so their code lives in Engine.dll rather than in whichever
    // (editor) module happens to emplace the first component.
    auto& registry = GetEngine().GetScene().GetRegistry();
    static_cast<void>(registry.storage<MeshRenderer>());
    static_cast<void>(registry.storage<Camera>());
    static_cast<void>(registry.storage<DirectionalLight>());
    return true;
}

void SceneRenderSubsystem::Update(const FrameContext&) {
    const Scene& scene = GetEngine().GetScene();
    AssetSystem& assets = GetEngine().GetAssets();
    RenderWorld& world = rendering_->GetWorld();

    UpdateCamera(scene, world);
    UpdateDirectionalLight(scene, world);

    world.renderItems.clear();
    for (const auto [entity, renderer] : scene.GetRegistry().view<MeshRenderer>().each()) {
        // Not loaded yet (or failed): skip it this frame. The passes bind both
        // unconditionally, an empty GPU handle would assert.
        const auto mesh = assets.Peek(renderer.mesh);
        const auto texture = assets.Peek(renderer.texture ? renderer.texture : whiteTexture_);
        if (!mesh || !texture) {
            continue;
        }
        world.renderItems.push_back({
            .worldMatrix = scene.GetWorldMatrix(entity),
            .mesh = mesh->mesh,
            .material = renderer.material,
            .texture = texture->texture,
        });
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
