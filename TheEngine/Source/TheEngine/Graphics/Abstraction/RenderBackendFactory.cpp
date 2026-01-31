#include <TheEngine/Graphics/Abstraction/RenderBackendFactory.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#if defined(_WIN32)
#include <TheEngine/Graphics/DirectX11/DirectX11RenderBackend.h>
#endif
#include <TheEngine/Graphics/OpenGL/OpenGLRenderBackend.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESRenderBackend.h>

using namespace TheEngine;

std::unique_ptr<IRenderBackend> TheEngine::createRenderBackend(RenderBackendType type, Logger& logger)
{
	switch (type)
	{
	case RenderBackendType::DirectX11:
#if defined(_WIN32)
		return std::make_unique<DirectX11RenderBackend>(logger);
#else
		THEENGINE_LOG_THROW(logger, std::runtime_error, Logger::LogLevel::Error,
			"DirectX11 backend not supported on this platform.");
		return nullptr;
#endif
	case RenderBackendType::OpenGL:
#if defined(THEENGINE_PLATFORM_ANDROID)
		return std::make_unique<OpenGLESRenderBackend>(logger);
#elif defined(THEENGINE_ENABLE_OPENGL)
		return std::make_unique<OpenGLRenderBackend>(logger);
#else
		THEENGINE_LOG_THROW(logger, std::runtime_error, Logger::LogLevel::Error,
			"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
		return nullptr;
#endif
	default:
		THEENGINE_LOG_THROW(logger, std::runtime_error, Logger::LogLevel::Error, "Unsupported render backend type.");
		return nullptr;
	}
}
