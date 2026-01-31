#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/GraphicsHandles.h>

namespace TheEngine
{
    class IndexBuffer;
    class IRenderContext;

    class IRenderDevice
    {
    public:
        virtual ~IRenderDevice() = default;

        virtual SwapChainPtr createSwapChain(const SwapChainDesc& desc) = 0;
        virtual DeviceContextPtr createDeviceContext() = 0;
        virtual ShaderBinaryPtr compileShader(const ShaderCompileDesc& desc) = 0;
        virtual GraphicsPipelineStatePtr createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) = 0;
        virtual VertexBufferPtr createVertexBuffer(const VertexBufferDesc& desc) = 0;
        virtual VertexShaderSignaturePtr createVertexShaderSignature(const VertexShaderSignatureDesc& desc) = 0;
        virtual IndexBufferPtr createIndexBuffer(const IndexBufferDesc& desc) = 0;

        virtual void executeCommandList(IRenderContext& context) = 0;
        virtual NativeGraphicsHandle getNativeDeviceHandle() const noexcept = 0;
    };

    using RenderDevicePtr = std::shared_ptr<IRenderDevice>;
}
