#include <TheEngine/Graphics/GraphicsDevice.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <TheEngine/Graphics/SwapChain.h>
#include <TheEngine/Graphics/DeviceContext.h>
#include <TheEngine/Graphics/ShaderBinary.h>
#include <TheEngine/Graphics/GraphicsPipelineState.h>
#include <TheEngine/Graphics/VertexBuffer.h>
#include <TheEngine/Graphics/VertexShaderSignature.h>
#include <TheEngine/Graphics/IndexBuffer.h>

using namespace TheEngine;

TheEngine::GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) : Base(desc.base)
{
	D3D_FEATURE_LEVEL featureLevel{};
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	//creating device
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		NULL, 0, D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_d3dContext), "Direct3D11 initialization failed.");
	//retrieving IDXGIDevice with queryinterface
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_d3dDevice->QueryInterface(IID_PPV_ARGS(&m_dxgiDevice)),
		"QueryInterface failed to retrieve IDXGIDevice");
	//getting IDXGIAdapter which is the parent of the m_dxgiDevice
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_dxgiDevice->GetParent(IID_PPV_ARGS(&m_dxgiAdapter)),
		"GetParent failed to retrieve IDXGIAdapter.");
	//getting the IDXGIFactory which is the parent of the m_dxgiAdapter
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_dxgiAdapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory)),
		"GetParent failed to retrieve IDXGIFactory");
}

TheEngine::GraphicsDevice::~GraphicsDevice()
{
}

SwapChainPtr TheEngine::GraphicsDevice::createSwapChain(const SwapChainDesc& desc)
{
	return std::make_shared<SwapChain>(desc, getGraphicsResourceDesc());
}

DeviceContextPtr TheEngine::GraphicsDevice::createDeviceContext()
{
	return std::make_shared<DeviceContext>(getGraphicsResourceDesc());
}

ShaderBinaryPtr TheEngine::GraphicsDevice::compileShader(const ShaderCompileDesc& desc)
{
	return std::make_shared<ShaderBinary>(desc, getGraphicsResourceDesc());
}

GraphicsPipelineStatePtr TheEngine::GraphicsDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	return std::make_shared<GraphicsPipelineState>(desc, getGraphicsResourceDesc());
}

VertexBufferPtr TheEngine::GraphicsDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
	return std::make_shared<VertexBuffer>(desc, getGraphicsResourceDesc());
}

VertexShaderSignaturePtr TheEngine::GraphicsDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
{
	return std::make_shared<VertexShaderSignature>(desc, getGraphicsResourceDesc());
}
IndexBufferPtr GraphicsDevice::createIndexBuffer(const IndexBufferDesc& d)
{
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = d.count * d.stride;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = d.data;

	Microsoft::WRL::ComPtr<ID3D11Buffer> buf;
	const HRESULT hr = m_d3dDevice->CreateBuffer(&bd, &init, &buf);
	if (FAILED(hr)) {
		THEENGINE_LOG_THROW_ERROR("CreateBuffer (IndexBuffer) failed.");
	}

	return std::make_shared<IndexBuffer>(buf.Get(), d.count, d.stride);
}
void TheEngine::GraphicsDevice::executeCommandList(IRenderContext& context)
{
	auto& dxContext = static_cast<DeviceContext&>(context);
	Microsoft::WRL::ComPtr<ID3D11CommandList> list{};
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(dxContext.m_context->FinishCommandList(false, &list)
		, "FinishCommandList failed");
	m_d3dContext->ExecuteCommandList(list.Get(),false);
}

GraphicsResourceDesc TheEngine::GraphicsDevice::getGraphicsResourceDesc() const noexcept
{
	return { {m_logger},shared_from_this(),*m_d3dDevice.Get(),*m_dxgiFactory.Get() };
}

