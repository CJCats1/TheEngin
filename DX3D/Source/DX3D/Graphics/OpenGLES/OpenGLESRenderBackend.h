#pragma once
#include <DX3D/Graphics/Abstraction/RenderBackend.h>
#include <DX3D/Core/Logger.h>

namespace dx3d
{
	class OpenGLESRenderBackend final : public IRenderBackend
	{
	public:
		explicit OpenGLESRenderBackend(Logger& logger);
		RenderDevicePtr createDevice(const GraphicsDeviceDesc& desc) override;
		std::unique_ptr<IImGuiBackend> createImGuiBackend() override;

	private:
		Logger& m_logger;
	};
}
