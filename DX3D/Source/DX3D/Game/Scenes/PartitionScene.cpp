// (moved) computeClusterIoU and remapDBSCANClusterIdsStable definitions are located below after all includes
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
#include <DX3D/Components/PanelComponent.h>
#include <random>
#include <iostream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <set>

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


    auto& moveButtonEntity = m_entityManager->createEntity("ToggleMovementButton");
    auto& moveButton = moveButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle Movement", 18.0f
    );
    moveButton.setScreenPosition(0.1f, 0.2f); // Position below other buttons
    moveButton.setNormalTint(Vec4(0.8f, 0.4f, 0.8f, 0.8f));
    moveButton.setOnClickCallback([this]() {
        m_entitiesMoving = !m_entitiesMoving;
        });

    // K-means clustering buttons
    auto& kmeansButtonEntity = m_entityManager->createEntity("KMeansButton");
    auto& kmeansButton = kmeansButtonEntity.addComponent<ButtonComponent>(
        device, L"K-Means Clustering", 18.0f
    );
    kmeansButton.setScreenPosition(0.1f, 0.25f);
    kmeansButton.setNormalTint(Vec4(0.2f, 0.8f, 0.8f, 0.8f));
    kmeansButton.setOnClickCallback([this]() {
        // Disable DBSCAN if it's enabled
        if (m_dbscanEnabled) {
            m_dbscanEnabled = false;
            // Reset DBSCAN colors to default
            for (auto& movingEntity : m_movingEntities) {
                if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
                    }
                }
            }
            m_dbscanClusters.clear();
            updateDBSCANButtonVisibility();

            // Switch to original (K-means) offset when leaving DBSCAN
            m_quadtreeVisualOffset = m_quadtreeVisualOffsetOriginal;
            updateQuadtreeVisualization();
            std::cout << "Offset: (" << m_quadtreeVisualOffset.x << ", " << m_quadtreeVisualOffset.y << ")" << std::endl;
        }
        
        m_kmeansEnabled = !m_kmeansEnabled;
        if (m_kmeansEnabled) {
            // Ensure K-means uses the original offset
            m_quadtreeVisualOffset = m_quadtreeVisualOffsetOriginal;
            updateQuadtreeVisualization();
            std::cout << "Offset: (" << m_quadtreeVisualOffset.x << ", " << m_quadtreeVisualOffset.y << ")" << std::endl;
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
        updateKMeansButtonVisibility();
        });

    // Decrease K button (moved up)
    auto& decreaseKButtonEntity = m_entityManager->createEntity("DecreaseKButton");
    auto& decreaseKButton = decreaseKButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease K", 18.0f
    );
    decreaseKButton.setScreenPosition(0.1f, 0.3f);
    decreaseKButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    decreaseKButton.setOnClickCallback([this]() {
        if (m_kmeansK > 2) {
            m_kmeansK--;
            if (m_kmeansEnabled) {
                performKMeansClustering();
            }
        }
        });
    m_decreaseKButton = &decreaseKButton;

    // Increase K button (moved down)
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
    m_increaseKButton = &increaseKButton;

    // Hull/Voronoi toggle button (visible only when K-means enabled)
    auto& hullVoronoiToggleEntity = m_entityManager->createEntity("HullVoronoiToggleButton");
    auto& hullVoronoiToggle = hullVoronoiToggleEntity.addComponent<ButtonComponent>(
        device, L"Toggle Hull/Voronoi", 18.0f
    );
    hullVoronoiToggle.setScreenPosition(0.1f, 0.4f);
    hullVoronoiToggle.setNormalTint(Vec4(0.2f, 0.8f, 0.5f, 0.8f));
    hullVoronoiToggle.setOnClickCallback([this, &hullVoronoiToggle]() {
        m_useVoronoi = !m_useVoronoi;
        hullVoronoiToggle.setText(m_useVoronoi ? L"Show Hulls" : L"Show Voronoi");
        updateQuadtreeVisualization();
    });
    m_kmeansHullVoronoiToggle = &hullVoronoiToggle;

    // Concentric circles dataset button
    auto& circlesButtonEntity = m_entityManager->createEntity("CirclesDatasetButton");
    auto& circlesButton = circlesButtonEntity.addComponent<ButtonComponent>(
        device, L"Spawn Circles Dataset", 18.0f
    );
    circlesButton.setScreenPosition(0.1f, 0.85f); // moved up000000000000000000000000000 to create more space
    circlesButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    circlesButton.setOnClickCallback([this]() {
        generateConcentricCirclesDataset();
    });

    // Concentric circles (light) dataset button
    auto& circlesLightButtonEntity = m_entityManager->createEntity("CirclesDatasetLightButton");
    auto& circlesLightButton = circlesLightButtonEntity.addComponent<ButtonComponent>(
        device, L"Spawn Circles Dataset (Light)", 18.0f
    );
    circlesLightButton.setScreenPosition(0.1f, 0.9f); // moved up to create more space
    circlesLightButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    circlesLightButton.setOnClickCallback([this]() {
        generateConcentricCirclesDatasetLight();
    });


    // Fast mode button
    auto& fastModeButtonEntity = m_entityManager->createEntity("FastModeButton");
    auto& fastModeButton = fastModeButtonEntity.addComponent<ButtonComponent>(
        device, L"Fast Mode", 18.0f
    );
    fastModeButton.setScreenPosition(0.1f, 0.45f);
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

    // DBSCAN clustering button
    auto& dbscanButtonEntity = m_entityManager->createEntity("DBSCANButton");
    auto& dbscanButton = dbscanButtonEntity.addComponent<ButtonComponent>(
        device, L"DBSCAN Clustering", 18.0f
    );
    dbscanButton.setScreenPosition(0.1f, 0.5f);
    dbscanButton.setNormalTint(Vec4(0.8f, 0.4f, 0.2f, 0.8f));
    dbscanButton.setOnClickCallback([this]() {
        // Disable K-means if it's enabled
        if (m_kmeansEnabled) {
            m_kmeansEnabled = false;
            // Reset K-means colors to default
            for (auto& movingEntity : m_movingEntities) {
                if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
                    }
                }
            }
            m_clusters.clear();
            updateKMeansButtonVisibility();
        }
        
        m_dbscanEnabled = !m_dbscanEnabled;
        if (m_dbscanEnabled) {
            // No offset change for DBSCAN; keep current offset
            performDBSCANClustering();
        } else {
            // Switch back to original offset
            m_quadtreeVisualOffset = m_quadtreeVisualOffsetOriginal;
            // Reset colors to default
            for (auto& movingEntity : m_movingEntities) {
                if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
                    }
                }
            }
            m_dbscanClusters.clear();
        }
        updateDBSCANButtonVisibility();
        updateQuadtreeVisualization();
        });

    // DBSCAN Epsilon decrease button (moved up)
    auto& decreaseEpsButtonEntity = m_entityManager->createEntity("DecreaseEpsButton");
    auto& decreaseEpsButton = decreaseEpsButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease Eps", 18.0f
    );
    decreaseEpsButton.setScreenPosition(0.1f, 0.55f);
    decreaseEpsButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    decreaseEpsButton.setOnClickCallback([this]() {
        if (m_dbscanEps > 10.0f) {
            m_dbscanEps -= 10.0f;
            if (m_dbscanEnabled) {
                performDBSCANClustering();
            }
        }
        });

    // DBSCAN Epsilon increase button (moved down)
    auto& increaseEpsButtonEntity = m_entityManager->createEntity("IncreaseEpsButton");
    auto& increaseEpsButton = increaseEpsButtonEntity.addComponent<ButtonComponent>(
        device, L"Increase Eps", 18.0f
    );
    increaseEpsButton.setScreenPosition(0.1f, 0.6f);
    increaseEpsButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    increaseEpsButton.setOnClickCallback([this]() {
        if (m_dbscanEps < 200.0f) {
            m_dbscanEps += 10.0f;
            if (m_dbscanEnabled) {
                performDBSCANClustering();
            }
        }
        });

    // DBSCAN MinPts decrease button (moved up)
    auto& decreaseMinPtsButtonEntity = m_entityManager->createEntity("DecreaseMinPtsButton");
    auto& decreaseMinPtsButton = decreaseMinPtsButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease MinPts", 18.0f
    );
    decreaseMinPtsButton.setScreenPosition(0.1f, 0.65f);
    decreaseMinPtsButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    decreaseMinPtsButton.setOnClickCallback([this]() {
        if (m_dbscanMinPts > 2) {
            m_dbscanMinPts--;
            if (m_dbscanEnabled) {
                performDBSCANClustering();
            }
        }
        });

    // DBSCAN MinPts increase button (moved down)
    auto& increaseMinPtsButtonEntity = m_entityManager->createEntity("IncreaseMinPtsButton");
    auto& increaseMinPtsButton = increaseMinPtsButtonEntity.addComponent<ButtonComponent>(
        device, L"Increase MinPts", 18.0f
    );
    increaseMinPtsButton.setScreenPosition(0.1f, 0.7f);
    increaseMinPtsButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    increaseMinPtsButton.setOnClickCallback([this]() {
        if (m_dbscanMinPts < 10) {
            m_dbscanMinPts++;
            if (m_dbscanEnabled) {
                performDBSCANClustering();
            }
        }
        });
    
    // Store DBSCAN button references
    m_increaseEpsButton = &increaseEpsButton;
    m_decreaseEpsButton = &decreaseEpsButton;
    m_increaseMinPtsButton = &increaseMinPtsButton;
    m_decreaseMinPtsButton = &decreaseMinPtsButton;

    // Partition control panel and buttons (right side)
    auto& partPanelEntity = m_entityManager->createEntity("PartitionPanel");
    auto& partPanel = partPanelEntity.addComponent<PanelComponent>(
        device,
        0.22f * GraphicsEngine::getWindowWidth(),
        0.18f * GraphicsEngine::getWindowHeight()
    );
    // Move to top-right corner
    partPanel.setScreenPosition(0.95f, 0.06f);
    partPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_partitionStatusPanel = &partPanel;

    auto& partStatusTextEntity = m_entityManager->createEntity("PartitionStatusText");
    m_partitionStatusText = &partStatusTextEntity.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"Partition: Quadtree",
        16.0f
    );
    // Text slightly below buttons
    m_partitionStatusText->setScreenPosition(0.95f, 0.18f);
    m_partitionStatusText->setColor(Vec4(1,1,1,1));

    auto& btnOctEntity = m_entityManager->createEntity("BtnQuadtree");
    auto& btnOct = btnOctEntity.addComponent<ButtonComponent>(device, L"Quadtree", 18.0f);
    btnOct.setScreenPosition(0.95f, 0.06f);
    btnOct.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnOct.setOnClickCallback([this]() {
        m_partitionType = PartitionType::Quadtree;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });

    auto& btnAabbEntity = m_entityManager->createEntity("BtnAABB");
    auto& btnAabb = btnAabbEntity.addComponent<ButtonComponent>(device, L"AABB", 18.0f);
    btnAabb.setScreenPosition(0.95f, 0.10f);
    btnAabb.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnAabb.setOnClickCallback([this]() {
        m_partitionType = PartitionType::AABB;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });

    auto& btnKdEntity = m_entityManager->createEntity("BtnKD");
    auto& btnKd = btnKdEntity.addComponent<ButtonComponent>(device, L"KD Tree", 18.0f);
    btnKd.setScreenPosition(0.95f, 0.14f);
    btnKd.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnKd.setOnClickCallback([this]() {
        m_partitionType = PartitionType::KDTree;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });


    // DBSCAN Hull/Voronoi toggle button
    auto& dbscanHVToggleEntity = m_entityManager->createEntity("DBSCANHullVoronoiToggleButton");
    auto& dbscanHVToggle = dbscanHVToggleEntity.addComponent<ButtonComponent>(
        device, L"DBSCAN: Show Voronoi", 18.0f
    );
    dbscanHVToggle.setScreenPosition(0.1f, 0.75f);
    dbscanHVToggle.setNormalTint(Vec4(0.8f, 0.4f, 0.2f, 0.8f));
    dbscanHVToggle.setOnClickCallback([this, &dbscanHVToggle]() {
        m_dbscanUseVoronoi = !m_dbscanUseVoronoi;
        dbscanHVToggle.setText(m_dbscanUseVoronoi ? L"DBSCAN: Show Hulls" : L"DBSCAN: Show Voronoi");
        updateQuadtreeVisualization();
    });
    m_dbscanHullVoronoiToggle = &dbscanHVToggle;

    // (removed) DBSCAN hull toggle button

    // Entity count display panel
    auto& entityCountPanelEntity = m_entityManager->createEntity("EntityCountPanel");
    auto& entityCountPanel = entityCountPanelEntity.addComponent<PanelComponent>(
        device, 
        0.25f * GraphicsEngine::getWindowWidth(),  // Convert to screen pixels
        0.15f * GraphicsEngine::getWindowHeight()  // Convert to screen pixels
    );
    entityCountPanel.setScreenPosition(0.9f, 0.26f);
    entityCountPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f)); // Semi-transparent black background

    // Entity count text
    auto& entityCountTextEntity = m_entityManager->createEntity("EntityCountText");
    m_entityCountText = &entityCountTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Entities: 0", 
        20.0f
    );
    m_entityCountText->setScreenPosition(0.9f, 0.28f);
    m_entityCountText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text

    // K-means test data panel
    auto& kmeansDataPanelEntity = m_entityManager->createEntity("KMeansDataPanel");
    auto& kmeansDataPanel = kmeansDataPanelEntity.addComponent<PanelComponent>(
        device, 
        0.3f * GraphicsEngine::getWindowWidth(),
        0.25f * GraphicsEngine::getWindowHeight()
    );
    kmeansDataPanel.setScreenPosition(0.9f, 0.3f);
    kmeansDataPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f)); // Semi-transparent black background
    m_kmeansDataPanel = &kmeansDataPanel;

    // K-means test data text elements
    auto& kmeansKTextEntity = m_entityManager->createEntity("KMeansKText");
    m_kmeansKText = &kmeansKTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"K: 3", 
        16.0f
    );
    m_kmeansKText->setScreenPosition(0.9f, 0.32f);
    m_kmeansKText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& kmeansIterationsTextEntity = m_entityManager->createEntity("KMeansIterationsText");
    m_kmeansIterationsText = &kmeansIterationsTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Iterations: 0", 
        16.0f
    );
    m_kmeansIterationsText->setScreenPosition(0.9f, 0.36f);
    m_kmeansIterationsText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& kmeansConvergedTextEntity = m_entityManager->createEntity("KMeansConvergedText");
    m_kmeansConvergedText = &kmeansConvergedTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Converged: No", 
        16.0f
    );
    m_kmeansConvergedText->setScreenPosition(0.9f, 0.4f);
    m_kmeansConvergedText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& kmeansClustersTextEntity = m_entityManager->createEntity("KMeansClustersText");
    m_kmeansClustersText = &kmeansClustersTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Active Clusters: 0", 
        16.0f
    );
    m_kmeansClustersText->setScreenPosition(0.9f, 0.44f);
    m_kmeansClustersText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& kmeansAvgDistanceTextEntity = m_entityManager->createEntity("KMeansAvgDistanceText");
    m_kmeansAvgDistanceText = &kmeansAvgDistanceTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Avg Distance: 0.0", 
        16.0f
    );
    m_kmeansAvgDistanceText->setScreenPosition(0.9f, 0.48f);
    m_kmeansAvgDistanceText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // DBSCAN test data panel
    auto& dbscanDataPanelEntity = m_entityManager->createEntity("DBSCANDataPanel");
    auto& dbscanDataPanel = dbscanDataPanelEntity.addComponent<PanelComponent>(
        device, 
        0.3f * GraphicsEngine::getWindowWidth(),
        0.2f * GraphicsEngine::getWindowHeight()
    );
    dbscanDataPanel.setScreenPosition(0.9f, 0.6f);
    dbscanDataPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f)); // Semi-transparent black background
    m_dbscanDataPanel = &dbscanDataPanel;

    // DBSCAN test data text elements
    auto& dbscanEpsTextEntity = m_entityManager->createEntity("DBSCANEpsText");
    m_dbscanEpsText = &dbscanEpsTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Eps: 50.0", 
        16.0f
    );
    m_dbscanEpsText->setScreenPosition(0.9f, 0.62f);
    m_dbscanEpsText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& dbscanMinPtsTextEntity = m_entityManager->createEntity("DBSCANMinPtsText");
    m_dbscanMinPtsText = &dbscanMinPtsTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"MinPts: 3", 
        16.0f
    );
    m_dbscanMinPtsText->setScreenPosition(0.9f, 0.66f);
    m_dbscanMinPtsText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& dbscanClustersTextEntity = m_entityManager->createEntity("DBSCANClustersText");
    m_dbscanClustersText = &dbscanClustersTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Clusters: 0", 
        16.0f
    );
    m_dbscanClustersText->setScreenPosition(0.9f, 0.7f);
    m_dbscanClustersText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Offset control panel
    auto& offsetPanelEntity = m_entityManager->createEntity("OffsetPanel");
    auto& offsetPanel = offsetPanelEntity.addComponent<PanelComponent>(
        device, 
        0.25f * GraphicsEngine::getWindowWidth(),
        0.15f * GraphicsEngine::getWindowHeight()
    );
    offsetPanel.setScreenPosition(0.1f, 0.8f);
    offsetPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f)); // Semi-transparent black background

    // Offset X text
    auto& offsetXTextEntity = m_entityManager->createEntity("OffsetXText");
    m_offsetXText = &offsetXTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Offset X: 500.0", 
        16.0f
    );
    m_offsetXText->setScreenPosition(0.1f, 0.82f);
    m_offsetXText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    m_offsetXText->setVisible(false);

    // Offset Y text
    auto& offsetYTextEntity = m_entityManager->createEntity("OffsetYText");
    m_offsetYText = &offsetYTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Offset Y: 0.0", 
        16.0f
    );
    m_offsetYText->setScreenPosition(0.1f, 0.86f);
    m_offsetYText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    m_offsetYText->setVisible(false);

    // Initially hide K-means related buttons and data
    updateKMeansButtonVisibility();
    updateDBSCANButtonVisibility();
    updateHullVoronoiToggleVisibility();
    // (removed) dbscan hull toggle visibility
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
        
        // Recompute DBSCAN periodically when enabled and entities are moving
        if (m_dbscanEnabled && m_entitiesMoving) {
            m_dbscanUpdateTimer += dt;
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

            // Draw boundary: Voronoi cells or Convex hulls depending on toggle
            if (m_useVoronoi) {
                // Compute Voronoi cell for this centroid within quadtree bounds
                Vec2 boundsCenter = Vec2(0.0f, 0.0f);
                Vec2 boundsSize = Vec2(800.0f, 600.0f);

                // Collect all sites
                std::vector<Vec2> allSites;
                allSites.reserve(m_clusters.size());
                for (const auto& c2 : m_clusters) allSites.push_back(c2.centroid);

                auto cell = computeVoronoiCell(cluster.centroid, allSites, boundsCenter, boundsSize);
                if (cell.size() >= 3) {
                    for (size_t i = 0; i < cell.size(); ++i) {
                        const Vec2 a = cell[i] + m_quadtreeVisualOffset;
                        const Vec2 b = cell[(i + 1) % cell.size()] + m_quadtreeVisualOffset;
                        m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                    }
                }
            }
            else {
                // Draw convex hull around cluster
                std::vector<Vec2> points;
                points.reserve(cluster.entityIndices.size());
                for (int entityIndex : cluster.entityIndices) {
                    if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                        points.push_back(m_movingEntities[entityIndex].qtEntity.position);
                    }
                }
                if (points.size() >= 3) {
                    auto hull = computeConvexHull(points);
                    if (hull.size() >= 3) {
                        for (size_t i = 0; i < hull.size(); ++i) {
                            const Vec2& a = hull[i] + m_quadtreeVisualOffset;
                            const Vec2& b = hull[(i + 1) % hull.size()] + m_quadtreeVisualOffset;
                            m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                        }
                    }
                }
            }
        }
    }
    
    // Draw DBSCAN cluster visualization if enabled
    if (m_dbscanEnabled && m_showDBSCANVisualization && !m_dbscanClusters.empty()) {
        // Precompute DBSCAN cluster centroids for Voronoi sites (if needed)
        std::vector<Vec2> dbscanCentroids;
        if (m_dbscanUseVoronoi) {
            dbscanCentroids.reserve(m_dbscanClusters.size());
            for (const auto& cluster : m_dbscanClusters) {
                Vec2 c(0.0f, 0.0f);
                int count = 0;
                for (int idx : cluster.entityIndices) {
                    if (idx >= 0 && idx < (int)m_movingEntities.size() && m_movingEntities[idx].active) {
                        c.x += m_movingEntities[idx].qtEntity.position.x;
                        c.y += m_movingEntities[idx].qtEntity.position.y;
                        ++count;
                    }
                }
                if (count > 0) {
                    c.x /= count; c.y /= count;
                }
                dbscanCentroids.push_back(c);
            }
        }

        // Draw cluster connections
        for (const auto& cluster : m_dbscanClusters) {
            if (cluster.entityIndices.size() < 2) continue; // Need at least 2 points to draw connections
            
            // Draw connections between entities in the same cluster
            for (size_t i = 0; i < cluster.entityIndices.size(); i++) {
                for (size_t j = i + 1; j < cluster.entityIndices.size(); j++) {
                    int entityIndex1 = cluster.entityIndices[i];
                    int entityIndex2 = cluster.entityIndices[j];
                    
                    if (entityIndex1 < m_movingEntities.size() && entityIndex2 < m_movingEntities.size() &&
                        m_movingEntities[entityIndex1].active && m_movingEntities[entityIndex2].active) {
                        
                        Vec2 pos1 = m_movingEntities[entityIndex1].qtEntity.position + m_quadtreeVisualOffset;
                        Vec2 pos2 = m_movingEntities[entityIndex2].qtEntity.position + m_quadtreeVisualOffset;
                        
                        // Only draw line if entities are within epsilon distance
                        if (calculateDistance(m_movingEntities[entityIndex1].qtEntity.position, 
                                             m_movingEntities[entityIndex2].qtEntity.position) <= m_dbscanEps) {
                            m_lineRenderer->addLine(pos1, pos2, cluster.color, 1.0f);
                        }
                    }
                }
            }

            // Draw convex hull around DBSCAN cluster only if Voronoi is off
            if (!m_dbscanUseVoronoi) {
                std::vector<Vec2> points;
                points.reserve(cluster.entityIndices.size());
                for (int entityIndex : cluster.entityIndices) {
                    if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                        points.push_back(m_movingEntities[entityIndex].qtEntity.position);
                    }
                }
                if (points.size() >= 3) {
                    auto hull = computeConvexHull(points);
                    if (hull.size() >= 3) {
                        for (size_t i = 0; i < hull.size(); ++i) {
                            const Vec2& a = hull[i] + m_quadtreeVisualOffset;
                            const Vec2& b = hull[(i + 1) % hull.size()] + m_quadtreeVisualOffset;
                            m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                        }
                    }
                }
            }
        }

        // Draw Voronoi partitions for DBSCAN clusters using their centroids (exclusive with hulls)
        if (m_dbscanUseVoronoi && !dbscanCentroids.empty()) {
            Vec2 boundsCenter = Vec2(0.0f, 0.0f);
            Vec2 boundsSize = Vec2(800.0f, 600.0f);
            for (size_t i = 0; i < m_dbscanClusters.size(); ++i) {
                auto cell = computeVoronoiCell(dbscanCentroids[i], dbscanCentroids, boundsCenter, boundsSize);
                if (cell.size() >= 3) {
                    const Vec4 color = m_dbscanClusters[i].color;
                    for (size_t e = 0; e < cell.size(); ++e) {
                        const Vec2 a = cell[e] + m_quadtreeVisualOffset;
                        const Vec2 b = cell[(e + 1) % cell.size()] + m_quadtreeVisualOffset;
                        m_lineRenderer->addLine(a, b, color, 2.0f);
                    }
                }
            }
        }
    }

    if (!m_showQuadtree) { respawnWorldAnchorSprite(); return; }

    if (m_partitionType == PartitionType::Quadtree) {
        // Draw Quadtree nodes
        std::vector<Quadtree*> nodes;
        m_quadtree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 center = node->getCenter();
            Vec2 size = node->getSize();
            Vec2 visualCenter = center + m_quadtreeVisualOffset;
            m_lineRenderer->addRect(visualCenter, size, Vec4(0.0f, 0.0f, 0.0f, 0.6f), 1.0f);
            const auto& entities = node->getEntities();
            for (const auto& entity : entities) {
                Vec2 visualEntityPos = entity.position + m_quadtreeVisualOffset;
                m_lineRenderer->addRect(visualEntityPos, entity.size, Vec4(0.0f, 0.0f, 0.0f, 0.8f), 2.0f);
            }
        }
    } else if (m_partitionType == PartitionType::AABB) {
        // Draw AABB tree nodes
        std::vector<AABBNode*> nodes;
        m_aabbTree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 visualCenter = node->center + m_quadtreeVisualOffset;
            Vec2 size = node->halfSize * 2.0f;
            m_lineRenderer->addRect(visualCenter, size, Vec4(0.0f, 0.0f, 1.0f, 0.5f), 1.0f);
            if (node->isLeaf) {
                for (const auto& e : node->entities) {
                    Vec2 visualEntityPos = e.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addRect(visualEntityPos, e.size, Vec4(0.0f, 0.0f, 0.0f, 0.8f), 2.0f);
                }
            }
        }
    } else {
        // Draw KD tree nodes
        std::vector<KDNode*> nodes;
        m_kdTree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 visualCenter = node->center + m_quadtreeVisualOffset;
            Vec2 size = node->halfSize * 2.0f;
            // Use black for KD tree visualization for better contrast
            Vec4 color = Vec4(0.0f, 0.0f, 0.0f, 0.6f);
            if (m_kdShowSplitLines && !node->isLeaf) {
                // Draw split line segment inside this node's box
                if (node->axis == 0) {
                    // vertical line at split x
                    float x = node->split + m_quadtreeVisualOffset.x;
                    float y0 = visualCenter.y - size.y * 0.5f;
                    float y1 = visualCenter.y + size.y * 0.5f;
                    m_lineRenderer->addLine(Vec2(x, y0), Vec2(x, y1), color, 1.5f);
                } else {
                    // horizontal line at split y
                    float y = node->split + m_quadtreeVisualOffset.y;
                    float x0 = visualCenter.x - size.x * 0.5f;
                    float x1 = visualCenter.x + size.x * 0.5f;
                    m_lineRenderer->addLine(Vec2(x0, y), Vec2(x1, y), color, 1.5f);
                }
            } else {
                m_lineRenderer->addRect(visualCenter, size, color, 1.0f);
            }
            if (node->isLeaf) {
                for (const auto& e : node->entities) {
                    Vec2 visualEntityPos = e.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addRect(visualEntityPos, e.size, Vec4(0.0f, 0.0f, 0.0f, 0.8f), 2.0f);
                }
            }
        }
    }
    respawnWorldAnchorSprite();
}

