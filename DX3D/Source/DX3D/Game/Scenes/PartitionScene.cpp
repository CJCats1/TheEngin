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
#include <limits>
#include <cmath>
#include <algorithm>

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

    float testCrossSize = 20.0f;
    Vec2 worldOrigin(0.0f, 0.0f);
    m_lineRenderer->addLine(
        Vec2(worldOrigin.x - testCrossSize, worldOrigin.y),
        Vec2(worldOrigin.x + testCrossSize, worldOrigin.y),
        Vec4(1.0f, 0.0f, 1.0f, 1.0f), 3.0f  // Magenta color, thick line
    );
    m_lineRenderer->addLine(
        Vec2(worldOrigin.x, worldOrigin.y - testCrossSize),
        Vec2(worldOrigin.x, worldOrigin.y + testCrossSize),
        Vec4(1.0f, 0.0f, 1.0f, 1.0f), 3.0f  // Magenta color, thick line
    );
    // Create invisible sprite at world origin to "anchor" the coordinate system
    //auto& anchorEntity = m_entityManager->createEntity("WorldOriginAnchor");
    //auto& anchorSprite = anchorEntity.addComponent<SpriteComponent>(
    //    device,
    //    L"DX3D/Assets/Textures/node.png",
    //    1.0f, 1.0f  // Tiny size
    //);
    //anchorSprite.setPosition(0.0f, 0.0f, 0.0f); // World origin
    //anchorSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Completely transparent
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

    // K-means clustering buttons
    auto& kmeansButtonEntity = m_entityManager->createEntity("KMeansButton");
    auto& kmeansButton = kmeansButtonEntity.addComponent<ButtonComponent>(
        device, L"K-Means Clustering", 18.0f
    );
    kmeansButton.setScreenPosition(0.1f, 0.3f);
    kmeansButton.setNormalTint(Vec4(0.2f, 0.8f, 0.8f, 0.8f));
    kmeansButton.setOnClickCallback([this]() {
        m_kmeansEnabled = !m_kmeansEnabled;
        if (m_kmeansEnabled) {
            performKMeansClustering();
        } else {
            // Reset colors to default
            for (auto& movingEntity : m_movingEntities) {
                if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
                    }
                }
            }
            m_clusters.clear();
        }
        });

    // Increase K button
    auto& increaseKButtonEntity = m_entityManager->createEntity("IncreaseKButton");
    auto& increaseKButton = increaseKButtonEntity.addComponent<ButtonComponent>(
        device, L"Increase K", 18.0f
    );
    increaseKButton.setScreenPosition(0.1f, 0.35f);
    increaseKButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    increaseKButton.setOnClickCallback([this]() {
        if (m_kmeansK < 8) {
            m_kmeansK++;
            if (m_kmeansEnabled) {
                performKMeansClustering();
            }
        }
        });

    // Decrease K button
    auto& decreaseKButtonEntity = m_entityManager->createEntity("DecreaseKButton");
    auto& decreaseKButton = decreaseKButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease K", 18.0f
    );
    decreaseKButton.setScreenPosition(0.1f, 0.4f);
    decreaseKButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    decreaseKButton.setOnClickCallback([this]() {
        if (m_kmeansK > 2) {
            m_kmeansK--;
            if (m_kmeansEnabled) {
                performKMeansClustering();
            }
        }
        });

    // Toggle cluster visualization button
    auto& toggleClusterVizButtonEntity = m_entityManager->createEntity("ToggleClusterVizButton");
    auto& toggleClusterVizButton = toggleClusterVizButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle Cluster Lines", 18.0f
    );
    toggleClusterVizButton.setScreenPosition(0.1f, 0.45f);
    toggleClusterVizButton.setNormalTint(Vec4(0.8f, 0.6f, 0.2f, 0.8f));
    toggleClusterVizButton.setOnClickCallback([this]() {
        m_showClusterVisualization = !m_showClusterVisualization;
        updateQuadtreeVisualization();
        });

    // Fast mode button
    auto& fastModeButtonEntity = m_entityManager->createEntity("FastModeButton");
    auto& fastModeButton = fastModeButtonEntity.addComponent<ButtonComponent>(
        device, L"Fast Mode", 18.0f
    );
    fastModeButton.setScreenPosition(0.1f, 0.5f);
    fastModeButton.setNormalTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
    fastModeButton.setOnClickCallback([this, &fastModeButton]() {
        m_fastMode = !m_fastMode;
        // Update button color to show state
        if (m_fastMode) {
            fastModeButton.setNormalTint(Vec4(0.8f, 0.2f, 0.2f, 0.8f)); // Red when active
        } else {
            fastModeButton.setNormalTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f)); // Green when inactive
        }
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
        
        // Update entity assignments dynamically if enabled and entities are moving
        if (m_kmeansEnabled && m_entitiesMoving) {
            m_kmeansUpdateTimer += dt;
            
            // Use faster update interval in fast mode
            float updateInterval = m_fastMode ? 0.02f : m_kmeansUpdateInterval;
            
            if (m_kmeansUpdateTimer >= updateInterval) {
                updateEntityAssignments();
                m_kmeansUpdateTimer = 0.0f;
            }
        }
        
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

    // Use world space rendering for quadtree visualization
    m_lineRenderer->enableScreenSpace(false);

    // Draw K-means cluster visualization if enabled
    if (m_kmeansEnabled && m_showClusterVisualization && !m_clusters.empty()) {
        // Draw cluster centroids
        for (const auto& cluster : m_clusters) {
            Vec2 visualCentroid = cluster.centroid + m_quadtreeVisualOffset;
            m_lineRenderer->addRect(visualCentroid, Vec2(15.0f, 15.0f), cluster.color, 3.0f);
            
            // Draw lines from centroid to entities in this cluster
            for (int entityIndex : cluster.entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addLine(visualCentroid, entityPos, cluster.color, 1.0f);
                }
            }
        }
    }

    if (!m_showQuadtree) return;

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

    // Handle mouse input for adding entities (only if not clicking on UI)
    if (input.wasMouseJustPressed(MouseClick::LeftMouse)) {
        // Check if mouse is over any UI elements first
        bool mouseOverUI = false;
        
        // Check if mouse is over any buttons
        auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
        for (auto* entity : buttonEntities) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                Vec2 mousePos = input.getMousePositionNDC();
                if (button->isPointInside(mousePos)) {
                    mouseOverUI = true;
                    break;
                }
            }
        }
        
        // Only spawn entity if not clicking on UI
        if (!mouseOverUI) {
            Vec2 mousePos = input.getMousePositionNDC();
            Vec2 worldPos = screenToWorldPosition(mousePos);
            addEntityAtPosition(worldPos);
        }
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

