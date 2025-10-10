#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <cmath>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <iostream>

using namespace dx3d;

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
    // frame begin/end handled centrally
}