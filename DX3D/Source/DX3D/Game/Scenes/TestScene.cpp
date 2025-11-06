#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Core/Entity.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Core/TransformComponent.h>
#include <cmath>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/FirmGuyComponent.h>
#include <DX3D/Components/FirmGuySystem.h>
#include <DX3D/Components/SoftGuyComponent.h>
#include <imgui.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <memory>
#include <functional>
#include <cstdio>

using namespace dx3d;

// Shared rotation state for FirmGuy box (rotate all four walls around common center)
static float s_firmGuyBoxRotation = 0.0f; // radians
static Vec2 s_firmGuyBoxPivot = Vec2(200.0f, 0.0f);

void TestScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();

    // Create camera entity
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    // Create sprite entities with different positions and sizes
    auto& cat1Entity = m_entityManager->createEntity("Cat1");
    auto& cat1Sprite = cat1Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 200.0f * 0.85f, 200.0f
    );
    cat1Sprite.setPosition(0.0f, 0.0f, 0.0f);

    // Add animation to cat1 (sine wave movement)
    auto& cat1Animation = cat1Entity.addComponent<AnimationComponent>();
    cat1Animation.setUpdateFunction([](Entity& entity, float dt) {
        static float time = 0.0f;
        time += dt;
        float speed = 1.0f;
        float amplitude = 100.0f;
        float newX = amplitude * sin(time * speed);

        if (auto* sprite = entity.getComponent<SpriteComponent>()) {
            Vec3 currentPos = sprite->getPosition();
            sprite->setPosition(newX, currentPos.y, currentPos.z);
        }
        });

    auto& cat2Entity = m_entityManager->createEntity("Cat2");
    auto& cat2Sprite = cat2Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 100.0f * 0.85f, 100.0f
    );
    cat2Sprite.setPosition(300.0f, -150.0f, 0.0f);

    // Add movement and rotation to cat2
    auto& cat2Movement = cat2Entity.addComponent<MovementComponent>(200.0f);
    auto& cat2Animation = cat2Entity.addComponent<AnimationComponent>();
    cat2Animation.setUpdateFunction([](Entity& entity, float dt) {
        if (auto* sprite = entity.getComponent<SpriteComponent>()) {
            float rotationSpeed = 1.0f; // radians per second
            sprite->rotateZ(dt * rotationSpeed);
        }
        });

    auto& cat3Entity = m_entityManager->createEntity("Cat3");
    auto& cat3Sprite = cat3Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 150.0f * 0.85f, 150.0f
    );
    cat3Sprite.setPosition(-250.0f, 200.0f, 0.0f);
    cat3Sprite.setTint(Vec4(0.0f, 1.0f, 0.0f, 0.5f));

    auto& cat4Entity = m_entityManager->createEntity("Cat4");
    auto& cat4Sprite = cat4Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 120.0f * 0.85f, 120.0f
    );
    cat4Sprite.setPosition(500.0f, 300.0f, 0.0f);
    cat4Sprite.setTint(Vec4(1.0f, 0.5f, 0.5f, 0.5f));

    auto& cat5Entity = m_entityManager->createEntity("Cat5");
    auto& cat5Sprite = cat5Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 160.0f * 0.85f, 160.0f
    );
    cat5Sprite.setPosition(-40.0f, -250.0f, 0.0f);
    cat5Sprite.setTint(Vec4(0.5f, 0.5f, 1.0f, 0.5f));
    // FirmGuy demo: one static ground rectangle and one falling circle
    {
        auto& ground = m_entityManager->createEntity("FG_Ground");
        auto& groundSprite = ground.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 400.0f, 20.0f);
        auto& groundRB = ground.addComponent<FirmGuyComponent>();
        groundRB.setRectangle(Vec2(200.0f, 10.0f));
        groundRB.setPosition(Vec2(0.0f, -250.0f));
        groundRB.setStatic(true);
        groundSprite.setPosition(0.0f, -250.0f, 0.0f);
        groundSprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.7f));
    }

    {
        auto& ball = m_entityManager->createEntity("FG_Ball");
        auto& ballSprite = ball.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/node.png", 20.0f, 20.0f);
        auto& ballRB = ball.addComponent<FirmGuyComponent>();
        ballRB.setCircle(10.0f);
        ballRB.setPosition(Vec2(200.0f, 0.0f)); // Start in center of the box
        ballRB.setVelocity(Vec2(30.0f, 20.0f)); // Some initial velocity
        ballRB.setRestitution(0.8f); // More bouncy
        ballRB.setFriction(0.99f); // Less friction
        ballSprite.setPosition(200.0f, 0.0f, 0.0f);
        ballSprite.setTint(Vec4(0.9f, 0.2f, 0.2f, 1.0f)); // Fully opaque
    }

    // Box made of 4 walls (top, bottom, left, right)
    {
        // Top wall
        auto& topWall = m_entityManager->createEntity("FG_TopWall");
        auto& topSprite = topWall.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 200.0f, 20.0f);
        auto& topRB = topWall.addComponent<FirmGuyComponent>();
        topRB.setRectangle(Vec2(100.0f, 10.0f));
        topRB.setPosition(Vec2(200.0f, 50.0f));
        topRB.setStatic(true);
        topSprite.setPosition(200.0f, 50.0f, 0.0f);
        topSprite.setTint(Vec4(0.2f, 0.2f, 0.8f, 0.7f));
        
        // Bottom wall
        auto& bottomWall = m_entityManager->createEntity("FG_BottomWall");
        auto& bottomSprite = bottomWall.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 200.0f, 20.0f);
        auto& bottomRB = bottomWall.addComponent<FirmGuyComponent>();
        bottomRB.setRectangle(Vec2(100.0f, 10.0f));
        bottomRB.setPosition(Vec2(200.0f, -50.0f));
        bottomRB.setStatic(true);
        bottomSprite.setPosition(200.0f, -50.0f, 0.0f);
        bottomSprite.setTint(Vec4(0.2f, 0.2f, 0.8f, 0.7f));
        
        // Left wall
        auto& leftWall = m_entityManager->createEntity("FG_LeftWall");
        auto& leftSprite = leftWall.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 20.0f, 100.0f);
        auto& leftRB = leftWall.addComponent<FirmGuyComponent>();
        leftRB.setRectangle(Vec2(10.0f, 50.0f));
        leftRB.setPosition(Vec2(100.0f, 0.0f));
        leftRB.setStatic(true);
        leftSprite.setPosition(100.0f, 0.0f, 0.0f);
        leftSprite.setTint(Vec4(0.2f, 0.2f, 0.8f, 0.7f));
        
        // Right wall
        auto& rightWall = m_entityManager->createEntity("FG_RightWall");
        auto& rightSprite = rightWall.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 20.0f, 100.0f);
        auto& rightRB = rightWall.addComponent<FirmGuyComponent>();
        rightRB.setRectangle(Vec2(10.0f, 50.0f));
        rightRB.setPosition(Vec2(300.0f, 0.0f));
        rightRB.setStatic(true);
        rightSprite.setPosition(300.0f, 0.0f, 0.0f);
        rightSprite.setTint(Vec4(0.2f, 0.2f, 0.8f, 0.7f));
        
        // Add rotation to all walls with U and O keys around a shared pivot
        auto& topAnimation = topWall.addComponent<AnimationComponent>();
        topAnimation.setUpdateFunction([](Entity& entity, float dt) {
            auto& input = Input::getInstance();
            float rotationSpeed = 2.0f; // radians per second
            
            if (input.isKeyDown(Key::U)) s_firmGuyBoxRotation += rotationSpeed * dt;
            if (input.isKeyDown(Key::O)) s_firmGuyBoxRotation -= rotationSpeed * dt;
            
            if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                // rotate original offset around shared pivot
                Vec3 basePos = Vec3(200.0f, 50.0f, 0.0f);
                Vec2 offset = Vec2(basePos.x, basePos.y) - s_firmGuyBoxPivot;
                float c = std::cos(s_firmGuyBoxRotation), s = std::sin(s_firmGuyBoxRotation);
                Vec2 rotated = Vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c) + s_firmGuyBoxPivot;
                sprite->setPosition(rotated.x, rotated.y, 0.0f);
                sprite->setRotationZ(s_firmGuyBoxRotation);
                if (auto* rb = entity.getComponent<FirmGuyComponent>()) { rb->setPosition(rotated); rb->setAngle(s_firmGuyBoxRotation); }
            }
        });
        
        auto& bottomAnimation = bottomWall.addComponent<AnimationComponent>();
        bottomAnimation.setUpdateFunction([](Entity& entity, float dt) {
            auto& input = Input::getInstance();
            float rotationSpeed = 2.0f;
            
            if (input.isKeyDown(Key::U)) s_firmGuyBoxRotation += rotationSpeed * dt;
            if (input.isKeyDown(Key::O)) s_firmGuyBoxRotation -= rotationSpeed * dt;
            
            if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                Vec3 basePos = Vec3(200.0f, -50.0f, 0.0f);
                Vec2 offset = Vec2(basePos.x, basePos.y) - s_firmGuyBoxPivot;
                float c = std::cos(s_firmGuyBoxRotation), s = std::sin(s_firmGuyBoxRotation);
                Vec2 rotated = Vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c) + s_firmGuyBoxPivot;
                sprite->setPosition(rotated.x, rotated.y, 0.0f);
                sprite->setRotationZ(s_firmGuyBoxRotation);
                if (auto* rb = entity.getComponent<FirmGuyComponent>()) { rb->setPosition(rotated); rb->setAngle(s_firmGuyBoxRotation); }
            }
        });
        
        auto& leftAnimation = leftWall.addComponent<AnimationComponent>();
        leftAnimation.setUpdateFunction([](Entity& entity, float dt) {
            auto& input = Input::getInstance();
            float rotationSpeed = 2.0f;
            
            if (input.isKeyDown(Key::U)) s_firmGuyBoxRotation += rotationSpeed * dt;
            if (input.isKeyDown(Key::O)) s_firmGuyBoxRotation -= rotationSpeed * dt;
            
            if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                Vec3 basePos = Vec3(100.0f, 0.0f, 0.0f);
                Vec2 offset = Vec2(basePos.x, basePos.y) - s_firmGuyBoxPivot;
                float c = std::cos(s_firmGuyBoxRotation), s = std::sin(s_firmGuyBoxRotation);
                Vec2 rotated = Vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c) + s_firmGuyBoxPivot;
                sprite->setPosition(rotated.x, rotated.y, 0.0f);
                sprite->setRotationZ(s_firmGuyBoxRotation);
                if (auto* rb = entity.getComponent<FirmGuyComponent>()) { rb->setPosition(rotated); rb->setAngle(s_firmGuyBoxRotation); }
            }
        });
        
        auto& rightAnimation = rightWall.addComponent<AnimationComponent>();
        rightAnimation.setUpdateFunction([](Entity& entity, float dt) {
            auto& input = Input::getInstance();
            float rotationSpeed = 2.0f;
            
            if (input.isKeyDown(Key::U)) s_firmGuyBoxRotation += rotationSpeed * dt;
            if (input.isKeyDown(Key::O)) s_firmGuyBoxRotation -= rotationSpeed * dt;
            
            if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                Vec3 basePos = Vec3(300.0f, 0.0f, 0.0f);
                Vec2 offset = Vec2(basePos.x, basePos.y) - s_firmGuyBoxPivot;
                float c = std::cos(s_firmGuyBoxRotation), s = std::sin(s_firmGuyBoxRotation);
                Vec2 rotated = Vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c) + s_firmGuyBoxPivot;
                sprite->setPosition(rotated.x, rotated.y, 0.0f);
                sprite->setRotationZ(s_firmGuyBoxRotation);
                if (auto* rb = entity.getComponent<FirmGuyComponent>()) { rb->setPosition(rotated); rb->setAngle(s_firmGuyBoxRotation); }
            }
        });
    }

    // Create SoftGuy examples for testing
    createSoftGuyExamples(device);

    // Add this inside TestScene::load(GraphicsEngine& engine), after the cat sprites
    auto& debugQuadEntity = m_entityManager->createEntity("DebugQuad");
    auto& debugQuad = debugQuadEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png", // You can also use a plain 1x1 white texture
        25.0f, 25.0f // width and height in pixels
    );
    debugQuad.setPosition(0.0f, 0.0f, 0.0f);
    debugQuad.enableScreenSpace(true);
    debugQuad.setScreenPosition(0.5f, 0.5f);

    // Tint so it stands out (semi-transparent red)
    debugQuad.setTint(Vec4(1.0f, 0.0f, 0.0f, 0.5f));




    auto& uiTest = m_entityManager->createEntity("UI_Text");
    auto& textTest = uiTest.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"<--Test           Test-->",
        24.0f
    );

    textTest.setFontFamily(L"Consolas");
    textTest.setColor(Vec4(0.0f, 0.0f, 1.0f, 1.0f)); // yellow
    textTest.setScreenPosition(0.16, 0.94);        // top-left of screen

    Vec2 textSize = textTest.getTextSize();

    auto& debugQuadTestEntity = m_entityManager->createEntity("DebugQuadTest");
    float padding = 10.0f;
    auto& debugQuadTest = debugQuadTestEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png", // You can also use a plain 1x1 white texture
        textSize.x + padding, textSize.y + padding // width and height in pixels
    );
    debugQuadTest.setPosition(0.0f, 0.0f, 0.0f);
    debugQuadTest.enableScreenSpace(true);
    debugQuadTest.setScreenPosition(0.16, 0.94);







    auto& testButtonE = m_entityManager->createEntity("UI_TextButton");

    auto& testButton = testButtonE.addComponent<ButtonComponent>(
        device,
        L"Make Cat Big", // Button text
        48.0f

    );

    // Position the button in screen space
    testButton.setScreenPosition(0.8f, 0.1f); // Bottom right area

    // Set button colors
    testButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.5f));   // Blue
    testButton.setHoveredTint(Vec4(0.4f, 0.8f, 1.0f, 0.5f));  // Light blue
    testButton.setPressedTint(Vec4(0.1f, 0.4f, 0.8f, 0.5f));  // Dark blue

    // Set button text properties
    testButton.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text
    testButton.setFontSize(18.0f);

    // Set the callback function - this will be called when button is clicked
    // Change this lambda in TestScene::load to capture 'this':
    testButton.setOnClickCallback([this]() {
        std::cout << "Test Button Clicked!" << std::endl;

        if (auto* cat1 = m_entityManager->findEntity("Cat1")) {
            if (auto* sprite = cat1->getComponent<SpriteComponent>()) {
                Vec3 currentScale = sprite->getScale(); // assuming getScale() exists
                float scaleFactor = 1.2f;              // increase by 20%
                sprite->setScale(currentScale * scaleFactor);
            }
        }
        });


    // 1. A button that resets cat1 scale
    auto& resetButtonE = m_entityManager->createEntity("UI_ResetButton");
    auto& resetButton = resetButtonE.addComponent<ButtonComponent>(device, L"Reset Cat", 36.0f);
    resetButton.setScreenPosition(0.8f, 0.2f);
    resetButton.setNormalTint(Vec4(0.6f, 0.2f, 1, 0.5f));
    resetButton.setOnClickCallback([this]() {
        if (auto* cat1 = m_entityManager->findEntity("Cat1"))
            if (auto* sprite = cat1->getComponent<SpriteComponent>())
                sprite->setScale(Vec3(1, 1, 1));
        });

    // 2. Text that updates each frame (fps counter)
    auto& fpsTextE = m_entityManager->createEntity("UI_FPS");
    auto& fpsText = fpsTextE.addComponent<TextComponent>(device, TextSystem::getRenderer(), L"FPS: 0", 20.0f);
    fpsText.setScreenPosition(0.05, 0.02);
    fpsText.setColor(Vec4(1, 1, 0, 1));
}

