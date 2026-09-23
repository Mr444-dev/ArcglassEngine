#include "engine/renderer/RenderDocCapture.h"
#include "engine/core/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Engine::Renderer {

// Minimal RenderDoc API v1.5.0 struct signature
struct RENDERDOC_API_1_5_0 {
    void* GetAPIVersion;
    void* SetCaptureOptionU32;
    void* SetCaptureOptionF32;
    void* GetCaptureOptionU32;
    void* GetCaptureOptionF32;
    void* SetFocusToggleKeys;
    void* SetCaptureKeys;
    void* GetOverlayBits;
    void* MaskOverlayBits;
    void* RemoveOverlayBits;
    void* SetOverlayBits;
    void* DiscardFrameCapture;
    void* ShowReplayUI;
    void* SetCaptureTitle;
    void* IsTargetControlConnected;
    void* TargetControlLoop;
    void* StartFrameCapture;
    void* IsFrameCapturing;
    void* EndFrameCapture;
    void* TriggerCapture;
};

using pRENDERDOC_GetAPI = int (*)(int version, void** outAPIPointers);

RenderDocCapture& RenderDocCapture::Get() noexcept {
    static RenderDocCapture instance;
    return instance;
}

void RenderDocCapture::Initialize() {
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    if (!mod) {
        // Not launched under RenderDoc
        m_isAvailable = false;
        ENGINE_LOG_INFO("RenderDoc: Not attached (renderdoc.dll not found). Safe fallback active.");
        return;
    }

    auto getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
    if (!getApi) {
        m_isAvailable = false;
        ENGINE_LOG_WARN("RenderDoc: RENDERDOC_GetAPI export missing.");
        return;
    }

    const int ret = getApi(10500 /* eRENDERDOC_API_Version_1_5_0 */, &m_renderDocAPI);
    if (ret == 1 && m_renderDocAPI) {
        m_isAvailable = true;
        ENGINE_LOG_INFO("RenderDoc: In-app API v1.5.0 successfully hooked.");
    } else {
        m_isAvailable = false;
        ENGINE_LOG_WARN("RenderDoc: Failed to initialize API version 1.5.0.");
    }
}

void RenderDocCapture::Shutdown() {
    m_renderDocAPI = nullptr;
    m_isAvailable = false;
    m_isCapturing = false;
}

void RenderDocCapture::TriggerCapture() noexcept {
    m_triggerNextFrame = true;
    ENGINE_LOG_INFO("RenderDoc: Frame capture requested for next frame.");
}

void RenderDocCapture::StartFrameCapture(void* deviceHandle, void* windowHandle) {
    if (!m_isAvailable || !m_triggerNextFrame) return;

    auto* rdoc = static_cast<RENDERDOC_API_1_5_0*>(m_renderDocAPI);
    if (rdoc && rdoc->StartFrameCapture) {
        using pfnStart = void (*)(void*, void*);
        reinterpret_cast<pfnStart>(rdoc->StartFrameCapture)(deviceHandle, windowHandle);
        m_isCapturing = true;
        ENGINE_LOG_INFO("RenderDoc: Frame capture started.");
    }
}

void RenderDocCapture::EndFrameCapture(void* deviceHandle, void* windowHandle) {
    if (!m_isAvailable || !m_isCapturing) return;

    auto* rdoc = static_cast<RENDERDOC_API_1_5_0*>(m_renderDocAPI);
    if (rdoc && rdoc->EndFrameCapture) {
        using pfnEnd = uint32_t (*)(void*, void*);
        reinterpret_cast<pfnEnd>(rdoc->EndFrameCapture)(deviceHandle, windowHandle);
        m_isCapturing = false;
        m_triggerNextFrame = false;
        m_captureCount++;
        ENGINE_LOG_INFO("RenderDoc: Frame capture completed (Total captures: {}).", m_captureCount);
    }
}

} // namespace Engine::Renderer
