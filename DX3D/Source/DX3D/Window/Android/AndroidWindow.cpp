#include <DX3D/Window/Window.h>
#include <DX3D/Core/AndroidPlatform.h>

dx3d::Window::Window(const WindowDesc& desc) : Base(desc.base), m_size(desc.size)
{
    (void)desc;
    m_handle = dx3d::platform::getNativeWindow();
}

dx3d::Window::~Window()
{
}
