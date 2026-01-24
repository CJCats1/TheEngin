/*MIT License

C++ 3D Game Tutorial Series (https://github.com/PardCode/CPP-3D-Game-Tutorial-Series)

Copyright (c) 2019-2025, PardCode

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#if defined(_WIN32)
#include <Windows.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#endif

dx3d::Display::Display(const DisplayDesc& desc): Window(desc.window)
{
	m_swapChain = desc.graphicsDevice.createSwapChain({ m_handle, m_size });
}

dx3d::IRenderSwapChain& dx3d::Display::getSwapChain() noexcept
{
	return *m_swapChain;
}

#if defined(_WIN32)
LRESULT dx3d::Display::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Call base class handler first
	LRESULT result = Window::handleMessage(hwnd, msg, wParam, lParam);

	// Handle window resize for swap chain
	if (msg == WM_SIZE && wParam != SIZE_MINIMIZED)
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);
		if (width > 0 && height > 0)
		{
			// Resize swap chain
			m_swapChain->resize(width, height);
			
			// Update GraphicsEngine static window size variables
			GraphicsEngine::setWindowWidth(static_cast<float>(width));
			GraphicsEngine::setWindowHeight(static_cast<float>(height));
			
			// Update ImGui display size
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		}
	}

	return result;
}
#endif