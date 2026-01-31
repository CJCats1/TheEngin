#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <TheEngine/Core/AndroidPlatform.h>
#include <TheEngine/Core/Input.h>
#include <TheEngine/Math/Geometry.h>
#include <TheEngine/Game/Game.h>
#include <backends/imgui_impl_android.h>
#include <android/input.h>

namespace
{
    constexpr const char* kLogTag = "TheEngine";

    int32_t onInputEvent(android_app* app, AInputEvent* event)
    {
        (void)app;
        
        // Always update our Input system first, regardless of ImGui
        const int32_t type = AInputEvent_getType(event);
        if (type == AINPUT_EVENT_TYPE_MOTION)
        {
            const int32_t action = AMotionEvent_getAction(event);
            const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
            const size_t pointerCount = AMotionEvent_getPointerCount(event);

            const float x = AMotionEvent_getX(event, 0);
            const float y = AMotionEvent_getY(event, 0);
            TheEngine::Input::getInstance().setMousePosition(TheEngine::Vec2(x, y));

            if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN)
            {
                if (pointerCount >= 2)
                {
                    TheEngine::Input::getInstance().setMouseDown(TheEngine::MouseClick::RightMouse);
                }
                else
                {
                    TheEngine::Input::getInstance().setMouseDown(TheEngine::MouseClick::LeftMouse);
                }
            }
            else if (masked == AMOTION_EVENT_ACTION_UP || masked == AMOTION_EVENT_ACTION_POINTER_UP)
            {
                if (pointerCount >= 2)
                {
                    TheEngine::Input::getInstance().setMouseUp(TheEngine::MouseClick::RightMouse);
                }
                else
                {
                    TheEngine::Input::getInstance().setMouseUp(TheEngine::MouseClick::LeftMouse);
                }
            }
            // MOVE events just update position (already done above)
        }
        else if (type == AINPUT_EVENT_TYPE_KEY)
        {
            const int32_t action = AKeyEvent_getAction(event);
            const int32_t keyCode = AKeyEvent_getKeyCode(event);
            if (action == AKEY_EVENT_ACTION_DOWN)
            {
                TheEngine::Input::getInstance().setKeyDown(keyCode);
            }
            else if (action == AKEY_EVENT_ACTION_UP)
            {
                TheEngine::Input::getInstance().setKeyUp(keyCode);
            }
        }
        
        // Then let ImGui handle the event (it will return true if it wants to capture)
        if (ImGui_ImplAndroid_HandleInputEvent(event))
        {
            return 1;
        }
        
        return 0;
    }

    void onAppCmd(android_app* app, int32_t cmd)
    {
        switch (cmd)
        {
        case APP_CMD_INIT_WINDOW:
            TheEngine::platform::setNativeWindow(app->window);
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Window initialized");
            break;
        case APP_CMD_TERM_WINDOW:
            TheEngine::platform::setNativeWindow(nullptr);
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Window terminated");
            break;
        case APP_CMD_GAINED_FOCUS:
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Gained focus");
            break;
        case APP_CMD_LOST_FOCUS:
            __android_log_print(ANDROID_LOG_INFO, kLogTag, "Lost focus");
            break;
        default:
            break;
        }
    }
}

void android_main(android_app* app)
{
    app->onAppCmd = onAppCmd;
    app->onInputEvent = onInputEvent;
    TheEngine::platform::setAssetManager(app->activity->assetManager);
    TheEngine::platform::setAndroidApp(app);

    __android_log_print(ANDROID_LOG_INFO, kLogTag, "TheEngine Android stub starting");

    while (!app->window)
    {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollAll(0, nullptr, &events,
            reinterpret_cast<void**>(&source)) >= 0)
        {
            if (source)
            {
                source->process(app, source);
            }

            if (app->destroyRequested)
            {
                __android_log_print(ANDROID_LOG_INFO, kLogTag, "Destroy requested before window");
                return;
            }
        }
    }

    TheEngine::platform::setNativeWindow(app->window);
    
    // Get actual window dimensions from ANativeWindow
    int windowWidth = ANativeWindow_getWidth(app->window);
    int windowHeight = ANativeWindow_getHeight(app->window);
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Window dimensions: %dx%d", windowWidth, windowHeight);
    
    TheEngine::Game game({ {static_cast<TheEngine::i32>(windowWidth), static_cast<TheEngine::i32>(windowHeight)}, TheEngine::Logger::LogLevel::Info });
    game.run();
}