void PartitionScene::respawnWorldAnchorSprite() {
    // Remove previous anchor entity
    if (m_entityManager->findEntity("WorldOriginAnchor")) {
        m_entityManager->removeEntity("WorldOriginAnchor");
    }
    // Spawn a fresh anchor sprite in screen space
    auto& device = m_lineRenderer->getDevice();
    auto& anchorEntity = m_entityManager->createEntity("WorldOriginAnchor");
    auto& anchorSprite = anchorEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png",
        1.0f,
        1.0f
    );
    // Place back at world origin in world space
    anchorSprite.setPosition(0.0f, 0.0f, 0.0f);
    anchorSprite.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    anchorSprite.setVisible(true);
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
        
        // Only n entity if not clicking on UI
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

    // Update entity count display
    if (m_entityCountText) {
        int activeEntityCount = 0;
        for (const auto& movingEntity : m_movingEntities) {
            if (movingEntity.active) {
                activeEntityCount++;
            }
        }
        
        std::wstring entityCountText = L"Entities: " + std::to_wstring(activeEntityCount);
        m_entityCountText->setText(entityCountText);
    }

    // Update K-means test data display
    updateKMeansTestData();
    
    // Update DBSCAN test data display
    updateDBSCANTestData();
    
    // Update offset controls
    updateOffsetControls(dt);
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

    // Rebuild quadtree and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    for (const auto& me : m_movingEntities) {
        m_quadtree->insert(me.qtEntity);
    }

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

    // Rebuild quadtree and insert entities
    m_quadtree = std::make_unique<Quadtree>(Vec2(0.0f, 0.0f), Vec2(800.0f, 600.0f), 4, 5);
    for (const auto& me : m_movingEntities) {
        m_quadtree->insert(me.qtEntity);
    }

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

    // Render sprites in world space (defer world anchor)
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    SpriteComponent* deferredWorldAnchor = nullptr;
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

    // Draw deferred world anchor last (after UI)
    if (deferredWorldAnchor && deferredWorldAnchor->isVisible() && deferredWorldAnchor->isValid()) {
        if (auto* cameraEntity2 = m_entityManager->findEntity("MainCamera")) {
            if (auto* camera2 = cameraEntity2->getComponent<Camera2D>()) {
                ctx.setViewMatrix(camera2->getViewMatrix());
                ctx.setProjectionMatrix(camera2->getProjectionMatrix());
            }
        }
        deferredWorldAnchor->draw(ctx);
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

void PartitionScene::updateKMeansButtonVisibility() {
    // Show/hide K-means related buttons based on K-means enabled state
    if (m_increaseKButton) {
        m_increaseKButton->setVisible(m_kmeansEnabled);
    }
    if (m_decreaseKButton) {
        m_decreaseKButton->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansHullVoronoiToggle) {
        m_kmeansHullVoronoiToggle->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansDataPanel) {
        m_kmeansDataPanel->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansKText) {
        m_kmeansKText->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansIterationsText) {
        m_kmeansIterationsText->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansConvergedText) {
        m_kmeansConvergedText->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansClustersText) {
        m_kmeansClustersText->setVisible(m_kmeansEnabled);
    }
    if (m_kmeansAvgDistanceText) {
        m_kmeansAvgDistanceText->setVisible(m_kmeansEnabled);
    }
}

void PartitionScene::updateHullVoronoiToggleVisibility() {
    if (m_kmeansHullVoronoiToggle) {
        m_kmeansHullVoronoiToggle->setVisible(m_kmeansEnabled);
        m_kmeansHullVoronoiToggle->setText(m_useVoronoi ? L"Show Hulls" : L"Show Voronoi");
    }
}

// (removed) updateDBSCANHullToggleVisibility

void PartitionScene::updateKMeansTestData() {
    if (!m_kmeansEnabled) return;

    // Update K value
    if (m_kmeansKText) {
        std::wstring kText = L"K: " + std::to_wstring(m_kmeansK);
        m_kmeansKText->setText(kText);
    }

    // Update iterations
    if (m_kmeansIterationsText) {
        std::wstring iterationsText = L"Iterations: " + std::to_wstring(m_kmeansIterations);
        m_kmeansIterationsText->setText(iterationsText);
    }

    // Update convergence status
    if (m_kmeansConvergedText) {
        std::wstring convergedText = m_kmeansConverged ? L"Converged: Yes" : L"Converged: No";
        m_kmeansConvergedText->setText(convergedText);
    }

    // Update active clusters count
    if (m_kmeansClustersText) {
        int activeClusters = 0;
        for (const auto& cluster : m_clusters) {
            if (!cluster.entityIndices.empty()) {
                activeClusters++;
            }
        }
        std::wstring clustersText = L"Active Clusters: " + std::to_wstring(activeClusters);
        m_kmeansClustersText->setText(clustersText);
    }

    // Update average distance to centroids
    if (m_kmeansAvgDistanceText) {
        float totalDistance = 0.0f;
        int validEntities = 0;
        
        for (int i = 0; i < m_movingEntities.size(); i++) {
            if (!m_movingEntities[i].active) continue;
            if (i >= m_entityClusterAssignments.size()) continue;
            
            int clusterIndex = m_entityClusterAssignments[i];
            if (clusterIndex >= 0 && clusterIndex < m_clusters.size()) {
                float distance = calculateDistance(
                    m_movingEntities[i].qtEntity.position, 
                    m_clusters[clusterIndex].centroid
                );
                totalDistance += distance;
                validEntities++;
            }
        }
        
        float avgDistance = validEntities > 0 ? totalDistance / validEntities : 0.0f;
        std::wstring avgDistanceText = L"Avg Distance: " + std::to_wstring(static_cast<int>(avgDistance * 10) / 10.0f);
        m_kmeansAvgDistanceText->setText(avgDistanceText);
    }
}

// DBSCAN clustering implementation
void PartitionScene::performDBSCANClustering() {
    if (m_movingEntities.empty()) return;
    
    // Clear previous results
    // Keep a copy of previous clusters for stable remapping
    m_prevDbscanClusters = m_dbscanClusters;
    m_dbscanClusters.clear();
    resetDBSCANLabels();
    
    int nextClusterId = 0;
    
    // Process each entity
    for (int i = 0; i < (int)m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        if (m_dbscanEntityLabels[i] != DBSCAN_UNVISITED) continue; // already visited

        // Get neighbors including the point itself
        std::vector<int> neighbors = getNeighbors(i);

        // Core point check: |N_eps(p)| >= MinPts
        if ((int)neighbors.size() < m_dbscanMinPts) {
            m_dbscanEntityLabels[i] = DBSCAN_NOISE; // mark as noise for now (may be upgraded to border)
            continue;
        }

        // Start new cluster
        int clusterId = nextClusterId++;
        DBSCANCluster cluster;
        cluster.clusterId = clusterId;
        cluster.color = getDBSCANClusterColor(clusterId);
        cluster.entityIndices.clear();

        // Expand cluster
        expandCluster(i, clusterId);

        // Collect members for visualization
        for (int j = 0; j < (int)m_movingEntities.size(); j++) {
            if (m_movingEntities[j].active && m_dbscanEntityLabels[j] == clusterId) {
                cluster.entityIndices.push_back(j);
            }
        }

        if (!cluster.entityIndices.empty()) {
            m_dbscanClusters.push_back(cluster);
        }
    }

    // Stable ID remapping and color reuse
    remapDBSCANClusterIdsStable();
    updateDBSCANEntityColors();
    updateQuadtreeVisualization();
}

void PartitionScene::expandCluster(int entityIndex, int clusterId) {
    // BFS over density-reachable set
    std::vector<int> queue = getNeighbors(entityIndex); // includes the point itself
    // Assign the seed point
    m_dbscanEntityLabels[entityIndex] = clusterId;

    for (size_t qi = 0; qi < queue.size(); ++qi) {
        int current = queue[qi];

        // If it was noise, upgrade to border (cluster member)
        if (m_dbscanEntityLabels[current] == DBSCAN_NOISE) {
            m_dbscanEntityLabels[current] = clusterId;
        }

        // If unvisited, assign to cluster
        if (m_dbscanEntityLabels[current] == DBSCAN_UNVISITED) {
            m_dbscanEntityLabels[current] = clusterId;

            // Check if current is a core point
            std::vector<int> currentNeighbors = getNeighbors(current);
            if ((int)currentNeighbors.size() >= m_dbscanMinPts) {
                // Append neighbors to queue for further expansion
                for (int nb : currentNeighbors) {
                    if (std::find(queue.begin(), queue.end(), nb) == queue.end()) {
                        queue.push_back(nb);
                    }
                }
            }
        }
    }
}

std::vector<int> PartitionScene::getNeighbors(int entityIndex) {
    std::vector<int> neighbors;
    
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return neighbors;
    if (!m_movingEntities[entityIndex].active) return neighbors;
    
    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position;
    
    // Include the point itself per the standard definition |N_eps(p)| >= MinPts
    neighbors.push_back(entityIndex);
    
    for (int i = 0; i < (int)m_movingEntities.size(); i++) {
        if (i == entityIndex || !m_movingEntities[i].active) continue;
        
        float distance = calculateDistance(entityPos, m_movingEntities[i].qtEntity.position);
        if (distance <= m_dbscanEps) {
            neighbors.push_back(i);
        }
    }
    
    return neighbors;
}

void PartitionScene::updateDBSCANEntityColors() {
    // Precompute DBSCAN centroids when using Voronoi mode to color noise by nearest partition
    std::vector<Vec2> voronoiCentroids;
    if (m_dbscanEnabled && m_dbscanUseVoronoi && !m_dbscanClusters.empty()) {
        voronoiCentroids.reserve(m_dbscanClusters.size());
        for (const auto& cluster : m_dbscanClusters) {
            Vec2 c(0.0f, 0.0f);
            int count = 0;
            for (int idx : cluster.entityIndices) {
                if (idx >= 0 && idx < (int)m_movingEntities.size() && m_movingEntities[idx].active) {
                    c.x += m_movingEntities[idx].qtEntity.position.x;
                    c.y += m_movingEntities[idx].qtEntity.position.y;
                    ++count;
                }
            }
            if (count > 0) { c.x /= count; c.y /= count; }
            voronoiCentroids.push_back(c);
        }
    }

    // Reset all entities to default color first
    for (auto& movingEntity : m_movingEntities) {
        if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f)); // Default green
            }
        }
    }
    
    // Color entities based on their cluster assignments
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        int label = m_dbscanEntityLabels[i];
        if (label >= 0) {
            // Find the cluster this entity belongs to
            for (const auto& cluster : m_dbscanClusters) {
                if (cluster.clusterId == label) {
                    if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                            sprite->setTint(cluster.color);
                        }
                    }
                    break;
                }
            }
        } else if (label == -1) {
            // Noise points - make them gray
            // In Voronoi mode: color noise by nearest cluster centroid to indicate partition
            if (m_dbscanEnabled && m_dbscanUseVoronoi && !voronoiCentroids.empty()) {
                Vec2 p = m_movingEntities[i].qtEntity.position;
                float best = std::numeric_limits<float>::max();
                int bestIdx = -1;
                for (int ci = 0; ci < (int)voronoiCentroids.size(); ++ci) {
                    float d = calculateDistanceSquared(p, voronoiCentroids[ci]);
                    if (d < best) { best = d; bestIdx = ci; }
                }
                if (bestIdx >= 0) {
                    if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                            sprite->setTint(m_dbscanClusters[bestIdx].color);
                        }
                    }
                    continue;
                }
            }
            // Default gray when not in Voronoi mode
            if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                    sprite->setTint(Vec4(0.5f, 0.5f, 0.5f, 0.8f));
                }
            }
        }
    }
}