void TestScene::update(float dt) {
    auto& input = Input::getInstance();

    // Update camera movement
    updateCameraMovement(dt);

    // Handle cat2 movement with arrow keys
    if (auto* cat2Entity = m_entityManager->findEntity("Cat2")) {
        if (auto* movement = cat2Entity->getComponent<MovementComponent>()) {
            Vec2 velocity(0.0f, 0.0f);
            float speed = movement->getSpeed();

            if (input.isKeyDown(Key::Up))    velocity.y += speed;
            if (input.isKeyDown(Key::Down))  velocity.y -= speed;
            if (input.isKeyDown(Key::Left))  velocity.x -= speed;
            if (input.isKeyDown(Key::Right)) velocity.x += speed;

            movement->setVelocity(velocity);
        }
    }
    if (auto* debugQuad = m_entityManager->findEntity("DebugQuad")) {
        float speed = 0.005f; // normalized space movement per frame
        auto debugQuadSprite = debugQuad->getComponent<SpriteComponent>();
        Vec2 newPos = debugQuadSprite->getScreenPosition();

        if (input.isKeyDown(Key::I)) {
            newPos.y += speed;
        }
        if (input.isKeyDown(Key::K)) {
            newPos.y -= speed;
        }
        if (input.isKeyDown(Key::J)) {
            newPos.x -= speed;
        }
        if (input.isKeyDown(Key::L)) {
            newPos.x += speed;
        }

        debugQuadSprite->setScreenPosition(newPos.x, newPos.y);
    }

    // Update all animation components
    auto animatedEntities = m_entityManager->getEntitiesWithComponent<AnimationComponent>();
    for (auto* entity : animatedEntities) {
        if (auto* animation = entity->getComponent<AnimationComponent>()) {
            animation->update(*entity, dt);
        }
    }

    // Update all movement components
    auto movingEntities = m_entityManager->getEntitiesWithComponent<MovementComponent>();
    for (auto* entity : movingEntities) {
        if (auto* movement = entity->getComponent<MovementComponent>()) {
            movement->update(*entity, dt);
        }
    }

    // Update FirmGuy physics last so input moved sprites can be overridden by physics bodies
    FirmGuySystem::update(*m_entityManager, dt);
    
    // Update SoftGuy physics system
    SoftGuySystem::update(*m_entityManager, dt);
    
    // Debug: Print ball position
    if (auto* ballEntity = m_entityManager->findEntity("FG_Ball")) {
        if (auto* ballRB = ballEntity->getComponent<FirmGuyComponent>()) {
            Vec2 pos = ballRB->getPosition();
            Vec2 vel = ballRB->getVelocity();
            printf("Ball: pos(%.1f, %.1f) vel(%.1f, %.1f)\n", pos.x, pos.y, vel.x, vel.y);
        }
    }
    auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
    for (auto* entity : buttonEntities) {
        if (auto* movement = entity->getComponent<ButtonComponent>()) {
            movement->update(dt);
        }
    }
    auto mouse = Input::getInstance().getMousePositionNDC();
    if (auto* debugQuad = m_entityManager->findEntity("DebugQuad")) {
        float speed = 0.005f; // normalized space movement per frame
        auto debugQuadSprite = debugQuad->getComponent<SpriteComponent>();
        Vec2 newPos = debugQuadSprite->getScreenPosition();

        if (input.isKeyDown(Key::I)) {
            newPos.y += speed;
        }
        if (input.isKeyDown(Key::K)) {
            newPos.y -= speed;
        }
        if (input.isKeyDown(Key::J)) {
            newPos.x -= speed;
        }
        if (input.isKeyDown(Key::L)) {
            newPos.x += speed;
        }

        debugQuadSprite->setScreenPosition(mouse.x, mouse.y);
    }
    static float timer = 0; static int frames = 0;
    timer += dt; frames++;
    if (timer >= 1.0f) {
        if (auto* fpsEntity = m_entityManager->findEntity("UI_FPS"))
            if (auto* fpsText = fpsEntity->getComponent<TextComponent>()) {
                fpsText->setText(L"FPS: " + std::to_wstring(frames));
            }
        frames = 0; timer = 0;
    }
    printf("%f,%f\n", mouse.x, mouse.y);
}

