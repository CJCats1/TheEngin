#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Core/Input.h>
#include <cmath>

using namespace dx3d;

void TestScene::load(GraphicsEngine& engine) {

    auto& device = engine.getGraphicsDevice();
    // Initialize camera with screen dimensions
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    m_camera = std::make_unique<Camera>(screenWidth, screenHeight);
    // Position camera at center
    m_camera->setPosition(0.0f, 0.0f);
    m_camera->setZoom(1.0f);
    // Create sprites with different positions and sizes
    auto cat1 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 200.0f * 0.85f, 200.0f
    );
    cat1->setPosition(0.0f, 0.0f, 0.0f); // Center
    m_sprites.push_back(std::move(cat1));
    auto cat2 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 100.0f * 0.85f, 100.0f
    );
    cat2->setPosition(300.0f, -150.0f, 0.0f); // Right and down
    m_sprites.push_back(std::move(cat2));
    auto cat3 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 150.0f * 0.85f, 150.0f
    );
    cat3->setPosition(-250.0f, 200.0f, 0.0f); // Left and up
    m_sprites.push_back(std::move(cat3));
    // Add more sprites for testing camera movement
    auto cat4 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 120.0f * 0.85f, 120.0f
    );
    cat4->setPosition(500.0f, 300.0f, 0.0f); // Far right and up
    m_sprites.push_back(std::move(cat4));
    auto cat5 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 160.0f * 0.85f, 160.0f
    );
    cat5->setPosition(-400.0f, -250.0f, 0.0f); // Far left and down
    m_sprites.push_back(std::move(cat5));
}

void TestScene::update(float dt) {
    static float time = 0.0f;
    time += dt;

    // Update camera based on input
    updateCameraMovement(dt);

    // Animate the first sprite (optional - you can remove this)
    if (!m_sprites.empty()) {
        float speed = 1.0f;
        float amplitude = 100.0f;
        float newX = amplitude * sin(time * speed);
        m_sprites[0]->setPosition(newX, m_sprites[0]->getPosition().y, m_sprites[0]->getPosition().z);
    }

    // Rotate the second sprite
    if (m_sprites.size() > 1) {
        Vec3 currentRot = m_sprites[1]->getRotation();
        m_sprites[1]->setRotation(currentRot.x, currentRot.y, currentRot.z + dt);
    }
}

void TestScene::updateCameraMovement(float dt) {
    auto& input = Input::getInstance();

    float baseSpeed = 300.0f; // pixels per second
    float fastSpeed = 600.0f; // speed when shift is held
    float zoomSpeed = 2.0f;   // zoom units per second

    // Check if shift is held for faster movement
    float currentSpeed = input.isKeyDown(Key::Shift) ? fastSpeed : baseSpeed;

    // Calculate movement delta using the Input system
    Vec2 moveDelta(0.0f, 0.0f);

    if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt; // Up
    if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt; // Down
    if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt; // Left
    if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt; // Right

    // Apply movement
    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        m_camera->move(moveDelta);
    }

    // Handle zoom
    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::Q)) zoomDelta -= zoomSpeed * dt; // Zoom out
    if (input.isKeyDown(Key::E)) zoomDelta += zoomSpeed * dt; // Zoom in

    if (zoomDelta != 0.0f) {
        m_camera->zoom(zoomDelta);
    }

    // Example of using "just pressed" - maybe to reset camera position
    if (input.isKeyDown(Key::Space)) {
        m_camera->setPosition(0.0f, 0.0f);
        m_camera->setZoom(1.0f);
    }

    // Example of using raw key codes if needed
    // if (input.isKeyDown(VK_ESCAPE)) {
    //     // Handle escape key
    // }
}

void TestScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();

    // Set camera matrices
    ctx.setViewMatrix(m_camera->getViewMatrix());
    ctx.setProjectionMatrix(m_camera->getProjectionMatrix());

    // Render all sprites
    for (auto& sprite : m_sprites) {
        if (sprite->isVisible() && sprite->isValid()) {
            sprite->draw(ctx);
        }
    }

    engine.endFrame(swapChain);
}