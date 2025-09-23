#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Quadtree.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <random>
#include <iostream>

using namespace dx3d;

void PartitionScene::load(GraphicsEngine& engine) {
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

    // Create line renderer for quadtree visualization
    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);

    // Initialize quadtree
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);

    // Create some test entities
    createTestEntities(device);

    // Create UI elements
    createUIElements(device);

    // Generate initial quadtree visualization
    updateQuadtreeVisualization();
}

void PartitionScene::createTestEntities(GraphicsDevice& device) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
    std::uniform_real_distribution<float> velDist(-100.0f, 100.0f); // velocity range

    for (int i = 0; i < 50; i++) {
        auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(i));
        m_entityCounter++;

        Vec2 position(posDist(gen), posDist(gen));
        Vec2 size(sizeDist(gen), sizeDist(gen));
        Vec2 velocity(velDist(gen), velDist(gen)); // Random initial velocity

        auto& sprite = entity.addComponent<SpriteComponent>(
            device,
            L"DX3D/Assets/Textures/node.png",
            size.x,
            size.y
        );
        sprite.setPosition(position.x, position.y, 0.0f);
        sprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));

        // Create moving entity data
        MovingEntity movingEntity;
        movingEntity.name = "TestEntity" + std::to_string(i);
        movingEntity.velocity = velocity;
        movingEntity.bounds = Vec2(400.0f, 300.0f); // Bounce boundaries
        movingEntity.qtEntity.position = position;
        movingEntity.qtEntity.size = size;
        movingEntity.qtEntity.id = i;
        movingEntity.active = true;

        m_movingEntities.push_back(movingEntity);

        // Add to quadtree
        m_quadtree->insert(movingEntity.qtEntity);
    }
}

void PartitionScene::createUIElements(GraphicsDevice& device) {
    // Add entities button
    auto& addButtonEntity = m_entityManager->createEntity("AddEntitiesButton");
    auto& addButton = addButtonEntity.addComponent<ButtonComponent>(
        device, L"Add Entities", 18.0f
    );
    addButton.setScreenPosition(0.1f, 0.1f);
    addButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    addButton.setOnClickCallback([this]() {
        addRandomEntities();
        });

    // Clear entities button
    auto& clearButtonEntity = m_entityManager->createEntity("ClearEntitiesButton");
    auto& clearButton = clearButtonEntity.addComponent<ButtonComponent>(
        device, L"Clear All", 18.0f
    );
    clearButton.setScreenPosition(0.1f, 0.15f);
    clearButton.setNormalTint(Vec4(0.8f, 0.2f, 0.2f, 0.8f));
    clearButton.setOnClickCallback([this]() {
        clearAllEntities();
        });

    // Toggle quadtree visualization button
    auto& toggleButtonEntity = m_entityManager->createEntity("ToggleQuadtreeButton");
    auto& toggleButton = toggleButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle Quadtree", 18.0f
    );
    toggleButton.setScreenPosition(0.1f, 0.2f);
    toggleButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    toggleButton.setOnClickCallback([this]() {
        m_showQuadtree = !m_showQuadtree;
        updateQuadtreeVisualization();
        });

    auto& moveButtonEntity = m_entityManager->createEntity("ToggleMovementButton");
    auto& moveButton = moveButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle Movement", 18.0f
    );
    moveButton.setScreenPosition(0.1f, 0.25f); // Position below other buttons
    moveButton.setNormalTint(Vec4(0.8f, 0.4f, 0.8f, 0.8f));
    moveButton.setOnClickCallback([this]() {
        m_entitiesMoving = !m_entitiesMoving;
        });
}