void TestScene::updateCameraMovement(float dt) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return;

    auto* camera = cameraEntity->getComponent<Camera2D>();
    if (!camera) return;

    auto& input = Input::getInstance();

    float baseSpeed = 300.0f;
    float fastSpeed = 600.0f;
    float zoomSpeed = 2.0f;

    float currentSpeed = input.isKeyDown(Key::Shift) ? fastSpeed : baseSpeed;

    Vec2 moveDelta(0.0f, 0.0f);
    if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt;
    if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt;
    if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt;
    if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt;

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        camera->move(moveDelta);
    }

    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::Q)) zoomDelta -= zoomSpeed * dt;
    if (input.isKeyDown(Key::E)) zoomDelta += zoomSpeed * dt;

    if (zoomDelta != 0.0f) {
        camera->zoom(zoomDelta);
    }

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(1.0f);
    }

}

void TestScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // ---------- PASS 1: world-space sprites using default pipeline ----------
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    // set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    for (auto* entity : spriteEntities) {
        if (entity->getName() == "DebugQuad") continue; // skip debug quad in world pass
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    // ---------- PASS 2: screen-space sprites (UI/debug) using text/screen pipeline ----------
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());

    // identity view, pixel-orthographic projection (0,0 top-left)
    // draw only screen-space sprites
    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isScreenSpace()) {
                sprite->draw(ctx);
            }
        }
    }


    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }

    for (auto* entity : m_entityManager->getEntitiesWithComponent<ButtonComponent>()) {
        if (auto* text = entity->getComponent<ButtonComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }
    
    // Render ImGui
    renderImGui(engine);
    
    // frame begin/end handled centrally
}

