#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/GraphicsHandles.h>

namespace dx3d
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
