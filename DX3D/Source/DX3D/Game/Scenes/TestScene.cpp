// Debug TestScene.cpp with detailed matrix output and step-by-step debugging
#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>

using namespace dx3d;


void TestScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    // Create sprite
    auto cat = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 0.5f, 1.0f
    );

    cat->setPosition(0.0f, 0.0f, 1.0f);
    m_sprites.push_back(std::move(cat));

    // Create and setup camera
    m_camera = std::make_unique<Camera>();
    m_camera->setup2D((float)engine.getWindowWidth(), (float)engine.getWindowHeight());
}

void TestScene::update(float dt) {
    static float time = 0.0f;
    time += dt;

    for (auto& sprite : m_sprites) {
        float speed = 1.0f;
        float amplitude = 1.0f;
        float newX = amplitude * sin(time * speed);
        sprite->setPosition(newX, sprite->getPosition().y, sprite->getPosition().z);
    }
}

void TestScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();


    for (auto& sprite : m_sprites) {
        if (sprite->isVisible() && sprite->isValid()) {
            sprite->draw(ctx);
        }
    }

    engine.endFrame(swapChain);
}