void TestScene::createSoftGuyExamples(GraphicsDevice& device) {
    // Create various SoftGuy examples to demonstrate the system
    
    // 1. Soft Circle - a bouncy ball
    SoftGuyConfig circleConfig;
    circleConfig.stiffness = 1500.0f;
    circleConfig.damping = 100.0f;
    circleConfig.nodeColor = Vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    circleConfig.beamColor = Vec4(1.0f, 0.3f, 0.0f, 0.8f);
    
    auto* softCircle = SoftGuyComponent::createCircle(
        *m_entityManager, "SoftCircle", 
        Vec2(-300.0f, 300.0f), 50.0f, 8, circleConfig
    );
    
    // 2. Soft Rectangle - a soft box
    SoftGuyConfig rectConfig;
    rectConfig.stiffness = 2000.0f;
    rectConfig.damping = 120.0f;
    rectConfig.nodeColor = Vec4(0.0f, 0.8f, 1.0f, 1.0f); // Cyan
    rectConfig.beamColor = Vec4(0.0f, 0.6f, 0.8f, 0.8f);
    
    auto* softRect = SoftGuyComponent::createRectangle(
        *m_entityManager, "SoftRectangle",
        Vec2(300.0f, 300.0f), Vec2(100.0f, 80.0f), 4, 3, rectConfig
    );
    
    // 3. Soft Triangle - a soft pyramid
    SoftGuyConfig triangleConfig;
    triangleConfig.stiffness = 1800.0f;
    triangleConfig.damping = 90.0f;
    triangleConfig.nodeColor = Vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
    triangleConfig.beamColor = Vec4(0.8f, 0.0f, 0.8f, 0.8f);
    
    auto* softTriangle = SoftGuyComponent::createTriangle(
        *m_entityManager, "SoftTriangle",
        Vec2(0.0f, 300.0f), 60.0f, triangleConfig
    );
    
    // 4. Soft Line - a soft rope
    SoftGuyConfig lineConfig;
    lineConfig.stiffness = 1200.0f;
    lineConfig.damping = 60.0f;
    lineConfig.nodeColor = Vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    lineConfig.beamColor = Vec4(0.3f, 0.3f, 0.3f, 0.8f);
    
    auto* softLine = SoftGuyComponent::createLine(
        *m_entityManager, "SoftLine",
        Vec2(-150.0f, 200.0f), Vec2(150.0f, 200.0f), 5, lineConfig
    );
    
    // 5. Create some FirmGuy objects to interact with the soft bodies
    // Red ball that will bounce off soft bodies
    Entity& redBall = m_entityManager->createEntity("RedBall");
    FirmGuyComponent& redBallPhysics = redBall.addComponent<FirmGuyComponent>();
    redBallPhysics.setCircle(25.0f);
    redBallPhysics.setPosition(Vec2(-200.0f, 500.0f));
    redBallPhysics.setVelocity(Vec2(50.0f, -200.0f));
    redBallPhysics.setMass(3.0f);
    redBallPhysics.setRestitution(0.7f);
    redBallPhysics.setFriction(0.9f);
    
    SpriteComponent& redBallSprite = redBall.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/node.png", 50.0f, 50.0f);
    redBallSprite.setPosition(redBallPhysics.getPosition().x, redBallPhysics.getPosition().y, 0.0f);
    
    // Blue box that will interact with soft bodies
    Entity& blueBox = m_entityManager->createEntity("BlueBox");
    FirmGuyComponent& blueBoxPhysics = blueBox.addComponent<FirmGuyComponent>();
    blueBoxPhysics.setRectangle(Vec2(30.0f, 30.0f));
    blueBoxPhysics.setPosition(Vec2(200.0f, 500.0f));
    blueBoxPhysics.setVelocity(Vec2(-30.0f, -150.0f));
    blueBoxPhysics.setMass(2.5f);
    blueBoxPhysics.setRestitution(0.5f);
    blueBoxPhysics.setFriction(0.95f);
    
    SpriteComponent& blueBoxSprite = blueBox.addComponent<SpriteComponent>(device, L"DX3D/Assets/Textures/beam.png", 60.0f, 60.0f);
    blueBoxSprite.setPosition(blueBoxPhysics.getPosition().x, blueBoxPhysics.getPosition().y, 0.0f);
}

