#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>z

using namespace dx3d;

void TestScene::load(GraphicsEngine& engine) {

    auto& device = engine.getGraphicsDevice();

    // Create sprite
    auto cat = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 0.5f, 1.0f
    );
    m_sprites.push_back(std::move(cat));
    m_camera = std::make_unique<Camera>();

    //setting up the camera but not actually using it lol 
    m_camera->setup2D((float)engine.getWindowWidth(), (float)engine.getWindowHeight());
}

void TestScene::update(float dt) {
    static float time = 0.0f;
    time += dt;

    for (auto& sprite : m_sprites) {
        // Control the speed with a multiplier
        float speed = 0.01f;  // Adjust this value (0.1f = very slow, 1.0f = fast)
        float amplitude = 0.7f;  // How far it moves (-0.7 to +0.7)

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
