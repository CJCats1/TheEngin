#include <DX3D/Game/Scenes/PhysicsTetrisScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <DX3D/Core/Input.h>
#include <iostream>
#include <set>
#include <iomanip>
#include <sstream>
#include <imgui.h>

using namespace dx3d;
void PhysicsTetrisScene::createTetriminoFrame(const std::string& baseName, Vec2 position, int tetriminoId) {
    std::string frameName = baseName + "_Frame";
    auto& frameEntity = m_entityManager->createEntity(frameName);
    frameEntity.addComponent<FrameComponent>(position, 0.0f);
}
void PhysicsTetrisScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();


    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->setPosition(0.0f, 0.0f); 



    // Initialize random number generation
    m_randomGen.seed(m_randomDevice());
    m_tetriminoDist = std::uniform_int_distribution<>(0, 6); // 7 tetrimino types

    // Setup camera
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(DEFAULT_CAMERA_ZOOM);
    
    // Engine UI replaced by ImGui; no engine-side buttons/panels are created
    
    // Initialize game systems
    initializeTetriminoTemplates();
    createPlayField();
    // createBoundaryWalls(); // Removed - no more boundary wall nodes

    // Spawn first tetrimino
    spawnNextTetrimino();
}

Vec2 PhysicsTetrisScene::calculateCenterOfMass(const TetriminoData& data) {
    Vec2 centerOfMass(0.0f, 0.0f);
    for (const auto& offset : data.nodeOffsets) {
        centerOfMass += offset;
    }
    centerOfMass *= (1.0f / data.nodeOffsets.size());
    return centerOfMass;
}

void PhysicsTetrisScene::initializeTetriminoTemplates() {
    m_tetriminoTemplates.clear();

    // Helper lambda to center a piece's offsets
    auto centerOffsets = [](TetriminoData& data) {
        Vec2 centerOfMass(0.0f, 0.0f);
        if (data.nodeOffsets.empty()) return;
        for (const auto& offset : data.nodeOffsets) {
            centerOfMass += offset;
        }
        centerOfMass *= (1.0f / data.nodeOffsets.size());

        for (auto& offset : data.nodeOffsets) {
            offset -= centerOfMass;
        }
        };

    // I-piece (Already centered)
    TetriminoData iPiece;
    iPiece.name = "I";
    iPiece.color = Vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
    iPiece.nodeOffsets = {
        Vec2(-45.0f, 0.0f), Vec2(-15.0f, 0.0f), Vec2(15.0f, 0.0f), Vec2(45.0f, 0.0f)
    };
    iPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3} };
    centerOffsets(iPiece); // This won't change it, but it's good practice
    m_tetriminoTemplates.push_back(iPiece);

    // O-piece (Already centered)
    TetriminoData oPiece;
    oPiece.name = "O";
    oPiece.color = Vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    oPiece.nodeOffsets = {
        Vec2(-15.0f, -15.0f), Vec2(15.0f, -15.0f), Vec2(15.0f, 15.0f), Vec2(-15.0f, 15.0f)
    };
    oPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 2}, {1, 3} };
    centerOffsets(oPiece); // This won't change it either
    m_tetriminoTemplates.push_back(oPiece);

    // T-piece (Center of mass was (0, -10))
    TetriminoData tPiece;
    tPiece.name = "T";
    tPiece.color = Vec4(0.5f, 0.0f, 0.5f, 1.0f); // Purple
    tPiece.nodeOffsets = {
        Vec2(0.0f, 40.0f),    // was (0, -40), now (0, -30) -> Center is (0, -10). Let's use a standard grid layout instead.
        Vec2(-20.0f, 20.0f),  // Top Left
        Vec2(20.0f, 20.0f),   // Top Right
        Vec2(0.0f, -20.0f),   // Bottom Middle
        Vec2(0.0f, 20.0f)     // Center of the horizontal bar
    };
    // Original: (0,-40), (-40,0), (0,0), (40,0). CoM = (0,-10).
    tPiece.nodeOffsets = {
        Vec2(0.0f, -22.5f),   // Becomes (0, -40) - (0, -10) * 0.75 scale
        Vec2(-30.0f, 7.5f),   // Becomes (-40, 0) - (0, -10) * 0.75 scale
        Vec2(0.0f, 7.5f),     // Becomes (0, 0) - (0, -10) * 0.75 scale
        Vec2(30.0f, 7.5f)     // Becomes (40, 0) - (0, -10) * 0.75 scale
    };
    tPiece.beamConnections = { {0, 2}, {1, 2}, {2, 3} };
    centerOffsets(tPiece); 
    m_tetriminoTemplates.push_back(tPiece);

    // S-piece (Center of mass was (20, 0))
    TetriminoData sPiece;
    sPiece.name = "S";
    sPiece.color = Vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    sPiece.nodeOffsets = {
        Vec2(-15.0f, -15.0f), Vec2(15.0f, -15.0f), Vec2(15.0f, 15.0f), Vec2(45.0f, 15.0f)
    };
    sPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3} };
    centerOffsets(sPiece);
    m_tetriminoTemplates.push_back(sPiece);

    // Z-piece (Center of mass was (-20, 0))
    TetriminoData zPiece;
    zPiece.name = "Z";
    zPiece.color = Vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    zPiece.nodeOffsets = {
        Vec2(15.0f, -15.0f), Vec2(-15.0f, -15.0f), Vec2(-15.0f, 15.0f), Vec2(-45.0f, 15.0f)
    };
    zPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3} };
    centerOffsets(zPiece);
    m_tetriminoTemplates.push_back(zPiece);

    TetriminoData jPiece;
    jPiece.name = "J";
    jPiece.color = Vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    // Original: (-40,-40), (-40,0), (0,0), (40,0). CoM = (-10, -10)
    jPiece.nodeOffsets = {
        Vec2(-30.0f, -30.0f), Vec2(-30.0f, 0.0f), Vec2(0.0f, 0.0f), Vec2(30.0f, 0.0f)
    };
    jPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3} };
    centerOffsets(jPiece);
    m_tetriminoTemplates.push_back(jPiece);

    // L-piece (Center of mass was (10, -10))
    TetriminoData lPiece;
    lPiece.name = "L";
    lPiece.color = Vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    lPiece.nodeOffsets = {
        Vec2(30.0f, -30.0f), Vec2(30.0f, 0.0f), Vec2(0.0f, 0.0f), Vec2(-30.0f, 0.0f)
    };
    lPiece.beamConnections = { {0, 1}, {1, 2}, {2, 3} };
    centerOffsets(lPiece);
    m_tetriminoTemplates.push_back(lPiece);
}
void PhysicsTetrisScene::renderFrameDebug(DeviceContext& ctx) {
    
    m_lineRenderer->updateBuffer();
    m_lineRenderer->draw(ctx);
}
void PhysicsTetrisScene::createPlayField() {
    auto& device = *m_graphicsDevice;

    // Left boundary marker
    auto& leftMarker = m_entityManager->createEntity("LeftBoundaryMarker");
    auto& leftSprite = leftMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        WALL_THICKNESS, PLAY_FIELD_HEIGHT
    );
    leftSprite.setPosition(-PLAY_FIELD_WIDTH / 2 - WALL_THICKNESS / 2, 0.0f, 0.0f);
    leftSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Right boundary marker  
    auto& rightMarker = m_entityManager->createEntity("RightBoundaryMarker");
    auto& rightSprite = rightMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        WALL_THICKNESS, PLAY_FIELD_HEIGHT
    );
    rightSprite.setPosition(PLAY_FIELD_WIDTH / 2 + WALL_THICKNESS / 2, 0.0f, 0.0f);
    rightSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Bottom boundary marker
    auto& bottomMarker = m_entityManager->createEntity("BottomBoundaryMarker");
    auto& bottomSprite = bottomMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        PLAY_FIELD_WIDTH + 2 * WALL_THICKNESS, WALL_THICKNESS
    );
    bottomSprite.setPosition(0.0f, -PLAY_FIELD_HEIGHT / 2 - WALL_THICKNESS / 2, 0.0f);
    bottomSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));
}

void PhysicsTetrisScene::createBoundaryWalls() {
    // Create invisible physics nodes that act as walls for collision
    // Left wall nodes
    for (int i = 0; i <= GRID_HEIGHT; i++) {
        float y = -PLAY_FIELD_HEIGHT / 2 + i * CELL_SIZE;
        std::string nodeName = "LeftWall_" + std::to_string(i);

        auto& wallNode = m_entityManager->createEntity(nodeName);
        wallNode.addComponent<NodeComponent>(Vec2(-PLAY_FIELD_WIDTH / 2 - NODE_SIZE, y), true);

        // Add invisible sprite for collision detection
        auto& sprite = wallNode.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/node.png",
            NODE_SIZE, NODE_SIZE
        );
        sprite.setPosition(-PLAY_FIELD_WIDTH / 2 - NODE_SIZE, y, 0.0f);
        sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Invisible
    }

    // Right wall nodes
    for (int i = 0; i <= GRID_HEIGHT; i++) {
        float y = -PLAY_FIELD_HEIGHT / 2 + i * CELL_SIZE;
        std::string nodeName = "RightWall_" + std::to_string(i);

        auto& wallNode = m_entityManager->createEntity(nodeName);
        wallNode.addComponent<NodeComponent>(Vec2(PLAY_FIELD_WIDTH / 2 + NODE_SIZE, y), true);

        // Add invisible sprite for collision detection
        auto& sprite = wallNode.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/node.png",
            NODE_SIZE, NODE_SIZE
        );
        sprite.setPosition(PLAY_FIELD_WIDTH / 2 + NODE_SIZE, y, 0.0f);
        sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Invisible
    }

    // Bottom wall nodes
    for (int i = 0; i <= GRID_WIDTH; i++) {
        float x = -PLAY_FIELD_WIDTH / 2 + i * CELL_SIZE;
        std::string nodeName = "BottomWall_" + std::to_string(i);

        auto& wallNode = m_entityManager->createEntity(nodeName);
        wallNode.addComponent<NodeComponent>(Vec2(x, -PLAY_FIELD_HEIGHT / 2 - NODE_SIZE), true);

        // Add invisible sprite for collision detection
        auto& sprite = wallNode.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/node.png",
            NODE_SIZE, NODE_SIZE
        );
        sprite.setPosition(x, -PLAY_FIELD_HEIGHT / 2 - NODE_SIZE, 0.0f);
        sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Invisible
    }
}

void PhysicsTetrisScene::createDebugToggleButton() {
    auto& debugButtonEntity = m_entityManager->createEntity("DebugToggleButton");
    
    // Create button with screen space positioning
    auto& debugButton = debugButtonEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Toggle Debug",
        DEBUG_BUTTON_FONT_SIZE,
        DEBUG_BUTTON_PADDING_X,
        DEBUG_BUTTON_PADDING_Y
    );
    
    // Position in top-left corner of screen
    debugButton.enableScreenSpace(true);
    debugButton.setScreenPosition(DEBUG_BUTTON_SCREEN_X, DEBUG_BUTTON_SCREEN_Y);
    
    // Set button styling
    debugButton.setNormalTint(Vec4(0.2f, 0.2f, 0.2f, 0.8f));   // Dark background
    debugButton.setHoveredTint(Vec4(0.3f, 0.3f, 0.3f, 0.9f));  // Lighter on hover
    debugButton.setPressedTint(Vec4(0.1f, 0.1f, 0.1f, 0.9f));  // Darker when pressed
    debugButton.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));    // White text
    
    // Set callback to toggle debug rendering
    debugButton.setOnClickCallback([this]() {
        m_showFrameDebug = !m_showFrameDebug;
        
        // Update button text to reflect current state
        auto* buttonEntity = m_entityManager->findEntity("DebugToggleButton");
        if (buttonEntity) {
            auto* button = buttonEntity->getComponent<ButtonComponent>();
            if (button) {
                button->setText(m_showFrameDebug ? L"Debug: ON" : L"Debug: OFF");
            }
        }
    });
    
    // Set initial text based on current debug state
    debugButton.setText(m_showFrameDebug ? L"Debug: ON" : L"Debug: OFF");
}

void PhysicsTetrisScene::createGameOverPanel() {
    auto& gameOverPanelEntity = m_entityManager->createEntity("GameOverPanel");
    
    // Create a large panel in the center of the screen
    auto& gameOverPanel = gameOverPanelEntity.addComponent<PanelComponent>(
        *m_graphicsDevice,
        GAME_OVER_PANEL_WIDTH,
        GAME_OVER_PANEL_HEIGHT,
        L"GAME OVER\n\nPress R to Restart",
        GAME_OVER_FONT_SIZE
    );
    
    // Position in center of screen
    gameOverPanel.setScreenPosition(GAME_OVER_SCREEN_X, GAME_OVER_SCREEN_Y);
    
    // Set panel styling
    gameOverPanel.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.9f));      // Dark semi-transparent background
    
    // Initially hidden (game starts playing)
    gameOverPanel.setVisible(false);
}

void PhysicsTetrisScene::spawnTetrimino(TetriminoType type, Vec2 position) {
    if (static_cast<int>(type) >= m_tetriminoTemplates.size()) return;

    const auto& data = m_tetriminoTemplates[static_cast<int>(type)];
    std::string baseName = createTetriminoNodes(data, position, m_nextTetriminoId);
    createTetriminoBeams(data, baseName, m_nextTetriminoId);
    createTetriminoFrame(baseName, position, m_nextTetriminoId);

    // DUCT TAPE FIX: Remove previous world origin anchor and create new one
    if (auto* oldAnchor = m_entityManager->findEntity("WorldOriginAnchor")) {
        m_entityManager->removeEntity("WorldOriginAnchor");
    }
    
    // Create invisible sprite at world origin to "anchor" the coordinate system
    auto& anchorEntity = m_entityManager->createEntity("WorldOriginAnchor");
    auto& anchorSprite = anchorEntity.addComponent<SpriteComponent>(
        *m_graphicsDevice,
        L"DX3D/Assets/Textures/node.png",
        1.0f, 1.0f  // Tiny size
    );
    anchorSprite.setPosition(0.0f, 0.0f, 0.0f); // World origin
    anchorSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Completely transparent

    m_nextTetriminoId++;
}