void TestScene::renderImGui(GraphicsEngine& engine) {
    // Create a spawn panel
    ImGui::Begin("Spawn Panel");
    
    // SoftGuy spawning section
    ImGui::Text("SoftGuy Objects");
    ImGui::Separator();
    
    if (ImGui::Button("Spawn Soft Circle")) {
        spawnSoftGuyCircle(Vec2(0.0f, 200.0f));
    }
    
    if (ImGui::Button("Spawn Soft Rectangle")) {
        spawnSoftGuyRectangle(Vec2(100.0f, 200.0f));
    }
    
    if (ImGui::Button("Spawn Soft Triangle")) {
        spawnSoftGuyTriangle(Vec2(-100.0f, 200.0f));
    }
    
    if (ImGui::Button("Spawn Soft Line")) {
        spawnSoftGuyLine(Vec2(0.0f, 300.0f));
    }
    
    ImGui::Spacing();
    
    // FirmGuy spawning section
    ImGui::Text("FirmGuy Objects");
    ImGui::Separator();
    
    if (ImGui::Button("Spawn Firm Circle")) {
        spawnFirmGuyCircle(Vec2(200.0f, 200.0f));
    }
    
    if (ImGui::Button("Spawn Firm Rectangle")) {
        spawnFirmGuyRectangle(Vec2(-200.0f, 200.0f));
    }
    
    ImGui::Spacing();
    
    // Physics controls
    ImGui::Text("Physics Controls");
    ImGui::Separator();
    
    static float gravity = -2000.0f;
    if (ImGui::SliderFloat("Gravity", &gravity, -5000.0f, 0.0f)) {
        SoftGuySystem::setGravity(gravity);
    }
    
    if (ImGui::Button("Reset All Physics")) {
        SoftGuySystem::resetAll(*m_entityManager);
        // Note: FirmGuySystem doesn't have resetAll, so we'll skip that for now
    }
    
    ImGui::Spacing();
    
    // Entity count display
    auto softGuyEntities = m_entityManager->getEntitiesWithComponent<SoftGuyComponent>();
    auto firmGuyEntities = m_entityManager->getEntitiesWithComponent<FirmGuyComponent>();
    
    ImGui::Text("Entity Count:");
    ImGui::Text("SoftGuy Objects: %d", (int)softGuyEntities.size());
    ImGui::Text("FirmGuy Objects: %d", (int)firmGuyEntities.size());
    
    ImGui::End();
}

