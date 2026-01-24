#pragma once
#include <DX3D/Graphics/Abstraction/RenderBackend.h>

namespace dx3d
{
	class OpenGLRenderBackend final : public IRenderBackend
	{
	public:
		explicit OpenGLRenderBackend(Logger& logger);
		RenderDevicePtr createDevice(const GraphicsDeviceDesc& desc) override;
		std::unique_ptr<IImGuiBackend> createImGuiBackend() override;
	private:
		Logger& m_logger;
	};
}
