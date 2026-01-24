#if defined(DX3D_PLATFORM_ANDROID)
#include <DX3D/Game/Game.h>
#include <DX3D/Core/AndroidPlatform.h>
#include <android_native_app_glue.h>

void dx3d::Game::run()
{
	android_app* app = dx3d::platform::getAndroidApp();
	if (!app)
	{
		return;
	}

	while (m_isRunning)
	{
		int events = 0;
		android_poll_source* source = nullptr;
		while (ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0)
		{
			if (source)
			{
				source->process(app, source);
			}

			if (app->destroyRequested)
			{
				m_isRunning = false;
				break;
			}
		}

		if (!m_isRunning)
		{
			break;
		}
		onInternalUpdate();
	}
}
#endif
