#pragma once

#include "engine/core/BaseTypes.h"
#include <array>

namespace Engine::Platform {

enum class KeyCode : Core::u16 {
    Unknown = 0,
    Space = 32,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    LeftShift, LeftControl, LeftAlt,
    RightShift, RightControl, RightAlt,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count = 512
};

enum class MouseButton : Core::u8 {
    Left = 0,
    Right = 1,
    Middle = 2,
    Count = 3
};

class Input {
public:
    static Input& Get();

    void BeginFrame();

    // Query state
    [[nodiscard]] bool IsKeyDown(KeyCode key) const noexcept;
    [[nodiscard]] bool WasKeyPressed(KeyCode key) const noexcept;
    [[nodiscard]] bool WasKeyReleased(KeyCode key) const noexcept;

    [[nodiscard]] bool IsMouseButtonDown(MouseButton button) const noexcept;
    [[nodiscard]] bool WasMouseButtonPressed(MouseButton button) const noexcept;
    [[nodiscard]] bool WasMouseButtonReleased(MouseButton button) const noexcept;

    [[nodiscard]] float GetMouseX() const noexcept { return m_mouseX; }
    [[nodiscard]] float GetMouseY() const noexcept { return m_mouseY; }
    [[nodiscard]] float GetMouseDeltaX() const noexcept { return m_mouseDeltaX; }
    [[nodiscard]] float GetMouseDeltaY() const noexcept { return m_mouseDeltaY; }
    [[nodiscard]] float GetMouseWheelDelta() const noexcept { return m_wheelDelta; }

    // Internal OS event dispatchers
    void SetKeyState(KeyCode key, bool isDown);
    void SetMouseButtonState(MouseButton button, bool isDown);
    void SetMousePosition(float x, float y);
    void AddMouseWheelDelta(float delta);

private:
    Input();
    ~Input() = default;

    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_keysCurrent{};
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_keysPrevious{};

    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonsCurrent{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonsPrevious{};

    float m_mouseX{0.0f};
    float m_mouseY{0.0f};
    float m_lastMouseX{0.0f};
    float m_lastMouseY{0.0f};
    float m_mouseDeltaX{0.0f};
    float m_mouseDeltaY{0.0f};
    float m_wheelDelta{0.0f};
};

} // namespace Engine::Platform
