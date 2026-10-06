#pragma once

#include "engine/core/BaseTypes.h"
#include <string>
#include <functional>

namespace Engine::Platform {

struct WindowDesc {
    std::string title{"Arcglass Engine"};
    Core::u32 width{1280};
    Core::u32 height{720};
    bool resizable{true};
    bool fullscreen{false};
};

class Window {
public:
    using ResizeCallback = std::function<void(Core::u32 width, Core::u32 height)>;
    using CloseCallback = std::function<void()>;

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Initialize(const WindowDesc& desc);
    void Shutdown();

    // Process OS messages (returns false if window requested close)
    bool PollEvents();

    [[nodiscard]] void* GetNativeHandle() const noexcept { return m_handle; }
    [[nodiscard]] Core::u32 GetWidth() const noexcept { return m_width; }
    [[nodiscard]] Core::u32 GetHeight() const noexcept { return m_height; }
    [[nodiscard]] float GetAspectRatio() const noexcept {
        return (m_height > 0) ? (static_cast<float>(m_width) / static_cast<float>(m_height)) : 1.0f;
    }
    [[nodiscard]] bool IsMinimized() const noexcept { return m_isMinimized; }
    [[nodiscard]] bool ShouldClose() const noexcept { return m_shouldClose; }

    void SetResizeCallback(ResizeCallback callback) { m_onResize = std::move(callback); }
    void SetCloseCallback(CloseCallback callback) { m_onClose = std::move(callback); }
    void SetTitle(const std::string& title);

private:
    void* m_handle{nullptr};
    std::string m_title;
    Core::u32 m_width{1280};
    Core::u32 m_height{720};
    bool m_isMinimized{false};
    bool m_shouldClose{false};

    ResizeCallback m_onResize;
    CloseCallback m_onClose;

    friend void* GetWindowInstance();
};

} // namespace Engine::Platform
