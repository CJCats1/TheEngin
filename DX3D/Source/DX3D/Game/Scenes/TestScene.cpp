#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <cmath>

using namespace dx3d;

void TestScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();

    // Create camera entity
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera>(screenWidth, screenHeight);
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

    auto& cat4Entity = m_entityManager->createEntity("Cat4");
    auto& cat4Sprite = cat4Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 120.0f * 0.85f, 120.0f
    );
    cat4Sprite.setPosition(500.0f, 300.0f, 0.0f);

    auto& cat5Entity = m_entityManager->createEntity("Cat5");
    auto& cat5Sprite = cat5Entity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/cat.jpg", 160.0f * 0.85f, 160.0f
    );
    cat5Sprite.setPosition(-400.0f, -250.0f, 0.0f);
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
}

void TestScene::updateCameraMovement(float dt) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return;

    auto* camera = cameraEntity->getComponent<Camera>();
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
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();

    // Find and set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // Render all sprite components
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    engine.endFrame(swapChain);
}