void TestScene::spawnSoftGuyCircle(Vec2 position) {
    static int circleCount = 0;
    circleCount++;
    
    SoftGuyConfig config;
    config.stiffness = 1500.0f;
    config.damping = 100.0f;
    config.nodeColor = Vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    config.beamColor = Vec4(1.0f, 0.3f, 0.0f, 0.8f);
    
    std::string name = "SpawnedSoftCircle_" + std::to_string(circleCount);
    SoftGuyComponent::createCircle(*m_entityManager, name, position, 50.0f, 8, config);
}

void TestScene::spawnSoftGuyRectangle(Vec2 position) {
    static int rectCount = 0;
    rectCount++;
    
    SoftGuyConfig config;
    config.stiffness = 2000.0f;
    config.damping = 120.0f;
    config.nodeColor = Vec4(0.0f, 0.8f, 1.0f, 1.0f); // Cyan
    config.beamColor = Vec4(0.0f, 0.6f, 0.8f, 0.8f);
    
    std::string name = "SpawnedSoftRect_" + std::to_string(rectCount);
    SoftGuyComponent::createRectangle(*m_entityManager, name, position, Vec2(100.0f, 80.0f), 4, 3, config);
}

void TestScene::spawnSoftGuyTriangle(Vec2 position) {
    static int triangleCount = 0;
    triangleCount++;
    
    SoftGuyConfig config;
    config.stiffness = 1800.0f;
    config.damping = 90.0f;
    config.nodeColor = Vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
    config.beamColor = Vec4(0.8f, 0.0f, 0.8f, 0.8f);
    
    std::string name = "SpawnedSoftTriangle_" + std::to_string(triangleCount);
    SoftGuyComponent::createTriangle(*m_entityManager, name, position, 60.0f, config);
}

