#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>

using namespace dx3d;

// Virtual key codes (Windows)
#define VK_W 0x57
#define VK_A 0x41
#define VK_S 0x53
#define VK_D 0x44
#define VK_Q 0x51
#define VK_E 0x45
#define VK_SHIFT 0x10

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
        device, L"DX3D/Assets/Textures/cat.jpg", 170.0f, 200.0f
    );
    cat1->setPosition(0.0f, 0.0f, 0.0f); // Center
    m_sprites.push_back(std::move(cat1));

    auto cat2 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 85.0f, 100.0f
    );
    cat2->setPosition(640.0f, 0.0f, 0.0f); 
    m_sprites.push_back(std::move(cat2));

    auto cat3 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 127.5f, 150.0f
    );
    cat3->setPosition(-640.0f, 0.0f, 0.0f); 
    m_sprites.push_back(std::move(cat3));

    // Add more sprites for testing camera movement
    auto cat4 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 102.0f, 120.0f
    );
    cat4->setPosition(0.0f, -300.0f, 0.0f); // Far right and up
    m_sprites.push_back(std::move(cat4));

    auto cat5 = std::make_unique<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 136.0f, 160.0f
    );
    cat5->setPosition(0.0f, 300.0f, 0.0f); // Far left and down
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
    float baseSpeed = 300.0f; // pixels per second
    float fastSpeed = 600.0f; // speed when shift is held
    float zoomSpeed = 2.0f;   // zoom units per second

    float currentSpeed = m_input.leftShift ? fastSpeed : baseSpeed;

    // Calculate movement delta
    Vec2 moveDelta(0.0f, 0.0f);

    if (m_input.wPressed) moveDelta.y += currentSpeed * dt; // Up
    if (m_input.sPressed) moveDelta.y -= currentSpeed * dt; // Down
    if (m_input.aPressed) moveDelta.x -= currentSpeed * dt; // Left
    if (m_input.dPressed) moveDelta.x += currentSpeed * dt; // Right

    // Apply movement
    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        m_camera->move(moveDelta);
    }

    // Handle zoom
    float zoomDelta = 0.0f;
    if (m_input.qPressed) zoomDelta -= zoomSpeed * dt; // Zoom out
    if (m_input.ePressed) zoomDelta += zoomSpeed * dt; // Zoom in

    if (zoomDelta != 0.0f) {
        m_camera->zoom(zoomDelta);
    }
}

void TestScene::onKeyDown(int keyCode) {
    switch (keyCode) {
    case VK_W: m_input.wPressed = true; break;
    case VK_A: m_input.aPressed = true; break;
    case VK_S: m_input.sPressed = true; break;
    case VK_D: m_input.dPressed = true; break;
    case VK_Q: m_input.qPressed = true; break;
    case VK_E: m_input.ePressed = true; break;
    case VK_SHIFT: m_input.leftShift = true; break;
    }
}

void TestScene::onKeyUp(int keyCode) {
    switch (keyCode) {
    case VK_W: m_input.wPressed = false; break;
    case VK_A: m_input.aPressed = false; break;
    case VK_S: m_input.sPressed = false; break;
    case VK_D: m_input.dPressed = false; break;
    case VK_Q: m_input.qPressed = false; break;
    case VK_E: m_input.ePressed = false; break;
    case VK_SHIFT: m_input.leftShift = false; break;
    }
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

// Alternative: If you don't have a separate input system, you can check keys directly in update()
// Here's an example using GetAsyncKeyState (Windows):

/*
void TestScene::updateCameraMovement(float dt) {
    float baseSpeed = 300.0f;
    float fastSpeed = 600.0f;
    float zoomSpeed = 2.0f;

    // Check key states directly (Windows only)
    bool wPressed = (GetAsyncKeyState(VK_W) & 0x8000) != 0;
    bool aPressed = (GetAsyncKeyState(VK_A) & 0x8000) != 0;
    bool sPressed = (GetAsyncKeyState(VK_S) & 0x8000) != 0;
    bool dPressed = (GetAsyncKeyState(VK_D) & 0x8000) != 0;
    bool qPressed = (GetAsyncKeyState(VK_Q) & 0x8000) != 0;
    bool ePressed = (GetAsyncKeyState(VK_E) & 0x8000) != 0;
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    float currentSpeed = shiftPressed ? fastSpeed : baseSpeed;

    Vec2 moveDelta(0.0f, 0.0f);
    if (wPressed) moveDelta.y += currentSpeed * dt;
    if (sPressed) moveDelta.y -= currentSpeed * dt;
    if (aPressed) moveDelta.x -= currentSpeed * dt;
    if (dPressed) moveDelta.x += currentSpeed * dt;

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        m_camera->move(moveDelta);
    }

    float zoomDelta = 0.0f;
    if (qPressed) zoomDelta -= zoomSpeed * dt;
    if (ePressed) zoomDelta += zoomSpeed * dt;

    if (zoomDelta != 0.0f) {
        m_camera->zoom(zoomDelta);
    }
}
*/