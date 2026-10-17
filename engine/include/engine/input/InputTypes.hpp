#pragma once

#include <cstddef>
#include <cstdint>

namespace engine {

/// Platform-independent key codes. Hosts translate their native codes (Win32 VK_*,
/// Qt::Key, ...) into these before feeding InputSubsystem.
enum class Key : std::uint8_t {
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Space, Enter, Escape, Tab, Backspace, Delete,
    Left, Right, Up, Down,
    Shift, Control, Alt,

    Count
};

enum class MouseButton : std::uint8_t {
    Left = 0,
    Right,
    Middle,

    Count
};

inline constexpr std::size_t KeyCount = static_cast<std::size_t>(Key::Count);
inline constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

}  // namespace engine
