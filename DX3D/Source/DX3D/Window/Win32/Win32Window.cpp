#include <DX3D/Window/Window.h>
#include <DX3D/Core/Input.h>
#include <Windows.h>
#include <stdexcept>

namespace dx3d {
    // Store a pointer to the current Window instance (simple, single-window case)
    static Window* g_windowInstance = nullptr;
}

// Forward global procedure → instance
static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (dx3d::g_windowInstance)
        return dx3d::g_windowInstance->handleMessage(hwnd, msg, wparam, lparam);

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

dx3d::Window::Window(const WindowDesc& desc) : Base(desc.base), m_size(desc.size)
{
    auto registerWindowClassFunction = []()
        {
            WNDCLASSEX wc{};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpszClassName = L"DX3DWindow";
            wc.lpfnWndProc = &WindowProcedure;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            return RegisterClassEx(&wc);
        };

    static const auto windowClassId = std::invoke(registerWindowClassFunction);

    if (!windowClassId)
        DX3DLogThrowError("RegisterClassEx failed.");

    RECT rc{ 0,0,m_size.width, m_size.height };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, false);

    m_handle = CreateWindowEx(
        0, MAKEINTATOM(windowClassId), L"TheEngine",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!m_handle)
        DX3DLogThrowError("CreateWindowEx failed.");

    // Set global instance
    g_windowInstance = this;
    Input::getInstance().setWindowHandle(static_cast<HWND>(m_handle));
    ShowWindow(static_cast<HWND>(m_handle), SW_SHOW);
}

dx3d::Window::~Window()
{
    DestroyWindow(static_cast<HWND>(m_handle));
    g_windowInstance = nullptr;
}

LRESULT dx3d::Window::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto& input = Input::getInstance();
    switch (msg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

        // Keyboard - Remove the repeat check that was blocking inputs
    case WM_KEYDOWN:
        input.setKeyDown((int)wParam);
        return 0;
    case WM_KEYUP:
        input.setKeyUp((int)wParam);
        return 0;

        // Mouse
    case WM_LBUTTONDOWN: input.setMouseDown(MouseClick::LeftMouse); return 0;
    case WM_LBUTTONUP:   input.setMouseUp(MouseClick::LeftMouse);   return 0;
    case WM_RBUTTONDOWN: input.setMouseDown(MouseClick::RightMouse); return 0;
    case WM_RBUTTONUP:   input.setMouseUp(MouseClick::RightMouse);   return 0;
    case WM_MBUTTONDOWN: input.setMouseDown(MouseClick::MiddleMouse); return 0;
    case WM_MBUTTONUP:   input.setMouseUp(MouseClick::MiddleMouse);   return 0;

        // ONLY reset on actual focus loss - when user clicks away from window
    case WM_KILLFOCUS:
        input.reset();
        return 0;

        // DO NOT reset on any other events - let the hardware state check in update() handle it
    case WM_SETFOCUS:
        // Only validate when we regain focus
        Input::getInstance().validateHardwareState();
        return 0;

    case WM_DISPLAYCHANGE:
        // Only validate when display changes
        Input::getInstance().validateHardwareState();
        return 0;

    case WM_POWERBROADCAST:
        return 0;  // Don't reset!

    case WM_SIZE:
        m_size.width = LOWORD(lParam);
        m_size.height = HIWORD(lParam);
        return 0;  // Don't reset!

    case WM_ACTIVATE:
        // Only reset if we're becoming completely inactive (not just losing active status)
        if (LOWORD(wParam) == WA_INACTIVE) {
            input.reset();
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