void PartitionScene::addRandomEntities() {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
    std::uniform_real_distribution<float> velDist(-100.0f, 100.0f);

    for (int i = 0; i < 10; i++) {
        auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));

        Vec2 position(posDist(gen), posDist(gen));
        Vec2 size(sizeDist(gen), sizeDist(gen));
        Vec2 velocity(velDist(gen), velDist(gen));

        auto& sprite = entity.addComponent<SpriteComponent>(
            device,
            L"DX3D/Assets/Textures/node.png",
            size.x,
            size.y
        );
        sprite.setPosition(position.x, position.y, 0.0f);
        sprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));

        // Create moving entity data
        MovingEntity movingEntity;
        movingEntity.name = "TestEntity" + std::to_string(m_entityCounter);
        movingEntity.velocity = velocity;
        movingEntity.bounds = Vec2(400.0f, 300.0f);
        movingEntity.qtEntity.position = position;
        movingEntity.qtEntity.size = size;
        movingEntity.qtEntity.id = m_entityCounter;
        movingEntity.active = true;

        m_movingEntities.push_back(movingEntity);
        m_quadtree->insert(movingEntity.qtEntity);

        m_entityCounter++;
    }

    updateQuadtreeVisualization();
}
void PartitionScene::updateMovingEntities(float dt) {
    if (!m_entitiesMoving) return;

    // Update positions
    for (auto& movingEntity : m_movingEntities) {
        if (!movingEntity.active) continue;

        // Update position
        movingEntity.qtEntity.position.x += movingEntity.velocity.x * dt;
        movingEntity.qtEntity.position.y += movingEntity.velocity.y * dt;

        // Boundary collision detection and bounce
        if (movingEntity.qtEntity.position.x - movingEntity.qtEntity.size.x * 0.5f <= -movingEntity.bounds.x ||
            movingEntity.qtEntity.position.x + movingEntity.qtEntity.size.x * 0.5f >= movingEntity.bounds.x) {
            movingEntity.velocity.x = -movingEntity.velocity.x;
            // Clamp position to bounds
            float halfWidth = movingEntity.qtEntity.size.x * 0.5f;
            if (movingEntity.qtEntity.position.x < -movingEntity.bounds.x + halfWidth) {
                movingEntity.qtEntity.position.x = -movingEntity.bounds.x + halfWidth;
            }
            else if (movingEntity.qtEntity.position.x > movingEntity.bounds.x - halfWidth) {
                movingEntity.qtEntity.position.x = movingEntity.bounds.x - halfWidth;
            }
        }

        if (movingEntity.qtEntity.position.y - movingEntity.qtEntity.size.y * 0.5f <= -movingEntity.bounds.y ||
            movingEntity.qtEntity.position.y + movingEntity.qtEntity.size.y * 0.5f >= movingEntity.bounds.y) {
            movingEntity.velocity.y = -movingEntity.velocity.y;
            // Clamp position to bounds
            float halfHeight = movingEntity.qtEntity.size.y * 0.5f;
            if (movingEntity.qtEntity.position.y < -movingEntity.bounds.y + halfHeight) {
                movingEntity.qtEntity.position.y = -movingEntity.bounds.y + halfHeight;
            }
            else if (movingEntity.qtEntity.position.y > movingEntity.bounds.y - halfHeight) {
                movingEntity.qtEntity.position.y = movingEntity.bounds.y - halfHeight;
            }
        }

        // Update sprite position to match
        if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                sprite->setPosition(
                    movingEntity.qtEntity.position.x,
                    movingEntity.qtEntity.position.y,
                    0.0f
                );
            }
        }
    }

    // Update quadtree periodically (not every frame for performance)
    m_updateTimer += dt;
    if (m_updateTimer >= m_updateInterval) {
        updateQuadtreePartitioning();
        m_updateTimer = 0.0f;
    }
}
void PartitionScene::updateQuadtreePartitioning() {
    // Clear and rebuild quadtree
    m_quadtree = std::make_unique<Quadtree>(
        Vec2(0.0f, 0.0f),
        Vec2(800.0f, 600.0f),
        4, 5
    );

    // Re-insert all active moving entities
    for (const auto& movingEntity : m_movingEntities) {
        if (movingEntity.active) {
            m_quadtree->insert(movingEntity.qtEntity);
        }
    }

    updateQuadtreeVisualization();
}
void PartitionScene::clearAllEntities() {
    // Clear moving entities vector
    m_movingEntities.clear();

    // Collect names of entities to remove
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();

    for (const auto& entity : entities) {
        std::string entityName = entity->getName();
        if (entityName.find("TestEntity") == 0) {
            entitiesToRemove.insert(entityName);
        }
    }

    // Remove entities
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }

    // Rebuild quadtree
    m_quadtree = std::make_unique<Quadtree>(
        Vec2(0.0f, 0.0f),
        Vec2(800.0f, 600.0f),
        4, 5
    );

    updateQuadtreeVisualization();
}
void PartitionScene::updateQuadtreeVisualization() {
    if (!m_lineRenderer) return;

    m_lineRenderer->clear();

    if (!m_showQuadtree) return;

    // Use world space rendering for quadtree visualization
    m_lineRenderer->enableScreenSpace(false);

    // Get all quadtree nodes and draw their boundaries
    std::vector<Quadtree*> nodes;
    m_quadtree->getAllNodes(nodes);

    for (auto* node : nodes) {
        Vec2 center = node->getCenter();
        Vec2 size = node->getSize();

        // Apply offset to center position for visualization
        Vec2 visualCenter = center + m_quadtreeVisualOffset;

        // Draw node boundary with offset
        m_lineRenderer->addRect(visualCenter, size, Vec4(1.0f, 1.0f, 1.0f, 0.5f), 1.0f);

        // Draw entities in this node with the same offset
        const auto& entities = node->getEntities();
        for (const auto& entity : entities) {
            Vec2 visualEntityPos = entity.position + m_quadtreeVisualOffset;
            m_lineRenderer->addRect(visualEntityPos, entity.size, Vec4(1.0f, 0.0f, 0.0f, 0.8f), 2.0f);
        }
    }
}

