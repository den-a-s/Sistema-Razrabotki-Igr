#pragma once

#include <string_view>

#include <engine/assets/AssetHandle.hpp>
#include <engine/assets/AssetTypes.hpp>
#include <engine/core/Subsystem.hpp>

namespace engine {

class RenderingSubsystem;
class Scene;
struct RenderWorld;

/// Fills the RenderWorld from the active scene: renderItems from MeshRenderer components
/// (replacing whatever was there), the camera from the first Camera and the directional
/// light from the first DirectionalLight. Opt-in: hosts that fill the RenderWorld
/// themselves (like the example game) simply don't register it.
/// Register after RenderingSubsystem.
class ENGINE_API SceneRenderSubsystem : public Subsystem {
public:
    [[nodiscard]] std::string_view GetName() const override;
    [[nodiscard]] bool Initialize() override;
    void Update(const FrameContext& frame) override;

private:
    void UpdateCamera(const Scene& scene, RenderWorld& world) const;
    static void UpdateDirectionalLight(const Scene& scene, RenderWorld& world);

    RenderingSubsystem* rendering_ = nullptr;
    /// Stands in for MeshRenderers without a texture.
    AssetHandle<TextureAsset> whiteTexture_{};
};

}  // namespace engine