Vec4 PartitionScene::getDBSCANClusterColor(int clusterIndex) {
    // Generate distinct colors for DBSCAN clusters
    static std::vector<Vec4> colors = {
        Vec4(1.0f, 0.0f, 0.0f, 0.8f), // Red
        Vec4(0.0f, 1.0f, 0.0f, 0.8f), // Green
        Vec4(0.0f, 0.0f, 1.0f, 0.8f), // Blue
        Vec4(1.0f, 1.0f, 0.0f, 0.8f), // Yellow
        Vec4(1.0f, 0.0f, 1.0f, 0.8f), // Magenta
        Vec4(0.0f, 1.0f, 1.0f, 0.8f), // Cyan
        Vec4(1.0f, 0.5f, 0.0f, 0.8f), // Orange
        Vec4(0.5f, 0.0f, 1.0f, 0.8f), // Purple
        Vec4(0.8f, 0.2f, 0.2f, 0.8f), // Dark Red
        Vec4(0.2f, 0.8f, 0.2f, 0.8f)  // Dark Green
    };
    
    return colors[clusterIndex % colors.size()];
}

void PartitionScene::resetDBSCANLabels() {
    m_dbscanEntityLabels.clear();
    m_dbscanEntityLabels.resize(m_movingEntities.size(), DBSCAN_UNVISITED);
}

