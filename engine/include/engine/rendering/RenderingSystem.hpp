#pragma once
#include <memory>
#include <vector>

#include <engine/core/Surface.hpp>
#include <engine/engine_export.hpp>

#include <engine/rendering/passes/GBufferPass.hpp>
#include <engine/rendering/passes/LightPass.hpp>
#include <engine/rendering/passes/PresentPass.hpp>
#include <engine/rendering/RenderingContext.hpp>
#include <engine/rendering/passes/RenderPass.hpp>
#include <engine/rendering/passes/ShadowPass.hpp>


namespace engine {

struct ENGINE_API RenderingSystem {
    /// Renders into a host-owned window; false when the device could not be created.
    bool Initialize(const NativeSurface& surface);
    /// Re-inits all screen size dependant resource and sets new screen size (We hope that HWND doesn't change during game run)
    bool Reinitialize(const NativeSurface& surface);
    void RenderFrame();
    ~RenderingSystem();

    [[nodiscard]] RenderingContext& GetRenderingContext();
    [[nodiscard]] RenderingResourceManager& GetResourceManager();
    [[nodiscard]] RenderWorld& GetWorld();

private:
    void InitializePasses(uint32_t screenWidth, uint32_t screenHeight);

    RenderingContext renderingContext_{};
    RenderWorld world_{};
    std::unique_ptr<RenderingResources> resources_;
    std::vector<std::unique_ptr<RenderPass>> renderPasses_{};
};

}
