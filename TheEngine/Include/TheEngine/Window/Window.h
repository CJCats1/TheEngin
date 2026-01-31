#pragma once
#include <TheEngine/Core\Base.h>
#include <TheEngine/Core\Common.h>

#if defined(_WIN32)
#include <Windows.h>
#else
struct ANativeWindow;
#endif

namespace TheEngine
{
#if defined(_WIN32)
    using NativeWindowHandle = HWND;
#else
    using NativeWindowHandle = ANativeWindow*;
#endif

    class Window : public Base
    {
    public:
        explicit Window(const WindowDesc& desc);
        virtual ~Window() override;

#if defined(_WIN32)
        // Called from global WndProc
        virtual LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

        inline NativeWindowHandle getHandle() const { return static_cast<NativeWindowHandle>(m_handle); }
        inline Rect getSize() const { return m_size; }

    protected:
        void* m_handle{};  // HWND stored as void*
        Rect m_size{};
    };
}
