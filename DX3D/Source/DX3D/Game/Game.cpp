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
#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Game/Scenes/ThreeDTestScene.h>


dx3d::Game::Game(const GameDesc& desc) :
    Base({ *std::make_unique<Logger>(desc.logLevel).release() }),
    m_loggerPtr(&m_logger)
{
    m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{ m_logger });
    m_display = std::make_unique<Display>(DisplayDesc{ {m_logger,desc.windowSize},m_graphicsEngine->getGraphicsDevice() });
    m_lastFrameTime = std::chrono::steady_clock::now();
    setScene(std::make_unique<dx3d::BridgeScene>());
    m_currentSceneType = SceneType::BridgeScene;

    DX3DLogInfo("Game initialized.");
}

dx3d::Game::~Game()
{
    DX3DLogInfo("Game deallocation started.");
}

const float fixedStep = 1.0f / 60.0f;
float accumulator = 0.0f;

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
    m_activeScene->update(frameTime);
    // Render once per frame
    if (m_activeScene)
        m_activeScene->render(*m_graphicsEngine, m_display->getSwapChain());
    else
    {
        m_graphicsEngine->beginFrame(m_display->getSwapChain());
        m_graphicsEngine->endFrame(m_display->getSwapChain());
    }
    input.update();

}

void dx3d::Game::setScene(std::unique_ptr<Scene> scene)
{
    m_activeScene = std::move(scene);

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