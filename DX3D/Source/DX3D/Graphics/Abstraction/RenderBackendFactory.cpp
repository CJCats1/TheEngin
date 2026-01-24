#include <DX3D/Graphics/Abstraction/RenderBackendFactory.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/DirectX11/DirectX11RenderBackend.h>
#include <DX3D/Graphics/OpenGL/OpenGLRenderBackend.h>

using namespace dx3d;

std::unique_ptr<IRenderBackend> dx3d::createRenderBackend(RenderBackendType type, Logger& logger)
{
	switch (type)
	{
	case RenderBackendType::DirectX11:
		return std::make_unique<DirectX11RenderBackend>(logger);
	case RenderBackendType::OpenGL:
#if defined(DX3D_ENABLE_OPENGL)
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
