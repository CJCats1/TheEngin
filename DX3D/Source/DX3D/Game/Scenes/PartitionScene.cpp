#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Quadtree.h>
#include <DX3D/Components/Octree.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <random>
#include <iostream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <set>
#include <imgui.h>

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

    // Initialize spatial partitions
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);

    // Create some test entities
    createTestEntities(device);

    // Disable engine UI; ImGui will provide controls

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
    auto& anchorEntity = m_entityManager->createEntity("WorldOriginAnchor");
    auto& anchorSprite = anchorEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png",
        1.0f, 1.0f  // Tiny size
    );
    anchorSprite.setPosition(0.0f, 0.0f, 0.0f); // World origin
    anchorSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Completely transparent
    anchorSprite.setVisible(true);
}

void PartitionScene::createTestEntities(GraphicsDevice& device) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
    std::uniform_real_distribution<float> velDist(-120.0f, 120.0f); // increased speed for further travel

    // Enforce a minimum spawn spacing to reduce clustering
    const float minSpawnDistance = 45.0f; // pixels
    std::vector<Vec2> acceptedPositions;

    for (int i = 0; i < 50; i++) {
        auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(i));
        m_entityCounter++;

        // Rejection sample until we find a position that is far enough from others
        Vec2 position;
        int attempts = 0;
        do {
            position = Vec2(posDist(gen), posDist(gen));
            attempts++;
            if (attempts > 200) break; // fail-safe to avoid infinite loops
        } while (std::any_of(acceptedPositions.begin(), acceptedPositions.end(), [&](const Vec2& p){
            float dx = p.x - position.x;
            float dy = p.y - position.y;
            return (dx*dx + dy*dy) < (minSpawnDistance * minSpawnDistance);
        }));
        acceptedPositions.push_back(position);
        Vec2 size(sizeDist(gen), sizeDist(gen));
        Vec2 velocity(velDist(gen), velDist(gen)); // Random initial velocity (faster)

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
        // Expand bounds so entities can travel further before bouncing
        movingEntity.bounds = Vec2(400.0f, 300.0f);
        movingEntity.qtEntity.position = position;
        movingEntity.qtEntity.size = size;
        movingEntity.qtEntity.id = i;
        movingEntity.active = true;

        m_movingEntities.push_back(movingEntity);

        // Add to quadtree
        m_quadtree->insert(movingEntity.qtEntity);
    }
}


void PartitionScene::setSimulationSpeed(dx3d::SimulationSpeed speed) {
    m_simulationSpeed = speed;
    
    // Update speed multiplier based on simulation speed
    switch (speed) {
        case dx3d::SimulationSpeed::Paused:
            m_simulationSpeedMultiplier = 0.0f;
            break;
        case dx3d::SimulationSpeed::Normal:
            m_simulationSpeedMultiplier = 1.0f;
            break;
        case dx3d::SimulationSpeed::Fast:
            m_simulationSpeedMultiplier = 2.0f;
            break;
        case dx3d::SimulationSpeed::VeryFast:
            m_simulationSpeedMultiplier = 4.0f;
            break;
    }
    
    // Update button states
    if (m_pauseButton) {
        m_pauseButton->setNormalTint(speed == dx3d::SimulationSpeed::Paused ? 
            Vec4(1.0f, 0.5f, 0.5f, 0.9f) : Vec4(0.8f, 0.2f, 0.2f, 0.9f));
    }
    if (m_playButton) {
        m_playButton->setNormalTint(speed == dx3d::SimulationSpeed::Normal ? 
            Vec4(0.5f, 1.0f, 0.5f, 0.9f) : Vec4(0.2f, 0.8f, 0.2f, 0.9f));
    }
    if (m_fastForwardButton) {
        m_fastForwardButton->setNormalTint((speed == dx3d::SimulationSpeed::Fast || speed == dx3d::SimulationSpeed::VeryFast) ? 
            Vec4(0.5f, 0.5f, 1.0f, 0.9f) : Vec4(0.2f, 0.2f, 0.8f, 0.9f));
    }
}

void PartitionScene::updateSpeedControls() {
    if (!m_speedIndicatorText) return;
    
    // Update speed indicator text
    std::wstring speedText = L"Speed: ";
    if (m_simulationSpeed == dx3d::SimulationSpeed::Paused) {
        speedText += L"Paused";
    } else {
        speedText += std::to_wstring(static_cast<int>(m_simulationSpeedMultiplier * 10) / 10.0f) + L"x";
    }
    m_speedIndicatorText->setText(speedText);
}

