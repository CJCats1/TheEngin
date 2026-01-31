#if defined(_WIN32)
#include <TheEngine/Window/Window.h>
#include <TheEngine/Core/Input.h>
#include <Windows.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <stdexcept>
#include <string>

namespace TheEngine {
    // Store a pointer to the current Window instance (simple, single-window case)
    static Window* g_windowInstance = nullptr;
}

namespace
{
    std::wstring buildIconPath()
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring exeDir = exePath;
        size_t lastSlash = exeDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos)
        {
            exeDir = exeDir.substr(0, lastSlash + 1);
        }
        return exeDir + L"..\\..\\..\\TheEngine\\Assets\\Textures\\CheckEngine.ico";
    }

    void applyWindowIcon(HWND hwnd)
    {
        const std::wstring iconPath = buildIconPath();
        HICON hIcon = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON,
            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE);
        HICON hIconSm = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);
        if (hIcon)
        {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        if (hIconSm)
        {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
        }
    }
}

// Forward global procedure → instance
// Forward declaration to satisfy the compiler even if the backend header
// is not picked up correctly by IntelliSense. The implementation is
// provided by imgui_impl_win32.cpp.
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return 1;
    if (TheEngine::g_windowInstance)
        return TheEngine::g_windowInstance->handleMessage(hwnd, msg, wparam, lparam);

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

TheEngine::Window::Window(const WindowDesc& desc) : Base(desc.base), m_size(desc.size)
{
    if (!desc.createNativeWindow)
    {
        m_handle = nullptr;
        return;
    }
    auto registerWindowClassFunction = []()
        {
            WNDCLASSEX wc{};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.hInstance = GetModuleHandle(nullptr);
            wc.lpszClassName = L"TheEngineWindow";
            wc.lpfnWndProc = &WindowProcedure;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            
            // Load the application icon
            // Get the executable directory
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring exeDir = exePath;
            size_t lastSlash = exeDir.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                exeDir = exeDir.substr(0, lastSlash + 1);
            }
            
            // Construct path to icon (relative to executable: ../../TheEngine/Assets/Textures/CheckEngine.ico)
            std::wstring iconPath = buildIconPath();
            
            // Load large icon (32x32 or system default)
            HICON hIcon = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON, 
                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 
                LR_LOADFROMFILE);
            // Load small icon (16x16)
            HICON hIconSm = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON,
                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                LR_LOADFROMFILE);
            
            if (hIcon && hIconSm) {
                wc.hIcon = hIcon;
                wc.hIconSm = hIconSm;
            } else {
                // Fallback to default if icon loading fails
                wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
                wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
            }
            
            return RegisterClassEx(&wc);
        };

    static const auto windowClassId = std::invoke(registerWindowClassFunction);

    if (!windowClassId)
        THEENGINE_LOG_THROW_ERROR("RegisterClassEx failed.");

    RECT rc{ 0,0,m_size.width, m_size.height };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, false);

    m_handle = CreateWindowEx(
        0, MAKEINTATOM(windowClassId), L"TheEngine",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!m_handle)
        THEENGINE_LOG_THROW_ERROR("CreateWindowEx failed.");

    // Set global instance
    g_windowInstance = this;
    applyWindowIcon(static_cast<HWND>(m_handle));
    Input::getInstance().setWindowHandle(static_cast<HWND>(m_handle));
    ShowWindow(static_cast<HWND>(m_handle), SW_SHOW);
}

TheEngine::Window::~Window()
{
    DestroyWindow(static_cast<HWND>(m_handle));
    g_windowInstance = nullptr;
}

LRESULT TheEngine::Window::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
#endif