std::string PhysicsTetrisScene::createTetriminoNodes(const TetriminoData& data, Vec2 basePosition, int tetriminoId) {
    std::string baseName = data.name + "_" + std::to_string(tetriminoId);

    for (size_t i = 0; i < data.nodeOffsets.size(); ++i) {
        Vec2 nodePos = basePosition + data.nodeOffsets[i];
        std::string nodeName = baseName + "_Node" + std::to_string(i);

        auto& nodeEntity = m_entityManager->createEntity(nodeName);

        // Add physics component
        nodeEntity.addComponent<NodeComponent>(nodePos, false);

        auto& sprite = nodeEntity.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/node.png",
            NODE_SIZE, NODE_SIZE
        );
        sprite.setPosition(nodePos.x, nodePos.y, 0.0f);
        sprite.setTint(data.color);
    }

    return baseName;
}

void PhysicsTetrisScene::createTetriminoBeams(const TetriminoData& data, const std::string& baseName, int tetriminoId) {
    for (size_t i = 0; i < data.beamConnections.size(); ++i) {
        int node1Idx = data.beamConnections[i].first;
        int node2Idx = data.beamConnections[i].second;

        std::string node1Name = baseName + "_Node" + std::to_string(node1Idx);
        std::string node2Name = baseName + "_Node" + std::to_string(node2Idx);
        std::string beamName = baseName + "_Beam" + std::to_string(i);

        auto* node1Entity = m_entityManager->findEntity(node1Name);
        auto* node2Entity = m_entityManager->findEntity(node2Name);

        if (!node1Entity || !node2Entity) continue;

        auto& beamEntity = m_entityManager->createEntity(beamName);

        auto& beamComponent = beamEntity.addComponent<BeamComponent>(node1Entity, node2Entity);

        auto& sprite = beamEntity.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/beam.png",
            1.0f, 1.0f
        );

        Vec2 center = beamComponent.getCenterPosition();
        sprite.setPosition(center.x, center.y, 0.0f);
        sprite.setTint(data.color);
    }
}

void PhysicsTetrisScene::spawnNextTetrimino() {
    TetriminoType type = static_cast<TetriminoType>(m_tetriminoDist(m_randomGen));
    spawnTetrimino(type, Vec2(0, 300)); 
    m_currentTetriminoId = m_nextTetriminoId - 1; 
    m_hasActiveTetrimino = true;
}

void PhysicsTetrisScene::update(float dt) {
    updateCameraMovement(dt);
    handleTetriminoInput(dt);
    
    

    // Check for game over condition
    if (!m_gameOver) {
        checkGameOverCondition();
    }
    
    // Handle tetrimino spawning only when no active piece and game not over
    if (!m_hasActiveTetrimino && !m_gameOver) {
        m_spawnTimer += dt;
        if (m_spawnTimer >= m_spawnInterval) {
            spawnNextTetrimino();
            m_spawnTimer = 0.0f;
        }
    }
    
    // Handle game over state
    if (m_gameOver) {
        handleGameOver();
    }

    // Check if current tetrimino has landed
    if (m_hasActiveTetrimino) {
        checkTetriminoLanded();
    }

    // Check for completed lines after pieces have settled
    checkAndClearLines();

    updateCollisions();
    updateUI();

    // Update frame debug visualization
    updateFrameDebugVisualization();
}

void PhysicsTetrisScene::updateCollisions() {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();

    // Node-to-node collisions 
    for (size_t i = 0; i < nodeEntities.size(); ++i) {
        for (size_t j = i + 1; j < nodeEntities.size(); ++j) {
            auto* node1 = nodeEntities[i]->getComponent<NodeComponent>();
            auto* node2 = nodeEntities[j]->getComponent<NodeComponent>();

            if (!node1 || !node2) continue;

            Vec2 pos1 = node1->getPosition();
            Vec2 pos2 = node2->getPosition();

            if (checkAABBCollision(pos1, NODE_SIZE, pos2, NODE_SIZE)) {
                resolveNodeCollision(nodeEntities[i], nodeEntities[j]);
            }
        }
    }

    // Node-to-beam collisions
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node) continue;

        for (auto* beamEntity : beamEntities) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (!beam) continue;

            // Don't check collision between node and its own beams
            if (isNodePartOfBeam(nodeEntity, beamEntity)) continue;

            if (checkNodeBeamCollision(nodeEntity, beamEntity)) {
                resolveNodeBeamCollision(nodeEntity, beamEntity);
            }
        }
    }
}
bool PhysicsTetrisScene::checkNodeBeamCollision(Entity* nodeEntity, Entity* beamEntity) {
    auto* node = nodeEntity->getComponent<NodeComponent>();
    auto* beam = beamEntity->getComponent<BeamComponent>();

    if (!node || !beam) return false;

    Vec2 nodePos = node->getPosition();
    float nodeRadius = NODE_SIZE * 0.5f;

    // Get beam endpoints
    auto* node1 = beam->getNode1()->getComponent<NodeComponent>();
    auto* node2 = beam->getNode2()->getComponent<NodeComponent>();

    if (!node1 || !node2) return false;

    Vec2 beamStart = node1->getPosition();
    Vec2 beamEnd = node2->getPosition();

    // Calculate distance from point to line segment
    float distance = distancePointToLineSegment(nodePos, beamStart, beamEnd);

    // Add some thickness to the beam for collision detection
    float beamThickness = 8.0f; // Adjust this value as needed

    return distance < (nodeRadius + beamThickness);
}
float PhysicsTetrisScene::distancePointToLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd) {
    Vec2 lineVec = lineEnd - lineStart;
    Vec2 pointVec = point - lineStart;

    float lineLength = lineVec.length();
    if (lineLength < 0.001f) {
        // Line is essentially a point
        return pointVec.length();
    }

    // Project point onto line
    float t = pointVec.dot(lineVec) / (lineLength * lineLength);

    // Clamp t to [0,1] to stay on the line segment
    t = std::max(0.0f, std::min(1.0f, t));

    // Find closest point on line segment
    Vec2 closestPoint = lineStart + lineVec * t;

    return (point - closestPoint).length();
}
void PhysicsTetrisScene::resolveNodeBeamCollision(Entity* nodeEntity, Entity* beamEntity) {
    auto* node = nodeEntity->getComponent<NodeComponent>();
    auto* beam = beamEntity->getComponent<BeamComponent>();

    if (!node || !beam || node->isPositionFixed()) return;

    Vec2 nodePos = node->getPosition();
    float nodeRadius = NODE_SIZE * 0.5f;

    // Get beam endpoints
    auto* node1 = beam->getNode1()->getComponent<NodeComponent>();
    auto* node2 = beam->getNode2()->getComponent<NodeComponent>();

    if (!node1 || !node2) return;

    Vec2 beamStart = node1->getPosition();
    Vec2 beamEnd = node2->getPosition();

    // Find closest point on beam to the node
    Vec2 closestPoint = getClosestPointOnLineSegment(nodePos, beamStart, beamEnd);

    Vec2 pushDirection = nodePos - closestPoint;
    float distance = pushDirection.length();

    if (distance < 0.001f) {
        // Node is exactly on the beam, push it perpendicular to the beam
        Vec2 beamDir = (beamEnd - beamStart).normalized();
        pushDirection = Vec2(-beamDir.y, beamDir.x); // Perpendicular vector
        distance = 1.0f;
    }
    else {
        pushDirection = pushDirection.normalized();
    }

    float beamThickness = 8.0f;
    float overlap = (nodeRadius + beamThickness) - distance;

    if (overlap > 0) {
        // Push the node away from the beam
        Vec2 correction = pushDirection * overlap;
        node->setPosition(nodePos + correction);

        // Apply velocity reflection/damping
        Vec2 velocity = node->getVelocity();
        Vec2 beamNormal = pushDirection;

        // Reflect velocity component perpendicular to beam
        float velocityAlongNormal = velocity.dot(beamNormal);
        if (velocityAlongNormal < 0) {
            Vec2 reflectedVelocity = velocity - beamNormal * velocityAlongNormal * 1.2f; // Slight bounce
            Vec2 impulse = beamNormal * velocityAlongNormal * 0.8f; // Calculate impulse for angular effect
            node->setVelocity(reflectedVelocity * 0.8f); // Add some damping
            
            // Apply angular impulse to frame from beam collision
            applyAngularImpulseToFrame(nodeEntity, nodePos, impulse);
        }

        // Update sprite position
        if (auto* sprite = nodeEntity->getComponent<SpriteComponent>()) {
            Vec2 newPos = node->getPosition();
            sprite->setPosition(newPos.x, newPos.y, 0.0f);
        }
    }
}
Vec2 PhysicsTetrisScene::getClosestPointOnLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd) {
    Vec2 lineVec = lineEnd - lineStart;
    Vec2 pointVec = point - lineStart;

    float lineLength = lineVec.length();
    if (lineLength < 0.001f) {
        return lineStart; // Line is essentially a point
    }

    // Project point onto line
    float t = pointVec.dot(lineVec) / (lineLength * lineLength);

    // Clamp t to [0,1] to stay on the line segment
    t = std::max(0.0f, std::min(1.0f, t));

    return lineStart + lineVec * t;
}
bool PhysicsTetrisScene::isNodePartOfBeam(Entity* nodeEntity, Entity* beamEntity) {
    auto* beam = beamEntity->getComponent<BeamComponent>();
    if (!beam) return false;

    Entity* node1 = beam->getNode1();
    Entity* node2 = beam->getNode2();

    return (nodeEntity == node1 || nodeEntity == node2);
}

bool PhysicsTetrisScene::checkAABBCollision(const Vec2& pos1, float size1, const Vec2& pos2, float size2) {
    float halfSize1 = size1 * 0.5f;
    float halfSize2 = size2 * 0.5f;

    return (pos1.x - halfSize1 < pos2.x + halfSize2 &&
        pos1.x + halfSize1 > pos2.x - halfSize2 &&
        pos1.y - halfSize1 < pos2.y + halfSize2 &&
        pos1.y + halfSize1 > pos2.y - halfSize2);
}

void PhysicsTetrisScene::resolveNodeCollision(Entity* node1Entity, Entity* node2Entity) {
    auto* node1 = node1Entity->getComponent<NodeComponent>();
    auto* node2 = node2Entity->getComponent<NodeComponent>();

    if (!node1 || !node2) return;

    Vec2 pos1 = node1->getPosition();
    Vec2 pos2 = node2->getPosition();

    Vec2 delta = pos1 - pos2;
    float distance = delta.length();

    if (distance > 0.0f && distance < NODE_SIZE) {
        Vec2 normal = delta.normalized();
        float overlap = NODE_SIZE - distance;
        Vec2 separation = normal * (overlap * 0.5f);

        // Only move non-fixed nodes
        if (!node1->isPositionFixed() && !node2->isPositionFixed()) {
            node1->setPosition(pos1 + separation);
            node2->setPosition(pos2 - separation);

            // Apply some velocity dampening
            Vec2 relativeVelocity = node1->getVelocity() - node2->getVelocity();
            float velocityAlongNormal = relativeVelocity.dot(normal);

            if (velocityAlongNormal < 0) {
                Vec2 impulse = normal * velocityAlongNormal * 0.5f;
                node1->setVelocity(node1->getVelocity() - impulse);
                node2->setVelocity(node2->getVelocity() + impulse);
                
                // Apply angular impulse to frames based on collision
                applyAngularImpulseToFrame(node1Entity, pos1, impulse);
                applyAngularImpulseToFrame(node2Entity, pos2, -impulse);
            }
        }
        else if (!node1->isPositionFixed()) {
            node1->setPosition(pos1 + separation * 2.0f);

            // Bounce off fixed objects
            Vec2 velocity = node1->getVelocity();
            float velocityAlongNormal = velocity.dot(normal);
            if (velocityAlongNormal < 0) {
                node1->setVelocity(velocity - normal * velocityAlongNormal * 1.5f);
            }
        }
        else if (!node2->isPositionFixed()) {
            node2->setPosition(pos2 - separation * 2.0f);

            // Bounce off fixed objects
            Vec2 velocity = node2->getVelocity();
            float velocityAlongNormal = velocity.dot(-normal);
            if (velocityAlongNormal < 0) {
                node2->setVelocity(velocity - (-normal) * velocityAlongNormal * 1.5f);
            }
        }
    }
}

void PhysicsTetrisScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    auto& ctx = engine.getContext();

    // Set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    if (auto* toon = engine.getToonPipeline())
        ctx.setGraphicsPipelineState(*toon);
    else
        ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.enableAlphaBlending();

    // Outline pre-pass: draw expanded silhouettes for nodes and beams
    {
        // Use default pipeline for solid tinted outlines (works with current tint buffer)
        ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
        ctx.enableAlphaBlending();
        ctx.enableTransparentDepth();

        const float outlineThickness = 2.0f; // pixels in world space
        const Vec2 offsets[] = {
            Vec2(-outlineThickness,  0.0f), Vec2( outlineThickness,  0.0f),
            Vec2( 0.0f, -outlineThickness), Vec2( 0.0f,  outlineThickness),
            Vec2(-outlineThickness, -outlineThickness), Vec2(-outlineThickness,  outlineThickness),
            Vec2( outlineThickness, -outlineThickness), Vec2( outlineThickness,  outlineThickness)
        };

        // Helper lambda to draw outline for a sprite component without mutating its transform
        auto drawOutlineForSprite = [&](Entity* entity, SpriteComponent* sprite)
        {
            if (!sprite || !sprite->isVisible() || !sprite->isValid()) return;

            // Use the component's 2D world matrix as base
            Mat4 baseWorld = sprite->getWorldMatrix();
            ctx.setTint(Vec4(0.0f, 0.0f, 0.0f, 1.0f));

            // Draw mesh at several offset positions to create a thick outline
            auto mesh = sprite->getMesh();
            if (!mesh) return;

            for (const Vec2& o : offsets)
            {
                Mat4 world = baseWorld * Mat4::translation(Vec3(o.x, o.y, 0.0f));
                ctx.setWorldMatrix(world);
                mesh->draw(ctx);
            }
        };

        // Beams
        for (auto* entity : m_entityManager->getEntitiesWithComponent<BeamComponent>())
        {
            if (auto* sprite = entity->getComponent<SpriteComponent>())
            {
                drawOutlineForSprite(entity, sprite);
            }
        }

        // Nodes
        for (auto* entity : m_entityManager->getEntitiesWithComponent<NodeComponent>())
        {
            if (auto* sprite = entity->getComponent<SpriteComponent>())
            {
                drawOutlineForSprite(entity, sprite);
            }
        }

        ctx.disableAlphaBlending();
        ctx.enableDefaultDepth();
    }

    // Render beams first (behind nodes)
    for (auto* entity : m_entityManager->getEntitiesWithComponent<BeamComponent>()) {
        auto* beam = entity->getComponent<BeamComponent>();
        auto* sprite = entity->getComponent<SpriteComponent>();
        if (beam && sprite && sprite->isVisible() && sprite->isValid()) {
            float stress = clamp(beam->getStressFactor(), 0.0f, 1.0f);
            Vec4 currentTint = sprite->getTint();
            // Mix stress coloring with base color
            Vec4 stressTint = Vec4(
                currentTint.x + stress * 0.5f,
                currentTint.y * (1.0f - stress * 0.5f),
                currentTint.z * (1.0f - stress * 0.5f),
                currentTint.w
            );
            sprite->setTint(stressTint);
            sprite->draw(ctx);
        }
    }

    // Render all sprites (nodes, boundaries, etc.)
    for (auto* entity : m_entityManager->getEntitiesWithComponent<SpriteComponent>()) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid() && !entity->getComponent<BeamComponent>()) {
                sprite->draw(ctx);
            }
        }
    }
    // Render frame debug visualization only if enabled
    if (m_showFrameDebug) {
        renderFrameDebug(ctx);
    }

    // Engine UI drawing removed; ImGui handles all on-screen controls/overlays
    // frame begin/end handled centrally
}

void PhysicsTetrisScene::fixedUpdate(float dt) {
    // Update physics systems with frame-based stabilization
    updateFrameBasedPhysics(dt);
    PhysicsSystem::updateBeams(*m_entityManager, dt);
}

void PhysicsTetrisScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Physics Tetris"))
    {
        ImGui::Text("%s", "Gameplay & Debug Panel");
        ImGui::Separator();

        // Status
        if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Active Piece: %s", m_hasActiveTetrimino ? "Yes" : "No");
            ImGui::Text("Game Over: %s", m_gameOver ? "Yes" : "No");
        }

        // Actions
        if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Restart", ImVec2(-FLT_MIN, 0))) restartGame();
            if (ImGui::Checkbox("Show Frame Debug", &m_showFrameDebug)) {
                // no-op hook; runtime controlled
            }
        }

        // Camera
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* cameraEntity = m_entityManager->findEntity("MainCamera");
            Camera2D* cam = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
            float zoom = cam ? cam->getZoom() : 1.0f;
            if (ImGui::SliderFloat("Zoom", &zoom, 0.4f, 2.0f, "%.2fx"))
            {
                if (cam) cam->setZoom(zoom);
            }
            if (ImGui::Button("Reset Camera", ImVec2(-FLT_MIN, 0)))
            {
                if (cam)
                {
                    cam->setPosition(0.0f, 0.0f);
                    cam->setZoom(DEFAULT_CAMERA_ZOOM);
                }
            }
        }

        // Tuning
        if (ImGui::CollapsingHeader("Tuning", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Frame Gravity");
            ImGui::SliderFloat("##fg", &m_debugFrameGravity, -1.0f, 1.0f, "%.3f");
            ImGui::Text("Node Gravity (Colliding)");
            ImGui::SliderFloat("##ng", &m_debugNodeGravityColliding, -1.0f, 1.0f, "%.3f");
            ImGui::Text("Spring Rotation Strength");
            ImGui::SliderFloat("##sr", &m_debugSpringRotationStrength, -2.0f, 2.0f, "%.3f");
            ImGui::Text("Angular Damping");
            ImGui::SliderFloat("##ad", &m_debugAngularDamping, 0.5f, 1.0f, "%.3f");
            ImGui::Text("Collision Threshold");
            ImGui::SliderFloat("##ct", &m_debugCollisionThreshold, 0.1f, 2.0f, "%.3f");
        }
    }
    ImGui::End();

    // Game Over overlay
    if (m_gameOver) {
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::SetNextWindowPos(ImVec2(0.5f, 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##GameOverOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("GAME OVER");
        if (ImGui::Button("Restart")) {
            restartGame();
        }
        ImGui::End();
    }
}

void PhysicsTetrisScene::updateFrameBasedPhysics(float dt) {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    auto frameEntities = m_entityManager->getEntitiesWithComponent<FrameComponent>();

    // Update frame positions and rotations based on node positions
    for (auto* frameEntity : frameEntities) {
        auto* frame = frameEntity->getComponent<FrameComponent>();
        if (!frame) continue;

        std::string frameName = frameEntity->getName();
        std::string tetriminoPrefix = frameName.substr(0, frameName.find("_Frame"));

        Vec2 averagePos(0.0f, 0.f);
        Vec2 averageVelocity(0.0f, 0.0f);
        int nodeCount = 0;
        std::vector<Vec2> nodePositions;
        std::vector<Vec2> nodeVelocities;

        // Calculate center of mass for this tetrimino
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(tetriminoPrefix) != std::string::npos &&
                nodeEntity->getName().find("_Node") != std::string::npos) {

                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    Vec2 nodePos = node->getPosition();
                    Vec2 nodeVel = node->getVelocity();
                    
                    averagePos += nodePos;
                    averageVelocity += nodeVel;
                    nodePositions.push_back(nodePos);
                    nodeVelocities.push_back(nodeVel);
                    nodeCount++;
                }
            }
        }

        if (nodeCount > 0) {
            averagePos *= (1.0f / nodeCount);
            averageVelocity *= (1.0f / nodeCount);

            // Apply physics to frames for non-active tetriminos
            bool isActiveTetrimino = (tetriminoPrefix.find("_" + std::to_string(m_currentTetriminoId)) != std::string::npos);
            
            if (!isActiveTetrimino) {
                // Apply very slight gravity to frames so whole pieces fall slightly
                Vec2 currentFramePos = frame->getPosition();
                Vec2 frameVelocity = frame->getVelocity();
                
                // Apply very small gravity to frames
                frameVelocity.y -= GRAVITY_ACCELERATION * dt * m_debugFrameGravity; // Debug frame gravity
                
                // Update frame position based on velocity
                Vec2 newFramePos = currentFramePos + frameVelocity * dt;
                
                // Apply boundary constraints to frame
                float leftBound = -PLAY_FIELD_WIDTH / 2 + FRAME_BOUNDARY_PADDING;
                float rightBound = PLAY_FIELD_WIDTH / 2 - FRAME_BOUNDARY_PADDING;
                float bottomBound = -PLAY_FIELD_HEIGHT / 2 + NODE_SIZE / 2;
                float topBound = PLAY_FIELD_HEIGHT / 2 + NODE_SIZE;
                
                // Constrain frame position to play field
                if (newFramePos.x < leftBound) {
                    newFramePos.x = leftBound;
                    frameVelocity.x = -frameVelocity.x * BOUNDARY_BOUNCE;
                }
                else if (newFramePos.x > rightBound) {
                    newFramePos.x = rightBound;
                    frameVelocity.x = -frameVelocity.x * BOUNDARY_BOUNCE;
                }
                
                if (newFramePos.y < bottomBound) {
                    newFramePos.y = bottomBound;
                    frameVelocity.y = -frameVelocity.y * BOUNDARY_BOUNCE;
                }
                else if (newFramePos.y > topBound) {
                    newFramePos.y = topBound;
                    frameVelocity.y = 0.0f;
                }
                
                frame->setPosition(newFramePos);
                
                // Apply velocity damping to prevent clumping and excessive movement
                frameVelocity *= 0.95f; // Light damping to prevent excessive bouncing
                frame->setVelocity(frameVelocity);
                
                // Apply angular velocity and update rotation
                float angularVel = frame->getAngularVelocity();
                float newRotation = frame->getRotation() + angularVel * dt;
                frame->setRotation(newRotation);
                
                // Apply light angular damping to allow natural rotation
                angularVel *= m_debugAngularDamping; // Debug angular damping
                frame->setAngularVelocity(angularVel);
                
                // Simple rotation toward springs with more force
                if (nodeCount >= 2) {
                    float rotationFromSprings = calculateSimpleSpringRotation(tetriminoPrefix);
                    frame->addAngularVelocity(rotationFromSprings);
                    
                    // Check for obvious instability - if center of mass is far from frame center
                    Vec2 framePos = frame->getPosition();
                    Vec2 centerOfMass = averagePos;
                    float imbalance = (centerOfMass - framePos).length();
                    
                    // If there's significant imbalance, add strong rotation
                    if (imbalance > 15.0f) {
                        float instabilityRotation = (rand() % 100 - 50) * 0.02f; // Random rotation for instability
                        frame->addAngularVelocity(instabilityRotation);
                    }

                }
            } else {
                // For active tetriminos, update frame position based on average node positions
            Vec2 currentPos = frame->getPosition();
            Vec2 targetPos = averagePos;
            frame->setPosition(currentPos + (targetPos - currentPos) * 0.2f);
            }
        }
    }

    // Apply physics to individual nodes (gravity, collision, frame forces)
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;

        // Find the frame for this node
        std::string nodeName = nodeEntity->getName();
        if (nodeName.find("_Node") == std::string::npos) continue;

        std::string tetriminoPrefix = nodeName.substr(0, nodeName.find("_Node"));
        std::string frameName = tetriminoPrefix + "_Frame";
        auto* frameEntity = m_entityManager->findEntity(frameName);

        bool isActiveTetrimino = (tetriminoPrefix.find("_" + std::to_string(m_currentTetriminoId)) != std::string::npos);

        // Check if this node is colliding with something
        bool isColliding = false;
        Vec2 nodePos = node->getPosition();
        
        // Check collision with other nodes (more precise collision detection)
        for (auto* otherNodeEntity : nodeEntities) {
            if (otherNodeEntity == nodeEntity) continue;
            auto* otherNode = otherNodeEntity->getComponent<NodeComponent>();
            if (!otherNode || otherNode->isPositionFixed()) continue;
            
            Vec2 otherPos = otherNode->getPosition();
            float distance = (nodePos - otherPos).length();
            if (distance < NODE_SIZE * m_debugCollisionThreshold) { // Debug collision threshold
                isColliding = true;
                break;
            }
        }
        
        // Check collision with ground
        if (nodePos.y <= -PLAY_FIELD_HEIGHT / 2 + NODE_SIZE) {
            isColliding = true;
        }

        // Apply gravity to ALL nodes - full gravity normally, small downward force when colliding
        Vec2 velocity = node->getVelocity();
        if (isColliding) {
            velocity.y -= GRAVITY_ACCELERATION * dt * m_debugNodeGravityColliding; // Debug gravity when colliding
        } else {
            velocity.y -= GRAVITY_ACCELERATION * dt; // Full gravity when not colliding
        }
        node->setVelocity(velocity);

        if (frameEntity) {
            auto* frame = frameEntity->getComponent<FrameComponent>();
            if (frame) {
                Vec2 framePos = frame->getPosition();

                // For active tetriminos, apply drop movement to frame
                if (isActiveTetrimino) {
                Vec2 dropMovement(0.0f, -9.8f*dt);
                Vec2 newPos = framePos + dropMovement;
                frame->setPosition(newPos);
                }

                // Extract node index to get original offset
                std::string nodeIndexStr = nodeName.substr(nodeName.find("_Node") + 5);
                int nodeIndex = std::stoi(nodeIndexStr);

                // Find the original offset for this node
                Vec2 originalOffset(0.0f, 0.0f);
                std::string tetriminoType = tetriminoPrefix.substr(0, tetriminoPrefix.find("_"));

                for (const auto& tetrimino : m_tetriminoTemplates) {
                    if (tetrimino.name == tetriminoType && nodeIndex < tetrimino.nodeOffsets.size()) {
                        originalOffset = tetrimino.nodeOffsets[nodeIndex];
                        break;
                    }
                }

                // Calculate target position based on frame
                framePos = frame->getPosition();
                float frameRot = frame->getRotation();
                Vec2 rotatedOffset = Vec2(
                    originalOffset.x * cosf(frameRot) - originalOffset.y * sinf(frameRot),
                    originalOffset.x * sinf(frameRot) + originalOffset.y * cosf(frameRot)
                );
                Vec2 targetPos = framePos + rotatedOffset;

                // Apply frame attraction force
                Vec2 currentPos = node->getPosition();
                Vec2 displacement = targetPos - currentPos;
                float distance = displacement.length();

                if (distance > 0.1f) {
                    // Strong forces to maintain tetrimino shape
                    float frameStiffness = isActiveTetrimino ? 5000.0f : 3000.0f; // Stronger for non-active
                    float dampingCoeff = isActiveTetrimino ? 80.0f : 60.0f; // Less damping for spring-back

                    Vec2 frameForce = displacement * frameStiffness;

                    // Apply damping
                    Vec2 nodeVelocity = node->getVelocity();
                    Vec2 dampingForce = nodeVelocity * -dampingCoeff;

                    // Combine forces
                    Vec2 totalForce = frameForce + dampingForce;

                    // Apply the force
                    float massFactor = isActiveTetrimino ? 0.02f : 0.015f;
                    Vec2 acceleration = totalForce * massFactor;
                    node->setVelocity(nodeVelocity + acceleration * dt);
                }
            }
        }
    }

    // Apply gravity to orphaned nodes (nodes without frames)
    applyGravityToAllOrphanedNodes();

    // Enhanced collision detection BEFORE position update
    updateCollisions();

    // Update node positions with boundary constraints
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (node && !node->isPositionFixed()) {
            Vec2 velocity = node->getVelocity();


            // Apply more damping to non-active pieces
            std::string nodeName = nodeEntity->getName();
            bool isActiveTetrimino = false;
            if (m_hasActiveTetrimino && m_currentTetriminoId >= 0) {
                std::string currentPrefix = "";
                for (const auto& tetrimino : m_tetriminoTemplates) {
                    std::string testName = tetrimino.name + "_" + std::to_string(m_currentTetriminoId);
                    if (nodeName.find(testName + "_Node") != std::string::npos) {
                        isActiveTetrimino = true;
                        break;
                    }
                }
            }

            float dampingFactor = isActiveTetrimino ? DAMPING_FACTOR_ACTIVE : DAMPING_FACTOR_LOCKED;
            velocity *= dampingFactor;
            node->setVelocity(velocity);

            // Update position
            Vec2 position = node->getPosition();
            Vec2 newPosition = position + velocity * dt;

            // Boundary constraints to prevent phasing through walls
            float leftBound = -PLAY_FIELD_WIDTH / 2 + NODE_BOUNDARY_PADDING;
            float rightBound = PLAY_FIELD_WIDTH / 2 - NODE_BOUNDARY_PADDING;
            float bottomBound = -PLAY_FIELD_HEIGHT / 2 + NODE_SIZE / 2;
            float topBound = PLAY_FIELD_HEIGHT / 2 + NODE_SIZE;

            // Hard boundary constraints
            if (newPosition.x < leftBound) {
                newPosition.x = leftBound;
                velocity.x = -velocity.x * BOUNDARY_BOUNCE;
            }
            else if (newPosition.x > rightBound) {
                newPosition.x = rightBound;
                velocity.x = -velocity.x * BOUNDARY_BOUNCE;
            }

            if (newPosition.y < bottomBound) {
                newPosition.y = bottomBound;
                velocity.y = -velocity.y * BOUNDARY_BOUNCE;
            }
            else if (newPosition.y > topBound) {
                newPosition.y = topBound;
                velocity.y = 0.0f;
            }

            node->setPosition(newPosition);
            node->setVelocity(velocity);

            // Update sprite position
            if (auto* sprite = nodeEntity->getComponent<SpriteComponent>()) {
                sprite->setPosition(newPosition.x, newPosition.y, 0.0f);
            }
        }
    }

}

