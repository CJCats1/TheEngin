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
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Game/Scenes/BridgeScene.h>
#include <DX3D/Game/Scenes/SpiderSolitaireScene.h>
#include <DX3D/Game/Scenes/PhysicsTetrisScene.h>
#include <DX3D/Game/Scenes/JellyTetrisReduxScene.h>
#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Game/Scenes/ThreeDTestScene.h>
#include <DX3D/Game/Scenes/MarbleMazeScene.h>
#include <DX3D/Game/Scenes/FlipFluidSimulationScene.h>
#include <DX3D/Graphics/SwapChain.h>

// Define the static Game instance
namespace dx3d {
    Game* Game::s_currentInstance = nullptr;
}
// ImGui/ImPlot
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <implot.h>
// Windows DPI functions
#include <windows.h>

dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    // Set the static reference to this Game instance
    s_currentInstance = this;

    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getGraphicsDevice() });

    m_lastFrameTime = std::chrono::steady_clock::now();
    setScene(std::make_unique<dx3d::BridgeScene>());
    m_currentSceneType = SceneType::BridgeScene;

    // Initialize ImGui (DX11 + Win32)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable docking and viewports for drag-out functionality
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Configure viewports properly
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;

    ImPlot::CreateContext();
    ImGui_ImplWin32_Init(m_display->getHandle());
    ImGui_ImplDX11_Init(m_graphicsEngine->getGraphicsDevice().getD3DDevice(),
        m_graphicsEngine->getContext().getD3DDeviceContext());


    // Apply dark style
    ImGui::StyleColorsDark();

    DX3DLogInfo("Game initialized (ImGui ready with fonts).");
}

dx3d::Game::~Game()
{
    // Clear the static reference
    s_currentInstance = nullptr;
    
    // Shutdown ImGui/ImPlot
    ImPlot::DestroyContext();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    DX3DLogInfo("Game deallocation started.");
}

