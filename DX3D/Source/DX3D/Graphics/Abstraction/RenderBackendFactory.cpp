#include <DX3D/Graphics/Abstraction/RenderBackendFactory.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#if defined(_WIN32)
#include <DX3D/Graphics/DirectX11/DirectX11RenderBackend.h>
#endif
#include <DX3D/Graphics/OpenGL/OpenGLRenderBackend.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESRenderBackend.h>

using namespace dx3d;

std::unique_ptr<IRenderBackend> dx3d::createRenderBackend(RenderBackendType type, Logger& logger)
{
	switch (type)
	{
	case RenderBackendType::DirectX11:
#if defined(_WIN32)
		return std::make_unique<DirectX11RenderBackend>(logger);
#else
		DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error,
			"DirectX11 backend not supported on this platform.");
		return nullptr;
#endif
	case RenderBackendType::OpenGL:
#if defined(DX3D_PLATFORM_ANDROID)
		return std::make_unique<OpenGLESRenderBackend>(logger);
#elif defined(DX3D_ENABLE_OPENGL)
		return std::make_unique<OpenGLRenderBackend>(logger);
#else
		DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error,
			"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
		return nullptr;
#endif
	default:
		DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error, "Unsupported render backend type.");
		return nullptr;
	}
}