void PhysicsTetrisScene::handleTetriminoInput(float dt) {
    if (m_currentTetriminoId < 0 || !m_hasActiveTetrimino) return;

    auto& input = Input::getInstance();

    // Find the current tetrimino's frame
    std::string currentTetrimino = "";
    for (const auto& tetrimino : m_tetriminoTemplates) {
        std::string frameName = tetrimino.name + "_" + std::to_string(m_currentTetriminoId) + "_Frame";
        auto* frameEntity = m_entityManager->findEntity(frameName);
        if (frameEntity) {
            currentTetrimino = tetrimino.name + "_" + std::to_string(m_currentTetriminoId);
            break;
        }
    }

    if (currentTetrimino.empty()) return;

    std::string frameName = currentTetrimino + "_Frame";
    auto* frameEntity = m_entityManager->findEntity(frameName);
    if (!frameEntity) return;

    auto* frame = frameEntity->getComponent<FrameComponent>();
    if (!frame) return;

    Vec2 framePos = frame->getPosition();
    bool moved = false;

    // Movement controls with proper timing
    static float moveTimer = 0.0f;
    static float fastDropTimer = 0.0f;

    moveTimer -= dt;
    fastDropTimer -= dt;

    if (moveTimer <= 0.0f) {
        Vec2 movement(0.0f, 0.0f);

        if (input.isKeyDown(Key::Left) ||input.isKeyDown(Key::A)) {  
            movement.x = -HORIZONTAL_MOVEMENT;
            moveTimer = MOVE_TIMER_INTERVAL;
            moved = true;
        }
        else if (input.isKeyDown(Key::Right) || input.isKeyDown(Key::D)) {  
            movement.x = HORIZONTAL_MOVEMENT;
            moveTimer = MOVE_TIMER_INTERVAL;
            moved = true;
        }

        if (moved && movement.x != 0.0f) {
            Vec2 newPos = framePos + movement;

            // Check bounds before moving
            float leftBound = -PLAY_FIELD_WIDTH / 2 + FRAME_BOUNDARY_PADDING;
            float rightBound = PLAY_FIELD_WIDTH / 2 - FRAME_BOUNDARY_PADDING;

            if (newPos.x >= leftBound && newPos.x <= rightBound) {
                frame->setPosition(newPos);

                // Apply stronger impulse to all nodes
                auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
                for (auto* nodeEntity : nodeEntities) {
                    if (nodeEntity->getName().find(currentTetrimino + "_Node") != std::string::npos) {
                        auto* node = nodeEntity->getComponent<NodeComponent>();
                        if (node && !node->isPositionFixed()) {
                        Vec2 velocity = node->getVelocity();
                        velocity += movement * MOVEMENT_IMPULSE;
                        node->setVelocity(velocity);
                        }
                    }
                }
            }
        }
    }

    if (fastDropTimer <= 0.0f && input.isKeyDown(Key::Down) || input.isKeyDown(Key::S)) {
        Vec2 dropMovement(0.0f, -FAST_DROP_MOVEMENT);
        Vec2 newPos = framePos + dropMovement;
        frame->setPosition(newPos);

        // Apply downward impulse
        auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(currentTetrimino + "_Node") != std::string::npos) {
                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    Vec2 velocity = node->getVelocity();
                    velocity += dropMovement * DROP_IMPULSE;
                    node->setVelocity(velocity);
                }
            }
        }
        fastDropTimer = FAST_DROP_TIMER_INTERVAL;
    }

    static float rotateTimer = 0.0f;
    rotateTimer -= dt;

    if (rotateTimer <= 0.0f) {
        float rotation = frame->getRotation();
        bool rotated = false;

        if (input.wasKeyJustPressed(Key::Q)) {
            rotation += ROTATION_ANGLE; // 90 degrees counter-clockwise
            rotated = true;
        }
        else if (input.wasKeyJustPressed(Key::E)) {
            rotation -= ROTATION_ANGLE; // 90 degrees clockwise  
            rotated = true;
        }

        if (rotated) {
            frame->setRotation(rotation);
            rotateTimer = ROTATE_TIMER_INTERVAL;

            // Apply rotational impulse
            auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
            for (auto* nodeEntity : nodeEntities) {
                if (nodeEntity->getName().find(currentTetrimino + "_Node") != std::string::npos) {
                    auto* node = nodeEntity->getComponent<NodeComponent>();
                    if (node && !node->isPositionFixed()) {
                        Vec2 velocity = node->getVelocity();
                        velocity += Vec2((rand() % 20 - 10) * 3.0f, (rand() % 20 - 10) * 3.0f);
                        node->setVelocity(velocity);
                    }
                }
            }
        }
    }
}

void PhysicsTetrisScene::checkTetriminoLanded() {
    if (m_currentTetriminoId < 0 || !m_hasActiveTetrimino) return;

    // Find current tetrimino nodes
    std::string currentTetrimino = "";
    for (const auto& tetrimino : m_tetriminoTemplates) {
        std::string testName = tetrimino.name + "_" + std::to_string(m_currentTetriminoId);
        std::string frameName = testName + "_Frame";
        if (m_entityManager->findEntity(frameName)) {
            currentTetrimino = testName;
            break;
        }
    }

    if (currentTetrimino.empty()) return;

    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    bool hasLanded = false;
    bool allNodesSettled = true;
    float settledVelocityThreshold = 30.0f;

    for (auto* nodeEntity : nodeEntities) {
        if (nodeEntity->getName().find(currentTetrimino + "_Node") != std::string::npos) {
            auto* node = nodeEntity->getComponent<NodeComponent>();
            if (!node || node->isPositionFixed()) continue;

            Vec2 pos = node->getPosition();
            Vec2 velocity = node->getVelocity();

            // Check if velocity is too high (not settled)
            if (velocity.length() > settledVelocityThreshold) {
                allNodesSettled = false;
            }

            // Check if hit bottom boundary
            if (pos.y <= -PLAY_FIELD_HEIGHT / 2 + NODE_SIZE * 1.5f) {
                hasLanded = true;
                break;
            }

            // Check collision with other settled tetriminos
            for (auto* otherNodeEntity : nodeEntities) {
                if (otherNodeEntity == nodeEntity) continue;

                std::string otherName = otherNodeEntity->getName();
                bool isCurrentTetrimino = (otherName.find(currentTetrimino + "_Node") != std::string::npos);
                bool isWall = (otherName.find("Wall") != std::string::npos);

                if (!isCurrentTetrimino && !isWall) {
                    auto* otherNode = otherNodeEntity->getComponent<NodeComponent>();
                    if (otherNode) {
                        Vec2 otherPos = otherNode->getPosition();
                        float distance = (pos - otherPos).length();

                        // If very close to another settled piece
                        if (distance < NODE_SIZE * 1.5f && allNodesSettled) {
                            hasLanded = true;
                            break;
                        }
                    }
                }
            }

            if (hasLanded) break;
        }
    }

    // Only lock if both conditions are met: landed AND settled
    if (hasLanded && allNodesSettled) {
        lockTetrimino(m_currentTetriminoId);
        m_hasActiveTetrimino = false;
        m_currentTetriminoId = -1;
    }
}
void PhysicsTetrisScene::lockTetrimino(int tetriminoId) {
    // DON'T remove the frame immediately - keep it for structural integrity
    // The frame will help keep the tetrimino shape even after it's locked

    // Instead, just mark it as non-controllable by clearing the active flags
    m_hasActiveTetrimino = false;
    m_currentTetriminoId = -1;

    // Reduce velocity of all nodes to help piece settle, but keep frame forces active
    std::string tetriminoPrefix = "";
    for (const auto& tetrimino : m_tetriminoTemplates) {
        std::string testName = tetrimino.name + "_" + std::to_string(tetriminoId);
        auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(testName + "_Node") != std::string::npos) {
                tetriminoPrefix = testName;
                break;
            }
        }
        if (!tetriminoPrefix.empty()) break;
    }

    if (!tetriminoPrefix.empty()) {
        auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(tetriminoPrefix + "_Node") != std::string::npos) {
                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    // Gentle velocity reduction to help settle, but don't kill all movement
                    Vec2 velocity = node->getVelocity();
                    velocity *= 0.7f;  // Less aggressive than 0.3f
                    node->setVelocity(velocity);
                }
            }
        }
    }

    // After finding the frame entity:
    std::string frameName = tetriminoPrefix + "_Frame";
    auto* frameEntity = m_entityManager->findEntity(frameName);
    if (frameEntity) {
        auto* frame = frameEntity->getComponent<FrameComponent>();
        if (frame) {
            // Keep the current rotation - don't reset it to 0
            // The tetrimino should maintain whatever rotation it had when it locked
        }
    }
}
void PhysicsTetrisScene::updateUI() {
    // Update UI elements (could add score, level, etc. here)
}
void PhysicsTetrisScene::updateCameraMovement(float dt) {
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

    // Only use WASD for camera when no active tetrimino, or use different keys
    bool hasActivePiece = m_hasActiveTetrimino;

    if (!hasActivePiece) {  // Only allow camera movement when no active piece
        //if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt;
        //if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt;
        //if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt;
        //if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt;
    }

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        camera->move(moveDelta);
    }

    // Keep zoom controls separate (no conflicts)
    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::I)) zoomDelta -= zoomSpeed * dt;      // Use - key
    if (input.isKeyDown(Key::J)) zoomDelta += zoomSpeed * dt;      // Use + key
    if (zoomDelta != 0.0f) camera->zoom(zoomDelta);

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(1.0f);
    }
}