void PartitionScene::addRandomEntities() {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
    std::uniform_real_distribution<float> velDist(-120.0f, 120.0f); // faster movement

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
        // Expanded bounds for further travel
        movingEntity.bounds = Vec2(600.0f, 450.0f);
        movingEntity.qtEntity.position = position;
        movingEntity.qtEntity.size = size;
        movingEntity.qtEntity.id = m_entityCounter;
        movingEntity.active = true;

        m_movingEntities.push_back(movingEntity);
        
        // Add to quadtree (supports incremental insertion)
        m_quadtree->insert(movingEntity.qtEntity);
        
        // AABBTree and KDTree need to be rebuilt as they don't support incremental insertion
        std::vector<QuadtreeEntity> allEntities;
        for (const auto& me : m_movingEntities) {
            if (me.active) allEntities.push_back(me.qtEntity);
        }
        m_aabbTree->buildFrom(allEntities);
        m_kdTree->buildFrom(allEntities);

        m_entityCounter++;
    }

    updateQuadtreeVisualization();
}
void PartitionScene::updateMovingEntities(float dt) {
    if (!m_entitiesMoving) return;
    
    // Apply simulation speed multiplier
    float effectiveDt = dt * m_simulationSpeedMultiplier;
    if (effectiveDt <= 0.0f) return; // Skip update if paused

    // Update POI attraction if enabled
    if (m_poiSystemEnabled && !m_pointsOfInterest.empty()) {
        updatePOIAttraction();
    }

    // Update positions
    for (auto& movingEntity : m_movingEntities) {
        if (!movingEntity.active) continue;

        // Update position with speed multiplier
        movingEntity.qtEntity.position.x += movingEntity.velocity.x * effectiveDt * m_entitySpeedMultiplier;
        movingEntity.qtEntity.position.y += movingEntity.velocity.y * effectiveDt * m_entitySpeedMultiplier;

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
    m_updateTimer += effectiveDt;
    if (m_updateTimer >= m_updateInterval) {
        updateQuadtreePartitioning();
        
        // Update entity assignments dynamically if enabled and entities are moving
        if (m_kmeansEnabled && m_entitiesMoving) {
            m_kmeansUpdateTimer += effectiveDt;
            
            // Use faster update interval in fast mode
            float updateInterval = m_fastMode ? 0.02f : m_kmeansUpdateInterval;
            
            if (m_kmeansUpdateTimer >= updateInterval) {
                updateEntityAssignments();
                m_kmeansUpdateTimer = 0.0f;
            }
        }
        
        // Recompute DBSCAN periodically when enabled and entities are moving
        if (m_dbscanEnabled && m_entitiesMoving) {
            m_dbscanUpdateTimer += effectiveDt;
            float dbscanInterval = m_fastMode ? 0.05f : m_dbscanUpdateInterval; // faster when fast mode
            if (m_dbscanUpdateTimer >= dbscanInterval) {
                performDBSCANClustering();
                m_dbscanUpdateTimer = 0.0f;
            }
        }
        
        m_updateTimer = 0.0f;
    }
}
void PartitionScene::updateQuadtreePartitioning() {
    if (m_partitionType == PartitionType::Quadtree) {
        // Clear and rebuild quadtree
        m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
        for (const auto& movingEntity : m_movingEntities) {
            if (movingEntity.active) m_quadtree->insert(movingEntity.qtEntity);
        }
    } else if (m_partitionType == PartitionType::AABB) {
        // Rebuild AABB tree
        std::vector<QuadtreeEntity> ents;
        ents.reserve(m_movingEntities.size());
        for (const auto& movingEntity : m_movingEntities) {
            if (movingEntity.active) ents.push_back(movingEntity.qtEntity);
        }
        m_aabbTree->buildFrom(ents);
    } else {
        // Rebuild KD tree
        std::vector<QuadtreeEntity> ents;
        ents.reserve(m_movingEntities.size());
        for (const auto& movingEntity : m_movingEntities) {
            if (movingEntity.active) ents.push_back(movingEntity.qtEntity);
        }
        m_kdTree->buildFrom(ents);
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

    // Rebuild active spatial partition empty
    if (m_partitionType == PartitionType::Quadtree) {
        m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    } else if (m_partitionType == PartitionType::AABB) {
        m_aabbTree->clear();
    } else {
        m_kdTree->clear();
    }

    updateQuadtreeVisualization();
}
// moved to PartitionScene.Visualization.cpp

// moved to PartitionScene.Visualization.cpp

void PartitionScene::update(float dt) {
    auto& input = Input::getInstance();

    if (m_is3DMode) {
        // Update 3D camera
        update3DCamera(dt);
        
        // Update 3D moving entities
        update3DMovingEntities(dt);
    } else {
        // Update 2D camera movement
        updateCameraMovement(dt);

        // Update 2D moving entities
        updateMovingEntities(dt);
    }

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
        
        // Only add entity or POI if not clicking on UI
        if (!mouseOverUI) {
            Vec2 mousePos = input.getMousePositionNDC();
            
            if (m_is3DMode) {
                Vec3 worldPos3D = screenToWorldPosition3D(mousePos);
                // For now, just add a 3D entity at the projected position
                // TODO: Implement proper 3D entity addition
                std::cout << "3D entity addition at: (" << worldPos3D.x << ", " << worldPos3D.y << ", " << worldPos3D.z << ")" << std::endl;
            } else {
                Vec2 worldPos = screenToWorldPosition(mousePos);
                
                if (m_addPOIMode) {
                    // Add POI at mouse position
                    addPointOfInterest(worldPos, "Custom POI");
                    m_addPOIMode = false; // Exit POI creation mode
                } else {
                    // Add entity at mouse position
                    addEntityAtPosition(worldPos);
                }
            }
        }
    }

    // Engine ButtonComponents updates disabled under ImGui UI

    // Engine text updates disabled under ImGui UI
    
    // Update offset controls
    updateOffsetControls(dt);
    
    // Shadow mapping controls
    if (input.wasKeyJustPressed(Key::F1)) {
        m_showShadowMapDebug = !m_showShadowMapDebug;
    }
    
    // Handle 3D mode toggle
    if (input.wasKeyJustPressed(Key::T)) {
        toggle3DMode();
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

void PartitionScene::updateOffsetControls(float dt) {
    auto& input = Input::getInstance();
    
    float currentSpeed = input.isKeyDown(Key::Shift) ? m_offsetSpeed * 2.0f : m_offsetSpeed;
    
    Vec2 offsetDelta(0.0f, 0.0f);
    if (input.isKeyDown(Key::I)) offsetDelta.y += currentSpeed * dt;  // Move up
    if (input.isKeyDown(Key::K)) offsetDelta.y -= currentSpeed * dt;  // Move down
    if (input.isKeyDown(Key::J)) offsetDelta.x -= currentSpeed * dt; // Move left
    if (input.isKeyDown(Key::L)) offsetDelta.x += currentSpeed * dt; // Move right
    
    if (offsetDelta.x != 0.0f || offsetDelta.y != 0.0f) {
        m_quadtreeVisualOffset.x += offsetDelta.x;
        m_quadtreeVisualOffset.y += offsetDelta.y;
        
        // Update the appropriate stored offset based on current mode
        if (m_dbscanEnabled) {
            m_quadtreeVisualOffsetDBScan = m_quadtreeVisualOffset;
        } else {
            m_quadtreeVisualOffsetOriginal = m_quadtreeVisualOffset;
        }
        
        // Update quadtree visualization with new offset
        updateQuadtreeVisualization();

        // Print current offset to console
        std::cout << "Offset: (" << m_quadtreeVisualOffset.x << ", " << m_quadtreeVisualOffset.y << ")" << std::endl;
    }
}

void PartitionScene::addEntityAtPosition(const Vec2& worldPos) {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));

    Vec2 size(20.0f, 20.0f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velDist(-80.0f, 80.0f); // faster movement
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
    // Expanded bounds for further travel
    movingEntity.bounds = Vec2(600.0f, 450.0f);
    movingEntity.qtEntity.position = worldPos;
    movingEntity.qtEntity.size = size;
    movingEntity.qtEntity.id = m_entityCounter;
    movingEntity.active = true;

    m_movingEntities.push_back(movingEntity);
    
    // Add to quadtree (supports incremental insertion)
    m_quadtree->insert(movingEntity.qtEntity);
    
    // AABBTree and KDTree need to be rebuilt as they don't support incremental insertion
    std::vector<QuadtreeEntity> allEntities;
    for (const auto& me : m_movingEntities) {
        if (me.active) allEntities.push_back(me.qtEntity);
    }
    m_aabbTree->buildFrom(allEntities);
    m_kdTree->buildFrom(allEntities);

    m_entityCounter++;
    updateQuadtreeVisualization();
}

void PartitionScene::generateConcentricCirclesDataset() {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    // Clear existing test entities
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();
    for (const auto& e : entities) {
        std::string name = e->getName();
        if (name.find("TestEntity") == 0) {
            entitiesToRemove.insert(name);
        }
    }
    for (const auto& n : entitiesToRemove) {
        m_entityManager->removeEntity(n);
    }
    m_movingEntities.clear();

    // Parameters for concentric circles
    Vec2 center(0.0f, 0.0f);
    // Target the top of the quadtree (height ~600, half-height ~300). Leave a small margin.
    float halfWidth = 400.0f;
    float halfHeight = 300.0f;
    float margin = 20.0f;
    float maxRadius = std::min(halfWidth, halfHeight) - margin; // ~280
    int numRings = 4;
    std::vector<float> radii;
    radii.reserve(numRings);
    for (int i = 1; i <= numRings; ++i) {
        radii.push_back(maxRadius * (static_cast<float>(i) / numRings));
    }
    // Scale counts so outer rings have more points
    std::vector<int> counts;
    counts.reserve(numRings);
    for (int i = 1; i <= numRings; ++i) {
        counts.push_back(120 + 80 * i); // 200, 280, 360, 440
    }
    float noiseSigma = 4.0f;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
    std::normal_distribution<float> noise(0.0f, noiseSigma);

    for (size_t r = 0; r < radii.size(); ++r) {
        float radius = radii[r];
        int num = counts[r];
        for (int i = 0; i < num; ++i) {
            float theta = angleDist(gen);
            Vec2 pos(
                center.x + (radius + noise(gen)) * std::cos(theta),
                center.y + (radius + noise(gen)) * std::sin(theta)
            );

            Vec2 size(14.0f, 14.0f);

            auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));
            auto& sprite = entity.addComponent<SpriteComponent>(
                device,
                L"DX3D/Assets/Textures/node.png",
                size.x,
                size.y
            );
            sprite.setPosition(pos.x, pos.y, 0.0f);
            sprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));

            MovingEntity movingEntity;
            movingEntity.name = "TestEntity" + std::to_string(m_entityCounter);
            movingEntity.velocity = Vec2(0.0f, 0.0f);
            movingEntity.bounds = Vec2(400.0f, 300.0f);
            movingEntity.qtEntity.position = pos;
            movingEntity.qtEntity.size = size;
            movingEntity.qtEntity.id = m_entityCounter;
            movingEntity.active = true;

            m_movingEntities.push_back(movingEntity);
            m_entityCounter++;
        }
    }

    // Rebuild all spatial partitions and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);
    
    // Collect all active entities for rebuilding
    std::vector<QuadtreeEntity> allEntities;
    for (const auto& me : m_movingEntities) {
        if (me.active) allEntities.push_back(me.qtEntity);
    }
    
    // Rebuild all spatial partitions
    for (const auto& entity : allEntities) {
        m_quadtree->insert(entity);
    }
    m_aabbTree->buildFrom(allEntities);
    m_kdTree->buildFrom(allEntities);

    // Stop movement for stable clustering reproduction
    m_entitiesMoving = false;

    // Run clustering if enabled
    if (m_kmeansEnabled) {
        performKMeansClustering();
    }
    if (m_dbscanEnabled) {
        performDBSCANClustering();
    }

    updateQuadtreeVisualization();
}

