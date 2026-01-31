#if defined(_WIN32)
#include <TheEngine/Game/Game.h>
#include <Windows.h>

void TheEngine::Game::run()
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

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		onInternalUpdate();
	}
}
#endif