void TestScene::spawnSoftGuyLine(Vec2 position) {
    static int lineCount = 0;
    lineCount++;
    
    SoftGuyConfig config;
    config.stiffness = 1200.0f;
    config.damping = 60.0f;
    config.nodeColor = Vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    config.beamColor = Vec4(0.3f, 0.3f, 0.3f, 0.8f);
    
    std::string name = "SpawnedSoftLine_" + std::to_string(lineCount);
    SoftGuyComponent::createLine(*m_entityManager, name, position, position + Vec2(100.0f, 0.0f), 5, config);
}

void TestScene::spawnFirmGuyCircle(Vec2 position) {
    static int firmCircleCount = 0;
    firmCircleCount++;
    
    std::string name = "SpawnedFirmCircle_" + std::to_string(firmCircleCount);
    Entity& entity = m_entityManager->createEntity(name);
    FirmGuyComponent& physics = entity.addComponent<FirmGuyComponent>();
    physics.setCircle(25.0f);
    physics.setPosition(position);
    physics.setVelocity(Vec2(0.0f, 0.0f));
    physics.setMass(2.0f);
    physics.setRestitution(0.7f);
    physics.setFriction(0.9f);
    
    // Note: We need a GraphicsDevice reference to create sprites
    // For now, we'll skip the sprite creation in the spawn functions
    // The physics will still work without visual representation
}

void TestScene::spawnFirmGuyRectangle(Vec2 position) {
    static int firmRectCount = 0;
    firmRectCount++;
    
    std::string name = "SpawnedFirmRect_" + std::to_string(firmRectCount);
    Entity& entity = m_entityManager->createEntity(name);
    FirmGuyComponent& physics = entity.addComponent<FirmGuyComponent>();
    physics.setRectangle(Vec2(30.0f, 30.0f));
    physics.setPosition(position);
    physics.setVelocity(Vec2(0.0f, 0.0f));
    physics.setMass(2.5f);
    physics.setRestitution(0.5f);
    physics.setFriction(0.95f);
    
    // Note: We need a GraphicsDevice reference to create sprites
    // For now, we'll skip the sprite creation in the spawn functions
    // The physics will still work without visual representation
}
