#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Quadtree.h>
#include <DX3D/Components/AABBTree.h>
#include <DX3D/Components/KDTree.h>
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
    camera.setZoom(1.0f); // Set zoom to 1.0 to see full screen bounds

    // Create line renderer for quadtree visualization
    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false); // Ensure world space rendering
    
    // Set the camera for the LineRenderer so it can use proper view/projection matrices
    m_lineRenderer->setCamera(&camera);
    
    // Set dedicated line pipeline for optimal performance
    if (auto* linePipeline = engine.getLinePipeline()) {
        m_lineRenderer->setLinePipeline(linePipeline);
    } else {
    }

    // Set quadtree bounds to screen size
    m_quadtreeSize = Vec2(screenWidth, screenHeight);
    
    // Set entity bounds smaller than quadtree bounds to keep entities well within quadtree
    float entityBoundsMultiplier = 0.5f; // Entities can only use 50% of the quadtree area
    m_entityBounds = Vec2(screenWidth * entityBoundsMultiplier, screenHeight * entityBoundsMultiplier);
    m_entitySpawnRange = Vec2(-m_entityBounds.x/2.0f, m_entityBounds.x/2.0f);
    
    // Initialize spatial partitions with aspect-ratio-matched bounds
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    // Initialize octree with 3D bounds that match the 3D scene
    // Use smaller maxEntities and higher maxDepth for more sensitive subdivision
    // Use much larger bounds to accommodate entity movement
    m_octree = std::make_unique<Octree>(Vec3(0.0f, 0.0f, 0.0f), Vec3(1000.0f, 1000.0f, 1000.0f), 2, 8);

    // Initialize light settings
    m_light1.enabled = true;
    m_light1.direction = Vec3(0.0f, -1.0f, 0.0f);
    m_light1.color = Vec3(1.0f, 1.0f, 1.0f);
    m_light1.intensity = 1.0f;
    m_light1.position = Vec3(0.0f, 50.0f, 0.0f);
    m_light1.target = Vec3(0.0f, 0.0f, 0.0f);
    m_light1.orthoSize = 100.0f;
    m_light1.nearPlane = 0.1f;
    m_light1.farPlane = 200.0f;
    
    m_light2.enabled = true;
    m_light2.direction = Vec3(0.6f, -0.5f, -0.3f);
    m_light2.color = Vec3(0.9f, 0.95f, 1.0f);
    m_light2.intensity = 0.6f;
    m_light2.position = Vec3(60.0f, 60.0f, 60.0f);
    m_light2.target = Vec3(0.0f, 0.0f, 0.0f);
    m_light2.orthoSize = 120.0f;
    m_light2.nearPlane = 0.1f;
    m_light2.farPlane = 250.0f;

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
    std::uniform_real_distribution<float> posDist(m_entitySpawnRange.x, m_entitySpawnRange.y); // Use dynamic spawn range
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
        movingEntity.bounds = m_entityBounds; // Use dynamic entity bounds
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
    std::uniform_real_distribution<float> posDistX(-m_entityBounds.x/2.0f, m_entityBounds.x/2.0f);
    std::uniform_real_distribution<float> posDistY(-m_entityBounds.y/2.0f, m_entityBounds.y/2.0f);
    std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
    std::uniform_real_distribution<float> velDist(-120.0f, 120.0f); // faster movement

    for (int i = 0; i < 10; i++) {
        auto& entity = m_entityManager->createEntity("TestEntity" + std::to_string(m_entityCounter));

        Vec2 position(posDistX(gen), posDistY(gen));
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
        // Use dynamic entity bounds to match quadtree
        movingEntity.bounds = m_entityBounds;
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
    if (true) {
        updateQuadtreePartitioning();
        
        // Update octree for moving entities (only in 3D mode)
        if (m_showOctree && m_is3DMode) {
            // Rebuild octree with current 3D entity positions
            m_octree->clear();
            int entitiesInserted = 0;
            
            // Get all 3D mesh entities directly from the entity manager
            auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
            for (auto* entity : mesh3DEntities) {
                // Exclude non-partition visualization/anchor objects
                const std::string& nm = entity->getName();
                if (nm == "GroundPlane" || nm == "UnitSquare" || nm == "WorldOriginAnchor" || nm.rfind("TransparentUnit3D", 0) == 0) {
                    continue;
                }
                if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                    if (meshComp->isVisible()) {
                        Vec3 actualPosition = meshComp->getPosition();
                        Vec3 actualScale = meshComp->getScale();
                        
                        // Use a reasonable entity size for octree (not the mesh scale which might be huge)
                        Vec3 entitySize = Vec3(2.0f, 2.0f, 2.0f); // Standard entity size
                        
                        // Only filter out entities that are extremely far out (like background planes)
                        if (actualPosition.x >= -1000.0f && actualPosition.x <= 1000.0f &&
                            actualPosition.y >= -1000.0f && actualPosition.y <= 1000.0f &&
                            actualPosition.z >= -1000.0f && actualPosition.z <= 1000.0f) {
                            
                            OctreeEntity octreeEntity;
                            octreeEntity.position = actualPosition;
                            octreeEntity.size = entitySize; // Use consistent entity size
                            octreeEntity.id = entitiesInserted; // Use insertion order as ID
                            m_octree->insert(octreeEntity);
                            entitiesInserted++;
                        }
                    }
                }
            }
            updateOctreeVisualization();
        }
        
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
        m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 4, 5);
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
        m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 4, 5);
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
        
        // Update octree visualization if enabled (only in 3D mode)
        if (m_showOctree && m_is3DMode) {
            updateOctreeVisualization();
        }

        // Print current offset to console
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
    // Use dynamic entity bounds to match quadtree
    movingEntity.bounds = m_entityBounds;
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
            movingEntity.bounds = m_entityBounds; // Use dynamic entity bounds to match quadtree
            movingEntity.qtEntity.position = pos;
            movingEntity.qtEntity.size = size;
            movingEntity.qtEntity.id = m_entityCounter;
            movingEntity.active = true;

            m_movingEntities.push_back(movingEntity);
            m_entityCounter++;
        }
    }

    // Rebuild all spatial partitions and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    
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

    // Always keep entities moving (UI toggle removed)
    m_entitiesMoving = true;

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
            movingEntity.bounds = m_entityBounds; // Use dynamic entity bounds to match quadtree
            movingEntity.qtEntity.position = pos;
            movingEntity.qtEntity.size = size;
            movingEntity.qtEntity.id = m_entityCounter;
            movingEntity.active = true;

            m_movingEntities.push_back(movingEntity);
            m_entityCounter++;
        }
    }

    // Rebuild all spatial partitions and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 4, 5);
    m_aabbTree = std::make_unique<AABBTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    m_kdTree = std::make_unique<KDTree>(Vec2(0.0f, 0.0f), m_quadtreeSize, 16, 16);
    
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

    m_entitiesMoving = true;

    if (m_kmeansEnabled) {
        performKMeansClustering();
    }
    if (m_dbscanEnabled) {
        performDBSCANClustering();
    }

    updateQuadtreeVisualization();
}

