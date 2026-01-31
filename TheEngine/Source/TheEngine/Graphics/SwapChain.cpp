#include <TheEngine/Graphics/SwapChain.h>

TheEngine::SwapChain::SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc) :
	GraphicsResource(gDesc), m_size(desc.winSize)
{
	if (!desc.winHandle) THEENGINE_LOG_THROW_INVALID_ARG("No window handle provided.");

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

	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_factory.CreateSwapChain(&m_device, &dxgiDesc, &m_swapChain),
		"CreateSwapChain failed.");

	reloadBuffers();
}

TheEngine::Rect TheEngine::SwapChain::getSize() const noexcept
{
	return m_size;
}

void TheEngine::SwapChain::present(bool vsync)
{
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_swapChain->Present(vsync, 0),
		"Present failed.");
}

void TheEngine::SwapChain::resize(int width, int height)
{
	if (width <= 0 || height <= 0)
		return;

	// Release existing render target view before resizing
	m_rtv.Reset();
	m_depthBuffer.Reset();
	m_dsv.Reset();

	// Resize the swap chain buffers
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(
		m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
		"ResizeBuffers failed."
	);

	// Update size
	m_size.width = width;
	m_size.height = height;

	// Reload buffers with new size
	reloadBuffers();
}

void TheEngine::SwapChain::reloadBuffers()
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer{};
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer)),
		"GetBuffer failed.");
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateRenderTargetView(buffer.Get(), nullptr, &m_rtv),
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

	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(
		m_device.CreateTexture2D(&depthDesc, nullptr, m_depthBuffer.GetAddressOf()),
		"CreateTexture2D for depth buffer failed."
	);
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(
		m_device.CreateDepthStencilView(m_depthBuffer.Get(), nullptr, m_dsv.GetAddressOf()),
		"CreateDepthStencilView failed."
	);
}