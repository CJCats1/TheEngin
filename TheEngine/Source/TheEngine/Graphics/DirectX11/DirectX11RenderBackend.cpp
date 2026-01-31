#include <TheEngine/Graphics/DirectX11/DirectX11RenderBackend.h>
#include <TheEngine/Graphics/GraphicsDevice.h>
#include <TheEngine/Graphics/DeviceContext.h>
#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

using namespace TheEngine;

namespace
{
	class D3D11ImGuiBackend final : public IImGuiBackend
	{
	public:
		void initialize(void* windowHandle, IRenderDevice& device, IRenderContext& context) override
		{
			IMGUI_CHECKVERSION();
			ImGuiIO& io = ImGui::GetIO();
			(void)io;
			ImGui_ImplWin32_Init(windowHandle);
			auto* d3dDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
			auto* d3dContext = static_cast<ID3D11DeviceContext*>(context.getNativeContextHandle());
			ImGui_ImplDX11_Init(d3dDevice, d3dContext);
		}

		void shutdown() override
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
		}

		void newFrame() override
		{
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
		}

		void renderDrawData() override
		{
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

		void invalidateDeviceObjects() override
		{
			ImGui_ImplDX11_InvalidateDeviceObjects();
		}

		void createDeviceObjects() override
		{
			ImGui_ImplDX11_CreateDeviceObjects();
		}

		void renderPlatformWindows() override
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	};
}

DirectX11RenderBackend::DirectX11RenderBackend(Logger& logger)
	: m_logger(logger)
{
}

RenderDevicePtr DirectX11RenderBackend::createDevice(const GraphicsDeviceDesc& desc)
{
	return std::make_shared<GraphicsDevice>(desc);
}

std::unique_ptr<IImGuiBackend> DirectX11RenderBackend::createImGuiBackend()
{
	return std::make_unique<D3D11ImGuiBackend>();
}
