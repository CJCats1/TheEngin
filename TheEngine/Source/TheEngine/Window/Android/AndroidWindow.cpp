#include <TheEngine/Window/Window.h>
#include <TheEngine/Core/AndroidPlatform.h>

TheEngine::Window::Window(const WindowDesc& desc) : Base(desc.base), m_size(desc.size)
{
    (void)desc;
    m_handle = TheEngine::platform::getNativeWindow();
}

TheEngine::Window::~Window()
{
}
