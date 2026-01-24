#pragma once
#include <DX3D/Graphics/Abstraction/RenderSwapChain.h>
#include <DX3D/Core/Logger.h>

struct ANativeWindow;

namespace dx3d
{
	class OpenGLESSwapChain final : public IRenderSwapChain
	{
	public:
		OpenGLESSwapChain(const SwapChainDesc& desc, Logger& logger);
		~OpenGLESSwapChain();

		Rect getSize() const noexcept override;
		void present(bool vsync = false) override;
		void resize(int width, int height) override;
		NativeGraphicsHandle getNativeSwapChainHandle() const noexcept override;
		void* getWindowHandle() const noexcept;

	private:
		Logger& m_logger;
		Rect m_size{};
		void* m_windowHandle{};
		void* m_display{};
		void* m_surface{};
		void* m_context{};
	};
}
