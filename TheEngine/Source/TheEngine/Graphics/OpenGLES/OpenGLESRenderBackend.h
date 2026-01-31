#pragma once
#include <TheEngine/Graphics/Abstraction/RenderBackend.h>
#include <TheEngine/Core/Logger.h>

namespace TheEngine
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
