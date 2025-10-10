#include <DX3D/Game/Scenes/PartitionScene.h>
using namespace dx3d;

void PartitionScene::createUIElements(GraphicsDevice& device) {
    // Add entities button
    auto& addButtonEntity = m_entityManager->createEntity("AddEntitiesButton");
    auto& addButton = addButtonEntity.addComponent<ButtonComponent>(
        device, L"Add Entities", 18.0f
    );
    addButton.setScreenPosition(0.1f, 0.1f);
    addButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    addButton.setOnClickCallback([this, &device]() {
        if (m_is3DMode) {
            addRandom3DEntities(device, 5);
        } else {
            addRandomEntities();
        }
        });

    // Clear entities button
    auto& clearButtonEntity = m_entityManager->createEntity("ClearEntitiesButton");
    auto& clearButton = clearButtonEntity.addComponent<ButtonComponent>(
        device, L"Clear All", 18.0f
    );
    clearButton.setScreenPosition(0.1f, 0.15f);
    clearButton.setNormalTint(Vec4(0.8f, 0.2f, 0.2f, 0.8f));
    clearButton.setOnClickCallback([this, &device]() {
        if (m_is3DMode) {
            clearAllEntities3D();
        } else {
            clearAllEntities();
        }
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
    circlesButton.setScreenPosition(0.1f, 0.85f);
    circlesButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    circlesButton.setOnClickCallback([this]() {
        generateConcentricCirclesDataset();
    });

    // Concentric circles (light) dataset button
    auto& circlesLightButtonEntity = m_entityManager->createEntity("CirclesDatasetLightButton");
    auto& circlesLightButton = circlesLightButtonEntity.addComponent<ButtonComponent>(
        device, L"Spawn Circles Dataset (Light)", 18.0f
    );
    circlesLightButton.setScreenPosition(0.1f, 0.9f);
    circlesLightButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    circlesLightButton.setOnClickCallback([this]() {
        generateConcentricCirclesDatasetLight();
    });

    // POI Management Buttons (moved to far right)
    auto& addPOIButtonEntity = m_entityManager->createEntity("AddPOIButton");
    auto& addPOIButton = addPOIButtonEntity.addComponent<ButtonComponent>(
        device, L"Add POI at Mouse", 18.0f
    );
    addPOIButton.setScreenPosition(0.9f, 0.56f);
    addPOIButton.setNormalTint(Vec4(1.0f, 1.0f, 0.0f, 0.8f));
    addPOIButton.setOnClickCallback([this]() {
        m_addPOIMode = true;
    });

    auto& clearPOIsButtonEntity = m_entityManager->createEntity("ClearPOIsButton");
    auto& clearPOIsButton = clearPOIsButtonEntity.addComponent<ButtonComponent>(
        device, L"Clear All POIs", 18.0f
    );
    clearPOIsButton.setScreenPosition(0.9f, 0.60f);
    clearPOIsButton.setNormalTint(Vec4(1.0f, 0.5f, 0.0f, 0.8f));
    clearPOIsButton.setOnClickCallback([this]() {
        clearAllPOIs();
    });

    auto& togglePOISystemButtonEntity = m_entityManager->createEntity("TogglePOISystemButton");
    auto& togglePOISystemButton = togglePOISystemButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle POI System", 18.0f
    );
    togglePOISystemButton.setScreenPosition(0.9f, 0.64f);
    togglePOISystemButton.setNormalTint(Vec4(0.0f, 1.0f, 0.0f, 0.8f));
    togglePOISystemButton.setOnClickCallback([this, &togglePOISystemButton]() {
        m_poiSystemEnabled = !m_poiSystemEnabled;
        if (m_poiSystemEnabled) {
            togglePOISystemButton.setNormalTint(Vec4(0.0f, 1.0f, 0.0f, 0.8f));
        } else {
            togglePOISystemButton.setNormalTint(Vec4(1.0f, 0.0f, 0.0f, 0.8f));
        }
    });

    // POI Strength Controls
    auto& decreasePOIStrengthButtonEntity = m_entityManager->createEntity("DecreasePOIStrengthButton");
    auto& decreasePOIStrengthButton = decreasePOIStrengthButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease POI Strength", 18.0f
    );
    decreasePOIStrengthButton.setScreenPosition(0.9f, 0.68f);
    decreasePOIStrengthButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    decreasePOIStrengthButton.setOnClickCallback([this]() {
        if (m_poiAttractionStrength > 0.1f) {
            m_poiAttractionStrength -= 0.1f;
        }
    });

    auto& increasePOIStrengthButtonEntity = m_entityManager->createEntity("IncreasePOIStrengthButton");
    auto& increasePOIStrengthButton = increasePOIStrengthButtonEntity.addComponent<ButtonComponent>(
        device, L"Increase POI Strength", 18.0f
    );
    increasePOIStrengthButton.setScreenPosition(0.9f, 0.72f);
    increasePOIStrengthButton.setNormalTint(Vec4(0.6f, 0.6f, 0.2f, 0.8f));
    increasePOIStrengthButton.setOnClickCallback([this]() {
        if (m_poiAttractionStrength < 5.0f) {
            m_poiAttractionStrength += 0.1f;
        }
    });

    // Entity Speed Controls
    auto& decreaseSpeedButtonEntity = m_entityManager->createEntity("DecreaseSpeedButton");
    auto& decreaseSpeedButton = decreaseSpeedButtonEntity.addComponent<ButtonComponent>(
        device, L"Decrease Speed", 18.0f
    );
    decreaseSpeedButton.setScreenPosition(0.9f, 0.76f);
    decreaseSpeedButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    decreaseSpeedButton.setOnClickCallback([this]() {
        if (m_entitySpeedMultiplier > 0.1f) {
            m_entitySpeedMultiplier -= 0.1f;
        }
    });

    auto& increaseSpeedButtonEntity = m_entityManager->createEntity("IncreaseSpeedButton");
    auto& increaseSpeedButton = increaseSpeedButtonEntity.addComponent<ButtonComponent>(
        device, L"Increase Speed", 18.0f
    );
    increaseSpeedButton.setScreenPosition(0.9f, 0.80f);
    increaseSpeedButton.setNormalTint(Vec4(0.2f, 0.6f, 1.0f, 0.8f));
    increaseSpeedButton.setOnClickCallback([this]() {
        if (m_entitySpeedMultiplier < 3.0f) {
            m_entitySpeedMultiplier += 0.1f;
        }
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
        if (m_fastMode) {
            fastModeButton.setNormalTint(Vec4(0.8f, 0.2f, 0.2f, 0.8f));
        } else {
            fastModeButton.setNormalTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f));
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
        if (m_kmeansEnabled) {
            m_kmeansEnabled = false;
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
            performDBSCANClustering();
        } else {
            m_quadtreeVisualOffset = m_quadtreeVisualOffsetOriginal;
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
    partPanel.setScreenPosition(0.85f, 0.06f);
    partPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_partitionStatusPanel = &partPanel;

    auto& partStatusTextEntity = m_entityManager->createEntity("PartitionStatusText");
    m_partitionStatusText = &partStatusTextEntity.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"Partition: Quadtree",
        16.0f
    );
    m_partitionStatusText->setScreenPosition(0.9, 0.18f);
    m_partitionStatusText->setColor(Vec4(1,1,1,1));

    auto& btnOctEntity = m_entityManager->createEntity("BtnQuadtree");
    auto& btnOct = btnOctEntity.addComponent<ButtonComponent>(device, L"Quadtree", 18.0f);
    btnOct.setScreenPosition(0.9f, 0.06f);
    btnOct.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnOct.setOnClickCallback([this]() {
        m_partitionType = PartitionType::Quadtree;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });

    auto& btnAabbEntity = m_entityManager->createEntity("BtnAABB");
    auto& btnAabb = btnAabbEntity.addComponent<ButtonComponent>(device, L"AABB", 18.0f);
    btnAabb.setScreenPosition(0.9f, 0.10f);
    btnAabb.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnAabb.setOnClickCallback([this]() {
        m_partitionType = PartitionType::AABB;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });

    auto& btnKdEntity = m_entityManager->createEntity("BtnKD");
    auto& btnKd = btnKdEntity.addComponent<ButtonComponent>(device, L"KD Tree", 18.0f);
    btnKd.setScreenPosition(0.9f, 0.14f);
    btnKd.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnKd.setOnClickCallback([this]() {
        m_partitionType = PartitionType::KDTree;
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });

    // Octree (3D mode alternative)
    auto& btnOctreeEntity = m_entityManager->createEntity("BtnOctree");
    auto& btnOctree = btnOctreeEntity.addComponent<ButtonComponent>(device, L"Octree", 18.0f);
    btnOctree.setScreenPosition(0.9f, 0.10f);
    btnOctree.setNormalTint(Vec4(0.6f,0.6f,0.2f,0.8f));
    btnOctree.setOnClickCallback([this]() {
        // Use Octree partition when in 3D
        m_partitionType = PartitionType::KDTree; // If Octree path exists, change this accordingly
        updateQuadtreePartitioning();
        updatePartitionStatusUI();
    });
    btnOctree.setVisible(false); // Toggled by updateUIForMode()

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

    // Entity count display panel
    auto& entityCountPanelEntity = m_entityManager->createEntity("EntityCountPanel");
    auto& entityCountPanel = entityCountPanelEntity.addComponent<PanelComponent>(
        device, 
        0.25f * GraphicsEngine::getWindowWidth(),
        0.15f * GraphicsEngine::getWindowHeight()
    );
    entityCountPanel.setScreenPosition(0.9f, 0.26f);
    entityCountPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));

    auto& entityCountTextEntity = m_entityManager->createEntity("EntityCountText");
    m_entityCountText = &entityCountTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Entities: 0", 
        20.0f
    );
    m_entityCountText->setScreenPosition(0.9f, 0.28f);
    m_entityCountText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // K-means test data panel
    auto& kmeansDataPanelEntity = m_entityManager->createEntity("KMeansDataPanel");
    auto& kmeansDataPanel = kmeansDataPanelEntity.addComponent<PanelComponent>(
        device, 
        0.3f * GraphicsEngine::getWindowWidth(),
        0.25f * GraphicsEngine::getWindowHeight()
    );
    kmeansDataPanel.setScreenPosition(0.9f, 0.3f);
    kmeansDataPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_kmeansDataPanel = &kmeansDataPanel;

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
    dbscanDataPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_dbscanDataPanel = &dbscanDataPanel;

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
    offsetPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));

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

    // POI Status Panel
    auto& poiStatusPanelEntity = m_entityManager->createEntity("POIStatusPanel");
    auto& poiStatusPanel = poiStatusPanelEntity.addComponent<PanelComponent>(
        device, 
        0.25f * GraphicsEngine::getWindowWidth(),
        0.12f * GraphicsEngine::getWindowHeight()
    );
    poiStatusPanel.setScreenPosition(0.9f, 0.85f);
    poiStatusPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.7f));
    m_poiStatusPanel = &poiStatusPanel;

    auto& poiStatusTextEntity = m_entityManager->createEntity("POIStatusText");
    m_poiStatusText = &poiStatusTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"POI System: Enabled", 
        16.0f
    );
    m_poiStatusText->setScreenPosition(0.9f, 0.87f);
    m_poiStatusText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& poiCountTextEntity = m_entityManager->createEntity("POICountText");
    m_poiCountText = &poiCountTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"POIs: 0", 
        16.0f
    );
    m_poiCountText->setScreenPosition(0.9f, 0.91f);
    m_poiCountText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& poiStrengthTextEntity = m_entityManager->createEntity("POIStrengthText");
    m_poiStrengthText = &poiStrengthTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"POI Strength: 0.3", 
        16.0f
    );
    m_poiStrengthText->setScreenPosition(0.9f, 0.95f);
    m_poiStrengthText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto& entitySpeedTextEntity = m_entityManager->createEntity("EntitySpeedText");
    m_entitySpeedText = &entitySpeedTextEntity.addComponent<TextComponent>(
        device, 
        TextSystem::getRenderer(),
        L"Entity Speed: 3.0x", 
        16.0f
    );
    m_entitySpeedText->setScreenPosition(0.9f, 0.99f);
    m_entitySpeedText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Create speed controls
    createSpeedControls(device);

    // 3D Mode toggle button
    auto& toggle3DModeButtonEntity = m_entityManager->createEntity("Toggle3DModeButton");
    auto& toggle3DModeButton = toggle3DModeButtonEntity.addComponent<ButtonComponent>(
        device, L"Toggle 3D Mode", 18.0f
    );
    toggle3DModeButton.setScreenPosition(0.1f, 0.95f);
    toggle3DModeButton.setNormalTint(Vec4(0.8f, 0.2f, 0.8f, 0.8f));
    toggle3DModeButton.setOnClickCallback([this]() {
        toggle3DMode();
    });
    m_toggle3DModeButton = &toggle3DModeButton;

    updateKMeansButtonVisibility();
    updateDBSCANButtonVisibility();
    updateHullVoronoiToggleVisibility();
    updateUIForMode();
}