void PhysicsTetrisScene::updateFrameDebugVisualization() {
    if (!m_lineRenderer) return;

    m_lineRenderer->enableScreenSpace(false); // Use world space coordinates
    m_lineRenderer->clear();
    
    // Add line scan debug visualization
    renderLineScanDebug();

    // TEST: Draw a single fixed cross at world origin to verify coordinate system
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

    auto frameEntities = m_entityManager->getEntitiesWithComponent<FrameComponent>();
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();

    // Render each tetrimino frame
    for (auto* frameEntity : frameEntities) {
        auto* frame = frameEntity->getComponent<FrameComponent>();
        if (!frame) continue;

        std::string frameName = frameEntity->getName();
        std::string tetriminoPrefix = frameName.substr(0, frameName.find("_Frame"));

        Vec2 framePos = frame->getPosition();
        float frameRot = frame->getRotation();

        // Calculate actual center of mass from current node positions
        Vec2 actualCenterOfMass(0.0f, 0.0f);
        int nodeCount = 0;
        std::vector<Vec2> nodePositions;

        // Collect all node positions for this tetrimino
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(tetriminoPrefix) != std::string::npos &&
                nodeEntity->getName().find("_Node") != std::string::npos) {
                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    Vec2 nodePos = node->getPosition();
                    actualCenterOfMass += nodePos;
                    nodePositions.push_back(nodePos);
                    nodeCount++;
                }
            }
        }

        if (nodeCount == 0) continue;

        actualCenterOfMass *= (1.0f / nodeCount);

        // Determine color based on tetrimino type and active state
        Vec4 frameColor = Vec4(1.0f, 1.0f, 1.0f, 0.8f); // Default white
        Vec4 centerColor = Vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for center of mass
        Vec4 targetColor = Vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for frame position

        // Check if this is the active tetrimino
        bool isActiveTetrimino = (tetriminoPrefix.find("_" + std::to_string(m_currentTetriminoId)) != std::string::npos);
        if (isActiveTetrimino) {
            frameColor = Vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for active
            targetColor = Vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan for active frame position
        }

        // 1. Render frame position as cross - using actual frame coordinates
        float crossSize = 15.0f;
        m_lineRenderer->addLine(
            Vec2(framePos.x - crossSize, framePos.y),
            Vec2(framePos.x + crossSize, framePos.y),
            targetColor, 2.0f
        );
        m_lineRenderer->addLine(
            Vec2(framePos.x, framePos.y - crossSize),
            Vec2(framePos.x, framePos.y + crossSize),
            targetColor, 2.0f
        );

        // 2. Render actual center of mass (calculated from node positions)
        float centerSize = 10.0f;
        m_lineRenderer->addLine(
            Vec2(actualCenterOfMass.x - centerSize, actualCenterOfMass.y - centerSize),
            Vec2(actualCenterOfMass.x + centerSize, actualCenterOfMass.y + centerSize),
            centerColor, 2.0f
        );
        m_lineRenderer->addLine(
            Vec2(actualCenterOfMass.x - centerSize, actualCenterOfMass.y + centerSize),
            Vec2(actualCenterOfMass.x + centerSize, actualCenterOfMass.y - centerSize),
            centerColor, 2.0f
        );

        // 3. Draw line connecting frame position to actual center of mass
        m_lineRenderer->addLine(framePos, actualCenterOfMass, Vec4(0.5f, 0.5f, 0.5f, 0.6f), 1.0f);

        // 4. Render frame orientation indicator (shows rotation)
        float orientationLength = 25.0f;
        Vec2 orientationEnd = framePos + Vec2(
            orientationLength * cosf(frameRot),
            orientationLength * sinf(frameRot)
        );
        m_lineRenderer->addLine(framePos, orientationEnd, frameColor, 3.0f);

        std::string tetriminoType = tetriminoPrefix.substr(0, tetriminoPrefix.find("_"));

        for (const auto& tetrimino : m_tetriminoTemplates) {
            if (tetrimino.name != tetriminoType) continue;

            for (size_t i = 0; i < tetrimino.nodeOffsets.size(); ++i) {
                Vec2 originalOffset = tetrimino.nodeOffsets[i];

                // Apply frame rotation to the offset
                Vec2 rotatedOffset = Vec2(
                    originalOffset.x * cosf(frameRot) - originalOffset.y * sinf(frameRot),
                    originalOffset.x * sinf(frameRot) + originalOffset.y * cosf(frameRot)
                );

                Vec2 targetNodePos = framePos + rotatedOffset;

                std::string nodeName = tetriminoPrefix + "_Node" + std::to_string(i);
                auto* nodeEntity = m_entityManager->findEntity(nodeName);
                if (nodeEntity) {
                    auto* node = nodeEntity->getComponent<NodeComponent>();
                    if (node && !node->isPositionFixed()) {
                        Vec2 actualNodePos = node->getPosition();
                        Vec2 delta = targetNodePos - actualNodePos;

                        m_lineRenderer->addCircle(targetNodePos, 3.0f, Vec4(0.0f, 1.0f, 0.0f, 0.5f), 1.0f, 8);

                        m_lineRenderer->addLine(actualNodePos, targetNodePos, Vec4(1.0f, 1.0f, 0.0f, 0.4f), 1.0f);
                    }
                }
            }
            break;
        }

        // 6. Render bounding box around tetrimino nodes
        if (nodePositions.size() > 1) {
            float minX = nodePositions[0].x, maxX = nodePositions[0].x;
            float minY = nodePositions[0].y, maxY = nodePositions[0].y;

            for (const Vec2& pos : nodePositions) {
                minX = std::min(minX, pos.x);
                maxX = std::max(maxX, pos.x);
                minY = std::min(minY, pos.y);
                maxY = std::max(maxY, pos.y);
            }

            // Add some padding
            minX -= NODE_SIZE * 0.5f;
            maxX += NODE_SIZE * 0.5f;
            minY -= NODE_SIZE * 0.5f;
            maxY += NODE_SIZE * 0.5f;

            m_lineRenderer->addRect(
                Vec2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f),
                Vec2(maxX - minX, maxY - minY),
                Vec4(0.3f, 0.3f, 1.0f, 0.3f),
                1.0f
            );
        }
    }
}

void PhysicsTetrisScene::checkAndClearLines() {
    // Scan horizontal lines from bottom to top of play field
    float playFieldBottom = -PLAY_FIELD_HEIGHT / 2;
    float playFieldTop = PLAY_FIELD_HEIGHT / 2;
    float lineSpacing = LINE_SCAN_SPACING;
    
    for (float y = playFieldBottom; y <= playFieldTop; y += lineSpacing) {
        if (isLineComplete(y)) {
            clearLine(y);
        }
    }
}

bool PhysicsTetrisScene::isLineComplete(float y, float tolerance) {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    int nodeCount = 0;
    
    // Count nodes that are close to this Y level and within the play field
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        // Skip wall nodes
        std::string nodeName = nodeEntity->getName();
        if (nodeName.find("Wall") != std::string::npos) continue;
        
        Vec2 pos = node->getPosition();
        
        // Check if node is at this Y level (within tolerance) and within play field X bounds
        if (abs(pos.y - y) <= LINE_SCAN_TOLERANCE && 
            pos.x >= -PLAY_FIELD_WIDTH / 2 && 
            pos.x <= PLAY_FIELD_WIDTH / 2) {
            nodeCount++;
        }
    }
    
    return nodeCount >= LINE_CLEAR_NODE_THRESHOLD ; // Line is complete if 5 or more nodes
}

void PhysicsTetrisScene::clearLine(float y, float tolerance) {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    std::vector<std::string> nodesToRemove;
    std::vector<std::string> beamsToRemove;
    std::set<std::string> affectedTetriminos; // Track which tetriminos are affected
    
    // Find all nodes in this line
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        std::string nodeName = nodeEntity->getName();
        if (nodeName.find("Wall") != std::string::npos) continue;
        
        Vec2 pos = node->getPosition();
        
        if (abs(pos.y - y) <= tolerance && 
            pos.x >= -PLAY_FIELD_WIDTH / 2 && 
            pos.x <= PLAY_FIELD_WIDTH / 2) {
            nodesToRemove.push_back(nodeName);
            
            // Extract tetrimino prefix (e.g., "I_0" from "I_0_Node1")
            size_t nodePos = nodeName.find("_Node");
            if (nodePos != std::string::npos) {
                std::string tetriminoPrefix = nodeName.substr(0, nodePos);
                affectedTetriminos.insert(tetriminoPrefix);
            }
        }
    }
    
    // Find and remove beams connected to these nodes
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    for (auto* beamEntity : beamEntities) {
        auto* beam = beamEntity->getComponent<BeamComponent>();
        if (!beam) continue;
        
        std::string node1Name = beam->getNode1()->getName();
        std::string node2Name = beam->getNode2()->getName();
        
        // If either node of the beam is being removed, remove the beam too
        bool shouldRemoveBeam = false;
        for (const std::string& nodeToRemove : nodesToRemove) {
            if (node1Name == nodeToRemove || node2Name == nodeToRemove) {
                shouldRemoveBeam = true;
                break;
            }
        }
        
        if (shouldRemoveBeam) {
            beamsToRemove.push_back(beamEntity->getName());
        }
    }
    
    // Remove frames for all affected tetriminos (any tetrimino that had nodes cleared)
    std::vector<std::string> framesToRemove;
    for (const std::string& tetriminoPrefix : affectedTetriminos) {
        std::string frameName = tetriminoPrefix + "_Frame";
        if (m_entityManager->findEntity(frameName)) {
            framesToRemove.push_back(frameName);
        }
    }
    
    // Remove the entities
    for (const std::string& beamName : beamsToRemove) {
        m_entityManager->removeEntity(beamName);
    }
    
    for (const std::string& nodeName : nodesToRemove) {
        m_entityManager->removeEntity(nodeName);
    }
    
    for (const std::string& frameName : framesToRemove) {
        m_entityManager->removeEntity(frameName);
    }
    
    // Apply gravity to orphaned nodes (nodes whose frames were removed)
    applyGravityToOrphanedNodes(affectedTetriminos, framesToRemove);
    
    // Check if the currently active tetrimino's frame was removed
    if (m_hasActiveTetrimino && m_currentTetriminoId >= 0) {
        for (const std::string& removedFrameName : framesToRemove) {
            // Extract tetrimino ID from frame name
            std::string tetriminoPrefix = removedFrameName.substr(0, removedFrameName.find("_Frame"));
            std::string currentTetriminoPrefix = "";
            
            // Find the current tetrimino prefix by checking all templates
            for (const auto& tetrimino : m_tetriminoTemplates) {
                std::string testPrefix = tetrimino.name + "_" + std::to_string(m_currentTetriminoId);
                if (tetriminoPrefix == testPrefix) {
                    currentTetriminoPrefix = testPrefix;
                    break;
                }
            }
            
            // If the removed frame belongs to the active tetrimino, reset active state
            if (!currentTetriminoPrefix.empty() && tetriminoPrefix == currentTetriminoPrefix) {
                m_hasActiveTetrimino = false;
                m_currentTetriminoId = -1;
                std::cout << "Active tetrimino frame removed - ready for new spawn" << std::endl;
                break;
            }
        }
    }
    
    // If we removed nodes, print debug info
    if (!nodesToRemove.empty()) {
        std::cout << "Cleared line at Y=" << y << " with " << nodesToRemove.size() << " nodes, " 
                  << framesToRemove.size() << " frames removed" << std::endl;
    }
}

void PhysicsTetrisScene::renderLineScanDebug() {
    if (!m_lineRenderer || !m_showFrameDebug) return;
    
    // Draw horizontal scan lines across the play field
    float playFieldBottom = -PLAY_FIELD_HEIGHT / 2;
    float playFieldTop = PLAY_FIELD_HEIGHT / 2;
    float lineSpacing = 20.0f;
    
    float playFieldLeft = -PLAY_FIELD_WIDTH / 2;
    float playFieldRight = PLAY_FIELD_WIDTH / 2;
    
    for (float y = playFieldBottom; y <= playFieldTop; y += lineSpacing) {
        // Choose color based on whether line is complete
        Vec4 lineColor = isLineComplete(y) ? 
            Vec4(1.0f, 0.0f, 0.0f, 0.8f) :  // Red for complete lines
            Vec4(0.3f, 0.3f, 0.3f, 0.3f);   // Gray for incomplete lines
        
        m_lineRenderer->addLine(
            Vec2(playFieldLeft, y),
            Vec2(playFieldRight, y),
            lineColor, 1.0f
        );
    }
    
    // Render line progress bars
    renderLineProgressBars();
}

float PhysicsTetrisScene::calculateLineProgress(float y, float tolerance) {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    int nodeCount = 0;
    
    // Count nodes that are close to this Y level and within the play field
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        // Skip wall nodes
        std::string nodeName = nodeEntity->getName();
        if (nodeName.find("Wall") != std::string::npos) continue;
        
        Vec2 pos = node->getPosition();
        
        // Check if node is at this Y level (within tolerance) and within play field X bounds
        if (abs(pos.y - y) <= tolerance && 
            pos.x >= -PLAY_FIELD_WIDTH / 2 && 
            pos.x <= PLAY_FIELD_WIDTH / 2) {
            nodeCount++;
        }
    }
    
    // Return progress as a percentage (0.0 to 1.0)
    return std::min(1.0f, static_cast<float>(nodeCount) / static_cast<float>(LINE_CLEAR_NODE_THRESHOLD));
}

void PhysicsTetrisScene::renderLineProgressBars() {
    if (!m_lineRenderer) return;
    
    float playFieldBottom = -PLAY_FIELD_HEIGHT / 2;
    float playFieldTop = PLAY_FIELD_HEIGHT / 2;
    float lineSpacing = 20.0f;
    
    float playFieldLeft = -PLAY_FIELD_WIDTH / 2;
    float playFieldRight = PLAY_FIELD_WIDTH / 2;
    
    // Position progress bars to the right of the play field
    float progressBarX = playFieldRight + 50.0f; // 50 pixels to the right of play field
    float progressBarWidth = 20.0f; // Width of progress bars
    float progressBarHeight = 15.0f; // Height of each progress bar
    
    for (float y = playFieldBottom; y <= playFieldTop; y += lineSpacing) {
        float progress = calculateLineProgress(y);
        
        // Determine color based on progress
        Vec4 progressColor;
        if (progress >= 1.0f) {
            progressColor = Vec4(1.0f, 0.0f, 0.0f, 0.8f); // Red for complete lines
        } else if (progress >= 0.7f) {
            progressColor = Vec4(1.0f, 1.0f, 0.0f, 0.8f); // Yellow for nearly complete
        } else if (progress >= 0.3f) {
            progressColor = Vec4(0.0f, 1.0f, 0.0f, 0.6f); // Green for partially filled
        } else {
            progressColor = Vec4(0.5f, 0.5f, 0.5f, 0.4f); // Gray for mostly empty
        }
        
        // Calculate the filled portion of the progress bar
        float filledWidth = progressBarWidth * progress;
        
        // Draw background (empty portion)
        if (progress < 1.0f) {
            Vec2 bgStart(progressBarX + filledWidth, y - progressBarHeight / 2);
            Vec2 bgEnd(progressBarX + progressBarWidth, y + progressBarHeight / 2);
            m_lineRenderer->addRect(
                Vec2((bgStart.x + bgEnd.x) / 2, (bgStart.y + bgEnd.y) / 2),
                Vec2(progressBarWidth - filledWidth, progressBarHeight),
                Vec4(0.2f, 0.2f, 0.2f, 0.3f), 1.0f
            );
        }
        
        // Draw filled portion
        if (filledWidth > 0.0f) {
            Vec2 fillStart(progressBarX, y - progressBarHeight / 2);
            Vec2 fillEnd(progressBarX + filledWidth, y + progressBarHeight / 2);
            m_lineRenderer->addRect(
                Vec2((fillStart.x + fillEnd.x) / 2, (fillStart.y + fillEnd.y) / 2),
                Vec2(filledWidth, progressBarHeight),
                progressColor, 1.0f
            );
        }
        
        // Draw border around the progress bar
        m_lineRenderer->addRect(
            Vec2(progressBarX + progressBarWidth / 2, y),
            Vec2(progressBarWidth, progressBarHeight),
            Vec4(1.0f, 1.0f, 1.0f, 0.5f), 1.0f
        );
    }
}