void PartitionScene::updateDBSCANButtonVisibility() {
    // Show/hide DBSCAN related buttons based on DBSCAN enabled state
    if (m_increaseEpsButton) {
        m_increaseEpsButton->setVisible(m_dbscanEnabled);
    }
    if (m_decreaseEpsButton) {
        m_decreaseEpsButton->setVisible(m_dbscanEnabled);
    }
    if (m_increaseMinPtsButton) {
        m_increaseMinPtsButton->setVisible(m_dbscanEnabled);
    }
    if (m_decreaseMinPtsButton) {
        m_decreaseMinPtsButton->setVisible(m_dbscanEnabled);
    }
    if (m_dbscanDataPanel) {
        m_dbscanDataPanel->setVisible(m_dbscanEnabled);
    }
    if (m_dbscanEpsText) {
        m_dbscanEpsText->setVisible(m_dbscanEnabled);
    }
    if (m_dbscanMinPtsText) {
        m_dbscanMinPtsText->setVisible(m_dbscanEnabled);
    }
    if (m_dbscanClustersText) {
        m_dbscanClustersText->setVisible(m_dbscanEnabled);
    }
    if (m_dbscanHullVoronoiToggle) {
        m_dbscanHullVoronoiToggle->setVisible(m_dbscanEnabled);
        m_dbscanHullVoronoiToggle->setText(m_dbscanUseVoronoi ? L"DBSCAN: Show Hulls" : L"DBSCAN: Show Voronoi");
    }
}

