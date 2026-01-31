#pragma once
#include <TheEngine/Graphics/Abstraction/RenderSwapChain.h>

namespace TheEngine
{
	class OpenGLSwapChain final : public IRenderSwapChain
	{
	public:
		OpenGLSwapChain(const SwapChainDesc& desc, Logger& logger);
		~OpenGLSwapChain();

		Rect getSize() const noexcept override;
		void present(bool vsync = false) override;
		void resize(int width, int height) override;
		void handleFramebufferResize(int width, int height);
		void pollInput();
		NativeGraphicsHandle getNativeSwapChainHandle() const noexcept override;
		void* getWindowHandle() const noexcept;
	private:
		Logger& m_logger;
		Rect m_size{};
		void* m_windowHandle{};
	};
}
