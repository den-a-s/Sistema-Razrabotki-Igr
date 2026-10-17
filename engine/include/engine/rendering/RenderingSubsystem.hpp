#pragma once

#include <string_view>

#include <engine/rendering/RenderingSystem.hpp>
#include <engine/core/Subsystem.hpp>
#include <engine/core/Surface.hpp>

namespace engine {

/// Bridges the existing RenderingSystem into the Engine kernel: it renders into
/// Engine::GetSurface(), so the host keeps owning the window and its message pump.
/// Surface size changes are not handled yet (no ResizeBuffers).
class ENGINE_API RenderingSubsystem : public Subsystem {
public:
    [[nodiscard]] std::string_view GetName() const override;
    [[nodiscard]] bool Initialize() override;
    void Render(const FrameContext& frame) override;

    [[nodiscard]] RenderingSystem& GetRendering() noexcept;
    [[nodiscard]] RenderingContext& GetContext();
    [[nodiscard]] RenderingResourceManager& GetResourceManager();
    [[nodiscard]] RenderWorld& GetWorld();
    /// The surface this subsystem renders into, as handed by the host.
    [[nodiscard]] const NativeSurface& GetSurface() const noexcept;

private:
    RenderingSystem renderer_{};
    NativeSurface surface_{};
};

}  // namespace engine