void PartitionScene::updateDBSCANTestData() {
    if (!m_dbscanEnabled) return;

    // Update Epsilon value
    if (m_dbscanEpsText) {
        std::wstring epsText = L"Eps: " + std::to_wstring(static_cast<int>(m_dbscanEps * 10) / 10.0f);
        m_dbscanEpsText->setText(epsText);
    }

    // Update MinPts value
    if (m_dbscanMinPtsText) {
        std::wstring minPtsText = L"MinPts: " + std::to_wstring(m_dbscanMinPts);
        m_dbscanMinPtsText->setText(minPtsText);
    }

    // Update clusters count
    if (m_dbscanClustersText) {
        std::wstring clustersText = L"Clusters: " + std::to_wstring(m_dbscanClusters.size());
        m_dbscanClustersText->setText(clustersText);
    }
}


// Convex hull (Monotone Chain) utilities
float PartitionScene::cross(const Vec2& o, const Vec2& a, const Vec2& b) {
    float ax = a.x - o.x;
    float ay = a.y - o.y;
    float bx = b.x - o.x;
    float by = b.y - o.y;
    return ax * by - ay * bx;
}

void PartitionScene::updatePartitionStatusUI() {
    if (!m_partitionStatusText) return;
    const wchar_t* name = L"Quadtree";
    if (m_partitionType == PartitionType::AABB) name = L"AABB";
    else if (m_partitionType == PartitionType::KDTree) name = L"KD Tree";
    std::wstring text = L"Partition: ";
    text += name;
    m_partitionStatusText->setText(text);
}