void PhysicsTetrisScene::applyGravityToOrphanedNodes(const std::set<std::string>& affectedTetriminos, const std::vector<std::string>& removedFrames) {
    // Get all remaining nodes that belonged to tetriminos whose frames were removed
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    for (const std::string& removedFrameName : removedFrames) {
        // Extract tetrimino prefix from frame name (remove "_Frame" suffix)
        std::string tetriminoPrefix = removedFrameName.substr(0, removedFrameName.find("_Frame"));
        
        // Find all remaining nodes for this tetrimino and apply gravity
        for (auto* nodeEntity : nodeEntities) {
            std::string nodeName = nodeEntity->getName();
            
            // Check if this node belongs to the tetrimino whose frame was removed
            if (nodeName.find(tetriminoPrefix + "_Node") != std::string::npos) {
                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    // Apply strong downward velocity to simulate gravity
                    Vec2 currentVelocity = node->getVelocity();
                    currentVelocity.y = -200.0f; // Strong downward force
                    currentVelocity.x *= 0.5f;   // Reduce horizontal movement
                    node->setVelocity(currentVelocity);
                }
            }
        }
    }
}

bool PhysicsTetrisScene::isNodeOrphaned(const std::string& nodeName) {
    // Extract tetrimino prefix (e.g., "I_0" from "I_0_Node1")
    size_t nodePos = nodeName.find("_Node");
    if (nodePos == std::string::npos) return false;
    
    std::string tetriminoPrefix = nodeName.substr(0, nodePos);
    std::string frameName = tetriminoPrefix + "_Frame";
    
    // Node is orphaned if its corresponding frame doesn't exist
    return m_entityManager->findEntity(frameName) == nullptr;
}

void PhysicsTetrisScene::applyGravityToAllOrphanedNodes() {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        std::string nodeName = nodeEntity->getName();
        
        // Skip wall nodes
        if (nodeName.find("Wall") != std::string::npos) continue;
        
        // Check if this node is orphaned (no frame exists)
        if (isNodeOrphaned(nodeName)) {
            // Apply continuous gravity to orphaned nodes
            Vec2 velocity = node->getVelocity();
            velocity.y -= GRAVITY_ACCELERATION * FRAME_TIME_ESTIMATE;
            
            // Add some air resistance to prevent infinite acceleration
            velocity *= AIR_RESISTANCE;
            
            node->setVelocity(velocity);
        }
    }
}

void PhysicsTetrisScene::checkGameOverCondition() {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    float topOfPlayField = PLAY_FIELD_HEIGHT / 2;
    float gameOverLine = topOfPlayField - GAME_OVER_THRESHOLD;
    
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        std::string nodeName = nodeEntity->getName();
        if (nodeName.find("Wall") != std::string::npos) continue;
        
        Vec2 pos = node->getPosition();
        
        // Check if any settled node (not the active tetrimino) is above the game over line
        bool isActiveNode = false;
        if (m_hasActiveTetrimino && m_currentTetriminoId >= 0) {
            for (const auto& tetrimino : m_tetriminoTemplates) {
                std::string activePrefix = tetrimino.name + "_" + std::to_string(m_currentTetriminoId);
                if (nodeName.find(activePrefix + "_Node") != std::string::npos) {
                    isActiveNode = true;
                    break;
                }
            }
        }
        
        // If this is a settled node (not active) and it's above the game over line
        if (!isActiveNode && pos.y >= gameOverLine) {
            m_gameOver = true;
            std::cout << "GAME OVER! Node reached top of play field at Y=" << pos.y << std::endl;
            
            // Show game over panel
            if (auto* panelEntity = m_entityManager->findEntity("GameOverPanel")) {
                if (auto* panel = panelEntity->getComponent<PanelComponent>()) {
                    panel->setVisible(true);
                }
            }
            return;
        }
    }
}

void PhysicsTetrisScene::handleGameOver() {
    m_gameOverTimer += 0.016f; // Assuming ~60fps
    
    // Stop active tetrimino control
    m_hasActiveTetrimino = false;
    m_currentTetriminoId = -1;
    
    // Check for restart input
    auto& input = Input::getInstance();
    if (input.wasKeyJustPressed(Key::R)) {
        restartGame();
    }
}

void PhysicsTetrisScene::restartGame() {
    std::cout << "Restarting game..." << std::endl;
    
    // Reset game state
    m_gameOver = false;
    m_gameOverTimer = 0.0f;
    m_hasActiveTetrimino = false;
    m_currentTetriminoId = -1;
    m_nextTetriminoId = 0;
    m_spawnTimer = 0.0f;
    
    // Hide game over panel
    if (auto* panelEntity = m_entityManager->findEntity("GameOverPanel")) {
        if (auto* panel = panelEntity->getComponent<PanelComponent>()) {
            panel->setVisible(false);
        }
    }
    
    // Clear all tetrimino entities
    const auto& allEntities = m_entityManager->getEntities();
    std::vector<std::string> entitiesToRemove;
    
    for (const auto& entityPtr : allEntities) {
        Entity* entity = entityPtr.get();
        std::string name = entity->getName();
        // Remove tetrimino nodes, beams, and frames, but keep UI and boundaries
        if (name.find("_Node") != std::string::npos || 
            name.find("_Beam") != std::string::npos || 
            name.find("_Frame") != std::string::npos ||
            name.find("WorldOriginAnchor") != std::string::npos) {
            entitiesToRemove.push_back(name);
        }
    }
    
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }
    
    // Spawn first tetrimino
    spawnNextTetrimino();
}

float PhysicsTetrisScene::calculateRotationFromNodes(const std::vector<Vec2>& nodePositions, const std::string& tetriminoPrefix) {
    if (nodePositions.size() < 2) return 0.0f;
    
    // Find the tetrimino type to get original offsets
    std::string tetriminoType = tetriminoPrefix.substr(0, tetriminoPrefix.find("_"));
    const TetriminoData* tetriminoData = nullptr;
    
    for (const auto& tetrimino : m_tetriminoTemplates) {
        if (tetrimino.name == tetriminoType) {
            tetriminoData = &tetrimino;
            break;
        }
    }
    
    if (!tetriminoData || tetriminoData->nodeOffsets.size() < 2) return 0.0f;
    
    // Calculate center of mass of current positions
    Vec2 currentCenter(0.0f, 0.0f);
    for (const Vec2& pos : nodePositions) {
        currentCenter += pos;
    }
    currentCenter *= (1.0f / nodePositions.size());
    
    // Use the first two nodes to determine rotation
    Vec2 currentDir = (nodePositions[1] - nodePositions[0]).normalized();
    Vec2 originalDir = (tetriminoData->nodeOffsets[1] - tetriminoData->nodeOffsets[0]).normalized();
    
    // Calculate angle between current and original directions
    float dot = currentDir.dot(originalDir);
    float det = currentDir.x * originalDir.y - currentDir.y * originalDir.x;
    float angle = atan2f(det, dot);
    
    return angle;
}

float PhysicsTetrisScene::calculateAngularVelocityFromNodes(const std::vector<Vec2>& nodePositions, const std::vector<Vec2>& nodeVelocities, const Vec2& centerOfMass) {
    if (nodePositions.size() < 2 || nodeVelocities.size() < 2) return 0.0f;
    
    // Calculate angular velocity from the first two nodes
    Vec2 pos1 = nodePositions[0] - centerOfMass;
    Vec2 pos2 = nodePositions[1] - centerOfMass;
    Vec2 vel1 = nodeVelocities[0];
    Vec2 vel2 = nodeVelocities[1];
    
    // Calculate angular velocity using cross product
    // ω = (r × v) / |r|²
    float angularVel1 = (pos1.x * vel1.y - pos1.y * vel1.x) / (pos1.length() * pos1.length() + 0.001f);
    float angularVel2 = (pos2.x * vel2.y - pos2.y * vel2.x) / (pos2.length() * pos2.length() + 0.001f);
    
    // Average the angular velocities
    return (angularVel1 + angularVel2) * 0.5f;
}

void PhysicsTetrisScene::applyAngularImpulseToFrame(Entity* nodeEntity, const Vec2& nodePosition, const Vec2& impulse) {
    // Find the frame for this node
    std::string nodeName = nodeEntity->getName();
    if (nodeName.find("_Node") == std::string::npos) return;
    
    std::string tetriminoPrefix = nodeName.substr(0, nodeName.find("_Node"));
    std::string frameName = tetriminoPrefix + "_Frame";
    auto* frameEntity = m_entityManager->findEntity(frameName);
    
    if (!frameEntity) return;
    
    auto* frame = frameEntity->getComponent<FrameComponent>();
    if (!frame) return;
    
    // Only apply to non-active tetriminos
    bool isActiveTetrimino = (tetriminoPrefix.find("_" + std::to_string(m_currentTetriminoId)) != std::string::npos);
    if (isActiveTetrimino) return;
    
    // Calculate angular impulse: τ = r × F
    Vec2 framePos = frame->getPosition();
    Vec2 leverArm = nodePosition - framePos;
    
    // Cross product: r × F = r.x * F.y - r.y * F.x
    float torque = leverArm.x * impulse.y - leverArm.y * impulse.x;
    
    // Apply angular impulse (scaled down to be realistic)
    float angularImpulse = torque * 0.00001f; // Much smaller scale factor to prevent chaotic rotation
    frame->addAngularVelocity(angularImpulse);
}

float PhysicsTetrisScene::calculateSpringBasedRotation(const std::string& tetriminoPrefix, const std::vector<Vec2>& nodePositions, const Vec2& centerOfMass) {
    if (nodePositions.size() < 2) return 0.0f;
    
    // First, check for obvious imbalance - if center of mass is far from frame center
    Vec2 framePos = centerOfMass; // Use center of mass as frame position
    float imbalanceMagnitude = 0.0f;
    
    // Calculate how much the nodes are displaced from their expected positions
    float totalDisplacement = 0.0f;
    for (const Vec2& nodePos : nodePositions) {
        float displacement = (nodePos - framePos).length();
        totalDisplacement += displacement;
    }
    float averageDisplacement = totalDisplacement / nodePositions.size();
    
    // If there's significant displacement, create strong rotation
    if (averageDisplacement > 10.0f) {
        // Calculate which side has more displacement (imbalance direction)
        float leftDisplacement = 0.0f, rightDisplacement = 0.0f;
        int leftCount = 0, rightCount = 0;
        
        for (const Vec2& nodePos : nodePositions) {
            Vec2 fromCenter = nodePos - framePos;
            if (fromCenter.x < 0) {
                leftDisplacement += fromCenter.length();
                leftCount++;
            } else {
                rightDisplacement += fromCenter.length();
                rightCount++;
            }
        }
        
        // Create rotation toward the more displaced side
        float imbalanceRotation = 0.0f;
        if (leftCount > 0 && rightCount > 0) {
            float leftAvg = leftDisplacement / leftCount;
            float rightAvg = rightDisplacement / rightCount;
            imbalanceRotation = (rightAvg - leftAvg) * 0.05f; // Strong rotation for imbalance
        }
        
        return imbalanceRotation;
    }
    
    // Get beam entities for this tetrimino
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    float totalTorque = 0.0f;
    int beamCount = 0;
    
    for (auto* beamEntity : beamEntities) {
        std::string beamName = beamEntity->getName();
        
        // Check if this beam belongs to the tetrimino
        if (beamName.find(tetriminoPrefix) != std::string::npos) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (!beam) continue;
            
            // Get beam endpoints
            auto* node1 = beam->getNode1()->getComponent<NodeComponent>();
            auto* node2 = beam->getNode2()->getComponent<NodeComponent>();
            if (!node1 || !node2) continue;
            
            Vec2 pos1 = node1->getPosition();
            Vec2 pos2 = node2->getPosition();
            
            // Calculate beam stress (how much it's stretched/compressed)
            float restLength = beam->getRestLength();
            float currentLength = (pos2 - pos1).length();
            float stress = (currentLength - restLength) / restLength;
            
            // Calculate beam direction from center of mass
            Vec2 beamCenter = (pos1 + pos2) * 0.5f;
            Vec2 beamDirection = (pos2 - pos1).normalized();
            Vec2 fromCenter = beamCenter - centerOfMass;
            
            // Calculate torque based on stress and position
            // Beams under tension (positive stress) want to pull the frame toward them
            // Beams under compression (negative stress) want to push the frame away
            float torque = stress * fromCenter.length() * 0.1f; // Increased scale factor
            
            // Determine rotation direction based on beam position relative to center
            float crossProduct = fromCenter.x * beamDirection.y - fromCenter.y * beamDirection.x;
            if (crossProduct != 0.0f) {
                torque *= (crossProduct > 0.0f ? 1.0f : -1.0f);
            }
            
            totalTorque += torque;
            beamCount++;
        }
    }
    
    if (beamCount == 0) return 0.0f;
    
    // Average the torque and apply stronger influence
    float averageTorque = totalTorque / beamCount;
    return averageTorque * 0.5f; // Much stronger rotation influence
}