// K-means clustering implementation
void PartitionScene::performKMeansClustering() {
    if (m_movingEntities.empty()) return;
    
    m_kmeansIterations = 0;
    m_kmeansConverged = false;
    
    // Initialize clusters
    m_clusters.clear();
    m_clusters.resize(m_kmeansK);
    
    // Initialize entity tracking
    initializeEntityTracking();
    
    initializeKMeansCentroids();
    
    // Perform K-means iterations
    while (m_kmeansIterations < m_maxKmeansIterations && !m_kmeansConverged) {
        assignEntitiesToClusters();
        updateClusterCentroids();
        m_kmeansIterations++;
    }
    
    // Update entity colors based on cluster assignments
    updateEntityColors();
    
    // Store current centroids for stability checking
    storePreviousCentroids();
    
    // Update quadtree visualization to show cluster lines
    updateQuadtreeVisualization();
}

void PartitionScene::initializeKMeansCentroids() {
    // Use existing centroids if available and stable
    if (!m_previousCentroids.empty() && m_previousCentroids.size() == m_kmeansK) {
        for (int i = 0; i < m_kmeansK; i++) {
            m_clusters[i].centroid = m_previousCentroids[i];
            m_clusters[i].color = getClusterColor(i);
            m_clusters[i].entityIndices.clear();
        }
        return;
    }
    
    // Initialize with random positions, but try to spread them out
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> yDist(-250.0f, 250.0f);
    
    for (int i = 0; i < m_kmeansK; i++) {
        // Try to place centroids away from existing ones
        Vec2 newCentroid;
        int attempts = 0;
        do {
            newCentroid = Vec2(xDist(gen), yDist(gen));
            attempts++;
        } while (attempts < 10 && [&]() {
            for (int j = 0; j < i; j++) {
                if (calculateDistance(newCentroid, m_clusters[j].centroid) < 100.0f) {
                    return true; // Too close to existing centroid
                }
            }
            return false;
        }());
        
        m_clusters[i].centroid = newCentroid;
        m_clusters[i].color = getClusterColor(i);
        m_clusters[i].entityIndices.clear();
    }
}