void PartitionScene::resetEntityColorsToDefault() {
    // Reset tint on all 2D moving entities back to the original green
    for (const auto& me : m_movingEntities) {
        if (auto* entity = m_entityManager->findEntity(me.name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
            }
        }
    }
}

void PartitionScene::drawClusterCenterLines() {
	if (!m_lineRenderer || m_is3DMode) return;
	// Only draw when a clustering mode is active and visualization is None
	if (m_clusteringMode == ClusteringMode::None) return;
	if (m_clusterVizMode != ClusterVizMode::None) return;

	// Draw from each entity to its cluster center.
	if (m_clusteringMode == ClusteringMode::KMeans && !m_clusters.empty()) {
		for (const auto& cluster : m_clusters) {
			const Vec4 color = cluster.color;
			for (int idx : cluster.entityIndices) {
				if (idx >= 0 && idx < (int)m_movingEntities.size()) {
					const Vec2 p = m_movingEntities[idx].qtEntity.position;
					m_lineRenderer->addLine(Vec2(cluster.centroid.x, cluster.centroid.y), p, color, 1.0f);
				}
			}
		}
	}

	if (m_clusteringMode == ClusteringMode::DBSCAN && !m_dbscanClusters.empty()) {
		for (const auto& c : m_dbscanClusters) {
			if (c.entityIndices.empty()) continue;
			Vec2 sum(0.0f, 0.0f);
			int count = 0;
			for (int idx : c.entityIndices) {
				if (idx >= 0 && idx < (int)m_movingEntities.size()) {
					sum.x += m_movingEntities[idx].qtEntity.position.x;
					sum.y += m_movingEntities[idx].qtEntity.position.y;
					count++;
				}
			}
			if (count == 0) continue;
			Vec2 center(sum.x / count, sum.y / count);
			for (int idx : c.entityIndices) {
				if (idx >= 0 && idx < (int)m_movingEntities.size()) {
					const Vec2 p = m_movingEntities[idx].qtEntity.position;
					m_lineRenderer->addLine(center, p, c.color, 1.0f);
				}
			}
		}
	}
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
        
        // Lighting setup with configurable lights
        std::vector<Vec3> lightDirs;
        std::vector<Vec3> lightColors;
        std::vector<float> lightIntensities;
        
        if (m_light1.enabled) {
            lightDirs.push_back(m_light1.direction);
            lightColors.push_back(m_light1.color);
            lightIntensities.push_back(m_light1.intensity);
        }
        
        if (m_light2.enabled) {
            lightDirs.push_back(m_light2.direction);
            lightColors.push_back(m_light2.color);
            lightIntensities.push_back(m_light2.intensity);
        }
        
        // If no lights are enabled, add a minimal ambient light
        if (lightDirs.empty()) {
            lightDirs.push_back(Vec3(0.0f, -1.0f, 0.0f));
            lightColors.push_back(Vec3(0.1f, 0.1f, 0.1f));
            lightIntensities.push_back(0.1f);
        }
        
        ctx.setLights(lightDirs, lightColors, lightIntensities);
        ctx.setCameraPosition(m_camera3D.getPosition());
        // Don't set global material - let individual entities handle their own materials
        
        // Set shadow maps for enabled lights only
        if (m_shadowSampler && m_shadowMap && m_shadowMap2 && m_enableShadowMapping) {
            std::vector<ID3D11ShaderResourceView*> shadowSRVs;
            std::vector<Mat4> shadowMatrices;
            
            if (m_light1.enabled && m_light1Shadows) {
                shadowSRVs.push_back(m_shadowMap->getDepthSRV());
                shadowMatrices.push_back(m_lightViewProj);
            }
            
            if (m_light2.enabled && m_light2Shadows) {
                shadowSRVs.push_back(m_shadowMap2->getDepthSRV());
                shadowMatrices.push_back(m_lightViewProj2);
            }
            
            if (!shadowSRVs.empty()) {
                ctx.setShadowMaps(shadowSRVs.data(), static_cast<int>(shadowSRVs.size()), m_shadowSampler.Get());
                ctx.setShadowMatrices(shadowMatrices);
            }
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
        
        // Render octree visualization lines in 3D mode
        if (m_showOctree && m_lineRenderer) {
            m_lineRenderer->draw(ctx);
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
            // If clustering is active with visualization None, append center lines before drawing
            bool appendedCenterLines = false;
            if (m_clusteringMode != ClusteringMode::None && m_clusterVizMode == ClusterVizMode::None) {
                drawClusterCenterLines();
                appendedCenterLines = true;
            }

            m_lineRenderer->draw(ctx);

            // If we appended center lines, immediately rebuild quadtree visualization to remove them
            if (appendedCenterLines) {
                updateQuadtreeVisualization();
            }

            // CRITICAL: Restore default pipeline for sprite rendering
            ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
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

        // Center-of-mass lines are handled in the single draw above when applicable
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
            // Entities always move; keep only speed slider
            m_entitiesMoving = true;
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
            
            // Quadtree controls removed for a cleaner 2D UI

            ImGui::Spacing();
            ImGui::Text("Clustering");
            ImGui::Separator();
            int clusterModeIdx = (m_clusteringMode == ClusteringMode::None) ? 0 : (m_clusteringMode == ClusteringMode::KMeans) ? 1 : 2;
            const char* clusterModes[] = { "None", "K-Means", "DBSCAN" };
            if (ImGui::Combo("Clustering Mode", &clusterModeIdx, clusterModes, 3)) {
                ClusteringMode newMode = (clusterModeIdx == 0) ? ClusteringMode::None : (clusterModeIdx == 1) ? ClusteringMode::KMeans : ClusteringMode::DBSCAN;
                if (newMode != m_clusteringMode) {
                    m_clusteringMode = newMode;
                    // Toggle systems and apply initial actions
                    if (m_clusteringMode == ClusteringMode::None) {
                        m_kmeansEnabled = false;
                        m_dbscanEnabled = false;
                        // Ensure no cluster visualization is rendered
                        m_showClusterVisualization = false;
                        m_showDBSCANVisualization = false;
                        m_useVoronoi = false;
                        m_dbscanUseVoronoi = false;
                        m_clusterVizMode = ClusterVizMode::None;
                        resetDBSCANLabels();
                        m_clusters.clear();
                        m_dbscanClusters.clear();
                        resetEntityColorsToDefault();
                        // Clear any previously drawn cluster visualization lines and restore quadtree lines
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    } else if (m_clusteringMode == ClusteringMode::KMeans) {
                        m_kmeansEnabled = true;
                        m_dbscanEnabled = false;
                        // Sync visualization flags to current viz mode
                        m_showDBSCANVisualization = false;
                        m_showClusterVisualization = (m_clusterVizMode != ClusterVizMode::None);
                        m_useVoronoi = (m_clusterVizMode == ClusterVizMode::Voronoi);
                        if (m_clusterVizMode == ClusterVizMode::None) {
                            if (m_lineRenderer && !m_is3DMode) {
                                m_lineRenderer->clear();
                                updateQuadtreeVisualization();
                            }
                        }
                        performKMeansClustering();
                        if (m_clusterVizMode == ClusterVizMode::None) {
                            if (m_lineRenderer && !m_is3DMode) {
                                m_lineRenderer->clear();
                                updateQuadtreeVisualization();
                            }
                        }
                    } else { // DBSCAN
                        m_kmeansEnabled = false;
                        m_dbscanEnabled = true;
                        // Sync visualization flags to current viz mode
                        m_showDBSCANVisualization = (m_clusterVizMode != ClusterVizMode::None);
                        m_dbscanUseVoronoi = (m_clusterVizMode == ClusterVizMode::Voronoi);
                        if (m_clusterVizMode == ClusterVizMode::None) {
                            if (m_lineRenderer && !m_is3DMode) {
                                m_lineRenderer->clear();
                                updateQuadtreeVisualization();
                            }
                        }
                        performDBSCANClustering();
                    }
                }
            }
            
            // Conditionally show controls for the selected clustering mode
            if (m_clusteringMode == ClusteringMode::KMeans) {
                ImGui::Checkbox("Fast Mode", &m_fastMode);
                ImGui::SliderInt("K (clusters)", &m_kmeansK, 1, 10);
                if (ImGui::Button("Run K-Means", ImVec2(-FLT_MIN, 0))) {
                    if (m_clusterVizMode == ClusterVizMode::None) {
                        m_showClusterVisualization = false;
                        m_useVoronoi = false;
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    }
                    performKMeansClustering();
                    if (m_clusterVizMode == ClusterVizMode::None) {
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    }
                }
            } else if (m_clusteringMode == ClusteringMode::DBSCAN) {
                ImGui::SliderFloat("DBSCAN Eps", &m_dbscanEps, 5.0f, 150.0f, "%.1f");
                ImGui::SliderInt("DBSCAN MinPts", &m_dbscanMinPts, 2, 10);
                if (ImGui::Button("Run DBSCAN", ImVec2(-FLT_MIN, 0))) {
                    if (m_clusterVizMode == ClusterVizMode::None) {
                        m_showClusterVisualization = false;
                        m_dbscanUseVoronoi = false;
                        m_showDBSCANVisualization = false;
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    }
                    performDBSCANClustering();
                    if (m_clusterVizMode == ClusterVizMode::None) {
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    }
                }
            }

            // Visualization mode dropdown (only when a clustering mode is active)
            if (m_clusteringMode != ClusteringMode::None) {
                int vizIdx = (m_clusterVizMode == ClusterVizMode::None) ? 0 : (m_clusterVizMode == ClusterVizMode::ConvexHull) ? 1 : 2;
                const char* vizModes[] = { "None", "Convex Hull", "Voronoi" };
                if (ImGui::Combo("Cluster Visualization", &vizIdx, vizModes, 3)) {
                    m_clusterVizMode = (vizIdx == 0) ? ClusterVizMode::None : (vizIdx == 1) ? ClusterVizMode::ConvexHull : ClusterVizMode::Voronoi;
                    // Map to existing flags used by rendering code
                    m_showClusterVisualization = (m_clusterVizMode != ClusterVizMode::None);
                    m_useVoronoi = (m_clusterVizMode == ClusterVizMode::Voronoi);
                    m_dbscanUseVoronoi = (m_clusterVizMode == ClusterVizMode::Voronoi);
                    // Also update DBSCAN-specific visibility switch
                    m_showDBSCANVisualization = (m_clusterVizMode != ClusterVizMode::None) && (m_clusteringMode == ClusteringMode::DBSCAN);
                    // If visualization is disabled, immediately clear any drawn viz and restore quadtree lines
                    if (m_clusterVizMode == ClusterVizMode::None) {
                        if (m_lineRenderer && !m_is3DMode) {
                            m_lineRenderer->clear();
                            updateQuadtreeVisualization();
                        }
                    }
                }
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
            if (ImGui::Button("Add Unit 3D Entity", ImVec2(150, 0))) {
                auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();
                addUnit3DEntity(device);
            }
            if (ImGui::Button("Clear 3D Entities", ImVec2(150, 0))) {
                clearAllEntities3D();
            }
            
            // 3D Spawn Bounds UI removed (always use current configured values)

            // Move Scene Simulation before Spatial Visualization
            ImGui::Spacing();
            ImGui::Text("Scene Simulation");
            ImGui::Separator();
            int speedIdxEarly = (m_simulationSpeed == SimulationSpeed::Paused) ? 0 :
                (m_simulationSpeed == SimulationSpeed::Normal) ? 1 :
                (m_simulationSpeed == SimulationSpeed::Fast) ? 2 : 3;
            const char* speedsEarly[] = { "Paused", "1x", "2x", "4x" };
            if (ImGui::Combo("Speed", &speedIdxEarly, speedsEarly, 4)) {
                switch (speedIdxEarly) {
                case 0: setSimulationSpeed(SimulationSpeed::Paused); break;
                case 1: setSimulationSpeed(SimulationSpeed::Normal); break;
                case 2: setSimulationSpeed(SimulationSpeed::Fast); break;
                case 3: setSimulationSpeed(SimulationSpeed::VeryFast); break;
                }
            }
            
            ImGui::Spacing();
            ImGui::Text("3D Spatial Visualization");
            ImGui::Separator();
            {
                int spatialIdx = (m_spatial3DType == Spatial3DType::Octree) ? 0 : 1;
                const char* spatialItems[] = { "Octree", "KD-Tree" };
                if (ImGui::Combo("Type##3DSpatial", &spatialIdx, spatialItems, 2)) {
                    m_spatial3DType = (spatialIdx == 0) ? Spatial3DType::Octree : Spatial3DType::KDTree;
                    // Set default visualization scales per selection
                    if (m_spatial3DType == Spatial3DType::KDTree) {
                        m_octreeVisualizationScale = 0.1f;
                    } else {
                        m_octreeVisualizationScale = 0.1f;
                    }
                    updateOctreeVisualization();
                }
            }
            if (m_spatial3DType == Spatial3DType::Octree) {
                if (ImGui::Checkbox("Show Octree", &m_showOctree)) {
                    if (m_showOctree) {
                        // Spawn an invisible transparent unit entity and unit square like random 3D spawns do
                        if (m_entityManager && !m_entityManager->findEntity("TransparentUnit3D_SpatialViz")) {
                            auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();
                            auto& transparentEntity = m_entityManager->createEntity("TransparentUnit3D_SpatialViz");
                            auto transparentMesh = Mesh::CreateCube(device, 1.0f);
                            if (transparentMesh) {
                                auto& transparentMeshComp = transparentEntity.addComponent<Mesh3DComponent>(transparentMesh);
                                transparentMeshComp.setPosition(Vec3(0.0f, 0.0f, 0.0f));
                                transparentMeshComp.setScale(Vec3(1.0f, 1.0f, 1.0f));
                                transparentMeshComp.setVisible(false);
                                transparentMeshComp.setMaterial(Vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f);
                            }
                            // Ensure unit square exists/updated
                            createOrUpdateUnitSquare();
                        }
                        updateOctreeVisualization();
                    }
                }
            } else {
                // For KD-Tree we reuse m_showOctree flag to toggle rendering on/off
                if (ImGui::Checkbox("Show KD-Tree", &m_showOctree)) {
                    if (m_showOctree) {
                        // Spawn an invisible transparent unit entity and unit square like random 3D spawns do
                        if (m_entityManager && !m_entityManager->findEntity("TransparentUnit3D_SpatialViz")) {
                            auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();
                            auto& transparentEntity = m_entityManager->createEntity("TransparentUnit3D_SpatialViz");
                            auto transparentMesh = Mesh::CreateCube(device, 1.0f);
                            if (transparentMesh) {
                                auto& transparentMeshComp = transparentEntity.addComponent<Mesh3DComponent>(transparentMesh);
                                transparentMeshComp.setPosition(Vec3(0.0f, 0.0f, 0.0f));
                                transparentMeshComp.setScale(Vec3(1.0f, 1.0f, 1.0f));
                                transparentMeshComp.setVisible(false);
                                transparentMeshComp.setMaterial(Vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f);
                            }
                            // Ensure unit square exists/updated
                            createOrUpdateUnitSquare();
                        }
                        updateOctreeVisualization();
                    }
                }
            }
            if (m_showOctree) {
                // Enforce fixed visual style
                m_showOctreeDepthColors = false;
                m_octreeLineThickness = 0.1f;
                
                ImGui::SliderInt("Max Depth", &m_octreeMaxDepth, 1, 8);
                
                // Octree/KD parameters for real-time adjustment
                static int octreeMaxEntities = 2;
                static int octreeMaxDepthDyn = 8;
                if (m_spatial3DType == Spatial3DType::Octree) {
                    if (ImGui::SliderInt("Max Entities per Node", &octreeMaxEntities, 1, 10)) {
                        m_octree = std::make_unique<Octree>(Vec3(0.0f, 0.0f, 0.0f), Vec3(1000.0f, 1000.0f, 1000.0f), octreeMaxEntities, octreeMaxDepthDyn);
                        for (const auto& entity3D : m_movingEntities3D) {
                            if (entity3D.active) {
                                OctreeEntity octreeEntity;
                                octreeEntity.position = entity3D.position;
                                octreeEntity.size = entity3D.size;
                                octreeEntity.id = entity3D.id;
                                m_octree->insert(octreeEntity);
                            }
                        }
                        updateOctreeVisualization();
                    }
                    if (ImGui::SliderInt("Octree Max Depth", &octreeMaxDepthDyn, 1, 10)) {
                        m_octree = std::make_unique<Octree>(Vec3(0.0f, 0.0f, 0.0f), Vec3(1000.0f, 1000.0f, 1000.0f), octreeMaxEntities, octreeMaxDepthDyn);
                        for (const auto& entity3D : m_movingEntities3D) {
                            if (entity3D.active) {
                                OctreeEntity octreeEntity;
                                octreeEntity.position = entity3D.position;
                                octreeEntity.size = entity3D.size;
                                octreeEntity.id = entity3D.id;
                                m_octree->insert(octreeEntity);
                            }
                        }
                        updateOctreeVisualization();
                    }
                } else {
                    if (ImGui::SliderInt("KD Max Depth", &m_kdMaxDepth, 1, 10)) {
                        updateOctreeVisualization();
                    }
                    if (ImGui::SliderInt("KD Leaf Capacity", &m_kdLeafCapacity, 1, 16)) {
                        updateOctreeVisualization();
                    }
                }
                if (ImGui::Button(m_spatial3DType == Spatial3DType::Octree ? "Update Octree Visualization" : "Update KD-Tree Visualization", ImVec2(-FLT_MIN, 0))) {
                    updateOctreeVisualization();
                }
                
                // Show octree statistics
                if (m_octree) {
                    std::vector<Octree*> allNodes;
                    m_octree->getAllNodes(allNodes);
                    int leafNodes = 0;
                    int totalEntities = 0;
                    int maxDepth = 0;
                    int nodesAtDepth[10] = {0}; // Track nodes at each depth
                    
                    for (auto* node : allNodes) {
                        if (node->isLeaf()) {
                            leafNodes++;
                            totalEntities += static_cast<int>(node->getEntities().size());
                        }
                        int depth = node->getDepth();
                        maxDepth = std::max(maxDepth, depth);
                        if (depth < 10) {
                            nodesAtDepth[depth]++;
                        }
                    }
                    
                    // Count actual mesh entities for comparison
                    int actualMeshCount = 0;
                    Vec3 minPos(999999.0f, 999999.0f, 999999.0f);
                    Vec3 maxPos(-999999.0f, -999999.0f, -999999.0f);
                    auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
                    for (auto* entity : mesh3DEntities) {
                        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                            if (meshComp->isVisible()) {
                                actualMeshCount++;
                                Vec3 pos = meshComp->getPosition();
                                minPos.x = std::min(minPos.x, pos.x);
                                minPos.y = std::min(minPos.y, pos.y);
                                minPos.z = std::min(minPos.z, pos.z);
                                maxPos.x = std::max(maxPos.x, pos.x);
                                maxPos.y = std::max(maxPos.y, pos.y);
                                maxPos.z = std::max(maxPos.z, pos.z);
                            }
                        }
                    }
                    
                    ImGui::Text(m_spatial3DType == Spatial3DType::Octree ? "Octree Stats:" : "KD-Tree Stats:");
                    ImGui::Text("Total Nodes: %d", static_cast<int>(allNodes.size()));
                    ImGui::Text("Leaf Nodes: %d", leafNodes);
                    ImGui::Text("Total Entities: %d", totalEntities);
                    ImGui::Text("Actual Mesh Count: %d", actualMeshCount);
                    ImGui::Text("Max Depth: %d", maxDepth);
                    
                    if (actualMeshCount > 0) {
                        ImGui::Text("Entity Bounds:");
                        ImGui::Text("Min: (%.1f, %.1f, %.1f)", minPos.x, minPos.y, minPos.z);
                        ImGui::Text("Max: (%.1f, %.1f, %.1f)", maxPos.x, maxPos.y, maxPos.z);
                        ImGui::Text("Size: (%.1f, %.1f, %.1f)", maxPos.x - minPos.x, maxPos.y - minPos.y, maxPos.z - minPos.z);
                    }
                    
                    ImGui::Text("Nodes by Depth:");
                    for (int i = 0; i <= maxDepth && i < 10; i++) {
                        if (nodesAtDepth[i] > 0) {
                            ImGui::Text("  Depth %d: %d nodes", i, nodesAtDepth[i]);
                        }
                    }
                }
            }
            // FPS Camera Controls UI removed

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
            ImGui::Text("Lighting");
            ImGui::Separator();
            
            // Light 1 Controls
            ImGui::Text("Light 1 (Top-down)");
            ImGui::Checkbox("Enable Light 1", &m_light1.enabled);
            if (m_light1.enabled) {
                float light1Color[3] = { m_light1.color.x, m_light1.color.y, m_light1.color.z };
                if (ImGui::ColorEdit3("Light 1 Color", light1Color)) {
                    m_light1.color = Vec3(light1Color[0], light1Color[1], light1Color[2]);
                }
                ImGui::SliderFloat("Light 1 Intensity", &m_light1.intensity, 0.0f, 3.0f, "%.2f");
                
                ImGui::Text("Position:");
                float light1Pos[3] = { m_light1.position.x, m_light1.position.y, m_light1.position.z };
                if (ImGui::DragFloat3("Light 1 Pos", light1Pos, 1.0f)) {
                    m_light1.position = Vec3(light1Pos[0], light1Pos[1], light1Pos[2]);
                }
                
                ImGui::Text("Target:");
                float light1Target[3] = { m_light1.target.x, m_light1.target.y, m_light1.target.z };
                if (ImGui::DragFloat3("Light 1 Target", light1Target, 1.0f)) {
                    m_light1.target = Vec3(light1Target[0], light1Target[1], light1Target[2]);
                }
                
                ImGui::SliderFloat("Light 1 Ortho Size", &m_light1.orthoSize, 50.0f, 200.0f, "%.1f");
                ImGui::SliderFloat("Light 1 Near", &m_light1.nearPlane, 0.01f, 10.0f, "%.2f");
                ImGui::SliderFloat("Light 1 Far", &m_light1.farPlane, 50.0f, 500.0f, "%.1f");
            }
            
            ImGui::Spacing();
            
            // Light 2 Controls
            ImGui::Text("Light 2 (Angled)");
            ImGui::Checkbox("Enable Light 2", &m_light2.enabled);
            if (m_light2.enabled) {
                float light2Color[3] = { m_light2.color.x, m_light2.color.y, m_light2.color.z };
                if (ImGui::ColorEdit3("Light 2 Color", light2Color)) {
                    m_light2.color = Vec3(light2Color[0], light2Color[1], light2Color[2]);
                }
                ImGui::SliderFloat("Light 2 Intensity", &m_light2.intensity, 0.0f, 3.0f, "%.2f");
                
                ImGui::Text("Position:");
                float light2Pos[3] = { m_light2.position.x, m_light2.position.y, m_light2.position.z };
                if (ImGui::DragFloat3("Light 2 Pos", light2Pos, 1.0f)) {
                    m_light2.position = Vec3(light2Pos[0], light2Pos[1], light2Pos[2]);
                }
                
                ImGui::Text("Target:");
                float light2Target[3] = { m_light2.target.x, m_light2.target.y, m_light2.target.z };
                if (ImGui::DragFloat3("Light 2 Target", light2Target, 1.0f)) {
                    m_light2.target = Vec3(light2Target[0], light2Target[1], light2Target[2]);
                }
                
                ImGui::SliderFloat("Light 2 Ortho Size", &m_light2.orthoSize, 50.0f, 200.0f, "%.1f");
                ImGui::SliderFloat("Light 2 Near", &m_light2.nearPlane, 0.01f, 10.0f, "%.2f");
                ImGui::SliderFloat("Light 2 Far", &m_light2.farPlane, 50.0f, 500.0f, "%.1f");
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Reset Lights to Default", ImVec2(200, 0))) {
                // Reset Light 1
                m_light1.enabled = true;
                m_light1.direction = Vec3(0.0f, -1.0f, 0.0f);
                m_light1.color = Vec3(1.0f, 1.0f, 1.0f);
                m_light1.intensity = 1.0f;
                m_light1.position = Vec3(0.0f, 50.0f, 0.0f);
                m_light1.target = Vec3(0.0f, 0.0f, 0.0f);
                m_light1.orthoSize = 100.0f;
                m_light1.nearPlane = 0.1f;
                m_light1.farPlane = 200.0f;
                
                // Reset Light 2
                m_light2.enabled = true;
                m_light2.direction = Vec3(0.6f, -0.5f, -0.3f);
                m_light2.color = Vec3(0.9f, 0.95f, 1.0f);
                m_light2.intensity = 0.6f;
                m_light2.position = Vec3(60.0f, 60.0f, 60.0f);
                m_light2.target = Vec3(0.0f, 0.0f, 0.0f);
                m_light2.orthoSize = 120.0f;
                m_light2.nearPlane = 0.1f;
                m_light2.farPlane = 250.0f;
            }

            // Shadow Mapping UI removed (controls handled elsewhere)
            
            // Background UI removed (dotted background always shown)
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

void PartitionScene::updateOctreeVisualization() {
    if (!m_showOctree || !m_lineRenderer) return;
    if (m_spatial3DType == Spatial3DType::Octree) {
        generateOctreeVisualization();
    } else {
        generateKDTreeVisualization();
    }
    
}

void PartitionScene::generateOctreeVisualization() {
    if (!m_octree || !m_lineRenderer || !m_is3DMode) {
        return;
    }
    
    // Clear existing lines before generating new ones
    m_lineRenderer->clear();
    
    // Get all nodes from the octree
    std::vector<Octree*> allNodes;
    m_octree->getAllNodes(allNodes);
    
    
    
    // Draw each node using 3D lines
    int nodesDrawn = 0;
    for (Octree* node : allNodes) {
        if (node) {
            // Only draw nodes that are within the max depth limit
            if (node->getDepth() > m_octreeMaxDepth) {
                continue;
            }
            
            // Only draw nodes that contain entities or have entities in their subtree
            // This prevents showing empty nodes while still showing the hierarchical structure
            if (!node->hasEntitiesInSubtree()) {
                continue; // Skip nodes with no entities in their subtree
            }
            
            // Additional debug: double-check that this node actually has entities
            if (node->getEntities().empty() && node->isLeaf()) {
                continue; // Skip empty leaf nodes
            }
            
            // The hasEntitiesInSubtree() check above should already handle this case
            
            // Apply transformations: scale, rotate, translate
            Vec3 center = node->getCenter() * m_octreeVisualizationScale;
            Vec3 size = node->getSize() * m_octreeVisualizationScale;
            
            // Apply rotation (convert degrees to radians)
            float rotX = m_octreeVisualizationRotation.x * 3.14159f / 180.0f;
            float rotY = m_octreeVisualizationRotation.y * 3.14159f / 180.0f;
            float rotZ = m_octreeVisualizationRotation.z * 3.14159f / 180.0f;
            
            // Apply translation offset
            center = center + m_octreeVisualizationOffset;
            
            // Rotate the entire octree structure around the origin (0,0,0)
            // This rotates the whole octree as one unit, not individual boxes
            center = rotatePointAroundOrigin(center, rotX, rotY, rotZ);
            
            // Determine color based on depth if enabled
            Vec4 lineColor = m_octreeLineColor;
            if (m_showOctreeDepthColors) {
                // Create a depth-based color gradient
                float depthRatio = static_cast<float>(node->getDepth()) / static_cast<float>(m_octreeMaxDepth);
                lineColor = Vec4(
                    0.2f + depthRatio * 0.8f,  // Red component
                    0.2f + (1.0f - depthRatio) * 0.8f,  // Green component
                    0.8f,  // Blue component
                    1.0f
                );
            }
            
            // Use thin lines for cleaner octree visualization
            float lineThickness = std::max(0.1f, m_octreeLineThickness * 0.3f); // Much thinner lines
            
            // Draw wireframe edges for clean octree visualization
            Vec3 halfSize = size * 0.5f;
            Vec3 min = center - halfSize;
            Vec3 max = center + halfSize;
            
            // Define the 8 corners of the box
            Vec3 corners[8] = {
                Vec3(min.x, min.y, min.z), // 0: min corner
                Vec3(max.x, min.y, min.z), // 1
                Vec3(max.x, min.y, max.z), // 2
                Vec3(min.x, min.y, max.z), // 3
                Vec3(min.x, max.y, min.z), // 4: max corner
                Vec3(max.x, max.y, min.z), // 5
                Vec3(max.x, max.y, max.z), // 6
                Vec3(min.x, max.y, max.z)  // 7
            };
            
            // Apply rotation to all corners (rotating around origin, not individual box centers)
            for (int i = 0; i < 8; i++) {
                corners[i] = rotatePointAroundOrigin(corners[i], rotX, rotY, rotZ);
            }
            
            // Draw 12 edges of the box with thin lines
            // Bottom face (4 edges)
            m_lineRenderer->addLine3D(corners[0], corners[1], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[1], corners[2], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[2], corners[3], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[3], corners[0], lineColor, lineThickness);
            
            // Top face (4 edges)
            m_lineRenderer->addLine3D(corners[4], corners[5], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[5], corners[6], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[6], corners[7], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[7], corners[4], lineColor, lineThickness);
            
            // Vertical edges (4 edges)
            m_lineRenderer->addLine3D(corners[0], corners[4], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[1], corners[5], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[2], corners[6], lineColor, lineThickness);
            m_lineRenderer->addLine3D(corners[3], corners[7], lineColor, lineThickness);
            
            nodesDrawn++;
        }
    }
    
    
}

void PartitionScene::generateKDTreeVisualization() {
    if (!m_lineRenderer || !m_is3DMode) return;
    m_lineRenderer->clear();

    // Collect 3D points from visible mesh components (exclude planes/anchors/transparent helpers)
    std::vector<Vec3> points;
    auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
    for (auto* entity : mesh3DEntities) {
        const std::string& nm = entity->getName();
        if (nm == "GroundPlane" || nm == "UnitSquare" || nm == "WorldOriginAnchor" || nm.rfind("TransparentUnit3D", 0) == 0) {
            continue;
        }
        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
            if (meshComp->isVisible()) {
                points.push_back(meshComp->getPosition());
            }
        }
    }
    if (points.empty()) return;

    // Compute bounds from points for tighter KD regions
    Vec3 minP(999999.0f, 999999.0f, 999999.0f);
    Vec3 maxP(-999999.0f, -999999.0f, -999999.0f);
    for (const auto& p : points) {
        minP.x = std::min(minP.x, p.x); minP.y = std::min(minP.y, p.y); minP.z = std::min(minP.z, p.z);
        maxP.x = std::max(maxP.x, p.x); maxP.y = std::max(maxP.y, p.y); maxP.z = std::max(maxP.z, p.z);
    }
    // Expand bounds for clearer visualization and future movement
    Vec3 baseExtent = (maxP - minP);
    Vec3 extra = Vec3(std::max(100.0f, baseExtent.x * 0.2f), std::max(100.0f, baseExtent.y * 0.2f), std::max(100.0f, baseExtent.z * 0.2f));
    minP = minP - extra;
    maxP = maxP + extra;
    Vec3 center = (minP + maxP) * 0.5f;
    Vec3 size = (maxP - minP);
    // Ensure non-zero extents
    size.x = std::max(size.x, 1.0f);
    size.y = std::max(size.y, 1.0f);
    size.z = std::max(size.z, 1.0f);

    // Build KD-tree nodes (flat list for visualization)
    std::vector<int> indices(points.size());
    for (int i = 0; i < (int)points.size(); ++i) indices[i] = i;
    std::vector<KDTree3DNode> nodes;

    // Parameters from UI
    int kdMaxDepth = std::max(1, m_kdMaxDepth);
    int kdLeafCap = std::max(1, m_kdLeafCapacity);
    buildKDTree3D(points, indices, center, size, 0, kdMaxDepth, kdLeafCap, 0, nodes);

    // Draw nodes similarly to octree: boxes per node (thin wireframe), depth-based colors
    for (const auto& n : nodes) {
        if (n.depth > m_octreeMaxDepth) continue;
        if (n.count == 0) continue;

        // Apply transforms
        Vec3 visCenter = (n.center * m_octreeVisualizationScale) + m_octreeVisualizationOffset;
        float rotX = m_octreeVisualizationRotation.x * 3.14159f / 180.0f;
        float rotY = m_octreeVisualizationRotation.y * 3.14159f / 180.0f;
        float rotZ = m_octreeVisualizationRotation.z * 3.14159f / 180.0f;
        visCenter = rotatePointAroundOrigin(visCenter, rotX, rotY, rotZ);

        Vec3 visSize = n.size * m_octreeVisualizationScale;

        Vec4 lineColor = m_octreeLineColor;
        if (m_showOctreeDepthColors) {
            float depthRatio = (float)n.depth / std::max(1, m_octreeMaxDepth);
            lineColor = Vec4(0.2f + depthRatio * 0.8f, 0.2f + (1.0f - depthRatio) * 0.8f, 0.8f, 1.0f);
        }
        float lineThickness = std::max(0.1f, m_octreeLineThickness * 0.3f);

        Vec3 halfSize = visSize * 0.5f;
        Vec3 min = visCenter - halfSize;
        Vec3 max = visCenter + halfSize;
        Vec3 corners[8] = {
            Vec3(min.x, min.y, min.z),
            Vec3(max.x, min.y, min.z),
            Vec3(max.x, min.y, max.z),
            Vec3(min.x, min.y, max.z),
            Vec3(min.x, max.y, min.z),
            Vec3(max.x, max.y, min.z),
            Vec3(max.x, max.y, max.z),
            Vec3(min.x, max.y, max.z)
        };
        for (int i = 0; i < 8; ++i) corners[i] = rotatePointAroundOrigin(corners[i], rotX, rotY, rotZ);
        m_lineRenderer->addLine3D(corners[0], corners[1], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[1], corners[2], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[2], corners[3], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[3], corners[0], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[4], corners[5], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[5], corners[6], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[6], corners[7], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[7], corners[4], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[0], corners[4], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[1], corners[5], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[2], corners[6], lineColor, lineThickness);
        m_lineRenderer->addLine3D(corners[3], corners[7], lineColor, lineThickness);
    }
}

void PartitionScene::buildKDTree3D(const std::vector<Vec3>& points, const std::vector<int>& indices, const Vec3& center, const Vec3& size, int depth, int maxDepth, int maxEntities, int axis, std::vector<KDTree3DNode>& outNodes) {
    if (indices.empty()) return;
    KDTree3DNode node;
    node.center = center;
    node.size = size;
    node.depth = depth;
    node.axis = axis;
    node.count = (int)indices.size();
    outNodes.push_back(node);

    if (depth >= maxDepth || (int)indices.size() <= maxEntities) {
        return;
    }

    // Compute median along current axis
    std::vector<int> sorted = indices;
    std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end(), [&](int a, int b){
        if (axis == 0) return points[a].x < points[b].x;
        if (axis == 1) return points[a].y < points[b].y;
        return points[a].z < points[b].z;
    });
    float splitValue;
    int medianIdx = sorted[sorted.size() / 2];
    if (axis == 0) splitValue = points[medianIdx].x; else if (axis == 1) splitValue = points[medianIdx].y; else splitValue = points[medianIdx].z;

    // Partition indices into left/right
    std::vector<int> left, right;
    left.reserve(sorted.size());
    right.reserve(sorted.size());
    for (int idx : sorted) {
        float v = (axis == 0) ? points[idx].x : (axis == 1) ? points[idx].y : points[idx].z;
        if (v <= splitValue) left.push_back(idx); else right.push_back(idx);
    }

    // Compute child centers/sizes using split position projected into node
    Vec3 half = size * 0.5f;
    Vec3 min = center - half;
    Vec3 max = center + half;
    Vec3 leftCenter = center;
    Vec3 rightCenter = center;
    Vec3 leftSize = size;
    Vec3 rightSize = size;
    if (axis == 0) {
        // clamp split into [min.x, max.x]
        float s = std::max(min.x, std::min(max.x, splitValue));
        leftCenter.x = (min.x + s) * 0.5f;
        rightCenter.x = (s + max.x) * 0.5f;
        leftSize.x = std::max(0.001f, s - min.x);
        rightSize.x = std::max(0.001f, max.x - s);
    } else if (axis == 1) {
        float s = std::max(min.y, std::min(max.y, splitValue));
        leftCenter.y = (min.y + s) * 0.5f;
        rightCenter.y = (s + max.y) * 0.5f;
        leftSize.y = std::max(0.001f, s - min.y);
        rightSize.y = std::max(0.001f, max.y - s);
    } else {
        float s = std::max(min.z, std::min(max.z, splitValue));
        leftCenter.z = (min.z + s) * 0.5f;
        rightCenter.z = (s + max.z) * 0.5f;
        leftSize.z = std::max(0.001f, s - min.z);
        rightSize.z = std::max(0.001f, max.z - s);
    }

    int nextAxis = (axis + 1) % 3;
    buildKDTree3D(points, left, leftCenter, leftSize, depth + 1, maxDepth, maxEntities, nextAxis, outNodes);
    buildKDTree3D(points, right, rightCenter, rightSize, depth + 1, maxDepth, maxEntities, nextAxis, outNodes);
}

void PartitionScene::drawOctreeNode(const Octree* node, int depth, const Vec3& center, const Vec3& size) {
    // This method can be used for more detailed node visualization if needed
    // For now, the generateOctreeVisualization method handles the drawing
}

Vec3 PartitionScene::rotatePointAroundOrigin(const Vec3& point, float rotX, float rotY, float rotZ) {
    Vec3 result = point;
    
    // Apply rotations in Z, Y, X order (same as before but around origin)
    // Z rotation
    float cosZ = std::cos(rotZ);
    float sinZ = std::sin(rotZ);
    float newX = result.x * cosZ - result.y * sinZ;
    float newY = result.x * sinZ + result.y * cosZ;
    result = Vec3(newX, newY, result.z);
    
    // Y rotation
    float cosY = std::cos(rotY);
    float sinY = std::sin(rotY);
    float newX2 = result.x * cosY + result.z * sinY;
    float newZ = -result.x * sinY + result.z * cosY;
    result = Vec3(newX2, result.y, newZ);
    
    // X rotation
    float cosX = std::cos(rotX);
    float sinX = std::sin(rotX);
    float newY2 = result.y * cosX - result.z * sinX;
    float newZ2 = result.y * sinX + result.z * cosX;
    result = Vec3(result.x, newY2, newZ2);
    
    return result;
}


