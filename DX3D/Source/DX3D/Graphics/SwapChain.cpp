#include <DX3D/Graphics/SwapChain.h>

dx3d::SwapChain::SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc) :
	GraphicsResource(gDesc), m_size(desc.winSize)
{
	if (!desc.winHandle) DX3DLogThrowInvalidArg("No window handle provided.");

	DXGI_SWAP_CHAIN_DESC dxgiDesc{};

	dxgiDesc.BufferDesc.Width = std::max(1, desc.winSize.width);
	dxgiDesc.BufferDesc.Height = std::max(1, desc.winSize.height);
	dxgiDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiDesc.BufferCount = 2;
	dxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	dxgiDesc.OutputWindow = static_cast<HWND>(desc.winHandle);
	dxgiDesc.SampleDesc.Count = 1;
	dxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiDesc.Windowed = TRUE;

	DX3DGraphicsLogThrowOnFail(m_factory.CreateSwapChain(&m_device, &dxgiDesc, &m_swapChain),
		"CreateSwapChain failed.");

	reloadBuffers();
}

dx3d::Rect dx3d::SwapChain::getSize() const noexcept
{
	return m_size;
}

void dx3d::SwapChain::present(bool vsync)
{
	DX3DGraphicsLogThrowOnFail(m_swapChain->Present(vsync, 0),
		"Present failed.");
}

void dx3d::SwapChain::reloadBuffers()
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer{};
	DX3DGraphicsLogThrowOnFail(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer)),
		"GetBuffer failed.");
	DX3DGraphicsLogThrowOnFail(m_device.CreateRenderTargetView(buffer.Get(), nullptr, &m_rtv),
		"CreateRenderTargetView failed.");

	// Create (or recreate) depth-stencil buffer and view matching backbuffer size
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = std::max(1, m_size.width);
	depthDesc.Height = std::max(1, m_size.height);
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	m_depthBuffer.Reset();
	m_dsv.Reset();

	DX3DGraphicsLogThrowOnFail(
		m_device.CreateTexture2D(&depthDesc, nullptr, m_depthBuffer.GetAddressOf()),
		"CreateTexture2D for depth buffer failed."
	);
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateDepthStencilView(m_depthBuffer.Get(), nullptr, m_dsv.GetAddressOf()),
		"CreateDepthStencilView failed."
	);
}