void PartitionScene::assignEntitiesToClusters() {
    // Clear previous assignments
    for (auto& cluster : m_clusters) {
        cluster.entityIndices.clear();
    }
    
    // Reset entity tracking
    std::fill(m_entityClusterAssignments.begin(), m_entityClusterAssignments.end(), -1);
    std::fill(m_entityDistancesToCentroids.begin(), m_entityDistancesToCentroids.end(), std::numeric_limits<float>::max());
    
    // Use quadtree for optimized spatial assignment
    if (m_quadtree) {
        // For each entity, use quadtree to find the closest cluster centroid
        for (int i = 0; i < m_movingEntities.size(); i++) {
            if (!m_movingEntities[i].active) continue;
            
            Vec2 entityPos = m_movingEntities[i].qtEntity.position;
            float minDistance = std::numeric_limits<float>::max();
            int closestCluster = 0;
            
            // Use quadtree to find nearby clusters more efficiently
            // Calculate dynamic search radius based on current cluster distribution
            float maxClusterDistance = 0.0f;
            for (int j = 0; j < m_kmeansK; j++) {
                float distanceSquared = calculateDistanceSquared(entityPos, m_clusters[j].centroid);
                float distance = std::sqrt(distanceSquared);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
                maxClusterDistance = std::max(maxClusterDistance, distance);
            }
            
            // Use quadtree to verify if there are any closer entities that might affect cluster boundaries
            float searchRadius = std::min(maxClusterDistance * 0.5f, 300.0f); // Dynamic search radius
            auto nearbyEntities = m_quadtree->query(entityPos, Vec2(searchRadius, searchRadius));
            
            // Check if any nearby entities are in different clusters that might be closer
            bool needsRecheck = false;
            for (const auto& qtEntity : nearbyEntities) {
                // Use helper method to find entity index
                int entityIdx = findEntityIndexByQuadtreeId(qtEntity.id);
                if (entityIdx == -1) continue;
                
                // Find which cluster this entity belongs to
                for (int clusterIdx = 0; clusterIdx < m_kmeansK; clusterIdx++) {
                    if (isEntityInCluster(entityIdx, clusterIdx)) {
                        float distanceToNearbyCluster = calculateDistance(entityPos, m_clusters[clusterIdx].centroid);
                        if (distanceToNearbyCluster < minDistance) {
                            minDistance = distanceToNearbyCluster;
                            closestCluster = clusterIdx;
                            needsRecheck = true;
                        }
                        break;
                    }
                }
            }
            
            // Assign entity to closest cluster
            m_clusters[closestCluster].entityIndices.push_back(i);
            m_entityClusterAssignments[i] = closestCluster;
            m_entityDistancesToCentroids[i] = minDistance;
        }
    } else {
        // Fallback to brute force assignment
        for (int i = 0; i < m_movingEntities.size(); i++) {
            if (!m_movingEntities[i].active) continue;
            
            float minDistance = std::numeric_limits<float>::max();
            int closestCluster = 0;
            
            for (int j = 0; j < m_kmeansK; j++) {
                float distance = calculateDistance(m_movingEntities[i].qtEntity.position, m_clusters[j].centroid);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
            }
            
            m_clusters[closestCluster].entityIndices.push_back(i);
            m_entityClusterAssignments[i] = closestCluster;
            m_entityDistancesToCentroids[i] = minDistance;
        }
    }
}