void PartitionScene::update(float dt) {
    auto& input = Input::getInstance();

    // Update camera movement
    updateCameraMovement(dt);

    // Update moving entities
    updateMovingEntities(dt);

    // Handle mouse input for adding entities
    if (input.wasMouseJustPressed(MouseClick::LeftMouse)) {
        Vec2 mousePos = input.getMousePositionNDC();
        Vec2 worldPos = screenToWorldPosition(mousePos);
        addEntityAtPosition(worldPos);
    }

    // Update button components
    auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
    for (auto* entity : buttonEntities) {
        if (auto* button = entity->getComponent<ButtonComponent>()) {
            button->update(dt);
        }
    }
}

void PartitionScene::updateCameraMovement(float dt) {
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

void PartitionScene::addEntityAtPosition(const Vec2& worldPos) {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));

    Vec2 size(20.0f, 20.0f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(-80.0f, 80.0f);
    Vec2 velocity(velDist(gen), velDist(gen));

    auto& sprite = entity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png",
        size.x,
        size.y
    );
    sprite.setPosition(worldPos.x, worldPos.y, 0.0f);
    sprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));

    // Create moving entity
    MovingEntity movingEntity;
    movingEntity.name = "TestEntity" + std::to_string(m_entityCounter);
    movingEntity.velocity = velocity;
    movingEntity.bounds = Vec2(400.0f, 300.0f);
    movingEntity.qtEntity.position = worldPos;
    movingEntity.qtEntity.size = size;
    movingEntity.qtEntity.id = m_entityCounter;
    movingEntity.active = true;

    m_movingEntities.push_back(movingEntity);
    m_quadtree->insert(movingEntity.qtEntity);

    m_entityCounter++;
    updateQuadtreeVisualization();
}

Vec2 PartitionScene::screenToWorldPosition(const Vec2& screenPos) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return Vec2(0.0f, 0.0f);

    auto* camera = cameraEntity->getComponent<Camera2D>();
    if (!camera) return Vec2(0.0f, 0.0f);

    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // screenPos is UV-like: (0,0) = bottom-left, (1,1) = top-right
    // Convert UV coordinates to NDC [-1,1] range
    float ndcX = screenPos.x * 2.0f - 1.0f;  // [0,1] -> [-1,1]
    float ndcY = screenPos.y * 2.0f - 1.0f;  // [0,1] -> [-1,1] (no flip, Y is already bottom-up)

    // Apply inverse projection to get view space coordinates
    float viewX = ndcX * (screenWidth * 0.5f) / camera->getZoom();
    float viewY = ndcY * (screenHeight * 0.5f) / camera->getZoom();

    // Transform from view space to world space by adding camera position
    Vec2 cameraPos = camera->getPosition();
    Vec2 worldPos;
    worldPos.x = viewX + cameraPos.x;
    worldPos.y = viewY + cameraPos.y;

    return worldPos;
}

void PartitionScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // Set camera matrices for world space rendering
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // FIXED: Render quadtree lines in world space FIRST (behind sprites)
    if (m_lineRenderer && m_showQuadtree) {
        m_lineRenderer->draw(ctx);
    }

    // Render sprites in world space
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    ctx.setScreenSpaceMatrices(screenWidth, screenHeight);

    // Render UI elements in screen space
    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx);
            }
        }
    }

    for (auto* entity : m_entityManager->getEntitiesWithComponent<ButtonComponent>()) {
        if (auto* button = entity->getComponent<ButtonComponent>()) {
            if (button->isVisible()) {
                button->draw(ctx);
            }
        }
    }

    engine.endFrame(swapChain);
}

void PartitionScene::fixedUpdate(float dt) {
    // Fixed update logic if needed
}