// Voronoi helpers: clip polygon with a half-plane n·x <= d
std::vector<Vec2> PartitionScene::clipPolygonWithHalfPlane(const std::vector<Vec2>& poly, const HalfPlane& hp) {
    std::vector<Vec2> out;
    if (poly.empty()) return out;
    auto inside = [&](const Vec2& p) -> bool { return hp.n.x * p.x + hp.n.y * p.y <= hp.d + 1e-4f; };
    auto intersect = [&](const Vec2& a, const Vec2& b) -> Vec2 {
        // Solve for t where n·(a + t(b-a)) = d
        Vec2 ab = b - a;
        float denom = hp.n.x * ab.x + hp.n.y * ab.y;
        if (std::abs(denom) < 1e-6f) return a; // parallel, return a
        float t = (hp.d - (hp.n.x * a.x + hp.n.y * a.y)) / denom;
        return a + ab * t;
    };

    for (size_t i = 0; i < poly.size(); ++i) {
        Vec2 curr = poly[i];
        Vec2 prev = poly[(i + poly.size() - 1) % poly.size()];
        bool currIn = inside(curr);
        bool prevIn = inside(prev);
        if (currIn) {
            if (!prevIn) out.push_back(intersect(prev, curr));
            out.push_back(curr);
        } else if (prevIn) {
            out.push_back(intersect(prev, curr));
        }
    }
    return out;
}