void PartitionScene::updateClusterCentroids() {
    bool converged = true;
    
    for (int i = 0; i < m_kmeansK; i++) {
        if (m_clusters[i].entityIndices.empty()) continue;
        
        Vec2 newCentroid(0.0f, 0.0f);
        int validEntityCount = 0;
        
        // Use quadtree to optimize centroid calculation for large clusters
        if (m_quadtree && m_clusters[i].entityIndices.size() > 10) {
            // Use quadtree to find entities in the cluster's spatial region
            Vec2 clusterCenter = m_clusters[i].centroid;
            float clusterRadius = 0.0f;
            
            // Calculate cluster radius based on current entities
            for (int entityIndex : m_clusters[i].entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    float distance = calculateDistance(clusterCenter, m_movingEntities[entityIndex].qtEntity.position);
                    clusterRadius = std::max(clusterRadius, distance);
                }
            }
            
            // Use quadtree to find entities in the cluster region
            Vec2 searchSize(clusterRadius * 1.5f, clusterRadius * 1.5f);
            auto nearbyEntities = m_quadtree->query(clusterCenter, searchSize);
            
            // Calculate centroid from quadtree results
            for (const auto& qtEntity : nearbyEntities) {
                // Use helper method to find entity index
                int entityIndex = findEntityIndexByQuadtreeId(qtEntity.id);
                if (entityIndex != -1 && isEntityInCluster(entityIndex, i)) {
                    newCentroid.x += qtEntity.position.x;
                    newCentroid.y += qtEntity.position.y;
                    validEntityCount++;
                }
            }
        } else {
            // Standard centroid calculation for small clusters
            for (int entityIndex : m_clusters[i].entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    newCentroid.x += m_movingEntities[entityIndex].qtEntity.position.x;
                    newCentroid.y += m_movingEntities[entityIndex].qtEntity.position.y;
                    validEntityCount++;
                }
            }
        }
        
        if (validEntityCount > 0) {
            newCentroid.x /= validEntityCount;
            newCentroid.y /= validEntityCount;
            
            // Check for convergence
            float distance = calculateDistance(m_clusters[i].centroid, newCentroid);
            float convergenceThreshold = m_fastMode ? 0.1f : 0.05f; // More lenient in fast mode
            if (distance > convergenceThreshold) {
                converged = false;
            }
            
            m_clusters[i].centroid = newCentroid;
        }
    }
    
    m_kmeansConverged = converged;
}

void PartitionScene::updateEntityColors() {
    for (int i = 0; i < m_kmeansK; i++) {
        for (int entityIndex : m_clusters[i].entityIndices) {
            if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) continue;
            
            if (auto* entity = m_entityManager->findEntity(m_movingEntities[entityIndex].name)) {
                if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                    // Immediate color update for cluster assignments
                    sprite->setTint(m_clusters[i].color);
                }
            }
        }
    }
}

Vec4 PartitionScene::getClusterColor(int clusterIndex) {
    // Generate distinct colors for clusters
    static std::vector<Vec4> colors = {
        Vec4(1.0f, 0.0f, 0.0f, 0.8f), // Red
        Vec4(0.0f, 1.0f, 0.0f, 0.8f), // Green
        Vec4(0.0f, 0.0f, 1.0f, 0.8f), // Blue
        Vec4(1.0f, 1.0f, 0.0f, 0.8f), // Yellow
        Vec4(1.0f, 0.0f, 1.0f, 0.8f), // Magenta
        Vec4(0.0f, 1.0f, 1.0f, 0.8f), // Cyan
        Vec4(1.0f, 0.5f, 0.0f, 0.8f), // Orange
        Vec4(0.5f, 0.0f, 1.0f, 0.8f)  // Purple
    };
    
    return colors[clusterIndex % colors.size()];
}

