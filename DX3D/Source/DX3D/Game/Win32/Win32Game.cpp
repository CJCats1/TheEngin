#include <DX3D/Game/Game.h>
#include <Windows.h>

void dx3d::Game::run()
{
	MSG msg{};
	while (m_isRunning)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				m_isRunning = false;
				break;
			}

			// Handle keyboard input for camera controls
			if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
			{
				onKeyDown(static_cast<int>(msg.wParam));
			}
			else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
			{
				onKeyUp(static_cast<int>(msg.wParam));
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		onInternalUpdate();
	}
}