// Compute bounded Voronoi cell for 'site' given all sites and rectangular bounds
std::vector<Vec2> PartitionScene::computeVoronoiCell(const Vec2& site, const std::vector<Vec2>& allSites, const Vec2& boundsCenter, const Vec2& boundsSize) {
    // Start with rectangle bounds polygon
    Vec2 hs = boundsSize * 0.5f;
    std::vector<Vec2> poly = {
        Vec2(boundsCenter.x - hs.x, boundsCenter.y - hs.y),
        Vec2(boundsCenter.x + hs.x, boundsCenter.y - hs.y),
        Vec2(boundsCenter.x + hs.x, boundsCenter.y + hs.y),
        Vec2(boundsCenter.x - hs.x, boundsCenter.y + hs.y)
    };

    for (const auto& other : allSites) {
        if (other.x == site.x && other.y == site.y) continue;
        // Bisector half-plane: keep points closer to 'site' than 'other'
        Vec2 m = (site + other) * 0.5f; // midpoint
        Vec2 n = other - site;          // normal pointing from site to other
        // Half-plane equation: n·x <= n·m
        HalfPlane hp{ n, n.x * m.x + n.y * m.y };
        poly = clipPolygonWithHalfPlane(poly, hp);
        if (poly.empty()) break;
    }

    return poly;
}

