#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/ShaderBinary.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/VertexShaderSignature.h>

using namespace dx3d;

dx3d::GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) : Base(desc.base)
{
	D3D_FEATURE_LEVEL featureLevel{};
	UINT createDeviceFlags{};
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	//creating device
	DX3DGraphicsLogThrowOnFail(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
		NULL, 0, D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_d3dContext), "Direct3D11 initialization failed.");
	//retrieving IDXGIDevice with queryinterface
	DX3DGraphicsLogThrowOnFail(m_d3dDevice->QueryInterface(IID_PPV_ARGS(&m_dxgiDevice)),
		"QueryInterface failed to retrieve IDXGIDevice");
	//getting IDXGIAdapter which is the parent of the m_dxgiDevice
	DX3DGraphicsLogThrowOnFail(m_dxgiDevice->GetParent(IID_PPV_ARGS(&m_dxgiAdapter)),
		"GetParent failed to retrieve IDXGIAdapter.");
	//getting the IDXGIFactory which is the parent of the m_dxgiAdapter
	DX3DGraphicsLogThrowOnFail(m_dxgiAdapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory)),
		"GetParent failed to retrieve IDXGIFactory");
}

dx3d::GraphicsDevice::~GraphicsDevice()
{
}

SwapChainPtr dx3d::GraphicsDevice::createSwapChain(const SwapChainDesc& desc)
{
	return std::make_shared<SwapChain>(desc, getGraphicsResourceDesc());
}

DeviceContextPtr dx3d::GraphicsDevice::createDeviceContext()
{
	return std::make_shared<DeviceContext>(getGraphicsResourceDesc());
}

ShaderBinaryPtr dx3d::GraphicsDevice::compileShader(const ShaderCompileDesc& desc)
{
	return std::make_shared<ShaderBinary>(desc, getGraphicsResourceDesc());
}

GraphicsPipelineStatePtr dx3d::GraphicsDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	return std::make_shared<GraphicsPipelineState>(desc, getGraphicsResourceDesc());
}

VertexBufferPtr dx3d::GraphicsDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
	return std::make_shared<VertexBuffer>(desc, getGraphicsResourceDesc());
}

VertexShaderSignaturePtr dx3d::GraphicsDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
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
		DX3DLogThrowError("CreateBuffer (IndexBuffer) failed.");
	}

	return std::make_shared<IndexBuffer>(buf.Get(), d.count, d.stride);
}
void dx3d::GraphicsDevice::executeCommandList(DeviceContext& context)
{
	Microsoft::WRL::ComPtr<ID3D11CommandList> list{};
	DX3DGraphicsLogThrowOnFail(context.m_context->FinishCommandList(false, &list)
		, "FinishCommandList failed");
	m_d3dContext->ExecuteCommandList(list.Get(),false);
}

GraphicsResourceDesc dx3d::GraphicsDevice::getGraphicsResourceDesc() const noexcept
{
	return { {m_logger},shared_from_this(),*m_d3dDevice.Get(),*m_dxgiFactory.Get() };
}