float PartitionScene::calculateDistance(const Vec2& a, const Vec2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool PartitionScene::shouldUpdateClustering() {
    if (m_clusters.empty() || m_previousCentroids.empty()) {
        return true; // First time or no previous data
    }
    
    if (m_clusters.size() != m_previousCentroids.size()) {
        return true; // Number of clusters changed
    }
    
    // Check if centroids have moved significantly
    float totalMovement = 0.0f;
    for (size_t i = 0; i < m_clusters.size(); i++) {
        float movement = calculateDistance(m_clusters[i].centroid, m_previousCentroids[i]);
        totalMovement += movement;
    }
    
    // Only update if centroids have moved significantly
    return totalMovement > m_clusterStabilityThreshold;
}

void PartitionScene::storePreviousCentroids() {
    m_previousCentroids.clear();
    for (const auto& cluster : m_clusters) {
        m_previousCentroids.push_back(cluster.centroid);
    }
}

// Dynamic clustering methods
void PartitionScene::initializeEntityTracking() {
    m_entityClusterAssignments.clear();
    m_entityDistancesToCentroids.clear();
    m_entityClusterAssignments.resize(m_movingEntities.size(), -1); // -1 means unassigned
    m_entityDistancesToCentroids.resize(m_movingEntities.size(), std::numeric_limits<float>::max());
}

void PartitionScene::ensureTrackingArraysSize() {
    size_t requiredSize = m_movingEntities.size();
    if (m_entityClusterAssignments.size() != requiredSize) {
        m_entityClusterAssignments.resize(requiredSize, -1);
    }
    if (m_entityDistancesToCentroids.size() != requiredSize) {
        m_entityDistancesToCentroids.resize(requiredSize, std::numeric_limits<float>::max());
    }
}

void PartitionScene::updateEntityAssignments() {
    if (m_clusters.empty()) return;
    
    // Ensure tracking arrays are properly sized
    ensureTrackingArraysSize();
    
    m_assignmentsChanged = false;
    
    // Update centroids first based on current assignments
    updateClusterCentroids();
    
    // Check each entity to see if it should change clusters
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        // Only update if entity has moved significantly
        if (hasEntityMovedSignificantly(i)) {
            updateSingleEntityAssignment(i);
        }
    }
    
    // Update colors if assignments changed
    if (m_assignmentsChanged) {
        updateEntityColors(); // Use immediate color updates for cluster changes
        updateQuadtreeVisualization();
    }
}

void PartitionScene::updateSingleEntityAssignment(int entityIndex) {
    if (m_clusters.empty()) return;
    
    // Bounds checking to prevent vector subscript out of range
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return;
    
    // Ensure tracking arrays are properly sized
    ensureTrackingArraysSize();
    
    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position;
    float minDistance = std::numeric_limits<float>::max();
    int closestCluster = 0;
    
    // Use quadtree for optimized nearest cluster search
    if (m_quadtree) {
        // First, do a quick check with current cluster assignments
        int currentAssignment = m_entityClusterAssignments[entityIndex];
        if (currentAssignment >= 0 && currentAssignment < m_clusters.size()) {
            float currentDistance = calculateDistance(entityPos, m_clusters[currentAssignment].centroid);
            minDistance = currentDistance;
            closestCluster = currentAssignment;
        }
        
        // Use quadtree to find nearby clusters more efficiently
        float searchRadius = std::min(minDistance * 1.5f, 200.0f); // Search radius based on current distance
        auto nearbyEntities = m_quadtree->query(entityPos, Vec2(searchRadius, searchRadius));
        
        // Check if any nearby entities suggest a closer cluster
        for (const auto& qtEntity : nearbyEntities) {
            // Use helper method to find entity index
            int entityIdx = findEntityIndexByQuadtreeId(qtEntity.id);
            if (entityIdx == -1) continue;
            
            // Find which cluster this entity belongs to
            for (int clusterIdx = 0; clusterIdx < m_kmeansK; clusterIdx++) {
                if (isEntityInCluster(entityIdx, clusterIdx)) {
                    float distanceToCluster = calculateDistance(entityPos, m_clusters[clusterIdx].centroid);
                    if (distanceToCluster < minDistance) {
                        minDistance = distanceToCluster;
                        closestCluster = clusterIdx;
                    }
                    break;
                }
            }
        }
        
        // If we didn't find a closer cluster through quadtree, do a limited brute force check
        if (closestCluster == currentAssignment) {
            // Only check clusters that are reasonably close
            for (int j = 0; j < m_kmeansK; j++) {
                if (j == currentAssignment) continue;
                
                float distance = calculateDistance(entityPos, m_clusters[j].centroid);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
            }
        }
    } else {
        // Fallback to brute force
        for (int j = 0; j < m_kmeansK; j++) {
            float distance = calculateDistance(entityPos, m_clusters[j].centroid);
            if (distance < minDistance) {
                minDistance = distance;
                closestCluster = j;
            }
        }
    }
    
    // Check if assignment changed
    int currentAssignment = m_entityClusterAssignments[entityIndex];
    if (currentAssignment != closestCluster) {
        // Remove from old cluster
        if (currentAssignment >= 0 && currentAssignment < m_clusters.size()) {
            auto& oldCluster = m_clusters[currentAssignment];
            oldCluster.entityIndices.erase(
                std::remove(oldCluster.entityIndices.begin(), oldCluster.entityIndices.end(), entityIndex),
                oldCluster.entityIndices.end()
            );
        }
        
        // Add to new cluster
        m_clusters[closestCluster].entityIndices.push_back(entityIndex);
        m_entityClusterAssignments[entityIndex] = closestCluster;
        m_entityDistancesToCentroids[entityIndex] = minDistance;
        m_assignmentsChanged = true;
    }
}