void PartitionScene::generateConcentricCirclesDatasetLight() {
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();

    // Clear existing test entities
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();
    for (const auto& e : entities) {
        std::string name = e->getName();
        if (name.find("TestEntity") == 0) {
            entitiesToRemove.insert(name);
        }
    }
    for (const auto& n : entitiesToRemove) {
        m_entityManager->removeEntity(n);
    }
    m_movingEntities.clear();

    // Parameters for concentric circles (lighter ~500 points total)
    Vec2 center(0.0f, 0.0f);
    float halfWidth = 400.0f;
    float halfHeight = 300.0f;
    float margin = 20.0f;
    float maxRadius = std::min(halfWidth, halfHeight) - margin;
    int numRings = 4;
    std::vector<float> radii;
    radii.reserve(numRings);
    for (int i = 1; i <= numRings; ++i) {
        radii.push_back(maxRadius * (static_cast<float>(i) / numRings));
    }
    // Choose counts summing roughly to 500
    std::vector<int> counts = { 70, 110, 150, 180 }; // total = 510
    float noiseSigma = 4.0f;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
    std::normal_distribution<float> noise(0.0f, noiseSigma);

    for (size_t r = 0; r < radii.size(); ++r) {
        float radius = radii[r];
        int num = counts[r];
        for (int i = 0; i < num; ++i) {
            float theta = angleDist(gen);
            Vec2 pos(
                center.x + (radius + noise(gen)) * std::cos(theta),
                center.y + (radius + noise(gen)) * std::sin(theta)
            );

            Vec2 size(14.0f, 14.0f);

            auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));
            auto& sprite = entity.addComponent<SpriteComponent>(
                device,
                L"DX3D/Assets/Textures/node.png",
                size.x,
                size.y
            );
            sprite.setPosition(pos.x, pos.y, 0.0f);
            sprite.setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));

            MovingEntity movingEntity;
            movingEntity.name = "TestEntity" + std::to_string(m_entityCounter);
            movingEntity.velocity = Vec2(0.0f, 0.0f);
            movingEntity.bounds = Vec2(400.0f, 300.0f);
            movingEntity.qtEntity.position = pos;
            movingEntity.qtEntity.size = size;
            movingEntity.qtEntity.id = m_entityCounter;
            movingEntity.active = true;

            m_movingEntities.push_back(movingEntity);
            m_entityCounter++;
        }
    }

    // Rebuild all spatial partitions and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 16, 16);
    
    // Collect all active entities for rebuilding
    std::vector<QuadtreeEntity> allEntities;
    for (const auto& me : m_movingEntities) {
        if (me.active) allEntities.push_back(me.qtEntity);
    }
    
    // Rebuild all spatial partitions
    for (const auto& entity : allEntities) {
        m_quadtree->insert(entity);
    }
    m_aabbTree->buildFrom(allEntities);
    m_kdTree->buildFrom(allEntities);

    m_entitiesMoving = false;

    if (m_kmeansEnabled) {
        performKMeansClustering();
    }
    if (m_dbscanEnabled) {
        performDBSCANClustering();
    }

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
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    if (m_is3DMode) {
        // Render shadow map first
        if (m_shadowMap) {
            renderShadowMap(engine);
        }
        
        // Begin main rendering
        // frame begin handled centrally
        
        // CRITICAL: Set render target to swap chain (like shadow map does)
        ctx.clearAndSetBackBuffer(swapChain, m_backgroundColor); // Use configurable background color
        
        // CRITICAL: Set viewport to full window size for 3D rendering
        ctx.setViewportSize(Rect{0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight)});
        
        // Render dotted background using the same shader as 2D mode
        if (m_showDottedBackground && engine.getBackgroundDotsPipeline()) {
            GraphicsEngine::renderBackgroundDots(ctx, engine.getBackgroundDotsPipeline(), 
                screenWidth, screenHeight, m_dotSpacing, m_dotRadius, m_backgroundColor);
        }
        
        // Enable depth testing for 3D rendering
        ctx.enableDepthTest();
        
        // Ensure 3D pipeline is active for meshes with shadow mapping
        ctx.setGraphicsPipelineState(engine.get3DPipeline());
        
        // Set 3D camera matrices
        ctx.setViewMatrix(m_camera3D.getViewMatrix());
        ctx.setProjectionMatrix(m_camera3D.getProjectionMatrix());
        
        // Lighting setup with two directional lights
        std::vector<Vec3> lightDirs{
            Vec3(0.0f, -1.0f, 0.0f),
            Vec3(0.6f, -0.5f, -0.3f)
        };
        std::vector<Vec3> lightColors{
            Vec3(1.0f, 1.0f, 1.0f),
            Vec3(0.9f, 0.95f, 1.0f)
        };
        std::vector<float> lightIntensities{ 1.0f, 0.6f };
        ctx.setLights(lightDirs, lightColors, lightIntensities);
        ctx.setCameraPosition(m_camera3D.getPosition());
        // Don't set global material - let individual entities handle their own materials
        
        // Set two shadow maps for lighting
        if (m_shadowSampler && m_shadowMap && m_shadowMap2) {
            ID3D11ShaderResourceView* srvs[2] = { m_shadowMap->getDepthSRV(), m_shadowMap2->getDepthSRV() };
            ctx.setShadowMaps(srvs, 2, m_shadowSampler.Get());
            std::vector<Mat4> mats{ m_lightViewProj, m_lightViewProj2 };
            ctx.setShadowMatrices(mats);
        }
        
        // Render 3D meshes
        auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
        
        for (auto* entity : mesh3DEntities) {
            if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                if (meshComp->isVisible()) {
                    // Set world matrix for proper 3D positioning
                    Mat4 worldMatrix = Mat4::identity();
                    worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                    worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                    ctx.setWorldMatrix(worldMatrix);
                    
                    // Don't set tint - let the material handle the color
                    meshComp->draw(ctx);
                }
            }
        }
    } else {
        // For 2D mode, use the normal GraphicsEngine beginFrame
        // frame begin handled centrally
        
        // Set 2D camera matrices for world space rendering
        if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
            if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
                ctx.setViewMatrix(camera->getViewMatrix());
                ctx.setProjectionMatrix(camera->getProjectionMatrix());
            }
        }
    }

    // Declare deferredWorldAnchor outside the if block
    SpriteComponent* deferredWorldAnchor = nullptr;
    
    if (!m_is3DMode) {
        // FIXED: Render quadtree lines in world space FIRST (behind sprites)
        if (m_lineRenderer && m_showQuadtree) {
            m_lineRenderer->draw(ctx);
        }

        // Render sprites in world space (defer world anchor)
        auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
        for (auto* entity : spriteEntities) {
            if (entity->getName() == "WorldOriginAnchor") {
                deferredWorldAnchor = entity->getComponent<SpriteComponent>();
                continue;
            }
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                if (sprite->isVisible() && sprite->isValid()) {
                    sprite->draw(ctx);
                }
            }
        }
    }

    // Render shadow map debug overlay if enabled
    if (m_is3DMode) {
        renderShadowMapDebug(engine);
    }

    // Switch to default pipeline for UI in screen space
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    
    // Ensure proper viewport for UI rendering
    ctx.setViewportSize(Rect{0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight)});
    ctx.setScreenSpaceMatrices(screenWidth, screenHeight);

    // Engine Text/Button rendering disabled under ImGui UI

    // Draw deferred world anchor last (after UI) - only in 2D mode
    if (!m_is3DMode && deferredWorldAnchor && deferredWorldAnchor->isVisible() && deferredWorldAnchor->isValid()) {
        if (auto* cameraEntity2 = m_entityManager->findEntity("MainCamera")) {
            if (auto* camera2 = cameraEntity2->getComponent<Camera2D>()) {
                ctx.setViewMatrix(camera2->getViewMatrix());
                ctx.setProjectionMatrix(camera2->getProjectionMatrix());
            }
        }
        deferredWorldAnchor->draw(ctx);
    }
}