float PhysicsTetrisScene::calculateSpringForceImbalance(const std::string& tetriminoPrefix) {
    // Get beam entities for this tetrimino
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    float totalStress = 0.0f;
    float maxStress = 0.0f;
    int beamCount = 0;
    
    for (auto* beamEntity : beamEntities) {
        std::string beamName = beamEntity->getName();
        
        // Check if this beam belongs to the tetrimino
        if (beamName.find(tetriminoPrefix) != std::string::npos) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (!beam) continue;
            
            // Get beam stress factor
            float stress = beam->getStressFactor();
            totalStress += stress;
            maxStress = std::max(maxStress, stress);
            beamCount++;
        }
    }
    
    if (beamCount == 0) return 0.0f;
    
    // Calculate imbalance based on average stress and maximum stress
    float averageStress = totalStress / beamCount;
    float imbalance = averageStress + (maxStress - averageStress) * 0.5f; // Weight toward max stress
    
    return std::min(imbalance, 1.0f); // Clamp to 0-1 range
}

float PhysicsTetrisScene::calculateSimpleSpringRotation(const std::string& tetriminoPrefix) {
    // Get beam entities for this tetrimino
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    float leftForce = 0.0f, rightForce = 0.0f;
    int leftCount = 0, rightCount = 0;
    
    for (auto* beamEntity : beamEntities) {
        std::string beamName = beamEntity->getName();
        
        // Check if this beam belongs to the tetrimino
        if (beamName.find(tetriminoPrefix) != std::string::npos) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (!beam) continue;
            
            // Get beam stress
            float stress = beam->getStressFactor();
            
            // Get beam center position to determine left vs right
            Vec2 beamCenter = beam->getCenterPosition();
            
            // Simple left/right classification based on beam center
            if (beamCenter.x < 0) { // Left side
                leftForce += stress;
                leftCount++;
            } else { // Right side
                rightForce += stress;
                rightCount++;
            }
        }
    }
    
    if (leftCount == 0 || rightCount == 0) return 0.0f;
    
    // Calculate average force on each side
    float leftAvg = leftForce / leftCount;
    float rightAvg = rightForce / rightCount;
    
    // Rotate AWAY from the side with more force (opposite direction)
    // If right side has more force, rotate counter-clockwise (negative) - away from right
    // If left side has more force, rotate clockwise (positive) - away from left
    float rotation = (leftAvg - rightAvg) * m_debugSpringRotationStrength; // Debug spring rotation strength
    return rotation;
}

void PhysicsTetrisScene::applyGravityToUnstableTetriminos() {
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    auto frameEntities = m_entityManager->getEntitiesWithComponent<FrameComponent>();
    
    for (auto* frameEntity : frameEntities) {
        auto* frame = frameEntity->getComponent<FrameComponent>();
        if (!frame) continue;
        
        std::string frameName = frameEntity->getName();
        std::string tetriminoPrefix = frameName.substr(0, frameName.find("_Frame"));
        
        // Skip active tetriminos
        bool isActiveTetrimino = (tetriminoPrefix.find("_" + std::to_string(m_currentTetriminoId)) != std::string::npos);
        if (isActiveTetrimino) continue;
        
        // Check if this tetrimino is unstable
        bool isUnstable = false;
        Vec2 framePos = frame->getPosition();
        int nodeCount = 0;
        float totalDisplacement = 0.0f;
        
        // Check displacement of all nodes from their frame positions
        for (auto* nodeEntity : nodeEntities) {
            if (nodeEntity->getName().find(tetriminoPrefix) != std::string::npos &&
                nodeEntity->getName().find("_Node") != std::string::npos) {
                
                auto* node = nodeEntity->getComponent<NodeComponent>();
                if (node && !node->isPositionFixed()) {
                    Vec2 nodePos = node->getPosition();
                    float displacement = (nodePos - framePos).length();
                    totalDisplacement += displacement;
                    nodeCount++;
                    
                    // If any node is significantly displaced, mark as unstable
                    if (displacement > 20.0f) {
                        isUnstable = true;
                    }
                }
            }
        }
        
        // If unstable, apply additional gravity to all nodes
        if (isUnstable && nodeCount > 0) {
            for (auto* nodeEntity : nodeEntities) {
                if (nodeEntity->getName().find(tetriminoPrefix) != std::string::npos &&
                    nodeEntity->getName().find("_Node") != std::string::npos) {
                    
                    auto* node = nodeEntity->getComponent<NodeComponent>();
                    if (node && !node->isPositionFixed()) {
                        Vec2 velocity = node->getVelocity();
                        // Apply additional downward gravity to unstable pieces
                        velocity.y -= 150.0f * 0.016f; // Additional gravity force
                        node->setVelocity(velocity);
                    }
                }
            }
        }
    }
}

float PhysicsTetrisScene::calculateTotalSpringForce(const std::string& tetriminoPrefix) {
    float totalStress = 0.0f;
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();

    for (auto* beamEntity : beamEntities) {
        std::string beamName = beamEntity->getName();
        if (beamName.find(tetriminoPrefix) != std::string::npos) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (beam) {
                totalStress += std::abs(beam->getStressFactor()); // Sum absolute stress
            }
        }
    }
    return totalStress;
}

void PhysicsTetrisScene::renderDebugTetrimino(const std::string& tetriminoPrefix, const Vec2& screenPosition, float scale) {
    // Get the tetrimino template data
    std::string tetriminoType = tetriminoPrefix.substr(0, tetriminoPrefix.find("_"));
    const TetriminoData* tetriminoData = nullptr;
    
    for (const auto& tetrimino : m_tetriminoTemplates) {
        if (tetrimino.name == tetriminoType) {
            tetriminoData = &tetrimino;
            break;
        }
    }
    
    if (!tetriminoData) return;

    // Render nodes from template data
    for (size_t i = 0; i < tetriminoData->nodeOffsets.size(); ++i) {
        Vec2 nodePos = screenPosition + tetriminoData->nodeOffsets[i] * scale * 0.01f; // Scale down for screen space
        
        // Color based on tetrimino type
        Vec4 nodeColor = tetriminoData->color;
        nodeColor.w = 0.8f; // Make slightly transparent
        
        // Draw node as a small circle
        m_lineRenderer->addCircle(nodePos, 0.005f, nodeColor, 1.0f, 8);
    }

    // Render beams from template data
    for (const auto& connection : tetriminoData->beamConnections) {
        int node1Idx = connection.first;
        int node2Idx = connection.second;
        
        if (node1Idx < tetriminoData->nodeOffsets.size() && node2Idx < tetriminoData->nodeOffsets.size()) {
            Vec2 pos1 = screenPosition + tetriminoData->nodeOffsets[node1Idx] * scale * 0.01f;
            Vec2 pos2 = screenPosition + tetriminoData->nodeOffsets[node2Idx] * scale * 0.01f;
            
            // Color based on tetrimino type
            Vec4 beamColor = tetriminoData->color;
            beamColor.w = 0.6f; // Make more transparent
            
            m_lineRenderer->addLine(pos1, pos2, beamColor, 1.0f);
        }
    }
}

