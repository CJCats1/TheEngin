#include <DX3D/Graphics/OpenGL/OpenGLSwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <imgui.h>
#include <Windows.h>
#include <string>

#if defined(DX3D_ENABLE_OPENGL)
#include <glad/glad.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

using namespace dx3d;

#if defined(DX3D_ENABLE_OPENGL)
namespace
{
	std::wstring buildIconPath()
	{
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		std::wstring exeDir = exePath;
		size_t lastSlash = exeDir.find_last_of(L"\\/");
		if (lastSlash != std::wstring::npos)
		{
			exeDir = exeDir.substr(0, lastSlash + 1);
		}
		return exeDir + L"..\\..\\..\\DX3D\\Assets\\Textures\\CheckEngine.ico";
	}

	void applyWindowIcon(HWND hwnd)
	{
		const std::wstring iconPath = buildIconPath();
		HICON hIcon = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE);
		HICON hIconSm = (HICON)LoadImageW(nullptr, iconPath.c_str(), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);
		if (hIcon)
		{
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
		if (hIconSm)
		{
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
		}
	}

	void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
	{
		auto* swapChain = static_cast<OpenGLSwapChain*>(glfwGetWindowUserPointer(window));
		if (swapChain)
		{
			swapChain->handleFramebufferResize(width, height);
		}
	}

	void glfwWindowCloseCallback(GLFWwindow* window)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	void syncKey(GLFWwindow* window, int glfwKey, int vkKey)
	{
		if (glfwGetKey(window, glfwKey) == GLFW_PRESS)
		{
			Input::getInstance().setKeyDown(vkKey);
		}
		else
		{
			Input::getInstance().setKeyUp(vkKey);
		}
	}

	void syncMouse(GLFWwindow* window, int glfwButton, MouseClick button)
	{
		if (glfwGetMouseButton(window, glfwButton) == GLFW_PRESS)
		{
			Input::getInstance().setMouseDown(button);
		}
		else
		{
			Input::getInstance().setMouseUp(button);
		}
	}

	void syncInput(GLFWwindow* window)
	{
		syncKey(window, GLFW_KEY_W, 'W');
		syncKey(window, GLFW_KEY_A, 'A');
		syncKey(window, GLFW_KEY_S, 'S');
		syncKey(window, GLFW_KEY_D, 'D');
		syncKey(window, GLFW_KEY_Q, 'Q');
		syncKey(window, GLFW_KEY_E, 'E');
		syncKey(window, GLFW_KEY_R, 'R');
		syncKey(window, GLFW_KEY_SPACE, VK_SPACE);
		syncKey(window, GLFW_KEY_LEFT, VK_LEFT);
		syncKey(window, GLFW_KEY_RIGHT, VK_RIGHT);
		syncKey(window, GLFW_KEY_UP, VK_UP);
		syncKey(window, GLFW_KEY_DOWN, VK_DOWN);
		syncKey(window, GLFW_KEY_LEFT_SHIFT, VK_SHIFT);
		syncKey(window, GLFW_KEY_RIGHT_SHIFT, VK_SHIFT);
		syncKey(window, GLFW_KEY_ESCAPE, VK_ESCAPE);

		syncMouse(window, GLFW_MOUSE_BUTTON_LEFT, MouseClick::LeftMouse);
		syncMouse(window, GLFW_MOUSE_BUTTON_RIGHT, MouseClick::RightMouse);
		syncMouse(window, GLFW_MOUSE_BUTTON_MIDDLE, MouseClick::MiddleMouse);
	}
}
#endif

OpenGLSwapChain::OpenGLSwapChain(const SwapChainDesc& desc, Logger& logger)
	: m_logger(logger), m_size(desc.winSize)
{
#if defined(DX3D_ENABLE_OPENGL)
	if (!glfwInit())
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error, "GLFW init failed.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(
		std::max(1, desc.winSize.width),
		std::max(1, desc.winSize.height),
		"TheEngine",
		nullptr,
		nullptr
	);
	if (!window)
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error, "GLFW window creation failed.");
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error, "OpenGL loader init failed.");
	}
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, glfwFramebufferSizeCallback);
	glfwSetWindowCloseCallback(window, glfwWindowCloseCallback);
	const HWND hwnd = glfwGetWin32Window(window);
	Input::getInstance().setWindowHandle(hwnd);
	applyWindowIcon(hwnd);
	handleFramebufferResize(desc.winSize.width, desc.winSize.height);
	m_windowHandle = window;
#else
	(void)desc;
	(void)logger;
#endif
}

OpenGLSwapChain::~OpenGLSwapChain()
{
#if defined(DX3D_ENABLE_OPENGL)
	if (m_windowHandle)
	{
		glfwDestroyWindow(static_cast<GLFWwindow*>(m_windowHandle));
		m_windowHandle = nullptr;
	}
	glfwTerminate();
#endif
}

Rect OpenGLSwapChain::getSize() const noexcept
{
#if defined(DX3D_ENABLE_OPENGL)
	if (m_windowHandle)
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_windowHandle), &w, &h);
		return Rect(w, h);
	}
#endif
	return m_size;
}

void OpenGLSwapChain::present(bool vsync)
{
#if defined(DX3D_ENABLE_OPENGL)
	if (m_windowHandle)
	{
		glfwSwapInterval(vsync ? 1 : 0);
		glfwSwapBuffers(static_cast<GLFWwindow*>(m_windowHandle));
	}
#else
	(void)vsync;
#endif
}

void OpenGLSwapChain::resize(int width, int height)
{
#if defined(DX3D_ENABLE_OPENGL)
	if (m_windowHandle)
	{
		glfwSetWindowSize(static_cast<GLFWwindow*>(m_windowHandle), width, height);
		m_size.width = width;
		m_size.height = height;
	}
#else
	(void)width;
	(void)height;
#endif
}

void OpenGLSwapChain::handleFramebufferResize(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return;
	}
	m_size.width = width;
	m_size.height = height;
	GraphicsEngine::setWindowWidth(static_cast<float>(width));
	GraphicsEngine::setWindowHeight(static_cast<float>(height));
	if (ImGui::GetCurrentContext())
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	}
}

void OpenGLSwapChain::pollInput()
{
#if defined(DX3D_ENABLE_OPENGL)
	if (m_windowHandle)
	{
		glfwPollEvents();
		syncInput(static_cast<GLFWwindow*>(m_windowHandle));
		if (glfwWindowShouldClose(static_cast<GLFWwindow*>(m_windowHandle)))
		{
			PostQuitMessage(0);
		}
	}
#endif
}

NativeGraphicsHandle OpenGLSwapChain::getNativeSwapChainHandle() const noexcept
{
	return m_windowHandle;
}

void* OpenGLSwapChain::getWindowHandle() const noexcept
{
	return m_windowHandle;
}
