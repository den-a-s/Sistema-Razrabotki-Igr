#pragma once

#include <bitset>
#include <string_view>

#include <SimpleMath.h>

#include <engine/core/Subsystem.hpp>
#include <engine/input/InputTypes.hpp>

namespace engine {

/// Keyboard and mouse state, per frame. The engine doesn't read the platform itself:
/// the host (the example game's window loop, the editor's viewport widget) feeds it
/// with the On*() calls, at any time on the main thread.
///
/// Update() latches everything fed since the previous frame, so during a frame all
/// subsystems see the same state. Register it first, before anything that reads it.
/// TODO: controller????
class ENGINE_API InputSubsystem final : public Subsystem {
public:
    [[nodiscard]] std::string_view GetName() const override;
    void Update(const FrameContext& frame) override;

    // --- Host side -------------------------------------------------------------
    void OnKey(Key key, bool down);
    void OnMouseButton(MouseButton button, bool down);
    /// Relative movement in pixels since the previous call (+x right, +y down).
    void OnMouseMove(float dx, float dy);
    /// In wheel notches, positive away from the user.
    void OnMouseWheel(float notches);
    /// Releases every key and button, e.g. when the window loses focus and would
    /// never deliver the release events.
    void OnFocusLost();

    // --- Readers: state of the current frame ------------------------------------
    [[nodiscard]] bool IsDown(Key key) const { return current_.keysDown.test(Index(key)); }
    /// Went down since the previous frame (also true for a tap that already went up).
    [[nodiscard]] bool WasPressed(Key key) const { return current_.keysPressed.test(Index(key)); }
    [[nodiscard]] bool WasReleased(Key key) const { return current_.keysReleased.test(Index(key)); }

    [[nodiscard]] bool IsDown(MouseButton button) const { return current_.buttonsDown.test(Index(button)); }
    [[nodiscard]] bool WasPressed(MouseButton button) const { return current_.buttonsPressed.test(Index(button)); }
    [[nodiscard]] bool WasReleased(MouseButton button) const { return current_.buttonsReleased.test(Index(button)); }

    /// Pixels moved since the previous frame.
    [[nodiscard]] DirectX::SimpleMath::Vector2 GetMouseDelta() const { return current_.mouseDelta; }
    [[nodiscard]] float GetWheelDelta() const { return current_.wheelDelta; }

private:
    struct State {
        std::bitset<KeyCount> keysDown, keysPressed, keysReleased;
        std::bitset<MouseButtonCount> buttonsDown, buttonsPressed, buttonsReleased;
        DirectX::SimpleMath::Vector2 mouseDelta{};
        float wheelDelta = 0.0f;
    };

    static std::size_t Index(Key key) { return static_cast<std::size_t>(key); }
    static std::size_t Index(MouseButton button) { return static_cast<std::size_t>(button); }

    /// Being fed by the host.
    State pending_{};
    /// Latched in Update(), what readers see.
    State current_{};
};

}  // namespace engine
