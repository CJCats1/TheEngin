#pragma once
#include <TheEngine/Graphics/Abstraction/RenderDevice.h>
#include <TheEngine/Core/Logger.h>

namespace TheEngine
{
	class OpenGLESDevice final : public IRenderDevice
	{
	public:
		explicit OpenGLESDevice(const GraphicsDeviceDesc& desc);

		SwapChainPtr createSwapChain(const SwapChainDesc& desc) override;
		DeviceContextPtr createDeviceContext() override;
		ShaderBinaryPtr compileShader(const ShaderCompileDesc& desc) override;
		GraphicsPipelineStatePtr createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
		VertexBufferPtr createVertexBuffer(const VertexBufferDesc& desc) override;
		VertexShaderSignaturePtr createVertexShaderSignature(const VertexShaderSignatureDesc& desc) override;
		IndexBufferPtr createIndexBuffer(const IndexBufferDesc& desc) override;
		void executeCommandList(IRenderContext& context) override;
		NativeGraphicsHandle getNativeDeviceHandle() const noexcept override;

	private:
		Logger& m_logger;
	};
}