void PartitionScene::fixedUpdate(float dt) {
    // Fixed update logic if needed
}

void PartitionScene::renderImGui(GraphicsEngine& engine)
{
    // Show ImGui metrics window for debugging
    //ImGui::ShowMetricsWindow();
    
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Partition Controls"))
    {
        if (!m_is3DMode) {
            ImGui::Text("2D Mode");
            ImGui::Separator();
            int speedIdx = (m_simulationSpeed == SimulationSpeed::Paused) ? 0 :
                           (m_simulationSpeed == SimulationSpeed::Normal) ? 1 :
                           (m_simulationSpeed == SimulationSpeed::Fast) ? 2 : 3;
            const char* speeds[] = { "Paused", "1x", "2x", "4x" };
            if (ImGui::Combo("Speed", &speedIdx, speeds, 4)) {
                switch (speedIdx) {
                case 0: setSimulationSpeed(SimulationSpeed::Paused); break;
                case 1: setSimulationSpeed(SimulationSpeed::Normal); break;
                case 2: setSimulationSpeed(SimulationSpeed::Fast); break;
                case 3: setSimulationSpeed(SimulationSpeed::VeryFast); break;
                }
            }
            ImGui::Checkbox("Entities Moving", &m_entitiesMoving);
                ImGui::SliderFloat("Entity Speed", &m_entitySpeedMultiplier, 0.2f, 10.0f, "%.2fx");

            ImGui::Spacing();
            ImGui::Text("Partition");
            ImGui::Separator();
            int partIdx = (m_partitionType == PartitionType::Quadtree) ? 0 : (m_partitionType == PartitionType::AABB) ? 1 : 2;
            const char* parts[] = { "Quadtree", "AABB", "KD-Tree" };
            if (ImGui::Combo("Type", &partIdx, parts, 3)) {
                m_partitionType = (partIdx == 0) ? PartitionType::Quadtree : (partIdx == 1) ? PartitionType::AABB : PartitionType::KDTree;
                updateQuadtreePartitioning();
            }
            ImGui::Checkbox("Show Quadtree Lines", &m_showQuadtree);

            ImGui::Spacing();
            ImGui::Text("Clustering");
            ImGui::Separator();
            if (ImGui::Checkbox("K-Means", &m_kmeansEnabled)) {
                if (m_kmeansEnabled) performKMeansClustering();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Fast Mode", &m_fastMode);
            ImGui::SliderInt("K (clusters)", &m_kmeansK, 1, 10);
            if (ImGui::Button("Run K-Means", ImVec2(-FLT_MIN, 0))) {
                performKMeansClustering();
            }
            if (ImGui::Checkbox("DBSCAN", &m_dbscanEnabled)) {
                if (m_dbscanEnabled) performDBSCANClustering();
            }
            ImGui::SliderFloat("DBSCAN Eps", &m_dbscanEps, 5.0f, 150.0f, "%.1f");
            ImGui::SliderInt("DBSCAN MinPts", &m_dbscanMinPts, 2, 10);
            if (ImGui::Button("Run DBSCAN", ImVec2(-FLT_MIN, 0))) {
                performDBSCANClustering();
            }

            ImGui::Spacing();
            ImGui::Text("Entities / Datasets");
            ImGui::Separator();
            if (ImGui::Button("Add 10 Random", ImVec2(-FLT_MIN, 0))) addRandomEntities();
            if (ImGui::Button("Clear Entities", ImVec2(-FLT_MIN, 0))) clearAllEntities();
            if (ImGui::Button("Generate Concentric (Heavy)", ImVec2(-FLT_MIN, 0))) generateConcentricCirclesDataset();
            if (ImGui::Button("Generate Concentric (Light)", ImVec2(-FLT_MIN, 0))) generateConcentricCirclesDatasetLight();

            ImGui::Spacing();
            ImGui::Text("POIs");
            ImGui::Separator();
            ImGui::Checkbox("Enable POI System", &m_poiSystemEnabled);
            if (ImGui::Button("Add POI Mode", ImVec2(-FLT_MIN, 0))) m_addPOIMode = true;
            if (ImGui::Button("Clear POIs", ImVec2(-FLT_MIN, 0))) clearAllPOIs();
        } else {
            ImGui::Text("3D Mode");
            //ImGui::Text("0123456789qwertyiuopasdfghjkl;zxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM");
            ImGui::Separator();
            
        
            
           
            if (ImGui::Button("Add 3D Entities", ImVec2(150, 0))) {
                auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();
                addRandom3DEntities(device, 5);
            }
            if (ImGui::Button("Clear 3D Entities", ImVec2(150, 0))) {
                clearAllEntities3D();
            }
            
            ImGui::Spacing();
            ImGui::Text("FPS Camera Controls");
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::BulletText("Right Mouse + Drag: Look around");
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Space/C: Up/Down");
            ImGui::BulletText("Shift: Run");
            ImGui::Spacing();
            ImGui::SliderFloat("Move Speed", &m_cameraMoveSpeed, 5.0f, 50.0f, "%.1f");
            ImGui::SliderFloat("Mouse Sensitivity", &m_cameraMouseSensitivity, 0.5f, 5.0f, "%.1f");
            ImGui::SliderFloat("Run Multiplier", &m_cameraRunMultiplier, 1.5f, 4.0f, "%.1f");
            
            ImGui::Spacing();
            ImGui::Text("Scene Simulation");
            ImGui::Separator();
            int speedIdx = (m_simulationSpeed == SimulationSpeed::Paused) ? 0 :
                (m_simulationSpeed == SimulationSpeed::Normal) ? 1 :
                (m_simulationSpeed == SimulationSpeed::Fast) ? 2 : 3;
            const char* speeds[] = { "Paused", "1x", "2x", "4x" };
            if (ImGui::Combo("Speed", &speedIdx, speeds, 4)) {
                switch (speedIdx) {
                case 0: setSimulationSpeed(SimulationSpeed::Paused); break;
                case 1: setSimulationSpeed(SimulationSpeed::Normal); break;
                case 2: setSimulationSpeed(SimulationSpeed::Fast); break;
                case 3: setSimulationSpeed(SimulationSpeed::VeryFast); break;
                }
            }

            ImGui::Spacing();
            ImGui::Text("Depth test");
            ImGui::Separator();
            ImGui::Checkbox("Show Depth Preview", &m_showShadowPreview);
            const char* lightItems[] = { "light 1", "light 2" };
            ImGui::Combo("Light##DepthPreview", &m_selectedShadowMap, lightItems, 2);
            ImGui::SliderFloat("Preview Size", &m_shadowPreviewSize, 100.0f, 400.0f, "%.0f");
            if (m_showShadowPreview) {
                ID3D11ShaderResourceView* srv = nullptr;
                if (m_selectedShadowMap == 0) {
                    if (m_shadowMap) srv = m_shadowMap->getDepthSRV();
                } else {
                    if (m_shadowMap2) srv = m_shadowMap2->getDepthSRV();
                }
                if (srv) {
                    // ImGui for DX11 expects ImTextureID to be ID3D11ShaderResourceView*
                    ImTextureID texId = (ImTextureID)srv;
                    // Flip Y so it matches on-screen orientation
                    ImVec2 size(m_shadowPreviewSize, m_shadowPreviewSize);
                    ImVec2 uv0(0.0f, 0.0f);
                    ImVec2 uv1(1.0f, 1.0f);
                    ImGui::Image(texId, size, uv0, uv1);
                } else {
                    ImGui::TextUnformatted("(no depth available)");
                }
            }

            ImGui::Spacing();
            ImGui::Text("Camera Presets");
            ImGui::Separator();
            int presetIdx = (m_cameraPreset == CameraPreset::FirstPerson) ? 0 : 
                           (m_cameraPreset == CameraPreset::TopDown) ? 1 : 2;
            const char* presets[] = { "First Person", "Top Down", "Isometric" };
            if (ImGui::Combo("Camera Mode", &presetIdx, presets, 3)) {
                m_cameraPreset = (presetIdx == 0) ? CameraPreset::FirstPerson : 
                                (presetIdx == 1) ? CameraPreset::TopDown : CameraPreset::Isometric;
                setCameraPreset(m_cameraPreset);
            }
            
            ImGui::Spacing();
            ImGui::Text("Background");
            ImGui::Separator();
            ImGui::Checkbox("Show Dotted Background", &m_showDottedBackground);
            if (m_showDottedBackground) {
                float bgColor[4] = { m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, m_backgroundColor.w };
                if (ImGui::ColorEdit4("Background Color", bgColor)) {
                    m_backgroundColor = Vec4(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
                }
                ImGui::SliderFloat("Dot Spacing", &m_dotSpacing, 20.0f, 80.0f, "%.1f");
                ImGui::SliderFloat("Dot Radius", &m_dotRadius, 0.5f, 3.0f, "%.1f");
                if (ImGui::Button("Reset to Default", ImVec2(150, 0))) {
                    m_backgroundColor = Vec4(0.27f, 0.39f, 0.55f, 1.0f); // Default dotted blue
                    m_dotSpacing = 40.0f;
                    m_dotRadius = 1.2f;
                }
            } else {
                float bgColor[4] = { m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, m_backgroundColor.w };
                if (ImGui::ColorEdit4("Solid Background Color", bgColor)) {
                    m_backgroundColor = Vec4(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
                }
                if (ImGui::Button("Black Background", ImVec2(150, 0))) {
                    m_backgroundColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Text("Scene");
        ImGui::Separator();
        if (ImGui::Button(m_is3DMode ? "Switch to 2D" : "Switch to 3D", ImVec2(150, 0))) {
            toggle3DMode();
        }
        if (ImGui::Button("Reset Camera", ImVec2(150, 0))) {
            if (!m_is3DMode) {
                if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
                    if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
                        camera->setPosition(0.0f, 0.0f);
                        camera->setZoom(1.0f);
                    }
                }
            } else {
                setCameraPreset(CameraPreset::TopDown);
            }
        }
    }
    ImGui::End();
}