bool PartitionScene::hasEntityMovedSignificantly(int entityIndex) {
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return false;
    
    // More responsive movement detection
    if (!m_entitiesMoving) return false;
    
    // Check if entity has moved a significant distance since last check
    // This could be enhanced with position tracking, but for now we'll be more aggressive
    return true; // Always check when entities are moving for maximum responsiveness
}

void PartitionScene::smoothColorTransitions() {
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        // Bounds checking
        if (i >= m_entityClusterAssignments.size()) continue;
        
        int clusterIndex = m_entityClusterAssignments[i];
        if (clusterIndex < 0 || clusterIndex >= m_clusters.size()) continue;
        
        if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                Vec4 targetColor = m_clusters[clusterIndex].color;
                Vec4 currentColor = sprite->getTint();
                
                // Check if we're close enough to target to avoid infinite lerping
                float colorDifference = std::abs(currentColor.x - targetColor.x) + 
                                      std::abs(currentColor.y - targetColor.y) + 
                                      std::abs(currentColor.z - targetColor.z);
                
                if (colorDifference < 0.01f) {
                    // Close enough, set exact target color
                    sprite->setTint(targetColor);
                } else {
                    // Smooth transition over time
                    float lerpFactor = 0.5f; // Faster transition
                    Vec4 newColor = Vec4(
                        currentColor.x + (targetColor.x - currentColor.x) * lerpFactor,
                        currentColor.y + (targetColor.y - currentColor.y) * lerpFactor,
                        currentColor.z + (targetColor.z - currentColor.z) * lerpFactor,
                        targetColor.w
                    );
                    
                    sprite->setTint(newColor);
                }
            }
        }
    }
}

// Quadtree optimization helper methods
int PartitionScene::findEntityIndexByQuadtreeId(int qtEntityId) {
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (m_movingEntities[i].qtEntity.id == qtEntityId && m_movingEntities[i].active) {
            return i;
        }
    }
    return -1;
}

float PartitionScene::calculateDistanceSquared(const Vec2& a, const Vec2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy; // Avoid sqrt for distance comparisons
}

bool PartitionScene::isEntityInCluster(int entityIndex, int clusterIndex) {
    if (clusterIndex < 0 || clusterIndex >= m_clusters.size()) return false;
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return false;
    
    const auto& cluster = m_clusters[clusterIndex];
    return std::find(cluster.entityIndices.begin(), cluster.entityIndices.end(), entityIndex) != cluster.entityIndices.end();
}

void PartitionScene::optimizeSpatialQueries() {
    // Pre-calculate cluster bounding boxes for faster spatial queries
    for (int i = 0; i < m_kmeansK; i++) {
        if (m_clusters[i].entityIndices.empty()) continue;
        
        // Calculate cluster bounding box
        Vec2 minPos(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vec2 maxPos(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        
        for (int entityIndex : m_clusters[i].entityIndices) {
            if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                Vec2 pos = m_movingEntities[entityIndex].qtEntity.position;
                minPos.x = std::min(minPos.x, pos.x);
                minPos.y = std::min(minPos.y, pos.y);
                maxPos.x = std::max(maxPos.x, pos.x);
                maxPos.y = std::max(maxPos.y, pos.y);
            }
        }
        
        // Store bounding box info for future optimizations
        // This could be used to optimize quadtree queries further
    }
}

