#pragma once
#include <DX3D/Graphics/Abstraction/RenderDevice.h>

namespace dx3d
{
	class OpenGLDevice final : public IRenderDevice
	{
	public:
		explicit OpenGLDevice(const GraphicsDeviceDesc& desc);

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
