#include "engine/platform/Window.h"
#include "engine/platform/Input.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "../resources/resource.h"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <windowsx.h>
#endif

namespace Engine::Platform {

#if defined(_WIN32)
namespace {

KeyCode VirtualKeyToEngineKey(WPARAM vk) {
    if (vk >= 'A' && vk <= 'Z') return static_cast<KeyCode>(vk);
    if (vk >= '0' && vk <= '9') return static_cast<KeyCode>(vk);
    switch (vk) {
        case VK_SPACE: return KeyCode::Space;
        case VK_ESCAPE: return KeyCode::Escape;
        case VK_RETURN: return KeyCode::Enter;
        case VK_TAB: return KeyCode::Tab;
        case VK_BACK: return KeyCode::Backspace;
        case VK_INSERT: return KeyCode::Insert;
        case VK_DELETE: return KeyCode::Delete;
        case VK_RIGHT: return KeyCode::Right;
        case VK_LEFT: return KeyCode::Left;
        case VK_DOWN: return KeyCode::Down;
        case VK_UP: return KeyCode::Up;
        case VK_PRIOR: return KeyCode::PageUp;
        case VK_NEXT: return KeyCode::PageDown;
        case VK_HOME: return KeyCode::Home;
        case VK_END: return KeyCode::End;
        case VK_SHIFT:
        case VK_LSHIFT: return KeyCode::LeftShift;
        case VK_RSHIFT: return KeyCode::RightShift;
        case VK_CONTROL:
        case VK_LCONTROL: return KeyCode::LeftControl;
        case VK_RCONTROL: return KeyCode::RightControl;
        case VK_MENU:
        case VK_LMENU: return KeyCode::LeftAlt;
        case VK_RMENU: return KeyCode::RightAlt;
        case VK_F1: return KeyCode::F1;
        case VK_F2: return KeyCode::F2;
        case VK_F3: return KeyCode::F3;
        case VK_F4: return KeyCode::F4;
        case VK_F5: return KeyCode::F5;
        case VK_F6: return KeyCode::F6;
        case VK_F7: return KeyCode::F7;
        case VK_F8: return KeyCode::F8;
        case VK_F9: return KeyCode::F9;
        case VK_F10: return KeyCode::F10;
        case VK_F11: return KeyCode::F11;
        case VK_F12: return KeyCode::F12;
        default: return KeyCode::Unknown;
    }
}

LRESULT CALLBACK EngineWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CLOSE: {
            if (window) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_SIZE:
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            KeyCode key = VirtualKeyToEngineKey(wParam);
            if (key != KeyCode::Unknown) {
                Input::Get().SetKeyState(key, true);
            }
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            KeyCode key = VirtualKeyToEngineKey(wParam);
            if (key != KeyCode::Unknown) {
                Input::Get().SetKeyState(key, false);
            }
            break;
        }
        case WM_MOUSEMOVE: {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));
            Input::Get().SetMousePosition(x, y);
            break;
        }
        case WM_LBUTTONDOWN:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Left, true);
            SetCapture(hwnd);
            break;
        case WM_LBUTTONUP:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Left, false);
            if (!Input::Get().IsMouseButtonDown(MouseButton::Right) && !Input::Get().IsMouseButtonDown(MouseButton::Middle)) ReleaseCapture();
            break;
        case WM_RBUTTONDOWN:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Right, true);
            SetCapture(hwnd);
            break;
        case WM_RBUTTONUP:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Right, false);
            if (!Input::Get().IsMouseButtonDown(MouseButton::Left) && !Input::Get().IsMouseButtonDown(MouseButton::Middle)) ReleaseCapture();
            break;
        case WM_MBUTTONDOWN:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Middle, true);
            SetCapture(hwnd);
            break;
        case WM_MBUTTONUP:
            Input::Get().SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
            Input::Get().SetMouseButtonState(MouseButton::Middle, false);
            if (!Input::Get().IsMouseButtonDown(MouseButton::Left) && !Input::Get().IsMouseButtonDown(MouseButton::Right)) ReleaseCapture();
            break;
        case WM_CAPTURECHANGED:
            Input::Get().SetMouseButtonState(MouseButton::Left, false);
            Input::Get().SetMouseButtonState(MouseButton::Right, false);
            Input::Get().SetMouseButtonState(MouseButton::Middle, false);
            break;
        case WM_MOUSEWHEEL: {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            Input::Get().AddMouseWheelDelta(delta);
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
} // namespace
#endif

bool Window::Initialize(const WindowDesc& desc) {
    Shutdown();

    m_title = desc.title;
    m_width = desc.width;
    m_height = desc.height;
    m_shouldClose = false;
    m_isMinimized = false;

#if defined(_WIN32)
    // Enable per-monitor DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"Engine_WindowClass_2026";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = EngineWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ARCGLASS_APP));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = className;

    RegisterClassExW(&wc);

    // Calculate window rectangle with borders
    RECT wr = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    AdjustWindowRectEx(&wr, style, FALSE, 0);

    const int windowWidth = wr.right - wr.left;
    const int windowHeight = wr.bottom - wr.top;

    std::wstring wTitle(m_title.begin(), m_title.end());

    HWND hwnd = CreateWindowExW(
        0,
        className,
        wTitle.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) {
        ENGINE_LOG_ERROR("Failed to create Win32 window.");
        return false;
    }

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    m_handle = hwnd;
    ENGINE_LOG_INFO("Platform window initialized ({}x{}).", m_width, m_height);
    return true;
#else
    return false;
#endif
}

Window::~Window() {
    Shutdown();
}

void Window::Shutdown() {
#if defined(_WIN32)
    if (m_handle) {
        HWND hwnd = static_cast<HWND>(m_handle);
        DestroyWindow(hwnd);
        m_handle = nullptr;
    }
#endif
    m_shouldClose = true;
}

bool Window::PollEvents() {
#if defined(_WIN32)
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_shouldClose = true;
            if (m_onClose) m_onClose();
            return false;
        }
        if (msg.hwnd == static_cast<HWND>(m_handle) && msg.message == WM_SIZE) {
            const Core::u32 width = LOWORD(msg.lParam);
            const Core::u32 height = HIWORD(msg.lParam);
            m_isMinimized = msg.wParam == SIZE_MINIMIZED || width == 0 || height == 0;
            if (!m_isMinimized && (width != m_width || height != m_height)) {
                m_width = width;
                m_height = height;
                if (m_onResize) m_onResize(width, height);
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return !m_shouldClose;
#else
    return false;
#endif
}

void Window::SetTitle(const std::string& title) {
    m_title = title;
#if defined(_WIN32)
    if (m_handle) {
        std::wstring wTitle(m_title.begin(), m_title.end());
        SetWindowTextW(static_cast<HWND>(m_handle), wTitle.c_str());
    }
#endif
}

} // namespace Engine::Platform
