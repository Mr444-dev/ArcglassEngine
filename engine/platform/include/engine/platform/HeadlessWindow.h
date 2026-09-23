#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/platform/Window.h"

namespace Engine::Platform {

class HeadlessWindow {
public:
    HeadlessWindow() = default;
    ~HeadlessWindow() = default;

    bool Initialize(const WindowDesc& desc) noexcept {
        m_width = desc.width;
        m_height = desc.height;
        m_shouldClose = false;
        return true;
    }

    void Shutdown() noexcept {
        m_shouldClose = true;
    }

    [[nodiscard]] bool PollEvents() noexcept {
        return !m_shouldClose;
    }

    [[nodiscard]] void* GetNativeHandle() const noexcept { return nullptr; }
    [[nodiscard]] Core::u32 GetWidth() const noexcept { return m_width; }
    [[nodiscard]] Core::u32 GetHeight() const noexcept { return m_height; }
    [[nodiscard]] bool ShouldClose() const noexcept { return m_shouldClose; }

private:
    Core::u32 m_width{1280};
    Core::u32 m_height{720};
    bool m_shouldClose{false};
};

} // namespace Engine::Platform
