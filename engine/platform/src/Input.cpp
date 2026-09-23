#include "engine/platform/Input.h"

namespace Engine::Platform {

Input::Input() {
    m_keysCurrent.fill(false);
    m_keysPrevious.fill(false);
    m_mouseButtonsCurrent.fill(false);
    m_mouseButtonsPrevious.fill(false);
}

Input& Input::Get() {
    static Input instance;
    return instance;
}

void Input::BeginFrame() {
    m_keysPrevious = m_keysCurrent;
    m_mouseButtonsPrevious = m_mouseButtonsCurrent;

    m_mouseDeltaX = m_mouseX - m_lastMouseX;
    m_mouseDeltaY = m_mouseY - m_lastMouseY;
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;

    m_wheelDelta = 0.0f;
}

bool Input::IsKeyDown(KeyCode key) const noexcept {
    const auto idx = static_cast<size_t>(key);
    return idx < m_keysCurrent.size() && m_keysCurrent[idx];
}

bool Input::WasKeyPressed(KeyCode key) const noexcept {
    const auto idx = static_cast<size_t>(key);
    return idx < m_keysCurrent.size() && m_keysCurrent[idx] && !m_keysPrevious[idx];
}

bool Input::WasKeyReleased(KeyCode key) const noexcept {
    const auto idx = static_cast<size_t>(key);
    return idx < m_keysCurrent.size() && !m_keysCurrent[idx] && m_keysPrevious[idx];
}

bool Input::IsMouseButtonDown(MouseButton button) const noexcept {
    const auto idx = static_cast<size_t>(button);
    return idx < m_mouseButtonsCurrent.size() && m_mouseButtonsCurrent[idx];
}

bool Input::WasMouseButtonPressed(MouseButton button) const noexcept {
    const auto idx = static_cast<size_t>(button);
    return idx < m_mouseButtonsCurrent.size() && m_mouseButtonsCurrent[idx] && !m_mouseButtonsPrevious[idx];
}

bool Input::WasMouseButtonReleased(MouseButton button) const noexcept {
    const auto idx = static_cast<size_t>(button);
    return idx < m_mouseButtonsCurrent.size() && !m_mouseButtonsCurrent[idx] && m_mouseButtonsPrevious[idx];
}

void Input::SetKeyState(KeyCode key, bool isDown) {
    const auto idx = static_cast<size_t>(key);
    if (idx < m_keysCurrent.size()) {
        m_keysCurrent[idx] = isDown;
    }
}

void Input::SetMouseButtonState(MouseButton button, bool isDown) {
    const auto idx = static_cast<size_t>(button);
    if (idx < m_mouseButtonsCurrent.size()) {
        m_mouseButtonsCurrent[idx] = isDown;
    }
}

void Input::SetMousePosition(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
}

void Input::AddMouseWheelDelta(float delta) {
    m_wheelDelta += delta;
}

} // namespace Engine::Platform
