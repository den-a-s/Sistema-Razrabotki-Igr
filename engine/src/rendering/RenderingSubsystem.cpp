#include <engine/rendering/RenderingSubsystem.hpp>

#include <engine/engine.hpp>

#include <spdlog/spdlog.h>

namespace engine {

std::string_view RenderingSubsystem::GetName() const {
    return "RenderingSubsystem";
}

bool RenderingSubsystem::Initialize() {
    surface_ = GetEngine().GetSurface();
    if (!surface_.IsValid()) {
        spdlog::error("RenderingSubsystem: the host gave the engine no window to render into");
        return false;
    }
    if (!renderer_.Initialize(surface_)) {
        spdlog::error("RenderingSubsystem: the renderer could not be created for {}x{}",
                      surface_.width, surface_.height);
        return false;
    }
    return true;
}

void RenderingSubsystem::Render(const FrameContext&) {
    renderer_.RenderFrame();
}

RenderingSystem& RenderingSubsystem::GetRendering() noexcept {
    return renderer_;
}

RenderingContext& RenderingSubsystem::GetContext() {
    return renderer_.GetRenderingContext();
}

RenderingResourceManager& RenderingSubsystem::GetResourceManager() {
    return renderer_.GetResourceManager();
}

RenderWorld& RenderingSubsystem::GetWorld() {
    return renderer_.GetWorld();
}

const NativeSurface& RenderingSubsystem::GetSurface() const noexcept {
    return surface_;
}

}  // namespace engine