const float fixedStep = 1.0f / 60.0f;
float accumulator = 0.0f;
float imguiRebuildDelay = 0.0f;
const float imguiRebuildInterval = 15.0f; // seconds
void dx3d::Game::onInternalUpdate()
{
    auto& input = Input::getInstance();
    if (input.isKeyDown(Key::Num1) && m_currentSceneType != SceneType::TestScene)
    {
        setScene(std::make_unique<dx3d::TestScene>());
        m_currentSceneType = SceneType::TestScene;
    }
    if (input.isKeyDown(Key::Num2) && m_currentSceneType != SceneType::BridgeScene)
    {
        setScene(std::make_unique<dx3d::BridgeScene>());
        m_currentSceneType = SceneType::BridgeScene;
    }
    if (input.isKeyDown(Key::Num3) && m_currentSceneType != SceneType::SpiderSolitaireScene)
    {
        setScene(std::make_unique<dx3d::SpiderSolitaireScene>());
        m_currentSceneType = SceneType::SpiderSolitaireScene;
    }
    if (input.isKeyDown(Key::Num4) && m_currentSceneType != SceneType::PhysicsTetrisScene)
    {
        setScene(std::make_unique<dx3d::PhysicsTetrisScene>());
        m_currentSceneType = SceneType::PhysicsTetrisScene;
    }

    if (input.isKeyDown(Key::Num5) && m_currentSceneType != SceneType::PartitionScene)
    {
        setScene(std::make_unique<dx3d::PartitionScene>());
        m_currentSceneType = SceneType::PartitionScene;
    }
    if (input.isKeyDown(Key::Num6) && m_currentSceneType != SceneType::ThreeDTestScene)
    {
        setScene(std::make_unique<dx3d::ThreeDTestScene>());
        m_currentSceneType = SceneType::ThreeDTestScene;
    }
    if (input.isKeyDown(Key::Num7) && m_currentSceneType != SceneType::MarbleMazeScene)
    {
        setScene(std::make_unique<dx3d::MarbleMazeScene>());
        m_currentSceneType = SceneType::MarbleMazeScene;
    }
    if (input.isKeyDown(Key::Num8) && m_currentSceneType != SceneType::FlipFluidSimulationScene)
    {
        setScene(std::make_unique<dx3d::FlipFluidSimulationScene>());
        m_currentSceneType = SceneType::FlipFluidSimulationScene;
    }
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
            m_activeScene->fixedUpdate(fixedStep); // physics & logic at fixed rate
        accumulator -= fixedStep;
    }
	//imguiRebuildDelay += frameTime;
    //while (imguiRebuildDelay >= imguiRebuildInterval)
    //{
    //    imguiRebuildDelay -= imguiRebuildInterval;
	//	imguiRebuild = true;
    //}
    // Start ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

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

        // Global scene switcher window
        ImGui::SetNextWindowSize(ImVec2(260, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Scenes"))
        {
            int currentIndex = 0;
            switch (m_currentSceneType)
            {
            case SceneType::TestScene: currentIndex = 0; break;
            case SceneType::BridgeScene: currentIndex = 1; break;
            case SceneType::SpiderSolitaireScene: currentIndex = 2; break;
            case SceneType::PhysicsTetrisScene: currentIndex = 3; break;
            case SceneType::JellyTetrisReduxScene: currentIndex = 4; break;
            case SceneType::PartitionScene: currentIndex = 5; break;
            case SceneType::ThreeDTestScene: currentIndex = 6; break;
            case SceneType::MarbleMazeScene: currentIndex = 7; break;
            case SceneType::FlipFluidSimulationScene: currentIndex = 8; break;
            default: break;
            }

            const char* items[] = {
                "TestScene",
                "BridgeScene",
                "SpiderSolitaireScene",
                "PhysicsTetrisScene",
                "JellyTetrisReduxScene",
                "PartitionScene",
                "ThreeDTestScene",
                "MarbleMazeScene",
                "FlipFluidSimulationScene"
            };

            if (ImGui::ListBox("##SceneList", &currentIndex, items, IM_ARRAYSIZE(items), 9))
            {
                // Switch scene when selection changes
                switch (currentIndex)
                {
                case 0: setScene(std::make_unique<dx3d::TestScene>()); m_currentSceneType = SceneType::TestScene; break;
                case 1: setScene(std::make_unique<dx3d::BridgeScene>()); m_currentSceneType = SceneType::BridgeScene; break;
                case 2: setScene(std::make_unique<dx3d::SpiderSolitaireScene>()); m_currentSceneType = SceneType::SpiderSolitaireScene; break;
                case 3: setScene(std::make_unique<dx3d::PhysicsTetrisScene>()); m_currentSceneType = SceneType::PhysicsTetrisScene; break;
                case 4: setScene(std::make_unique<dx3d::JellyTetrisReduxScene>()); m_currentSceneType = SceneType::JellyTetrisReduxScene; break;
                case 5: setScene(std::make_unique<dx3d::PartitionScene>()); m_currentSceneType = SceneType::PartitionScene; break;
                case 6: setScene(std::make_unique<dx3d::ThreeDTestScene>()); m_currentSceneType = SceneType::ThreeDTestScene; break;
                case 7: setScene(std::make_unique<dx3d::MarbleMazeScene>()); m_currentSceneType = SceneType::MarbleMazeScene; break;
                case 8: setScene(std::make_unique<dx3d::FlipFluidSimulationScene>()); m_currentSceneType = SceneType::FlipFluidSimulationScene; break;
                default: break;
                }
            }
            ImGui::TextDisabled("Hotkeys: 1-9 switch scenes");
        }
        ImGui::End();
    }
    else
    {
        m_graphicsEngine->beginFrame(m_display->getSwapChain());
    }

    // Render ImGui on top
    {
        auto* d3dCtx = m_graphicsEngine->getContext().getD3DDeviceContext();
        ID3D11ShaderResourceView* nullSrvs[16] = {};
        ID3D11SamplerState* nullSamplers[16] = {};
        d3dCtx->PSSetShaderResources(0, 16, nullSrvs);
        d3dCtx->VSSetShaderResources(0, 16, nullSrvs);
        d3dCtx->GSSetShaderResources(0, 16, nullSrvs);
        d3dCtx->PSSetSamplers(0, 16, nullSamplers);
    }
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport rendering for drag-out functionality
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
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
    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();
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