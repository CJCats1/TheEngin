#if defined(THEENGINE_PLATFORM_ANDROID)
#include <TheEngine/Game/Game.h>
#include <TheEngine/Core/AndroidPlatform.h>
#include <android_native_app_glue.h>

void TheEngine::Game::run()
{
	android_app* app = TheEngine::platform::getAndroidApp();
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