void PartitionScene::createSpeedControls(GraphicsDevice& device) {
    auto& speedPanelEntity = m_entityManager->createEntity("SpeedControlPanel");
    auto& speedPanel = speedPanelEntity.addComponent<PanelComponent>(
        device,
        0.4f * GraphicsEngine::getWindowWidth(),
        0.12f * GraphicsEngine::getWindowHeight()
    );
    speedPanel.setScreenPosition(0.3f, 0.02f);
    speedPanel.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.8f));
    m_speedControlPanel = &speedPanel;

    auto& pauseButtonEntity = m_entityManager->createEntity("PauseButton");
    auto& pauseButton = pauseButtonEntity.addComponent<ButtonComponent>(
        device, L"||", 24.0f
    );
    pauseButton.setScreenPosition(0.49f, 0.95f);
    pauseButton.setNormalTint(Vec4(0.8f, 0.2f, 0.2f, 0.9f));
    pauseButton.setOnClickCallback([this]() {
        setSimulationSpeed(dx3d::SimulationSpeed::Paused);
    });
    m_pauseButton = &pauseButton;

    auto& playButtonEntity = m_entityManager->createEntity("PlayButton");
    auto& playButton = playButtonEntity.addComponent<ButtonComponent>(
        device, L">", 24.0f
    );
    playButton.setScreenPosition(0.515f, 0.95f);
    playButton.setNormalTint(Vec4(0.2f, 0.8f, 0.2f, 0.9f));
    playButton.setOnClickCallback([this]() {
        setSimulationSpeed(dx3d::SimulationSpeed::Normal);
    });
    m_playButton = &playButton;

    auto& fastForwardButtonEntity = m_entityManager->createEntity("FastForwardButton");
    auto& fastForwardButton = fastForwardButtonEntity.addComponent<ButtonComponent>(
        device, L">>", 24.0f
    );
    fastForwardButton.setScreenPosition(0.545f, 0.95f);
    fastForwardButton.setNormalTint(Vec4(0.2f, 0.2f, 0.8f, 0.9f));
    fastForwardButton.setOnClickCallback([this]() {
        if (m_simulationSpeed == dx3d::SimulationSpeed::Fast) {
            setSimulationSpeed(dx3d::SimulationSpeed::VeryFast);
        } else {
            setSimulationSpeed(dx3d::SimulationSpeed::Fast);
        }
    });
    m_fastForwardButton = &fastForwardButton;

    auto& speedTextEntity = m_entityManager->createEntity("SpeedIndicatorText");
    m_speedIndicatorText = &speedTextEntity.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"Speed: Paused",
        18.0f
    );
    m_speedIndicatorText->setScreenPosition(0.5f, 0.90f);
    m_speedIndicatorText->setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void PartitionScene::updateKMeansButtonVisibility() {
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

void PartitionScene::updateKMeansTestData() {
    if (!m_kmeansEnabled) return;
    if (m_kmeansKText) {
        std::wstring kText = L"K: " + std::to_wstring(m_kmeansK);
        m_kmeansKText->setText(kText);
    }
    if (m_kmeansIterationsText) {
        std::wstring iterationsText = L"Iterations: " + std::to_wstring(m_kmeansIterations);
        m_kmeansIterationsText->setText(iterationsText);
    }
    if (m_kmeansConvergedText) {
        std::wstring convergedText = m_kmeansConverged ? L"Converged: Yes" : L"Converged: No";
        m_kmeansConvergedText->setText(convergedText);
    }
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

void PartitionScene::updateDBSCANButtonVisibility() {
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
    if (m_dbscanEpsText) {
        std::wstring epsText = L"Eps: " + std::to_wstring(static_cast<int>(m_dbscanEps * 10) / 10.0f);
        m_dbscanEpsText->setText(epsText);
    }
    if (m_dbscanMinPtsText) {
        std::wstring minPtsText = L"MinPts: " + std::to_wstring(m_dbscanMinPts);
        m_dbscanMinPtsText->setText(minPtsText);
    }
    if (m_dbscanClustersText) {
        std::wstring clustersText = L"Clusters: " + std::to_wstring(m_dbscanClusters.size());
        m_dbscanClustersText->setText(clustersText);
    }
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

void PartitionScene::updateUIForMode() {
    // Hide analytics/dataset/POI UI in 3D; simplify partition buttons
    const bool is3D = m_is3DMode;

    // Dataset buttons
    if (auto* e = m_entityManager->findEntity("CirclesDatasetButton")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(!is3D);
    }
    if (auto* e = m_entityManager->findEntity("CirclesDatasetLightButton")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(!is3D);
    }

    // K-Means
    if (auto* e = m_entityManager->findEntity("KMeansButton")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(!is3D);
    }
    if (m_increaseKButton) m_increaseKButton->setVisible(!is3D && m_kmeansEnabled);
    if (m_decreaseKButton) m_decreaseKButton->setVisible(!is3D && m_kmeansEnabled);
    if (m_kmeansDataPanel) m_kmeansDataPanel->setVisible(!is3D && m_kmeansEnabled);
    if (m_kmeansHullVoronoiToggle) m_kmeansHullVoronoiToggle->setVisible(!is3D && m_kmeansEnabled);

    // DBSCAN
    if (auto* e = m_entityManager->findEntity("DBSCANButton")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(!is3D);
    }
    if (m_dbscanDataPanel) m_dbscanDataPanel->setVisible(!is3D && m_dbscanEnabled);
    if (m_increaseEpsButton) m_increaseEpsButton->setVisible(!is3D && m_dbscanEnabled);
    if (m_decreaseEpsButton) m_decreaseEpsButton->setVisible(!is3D && m_dbscanEnabled);
    if (m_increaseMinPtsButton) m_increaseMinPtsButton->setVisible(!is3D && m_dbscanEnabled);
    if (m_decreaseMinPtsButton) m_decreaseMinPtsButton->setVisible(!is3D && m_dbscanEnabled);
    if (m_dbscanHullVoronoiToggle) m_dbscanHullVoronoiToggle->setVisible(!is3D && m_dbscanEnabled);

    // POI controls and status (right side)
    const char* poiButtons[] = { "AddPOIButton", "ClearPOIsButton", "TogglePOISystemButton",
        "DecreasePOIStrengthButton", "IncreasePOIStrengthButton" };
    for (const char* name : poiButtons) {
        if (auto* e = m_entityManager->findEntity(name)) {
            if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(!is3D);
        }
    }
    if (m_poiStatusPanel) m_poiStatusPanel->setVisible(!is3D);
    if (m_poiStatusText) m_poiStatusText->setVisible(!is3D);
    if (m_poiCountText) m_poiCountText->setVisible(!is3D);
    if (m_poiStrengthText) m_poiStrengthText->setVisible(!is3D);

    // Partition buttons: in 3D show only KD Tree and Octree; hide AABB/Quadtree button labels
    if (auto* e = m_entityManager->findEntity("BtnQuadtree")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(is3D ? false : true);
    }
    if (auto* e = m_entityManager->findEntity("BtnAABB")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(is3D ? false : true);
    }
    if (auto* e = m_entityManager->findEntity("BtnKD")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(true);
    }
    // Reuse Quadtree button spot for Octree when in 3D if we have one
    if (auto* e = m_entityManager->findEntity("BtnOctree")) {
        if (auto* b = e->getComponent<ButtonComponent>()) b->setVisible(is3D);
    }

    // Update partition status text to reflect available options
    updatePartitionStatusUI();
}


