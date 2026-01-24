#include <DX3D/Graphics/OpenGL/OpenGLRenderBackend.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/OpenGL/OpenGLDevice.h>
#include <imgui.h>

#if defined(DX3D_ENABLE_OPENGL)
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <DX3D/Core/Input.h>
#endif

using namespace dx3d;

namespace
{
#if defined(DX3D_ENABLE_OPENGL)
	int mapGlfwKeyToWin32(int key)
	{
		if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
		{
			return key;
		}
		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
		{
			return key;
		}
		switch (key)
		{
		case GLFW_KEY_LEFT: return VK_LEFT;
		case GLFW_KEY_RIGHT: return VK_RIGHT;
		case GLFW_KEY_UP: return VK_UP;
		case GLFW_KEY_DOWN: return VK_DOWN;
		case GLFW_KEY_SPACE: return VK_SPACE;
		case GLFW_KEY_LEFT_SHIFT:
		case GLFW_KEY_RIGHT_SHIFT:
			return VK_SHIFT;
		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			return VK_CONTROL;
		case GLFW_KEY_LEFT_ALT:
		case GLFW_KEY_RIGHT_ALT:
			return VK_MENU;
		case GLFW_KEY_ESCAPE: return VK_ESCAPE;
		case GLFW_KEY_ENTER: return VK_RETURN;
		default:
			return 0;
		}
	}

	void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
		const int mapped = mapGlfwKeyToWin32(key);
		if (!mapped)
		{
			return;
		}
		if (action == GLFW_PRESS)
		{
			Input::getInstance().setKeyDown(mapped);
		}
		else if (action == GLFW_RELEASE)
		{
			Input::getInstance().setKeyUp(mapped);
		}
	}

	void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			action == GLFW_PRESS ? Input::getInstance().setMouseDown(MouseClick::LeftMouse)
				: Input::getInstance().setMouseUp(MouseClick::LeftMouse);
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			action == GLFW_PRESS ? Input::getInstance().setMouseDown(MouseClick::RightMouse)
				: Input::getInstance().setMouseUp(MouseClick::RightMouse);
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			action == GLFW_PRESS ? Input::getInstance().setMouseDown(MouseClick::MiddleMouse)
				: Input::getInstance().setMouseUp(MouseClick::MiddleMouse);
		}
	}

	void glfwCursorPosCallback(GLFWwindow* window, double x, double y)
	{
		ImGui_ImplGlfw_CursorPosCallback(window, x, y);
	}

	void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	}

	void glfwCharCallback(GLFWwindow* window, unsigned int c)
	{
		ImGui_ImplGlfw_CharCallback(window, c);
	}
#endif

	class OpenGLImGuiBackend final : public IImGuiBackend
	{
	public:
		void initialize(void* windowHandle, IRenderDevice& device, IRenderContext& context) override
		{
#if defined(DX3D_ENABLE_OPENGL)
			IMGUI_CHECKVERSION();
			auto* glfwWindow = static_cast<GLFWwindow*>(windowHandle);
			ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
			glfwSetKeyCallback(glfwWindow, glfwKeyCallback);
			glfwSetMouseButtonCallback(glfwWindow, glfwMouseButtonCallback);
			glfwSetCursorPosCallback(glfwWindow, glfwCursorPosCallback);
			glfwSetScrollCallback(glfwWindow, glfwScrollCallback);
			glfwSetCharCallback(glfwWindow, glfwCharCallback);
			ImGui_ImplOpenGL3_Init("#version 150");
#else
			(void)windowHandle;
			(void)device;
			(void)context;
			throw std::runtime_error("OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
#endif
		}

		void shutdown() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
#endif
		}

		void newFrame() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
#endif
		}

		void renderDrawData() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
		}

		void invalidateDeviceObjects() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui_ImplOpenGL3_DestroyDeviceObjects();
#endif
		}

		void createDeviceObjects() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui_ImplOpenGL3_CreateDeviceObjects();
#endif
		}

		void renderPlatformWindows() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
#endif
		}
	};
}

OpenGLRenderBackend::OpenGLRenderBackend(Logger& logger)
	: m_logger(logger)
{
}

RenderDevicePtr OpenGLRenderBackend::createDevice(const GraphicsDeviceDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLDevice>(desc);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

std::unique_ptr<IImGuiBackend> OpenGLRenderBackend::createImGuiBackend()
{
	return std::make_unique<OpenGLImGuiBackend>();
}
