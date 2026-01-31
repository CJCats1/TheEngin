#include <TheEngine/Graphics/OpenGLES/OpenGLESRenderBackend.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESDevice.h>
#include <imgui.h>

#if defined(THEENGINE_PLATFORM_ANDROID)
#include <android/native_window.h>
#include <backends/imgui_impl_android.h>
#include <backends/imgui_impl_opengl3.h>
#endif

using namespace TheEngine;

namespace
{
	class OpenGLESImGuiBackend final : public IImGuiBackend
	{
	public:
		void initialize(void* windowHandle, IRenderDevice& device, IRenderContext& context) override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			(void)device;
			(void)context;
			auto* nativeWindow = static_cast<ANativeWindow*>(windowHandle);
			ImGui_ImplAndroid_Init(nativeWindow);
			ImGui_ImplOpenGL3_Init("#version 300 es");
#else
			(void)windowHandle;
			(void)device;
			(void)context;
			throw std::runtime_error("OpenGLES backend only supported on Android.");
#endif
		}

		void shutdown() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplAndroid_Shutdown();
#endif
		}

		void newFrame() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplAndroid_NewFrame();
#endif
		}

		void renderDrawData() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
		}

		void invalidateDeviceObjects() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			ImGui_ImplOpenGL3_DestroyDeviceObjects();
#endif
		}

		void createDeviceObjects() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			ImGui_ImplOpenGL3_CreateDeviceObjects();
#endif
		}

		void renderPlatformWindows() override
		{
		}
	};
}

OpenGLESRenderBackend::OpenGLESRenderBackend(Logger& logger)
	: m_logger(logger)
{
}

RenderDevicePtr OpenGLESRenderBackend::createDevice(const GraphicsDeviceDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESDevice>(desc);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

std::unique_ptr<IImGuiBackend> OpenGLESRenderBackend::createImGuiBackend()
{
	return std::make_unique<OpenGLESImGuiBackend>();
}