void PhysicsTetrisScene::createDebugControls() {
    // Always create the controls, but they'll be hidden initially
    
    // Frame Gravity Controls
    auto& frameGravityUpEntity = m_entityManager->createEntity("FrameGravityUp");
    auto& frameGravityUpButton = frameGravityUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Frame Gravity +",
        14.0f, 8.0f, 6.0f
    );
    frameGravityUpButton.enableScreenSpace(true);
    frameGravityUpButton.setScreenPosition(0.1f, 0.1f);
    frameGravityUpButton.setVisible(false); // Initially hidden
    frameGravityUpButton.setOnClickCallback([this]() {
        m_debugFrameGravity = std::min(1.0f, m_debugFrameGravity + 0.05f);
        std::cout << "Frame Gravity: " << m_debugFrameGravity << std::endl;
    });
    
    auto& frameGravityDownEntity = m_entityManager->createEntity("FrameGravityDown");
    auto& frameGravityDownButton = frameGravityDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Frame Gravity -",
        14.0f, 8.0f, 6.0f
    );
    frameGravityDownButton.enableScreenSpace(true);
    frameGravityDownButton.setScreenPosition(0.1f, 0.15f);
    frameGravityDownButton.setVisible(false); // Initially hidden
    frameGravityDownButton.setOnClickCallback([this]() {
        m_debugFrameGravity = std::max(-1.0f, m_debugFrameGravity - 0.05f);
        std::cout << "Frame Gravity: " << m_debugFrameGravity << std::endl;
    });
    
    // Frame Gravity Fine Tuning
    auto& frameGravityFineUpEntity = m_entityManager->createEntity("FrameGravityFineUp");
    auto& frameGravityFineUpButton = frameGravityFineUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"++",
        12.0f, 6.0f, 4.0f
    );
    frameGravityFineUpButton.enableScreenSpace(true);
    frameGravityFineUpButton.setScreenPosition(0.25f, 0.1f);
    frameGravityFineUpButton.setVisible(false);
    frameGravityFineUpButton.setOnClickCallback([this]() {
        m_debugFrameGravity = std::min(1.0f, m_debugFrameGravity + 0.001f);
        std::cout << "Frame Gravity: " << m_debugFrameGravity << std::endl;
    });
    
    auto& frameGravityFineDownEntity = m_entityManager->createEntity("FrameGravityFineDown");
    auto& frameGravityFineDownButton = frameGravityFineDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"--",
        12.0f, 6.0f, 4.0f
    );
    frameGravityFineDownButton.enableScreenSpace(true);
    frameGravityFineDownButton.setScreenPosition(0.25f, 0.15f);
    frameGravityFineDownButton.setVisible(false);
    frameGravityFineDownButton.setOnClickCallback([this]() {
        m_debugFrameGravity = std::max(-1.0f, m_debugFrameGravity - 0.001f);
        std::cout << "Frame Gravity: " << m_debugFrameGravity << std::endl;
    });
    
    // Frame Gravity Text Label
    auto& frameGravityLabelEntity = m_entityManager->createEntity("FrameGravityLabel");
    auto& frameGravityLabel = frameGravityLabelEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Frame Gravity: 0.100",
        12.0f, 0.0f, 0.0f
    );
    frameGravityLabel.enableScreenSpace(true);
    frameGravityLabel.setScreenPosition(0.35f, 0.125f);
    frameGravityLabel.setVisible(false);
    frameGravityLabel.setNormalTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
    frameGravityLabel.setHoveredTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    frameGravityLabel.setPressedTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    frameGravityLabel.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Node Gravity When Colliding Controls
    auto& nodeGravityUpEntity = m_entityManager->createEntity("NodeGravityUp");
    auto& nodeGravityUpButton = nodeGravityUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Node Gravity +",
        14.0f, 8.0f, 6.0f
    );
    nodeGravityUpButton.enableScreenSpace(true);
    nodeGravityUpButton.setScreenPosition(0.1f, 0.25f);
    nodeGravityUpButton.setVisible(false); // Initially hidden
    nodeGravityUpButton.setOnClickCallback([this]() {
        m_debugNodeGravityColliding = std::min(1.0f, m_debugNodeGravityColliding + 0.05f);
        std::cout << "Node Gravity (Colliding): " << m_debugNodeGravityColliding << std::endl;
    });
    
    auto& nodeGravityDownEntity = m_entityManager->createEntity("NodeGravityDown");
    auto& nodeGravityDownButton = nodeGravityDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Node Gravity -",
        14.0f, 8.0f, 6.0f
    );
    nodeGravityDownButton.enableScreenSpace(true);
    nodeGravityDownButton.setScreenPosition(0.1f, 0.3f);
    nodeGravityDownButton.setVisible(false); // Initially hidden
    nodeGravityDownButton.setOnClickCallback([this]() {
        m_debugNodeGravityColliding = std::max(-1.0f, m_debugNodeGravityColliding - 0.05f);
        std::cout << "Node Gravity (Colliding): " << m_debugNodeGravityColliding << std::endl;
    });
    
    // Node Gravity Fine Tuning
    auto& nodeGravityFineUpEntity = m_entityManager->createEntity("NodeGravityFineUp");
    auto& nodeGravityFineUpButton = nodeGravityFineUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"++",
        12.0f, 6.0f, 4.0f
    );
    nodeGravityFineUpButton.enableScreenSpace(true);
    nodeGravityFineUpButton.setScreenPosition(0.25f, 0.25f);
    nodeGravityFineUpButton.setVisible(false);
    nodeGravityFineUpButton.setOnClickCallback([this]() {
        m_debugNodeGravityColliding = std::min(1.0f, m_debugNodeGravityColliding + 0.001f);
        std::cout << "Node Gravity (Colliding): " << m_debugNodeGravityColliding << std::endl;
    });
    
    auto& nodeGravityFineDownEntity = m_entityManager->createEntity("NodeGravityFineDown");
    auto& nodeGravityFineDownButton = nodeGravityFineDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"--",
        12.0f, 6.0f, 4.0f
    );
    nodeGravityFineDownButton.enableScreenSpace(true);
    nodeGravityFineDownButton.setScreenPosition(0.25f, 0.3f);
    nodeGravityFineDownButton.setVisible(false);
    nodeGravityFineDownButton.setOnClickCallback([this]() {
        m_debugNodeGravityColliding = std::max(-1.0f, m_debugNodeGravityColliding - 0.001f);
        std::cout << "Node Gravity (Colliding): " << m_debugNodeGravityColliding << std::endl;
    });
    
    // Node Gravity Text Label
    auto& nodeGravityLabelEntity = m_entityManager->createEntity("NodeGravityLabel");
    auto& nodeGravityLabel = nodeGravityLabelEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Node Gravity: 0.200",
        12.0f, 0.0f, 0.0f
    );
    nodeGravityLabel.enableScreenSpace(true);
    nodeGravityLabel.setScreenPosition(0.35f, 0.275f);
    nodeGravityLabel.setVisible(false);
    nodeGravityLabel.setNormalTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    nodeGravityLabel.setHoveredTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    nodeGravityLabel.setPressedTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    nodeGravityLabel.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Spring Rotation Strength Controls
    auto& springRotationUpEntity = m_entityManager->createEntity("SpringRotationUp");
    auto& springRotationUpButton = springRotationUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Spring Rotation +",
        14.0f, 8.0f, 6.0f
    );
    springRotationUpButton.enableScreenSpace(true);
    springRotationUpButton.setScreenPosition(0.1f, 0.4f);
    springRotationUpButton.setVisible(false); // Initially hidden
    springRotationUpButton.setOnClickCallback([this]() {
        m_debugSpringRotationStrength = std::min(2.0f, m_debugSpringRotationStrength + 0.1f);
        std::cout << "Spring Rotation Strength: " << m_debugSpringRotationStrength << std::endl;
    });
    
    auto& springRotationDownEntity = m_entityManager->createEntity("SpringRotationDown");
    auto& springRotationDownButton = springRotationDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Spring Rotation -",
        14.0f, 8.0f, 6.0f
    );
    springRotationDownButton.enableScreenSpace(true);
    springRotationDownButton.setScreenPosition(0.1f, 0.45f);
    springRotationDownButton.setVisible(false); // Initially hidden
    springRotationDownButton.setOnClickCallback([this]() {
        m_debugSpringRotationStrength = std::max(-2.0f, m_debugSpringRotationStrength - 0.1f);
        std::cout << "Spring Rotation Strength: " << m_debugSpringRotationStrength << std::endl;
    });
    
    // Spring Rotation Fine Tuning
    auto& springRotationFineUpEntity = m_entityManager->createEntity("SpringRotationFineUp");
    auto& springRotationFineUpButton = springRotationFineUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"++",
        12.0f, 6.0f, 4.0f
    );
    springRotationFineUpButton.enableScreenSpace(true);
    springRotationFineUpButton.setScreenPosition(0.25f, 0.4f);
    springRotationFineUpButton.setVisible(false);
    springRotationFineUpButton.setOnClickCallback([this]() {
        m_debugSpringRotationStrength = std::min(2.0f, m_debugSpringRotationStrength + 0.001f);
        std::cout << "Spring Rotation Strength: " << m_debugSpringRotationStrength << std::endl;
    });
    
    auto& springRotationFineDownEntity = m_entityManager->createEntity("SpringRotationFineDown");
    auto& springRotationFineDownButton = springRotationFineDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"--",
        12.0f, 6.0f, 4.0f
    );
    springRotationFineDownButton.enableScreenSpace(true);
    springRotationFineDownButton.setScreenPosition(0.25f, 0.45f);
    springRotationFineDownButton.setVisible(false);
    springRotationFineDownButton.setOnClickCallback([this]() {
        m_debugSpringRotationStrength = std::max(-2.0f, m_debugSpringRotationStrength - 0.001f);
        std::cout << "Spring Rotation Strength: " << m_debugSpringRotationStrength << std::endl;
    });
    
    // Spring Rotation Text Label
    auto& springRotationLabelEntity = m_entityManager->createEntity("SpringRotationLabel");
    auto& springRotationLabel = springRotationLabelEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Spring Rotation: 0.500",
        12.0f, 0.0f, 0.0f
    );
    springRotationLabel.enableScreenSpace(true);
    springRotationLabel.setScreenPosition(0.35f, 0.425f);
    springRotationLabel.setVisible(false);
    springRotationLabel.setNormalTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    springRotationLabel.setHoveredTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    springRotationLabel.setPressedTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    springRotationLabel.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Angular Damping Controls
    auto& angularDampingUpEntity = m_entityManager->createEntity("AngularDampingUp");
    auto& angularDampingUpButton = angularDampingUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Angular Damping +",
        14.0f, 8.0f, 6.0f
    );
    angularDampingUpButton.enableScreenSpace(true);
    angularDampingUpButton.setScreenPosition(0.1f, 0.55f);
    angularDampingUpButton.setVisible(false); // Initially hidden
    angularDampingUpButton.setOnClickCallback([this]() {
        m_debugAngularDamping = std::min(1.0f, m_debugAngularDamping + 0.01f);
        std::cout << "Angular Damping: " << m_debugAngularDamping << std::endl;
    });
    
    auto& angularDampingDownEntity = m_entityManager->createEntity("AngularDampingDown");
    auto& angularDampingDownButton = angularDampingDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Angular Damping -",
        14.0f, 8.0f, 6.0f
    );
    angularDampingDownButton.enableScreenSpace(true);
    angularDampingDownButton.setScreenPosition(0.1f, 0.6f);
    angularDampingDownButton.setVisible(false); // Initially hidden
    angularDampingDownButton.setOnClickCallback([this]() {
        m_debugAngularDamping = std::max(0.5f, m_debugAngularDamping - 0.01f);
        std::cout << "Angular Damping: " << m_debugAngularDamping << std::endl;
    });
    
    // Angular Damping Fine Tuning
    auto& angularDampingFineUpEntity = m_entityManager->createEntity("AngularDampingFineUp");
    auto& angularDampingFineUpButton = angularDampingFineUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"++",
        12.0f, 6.0f, 4.0f
    );
    angularDampingFineUpButton.enableScreenSpace(true);
    angularDampingFineUpButton.setScreenPosition(0.25f, 0.55f);
    angularDampingFineUpButton.setVisible(false);
    angularDampingFineUpButton.setOnClickCallback([this]() {
        m_debugAngularDamping = std::min(1.0f, m_debugAngularDamping + 0.001f);
        std::cout << "Angular Damping: " << m_debugAngularDamping << std::endl;
    });
    
    auto& angularDampingFineDownEntity = m_entityManager->createEntity("AngularDampingFineDown");
    auto& angularDampingFineDownButton = angularDampingFineDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"--",
        12.0f, 6.0f, 4.0f
    );
    angularDampingFineDownButton.enableScreenSpace(true);
    angularDampingFineDownButton.setScreenPosition(0.25f, 0.6f);
    angularDampingFineDownButton.setVisible(false);
    angularDampingFineDownButton.setOnClickCallback([this]() {
        m_debugAngularDamping = std::max(0.5f, m_debugAngularDamping - 0.001f);
        std::cout << "Angular Damping: " << m_debugAngularDamping << std::endl;
    });
    
    // Angular Damping Text Label
    auto& angularDampingLabelEntity = m_entityManager->createEntity("AngularDampingLabel");
    auto& angularDampingLabel = angularDampingLabelEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Angular Damping: 0.950",
        12.0f, 0.0f, 0.0f
    );
    angularDampingLabel.enableScreenSpace(true);
    angularDampingLabel.setScreenPosition(0.35f, 0.575f);
    angularDampingLabel.setVisible(false);
    angularDampingLabel.setNormalTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    angularDampingLabel.setHoveredTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    angularDampingLabel.setPressedTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    angularDampingLabel.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // Collision Threshold Controls
    auto& collisionThresholdUpEntity = m_entityManager->createEntity("CollisionThresholdUp");
    auto& collisionThresholdUpButton = collisionThresholdUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Collision Threshold +",
        14.0f, 8.0f, 6.0f
    );
    collisionThresholdUpButton.enableScreenSpace(true);
    collisionThresholdUpButton.setScreenPosition(0.1f, 0.7f);
    collisionThresholdUpButton.setVisible(false); // Initially hidden
    collisionThresholdUpButton.setOnClickCallback([this]() {
        m_debugCollisionThreshold = std::min(2.0f, m_debugCollisionThreshold + 0.1f);
        std::cout << "Collision Threshold: " << m_debugCollisionThreshold << std::endl;
    });
    
    auto& collisionThresholdDownEntity = m_entityManager->createEntity("CollisionThresholdDown");
    auto& collisionThresholdDownButton = collisionThresholdDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Collision Threshold -",
        14.0f, 8.0f, 6.0f
    );
    collisionThresholdDownButton.enableScreenSpace(true);
    collisionThresholdDownButton.setScreenPosition(0.1f, 0.75f);
    collisionThresholdDownButton.setVisible(false); // Initially hidden
    collisionThresholdDownButton.setOnClickCallback([this]() {
        m_debugCollisionThreshold = std::max(0.1f, m_debugCollisionThreshold - 0.1f);
        std::cout << "Collision Threshold: " << m_debugCollisionThreshold << std::endl;
    });
    
    // Collision Threshold Fine Tuning
    auto& collisionThresholdFineUpEntity = m_entityManager->createEntity("CollisionThresholdFineUp");
    auto& collisionThresholdFineUpButton = collisionThresholdFineUpEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"++",
        12.0f, 6.0f, 4.0f
    );
    collisionThresholdFineUpButton.enableScreenSpace(true);
    collisionThresholdFineUpButton.setScreenPosition(0.25f, 0.7f);
    collisionThresholdFineUpButton.setVisible(false);
    collisionThresholdFineUpButton.setOnClickCallback([this]() {
        m_debugCollisionThreshold = std::min(2.0f, m_debugCollisionThreshold + 0.001f);
        std::cout << "Collision Threshold: " << m_debugCollisionThreshold << std::endl;
    });
    
    auto& collisionThresholdFineDownEntity = m_entityManager->createEntity("CollisionThresholdFineDown");
    auto& collisionThresholdFineDownButton = collisionThresholdFineDownEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"--",
        12.0f, 6.0f, 4.0f
    );
    collisionThresholdFineDownButton.enableScreenSpace(true);
    collisionThresholdFineDownButton.setScreenPosition(0.25f, 0.75f);
    collisionThresholdFineDownButton.setVisible(false);
    collisionThresholdFineDownButton.setOnClickCallback([this]() {
        m_debugCollisionThreshold = std::max(0.1f, m_debugCollisionThreshold - 0.001f);
        std::cout << "Collision Threshold: " << m_debugCollisionThreshold << std::endl;
    });
    
    // Collision Threshold Text Label
    auto& collisionThresholdLabelEntity = m_entityManager->createEntity("CollisionThresholdLabel");
    auto& collisionThresholdLabel = collisionThresholdLabelEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Collision Threshold: 0.800",
        12.0f, 0.0f, 0.0f
    );
    collisionThresholdLabel.enableScreenSpace(true);
    collisionThresholdLabel.setScreenPosition(0.35f, 0.725f);
    collisionThresholdLabel.setVisible(false);
    collisionThresholdLabel.setNormalTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    collisionThresholdLabel.setHoveredTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    collisionThresholdLabel.setPressedTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    collisionThresholdLabel.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void PhysicsTetrisScene::updateDebugControls() {
    // Show/hide debug controls based on debug toggle
    bool showControls = m_showFrameDebug;
    
    std::vector<std::string> controlNames = {
        "FrameGravityUp", "FrameGravityDown", "FrameGravityFineUp", "FrameGravityFineDown", "FrameGravityLabel",
        "NodeGravityUp", "NodeGravityDown", "NodeGravityFineUp", "NodeGravityFineDown", "NodeGravityLabel",
        "SpringRotationUp", "SpringRotationDown", "SpringRotationFineUp", "SpringRotationFineDown", "SpringRotationLabel",
        "AngularDampingUp", "AngularDampingDown", "AngularDampingFineUp", "AngularDampingFineDown", "AngularDampingLabel",
        "CollisionThresholdUp", "CollisionThresholdDown", "CollisionThresholdFineUp", "CollisionThresholdFineDown", "CollisionThresholdLabel"
    };
    
    for (const auto& name : controlNames) {
        if (auto* entity = m_entityManager->findEntity(name)) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                button->setVisible(showControls);
                if (showControls) {
                    button->update(0.016f); // Update button
                }
            }
        }
    }
    
    // Update text labels with current values
    if (showControls) {
        // Update Frame Gravity Label
        if (auto* entity = m_entityManager->findEntity("FrameGravityLabel")) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                std::wstring text = L"Frame Gravity: " + std::to_wstring(m_debugFrameGravity).substr(0, 5);
                button->setText(text);
            }
        }
        
        // Update Node Gravity Label
        if (auto* entity = m_entityManager->findEntity("NodeGravityLabel")) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                std::wstring text = L"Node Gravity: " + std::to_wstring(m_debugNodeGravityColliding).substr(0, 5);
                button->setText(text);
            }
        }
        
        // Update Spring Rotation Label
        if (auto* entity = m_entityManager->findEntity("SpringRotationLabel")) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                std::wstring text = L"Spring Rotation: " + std::to_wstring(m_debugSpringRotationStrength).substr(0, 5);
                button->setText(text);
            }
        }
        
        // Update Angular Damping Label
        if (auto* entity = m_entityManager->findEntity("AngularDampingLabel")) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                std::wstring text = L"Angular Damping: " + std::to_wstring(m_debugAngularDamping).substr(0, 5);
                button->setText(text);
            }
        }
        
        // Update Collision Threshold Label
        if (auto* entity = m_entityManager->findEntity("CollisionThresholdLabel")) {
            if (auto* button = entity->getComponent<ButtonComponent>()) {
                std::wstring text = L"Collision Threshold: " + std::to_wstring(m_debugCollisionThreshold).substr(0, 5);
                button->setText(text);
            }
        }
    }
}
