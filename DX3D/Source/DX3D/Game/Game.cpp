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

#include <DX3D/Game/Game.h>
#if !defined(DX3D_PLATFORM_ANDROID)
#include <DX3D/Graphics/OpenGL/OpenGLSwapChain.h>
#endif
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/Input.h>
#if defined(_WIN32)
#include <DX3D/Graphics/DirectWriteText.h>
#endif
#include <DX3D/Game/Scenes/TestScenes/TestScene.h>
#include <DX3D/Game/Scenes/TestScenes/ThreeDTestScene.h>
#include <DX3D/Core/EntityManager.h>

// Define the static Game instance
namespace dx3d {
    Game* Game::s_currentInstance = nullptr;
}
// ImGui/ImPlot
#include <imgui.h>
#include <imgui_internal.h>
#if defined(_WIN32)
#include <implot.h>
#endif
#if defined(_WIN32)
// Windows DPI functions
#include <windows.h>
#endif

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    // Set the static reference to this Game instance
    s_currentInstance = this;

    // Update GraphicsEngine static window dimensions
    float setWidth = static_cast<float>(desc.windowSize.width);
    float setHeight = static_cast<float>(desc.windowSize.height);
    GraphicsEngine::setWindowWidth(setWidth);
    GraphicsEngine::setWindowHeight(setHeight);
    
    #if defined(DX3D_PLATFORM_ANDROID)
    #include <android/log.h>
    __android_log_print(ANDROID_LOG_INFO, "Game", "Set window dims to %fx%f, current: %fx%f",
                       setWidth, setHeight, GraphicsEngine::getWindowWidth(), GraphicsEngine::getWindowHeight());
    #endif

    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    WindowDesc windowDesc{ {m_logger}, desc.windowSize };
    if (m_graphicsEngine->getBackendType() == RenderBackendType::OpenGL)
    {
        windowDesc.createNativeWindow = false;
    }
    m_display = std::make_unique<Display>(DisplayDesc{ windowDesc, m_graphicsEngine->getGraphicsDevice() });
    m_graphicsEngine->initializePipelines();

    m_lastFrameTime = std::chrono::steady_clock::now();
    setScene(std::make_unique<dx3d::TestScene>());
    m_currentSceneType = SceneType::None;

    // Initialize ImGui (DX11 + Win32)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking and viewports for drag-out functionality
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    if (m_graphicsEngine->getBackendType() == RenderBackendType::OpenGL)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }

    // Configure viewports properly
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;

#if defined(_WIN32)
    ImPlot::CreateContext();
#endif
    void* imguiWindowHandle = m_display->getHandle();
    if (m_graphicsEngine->getBackendType() == RenderBackendType::OpenGL)
    {
#if defined(DX3D_PLATFORM_ANDROID)
        imguiWindowHandle = m_display->getHandle();
#else
        imguiWindowHandle = m_display->getSwapChain().getNativeSwapChainHandle();
#endif
    }
    m_graphicsEngine->getImGuiBackend().initialize(
        imguiWindowHandle,
        m_graphicsEngine->getGraphicsDevice(),
        m_graphicsEngine->getContext()
    );


    // Apply dark style
    ImGui::StyleColorsDark();

#if defined(DX3D_PLATFORM_ANDROID)
    // Scale up fonts for Android to make text more readable on mobile screens
    io.FontGlobalScale = 2.5f; // Increase font size by 2.5x for better readability
    // Also scale up the UI elements
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2.0f); // Scale all UI elements (padding, spacing, etc.) by 2x
#endif

    DX3DLogInfo("Game initialized (ImGui ready with fonts).");
}

dx3d::Game::~Game()
{
    // Clear the static reference
    s_currentInstance = nullptr;
    
    // Shutdown ImGui/ImPlot
#if defined(_WIN32)
    ImPlot::DestroyContext();
#endif
    m_graphicsEngine->getImGuiBackend().shutdown();
    ImGui::DestroyContext();
    DX3DLogInfo("Game deallocation started.");
}

