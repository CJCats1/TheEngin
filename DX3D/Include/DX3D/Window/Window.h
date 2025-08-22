#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Common.h>

namespace dx3d
{
    class Window : public Base
    {
    public:
        explicit Window(const WindowDesc& desc);
        virtual ~Window() override;

        // Called from global WndProc
        LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        inline HWND getHandle() const { return static_cast<HWND>(m_handle); }
        inline Rect getSize() const { return m_size; }

    protected:
        void* m_handle{};  // HWND stored as void*
        Rect m_size{};
    };
}
