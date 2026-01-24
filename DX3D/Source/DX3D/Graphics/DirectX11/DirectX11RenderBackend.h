#pragma once
#include <DX3D/Graphics/Abstraction/RenderBackend.h>

namespace dx3d
{
	class DirectX11RenderBackend final : public IRenderBackend
	{
	public:
		explicit DirectX11RenderBackend(Logger& logger);
		RenderDevicePtr createDevice(const GraphicsDeviceDesc& desc) override;
		std::unique_ptr<IImGuiBackend> createImGuiBackend() override;
	private:
		Logger& m_logger;
	};
}
