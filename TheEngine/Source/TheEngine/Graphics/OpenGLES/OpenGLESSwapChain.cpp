#include <TheEngine/Graphics/OpenGLES/OpenGLESSwapChain.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <android/native_window.h>
#include <EGL/egl.h>

using namespace TheEngine;

OpenGLESSwapChain::OpenGLESSwapChain(const SwapChainDesc& desc, Logger& logger)
	: m_logger(logger), m_size(desc.winSize)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	auto* window = static_cast<ANativeWindow*>(desc.winHandle);
	if (!window)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "ANativeWindow is null.");
	}

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglGetDisplay failed.");
	}

	if (eglInitialize(display, nullptr, nullptr) == EGL_FALSE)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglInitialize failed.");
	}

	const EGLint configAttribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	EGLConfig config = nullptr;
	EGLint numConfigs = 0;
	if (eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) == EGL_FALSE || numConfigs == 0)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglChooseConfig failed.");
	}

	EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
	if (surface == EGL_NO_SURFACE)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglCreateWindowSurface failed.");
	}

	const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	if (context == EGL_NO_CONTEXT)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglCreateContext failed.");
	}

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error, "eglMakeCurrent failed.");
	}

	m_windowHandle = window;
	m_display = display;
	m_surface = surface;
	m_context = context;
	m_size.width = ANativeWindow_getWidth(window);
	m_size.height = ANativeWindow_getHeight(window);
#else
	(void)desc;
	(void)logger;
#endif
}

OpenGLESSwapChain::~OpenGLESSwapChain()
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	if (m_display)
	{
		EGLDisplay display = static_cast<EGLDisplay>(m_display);
		EGLSurface surface = static_cast<EGLSurface>(m_surface);
		EGLContext context = static_cast<EGLContext>(m_context);
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (surface)
		{
			eglDestroySurface(display, surface);
		}
		if (context)
		{
			eglDestroyContext(display, context);
		}
		eglTerminate(display);
	}
#endif
}

Rect OpenGLESSwapChain::getSize() const noexcept
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	if (m_windowHandle)
	{
		auto* window = static_cast<ANativeWindow*>(m_windowHandle);
		return Rect(ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
	}
#endif
	return m_size;
}

void OpenGLESSwapChain::present(bool vsync)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	if (m_display && m_surface)
	{
		EGLDisplay display = static_cast<EGLDisplay>(m_display);
		eglSwapInterval(display, vsync ? 1 : 0);
		eglSwapBuffers(display, static_cast<EGLSurface>(m_surface));
	}
#else
	(void)vsync;
#endif
}

void OpenGLESSwapChain::resize(int width, int height)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	m_size.width = width;
	m_size.height = height;
#else
	(void)width;
	(void)height;
#endif
}

NativeGraphicsHandle OpenGLESSwapChain::getNativeSwapChainHandle() const noexcept
{
	return m_surface;
}

void* OpenGLESSwapChain::getWindowHandle() const noexcept
{
	return m_windowHandle;
}
