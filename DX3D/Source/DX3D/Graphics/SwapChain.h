#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Graphics/Abstraction/RenderSwapChain.h>
namespace dx3d
{
	class SwapChain final : public GraphicsResource, public IRenderSwapChain
	{
	public:
		SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc);
		Rect getSize() const noexcept override;
		IDXGISwapChain* getD3DSwapChain() const noexcept { return m_swapChain.Get(); }
		NativeGraphicsHandle getNativeSwapChainHandle() const noexcept override { return m_swapChain.Get(); }

		void present(bool vsync = false) override;
		void resize(int width, int height) override;
	private:
		void reloadBuffers();
	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain{};
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv{};
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthBuffer{};
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv{};
		Rect m_size{};

		friend class DeviceContext;
	};
}