const float fixedStep = 1.0f / 60.0f;
float accumulator = 0.0f;
float imguiRebuildDelay = 0.0f;
const float imguiRebuildInterval = 15.0f; // seconds
void dx3d::Game::onInternalUpdate()
{
    if (m_graphicsEngine->getBackendType() == RenderBackendType::OpenGL)
    {
#if !defined(DX3D_PLATFORM_ANDROID)
    if (auto* glSwapChain = dynamic_cast<OpenGLSwapChain*>(&m_display->getSwapChain()))
    {
        glSwapChain->pollInput();
    }
#endif
    }
    auto& input = Input::getInstance();
    if (input.isKeyDown(Key::Escape))
    {
        m_isRunning = false;
        return;
    }
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - m_lastFrameTime;
    m_lastFrameTime = now;
    float frameTime = elapsed.count();

    // Clamp frameTime just in case
    if (frameTime > 0.25f) frameTime = 0.25f;

    accumulator += frameTime;

    while (accumulator >= fixedStep)
    {
        if (m_activeScene)
        {
            // Scene-specific fixed update logic
            m_activeScene->fixedUpdate(fixedStep);
        }
        accumulator -= fixedStep;
    }
	//imguiRebuildDelay += frameTime;
    //while (imguiRebuildDelay >= imguiRebuildInterval)
    //{
    //    imguiRebuildDelay -= imguiRebuildInterval;
	//	imguiRebuild = true;
    //}
    // Start ImGui frame
    m_graphicsEngine->getImGuiBackend().newFrame();

    // CRITICAL: Ensure display size is correct every frame
    ImGuiIO& io = ImGui::GetIO();

    ImGui::NewFrame();

    // Note: Removed runtime atlas modification to prevent access violations
    // The font atlas should be properly sized during initialization

    m_activeScene->update(frameTime);

    // Render once per frame
    if (m_activeScene)
    {
        // Centralized frame begin/end: start frame here
        m_graphicsEngine->beginFrame(m_display->getSwapChain());
        m_activeScene->render(*m_graphicsEngine, m_display->getSwapChain());
        // Per-scene ImGui UI
        m_activeScene->renderImGui(*m_graphicsEngine);
    }
    else
    {
        m_graphicsEngine->beginFrame(m_display->getSwapChain());
    }

    // Render ImGui on top
    m_graphicsEngine->getContext().clearShaderResourceBindings();
    ImGui::Render();
    m_graphicsEngine->getImGuiBackend().renderDrawData();

    // Handle multi-viewport rendering for drag-out functionality
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        m_graphicsEngine->getImGuiBackend().renderPlatformWindows();
    }

    // Present after ImGui so UI is visible
    m_graphicsEngine->endFrame(m_display->getSwapChain());

    input.update();
    //if(true)
    if(imguiRebuild)
    {
        ImguiRebuild();
        imguiRebuild = false;
    }
}
void dx3d::Game::ImguiRebuild()
{
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    io.Fonts->AddFontDefault();

    // Recreate backend resources for the new atlas
    m_graphicsEngine->getImGuiBackend().invalidateDeviceObjects();
    m_graphicsEngine->getImGuiBackend().createDeviceObjects();
}
void dx3d::Game::setScene(std::unique_ptr<Scene> scene)
{
    m_activeScene = std::move(scene);
    imguiRebuild = true;
    if (m_activeScene)
    {
        m_activeScene->load(*m_graphicsEngine);
    }
}

void dx3d::Game::onKeyDown(int keyCode)
{
    // Simply forward to input system - no scene-specific logic needed here
    Input::getInstance().setKeyDown(keyCode);
}

void dx3d::Game::onKeyUp(int keyCode)
{
    // Simply forward to input system - no scene-specific logic needed here
    Input::getInstance().setKeyUp(keyCode);
}

void dx3d::Game::TriggerImguiRebuild()
{
    if (s_currentInstance) {
        s_currentInstance->SetImguiRebuild(true);
    }
}
