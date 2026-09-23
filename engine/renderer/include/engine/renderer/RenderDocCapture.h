#pragma once

#include "engine/core/BaseTypes.h"
#include <string>

namespace Engine::Renderer {

class RenderDocCapture {
public:
    static RenderDocCapture& Get() noexcept;

    void Initialize();
    void Shutdown();

    void TriggerCapture() noexcept;
    void StartFrameCapture(void* deviceHandle = nullptr, void* windowHandle = nullptr);
    void EndFrameCapture(void* deviceHandle = nullptr, void* windowHandle = nullptr);

    [[nodiscard]] bool IsAvailable() const noexcept { return m_isAvailable; }
    [[nodiscard]] bool IsCapturing() const noexcept { return m_isCapturing; }
    [[nodiscard]] Core::u32 GetCaptureCount() const noexcept { return m_captureCount; }

private:
    RenderDocCapture() = default;
    ~RenderDocCapture() = default;

    void* m_renderDocAPI{nullptr};
    bool m_isAvailable{false};
    bool m_triggerNextFrame{false};
    bool m_isCapturing{false};
    Core::u32 m_captureCount{0};
};

} // namespace Engine::Renderer