std::vector<Vec2> PartitionScene::computeConvexHull(const std::vector<Vec2>& points) {
    if (points.size() <= 1) return points;

    std::vector<Vec2> pts = points;
    std::sort(pts.begin(), pts.end(), [](const Vec2& p1, const Vec2& p2) {
        if (p1.x == p2.x) return p1.y < p2.y;
        return p1.x < p2.x;
    });

    std::vector<Vec2> lower;
    for (const auto& p : pts) {
        while (lower.size() >= 2 && cross(lower[lower.size()-2], lower.back(), p) <= 0.0f) {
            lower.pop_back();
        }
        lower.push_back(p);
    }

    std::vector<Vec2> upper;
    for (int i = static_cast<int>(pts.size()) - 1; i >= 0; --i) {
        const auto& p = pts[i];
        while (upper.size() >= 2 && cross(upper[upper.size()-2], upper.back(), p) <= 0.0f) {
            upper.pop_back();
        }
        upper.push_back(p);
    }

    // Concatenate lower and upper to get full hull; omit last point of each as it repeats first of the other
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

float PartitionScene::computeClusterIoU(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.empty() && b.empty()) return 1.0f;
    if (a.empty() || b.empty()) return 0.0f;
    std::set<int> sa(a.begin(), a.end());
    std::set<int> sb(b.begin(), b.end());
    size_t inter = 0;
    for (int v : sa) if (sb.count(v)) inter++;
    size_t uni = sa.size();
    for (int v : sb) if (!sa.count(v)) uni++;
    if (uni == 0) return 0.0f;
    return static_cast<float>(inter) / static_cast<float>(uni);
}

void PartitionScene::remapDBSCANClusterIdsStable() {
    if (m_dbscanClusters.empty()) return;

    // Temporary ids and provisional colors
    for (size_t i = 0; i < m_dbscanClusters.size(); ++i) {
        m_dbscanClusters[i].clusterId = static_cast<int>(i);
        if (m_dbscanClusters[i].color.w == 0.0f) {
            m_dbscanClusters[i].color = getDBSCANClusterColor(static_cast<int>(i));
        }
    }

    const float MATCH_THRESHOLD = 0.15f;
    std::vector<int> prevAssigned(m_prevDbscanClusters.size(), 0);
    for (auto& newC : m_dbscanClusters) {
        float best = -1.0f;
        int bestIdx = -1;
        for (size_t j = 0; j < m_prevDbscanClusters.size(); ++j) {
            if (prevAssigned[j]) continue;
            float iou = computeClusterIoU(newC.entityIndices, m_prevDbscanClusters[j].entityIndices);
            if (iou > best) { best = iou; bestIdx = static_cast<int>(j); }
        }
        if (bestIdx >= 0 && best >= MATCH_THRESHOLD) {
            newC.clusterId = m_prevDbscanClusters[bestIdx].clusterId;
            newC.color = m_prevDbscanClusters[bestIdx].color;
            prevAssigned[bestIdx] = 1;
        } else {
            newC.clusterId = m_nextDbscanClusterId++;
            newC.color = getDBSCANClusterColor(newC.clusterId);
        }
    }

    // Rewrite labels to final ids
    for (int i = 0; i < static_cast<int>(m_movingEntities.size()); ++i) {
        if (!m_movingEntities[i].active) continue;
        int label = m_dbscanEntityLabels[i];
        if (label >= 0) {
            for (const auto& c : m_dbscanClusters) {
                if (std::find(c.entityIndices.begin(), c.entityIndices.end(), i) != c.entityIndices.end()) {
                    m_dbscanEntityLabels[i] = c.clusterId;
                    break;
                }
            }
        }
    }
}
