#pragma once
#if defined(_WIN32)
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <d3d11.h>
#include <wrl.h>

namespace dx3d
{

    class GraphicsDevice final : public Base, public IRenderDevice, public std::enable_shared_from_this<GraphicsDevice>
    {
    public:
        explicit GraphicsDevice(const GraphicsDeviceDesc& desc);
        virtual ~GraphicsDevice() override;

        SwapChainPtr              createSwapChain(const SwapChainDesc& desc) override;
        DeviceContextPtr          createDeviceContext() override;
        ShaderBinaryPtr           compileShader(const ShaderCompileDesc& desc) override;
        GraphicsPipelineStatePtr  createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
        VertexBufferPtr           createVertexBuffer(const VertexBufferDesc& desc) override;
        VertexShaderSignaturePtr  createVertexShaderSignature(const VertexShaderSignatureDesc& desc) override;

        IndexBufferPtr            createIndexBuffer(const IndexBufferDesc& desc) override;

        void executeCommandList(IRenderContext& context) override;
        ID3D11Device* getD3DDevice() const noexcept { return m_d3dDevice.Get(); }
        NativeGraphicsHandle getNativeDeviceHandle() const noexcept override { return m_d3dDevice.Get(); }
        GraphicsResourceDesc getGraphicsResourceDesc() const noexcept;
    private:

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>        m_d3dDevice{};
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext{};
        Microsoft::WRL::ComPtr<IDXGIDevice>         m_dxgiDevice{};
        Microsoft::WRL::ComPtr<IDXGIAdapter>        m_dxgiAdapter{};
        Microsoft::WRL::ComPtr<IDXGIFactory>        m_dxgiFactory{};
    };
}
#else
namespace dx3d
{
    class GraphicsDevice;
}
#endif