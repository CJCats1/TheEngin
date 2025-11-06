#include <DX3D/Game/Scenes/JellyTetrisReduxScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <iostream>
#include <imgui.h>
#include <cmath>
#include <chrono>

using namespace dx3d;


void JellyTetrisReduxScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();


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
    
    // Initialize game systems
    initializeTetriminoTemplates();
    createPlayField();
}


void JellyTetrisReduxScene::initializeTetriminoTemplates() {
    m_tetriminoTemplates.clear();

    // I-piece: 4 squares in a line
    JellyTetriminoData iPiece;
    iPiece.name = "I";
    iPiece.color = Vec4(0.0f, 0.8f, 1.0f, 1.0f); // Bright Cyan
    iPiece.squarePieces = {
        SquarePiece(Vec2(-37.5f, 0.0f), iPiece.color),
        SquarePiece(Vec2(-12.5f, 0.0f), iPiece.color),
        SquarePiece(Vec2(12.5f, 0.0f), iPiece.color),
        SquarePiece(Vec2(37.5f, 0.0f), iPiece.color)
    };
    m_tetriminoTemplates.push_back(iPiece);

    // O-piece: 2x2 block (4 squares)
    JellyTetriminoData oPiece;
    oPiece.name = "O";
    oPiece.color = Vec4(1.0f, 0.9f, 0.0f, 1.0f); // Bright Yellow
    oPiece.squarePieces = {
        SquarePiece(Vec2(-12.5f, 12.5f), oPiece.color),   // Top left
        SquarePiece(Vec2(12.5f, 12.5f), oPiece.color),    // Top right
        SquarePiece(Vec2(-12.5f, -12.5f), oPiece.color),  // Bottom left
        SquarePiece(Vec2(12.5f, -12.5f), oPiece.color)    // Bottom right
    };
    m_tetriminoTemplates.push_back(oPiece);

    // T-piece: 3 squares in T formation
    JellyTetriminoData tPiece;
    tPiece.name = "T";
    tPiece.color = Vec4(0.8f, 0.0f, 0.8f, 1.0f); // Bright Purple
    tPiece.squarePieces = {
        SquarePiece(Vec2(0.0f, 25.0f), tPiece.color),    // Top
        SquarePiece(Vec2(-25.0f, 0.0f), tPiece.color),   // Left
        SquarePiece(Vec2(0.0f, 0.0f), tPiece.color),     // Center
        SquarePiece(Vec2(25.0f, 0.0f), tPiece.color)     // Right
    };
    m_tetriminoTemplates.push_back(tPiece);

    // S-piece: 4 squares in S formation
    JellyTetriminoData sPiece;
    sPiece.name = "S";
    sPiece.color = Vec4(0.0f, 0.9f, 0.2f, 1.0f); // Bright Green
    sPiece.squarePieces = {
        SquarePiece(Vec2(-25.0f, 25.0f), sPiece.color),  // Top left
        SquarePiece(Vec2(0.0f, 25.0f), sPiece.color),    // Top right
        SquarePiece(Vec2(0.0f, 0.0f), sPiece.color),     // Bottom left
        SquarePiece(Vec2(25.0f, 0.0f), sPiece.color)     // Bottom right
    };
    m_tetriminoTemplates.push_back(sPiece);

    // Z-piece: 4 squares in Z formation
    JellyTetriminoData zPiece;
    zPiece.name = "Z";
    zPiece.color = Vec4(1.0f, 0.2f, 0.2f, 1.0f); // Bright Red
    zPiece.squarePieces = {
        SquarePiece(Vec2(25.0f, 25.0f), zPiece.color),   // Top right
        SquarePiece(Vec2(0.0f, 25.0f), zPiece.color),    // Top left
        SquarePiece(Vec2(0.0f, 0.0f), zPiece.color),     // Bottom right
        SquarePiece(Vec2(-25.0f, 0.0f), zPiece.color)    // Bottom left
    };
    m_tetriminoTemplates.push_back(zPiece);

    // J-piece: 4 squares in J formation
    JellyTetriminoData jPiece;
    jPiece.name = "J";
    jPiece.color = Vec4(0.2f, 0.2f, 1.0f, 1.0f); // Bright Blue
    jPiece.squarePieces = {
        SquarePiece(Vec2(-25.0f, 25.0f), jPiece.color),  // Top left
        SquarePiece(Vec2(-25.0f, 0.0f), jPiece.color),   // Middle left
        SquarePiece(Vec2(0.0f, 0.0f), jPiece.color),     // Center
        SquarePiece(Vec2(25.0f, 0.0f), jPiece.color)     // Right
    };
    m_tetriminoTemplates.push_back(jPiece);

    // L-piece: 4 squares in L formation
    JellyTetriminoData lPiece;
    lPiece.name = "L";
    lPiece.color = Vec4(1.0f, 0.6f, 0.0f, 1.0f); // Bright Orange
    lPiece.squarePieces = {
        SquarePiece(Vec2(25.0f, 25.0f), lPiece.color),   // Top right
        SquarePiece(Vec2(25.0f, 0.0f), lPiece.color),     // Middle right
        SquarePiece(Vec2(0.0f, 0.0f), lPiece.color),      // Center
        SquarePiece(Vec2(-25.0f, 0.0f), lPiece.color)     // Left
    };
    m_tetriminoTemplates.push_back(lPiece);
}


void JellyTetrisReduxScene::createPlayField() {
    auto& device = *m_graphicsDevice;

    // Use test mode width if enabled, otherwise normal width
    float fieldWidth = m_testMode ? PLAY_FIELD_WIDTH * 5.0f : PLAY_FIELD_WIDTH;
    
    // Left boundary marker
    auto& leftMarker = m_entityManager->createEntity("LeftBoundaryMarker");
    auto& leftSprite = leftMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        WALL_THICKNESS, PLAY_FIELD_HEIGHT
    );
    leftSprite.setPosition(-fieldWidth / 2 - WALL_THICKNESS / 2, 0.0f, 0.0f);
    leftSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Right boundary marker  
    auto& rightMarker = m_entityManager->createEntity("RightBoundaryMarker");
    auto& rightSprite = rightMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        WALL_THICKNESS, PLAY_FIELD_HEIGHT
    );
    rightSprite.setPosition(fieldWidth / 2 + WALL_THICKNESS / 2, 0.0f, 0.0f);
    rightSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Bottom boundary marker
    auto& bottomMarker = m_entityManager->createEntity("BottomBoundaryMarker");
    auto& bottomSprite = bottomMarker.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        fieldWidth + 2 * WALL_THICKNESS, WALL_THICKNESS
    );
    bottomSprite.setPosition(0.0f, -PLAY_FIELD_HEIGHT / 2 - WALL_THICKNESS / 2, 0.0f);
    bottomSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));
}

void JellyTetrisReduxScene::spawnTetrimino(TetriminoReduxType type, Vec2 position) {
    if (static_cast<int>(type) >= m_tetriminoTemplates.size()) return;

    const auto& data = m_tetriminoTemplates[static_cast<int>(type)];
    std::string baseName = createTetriminoNodes(data, position, m_nextTetriminoId);
    createTetriminoBeams(data, baseName, m_nextTetriminoId);

    m_nextTetriminoId++;
    
    // Mark spatial grid as dirty when new entities are added
    m_spatialGridDirty = true;
}

std::string JellyTetrisReduxScene::createTetriminoNodes(const JellyTetriminoData& data, Vec2 basePosition, int tetriminoId) {
    std::string baseName = data.name + "_" + std::to_string(tetriminoId);
    
    // Use a string-based key for position mapping to avoid Vec2 comparison issues
    std::map<std::string, std::string> nodePositions; // position string -> nodeName
    int nodeIndex = 0;

    for (size_t squareIndex = 0; squareIndex < data.squarePieces.size(); ++squareIndex) {
        const SquarePiece& square = data.squarePieces[squareIndex];
        Vec2 squareWorldPos = basePosition + square.getCenterPosition();

        // Create nodes for this square piece
        for (size_t i = 0; i < square.getNodeOffsets().size(); ++i) {
            Vec2 nodePos = squareWorldPos + square.getNodeOffsets()[i];
            
            // Create a string key for the position
            std::string posKey = std::to_string(static_cast<int>(nodePos.x)) + "," + 
                                std::to_string(static_cast<int>(nodePos.y));
            
            // Check if a node already exists at this position (from adjacent squares)
            std::string nodeName;
            if (nodePositions.find(posKey) != nodePositions.end()) {
                // Node already exists, reuse it
                nodeName = nodePositions[posKey];
            } else {
                // Create new node
                nodeName = baseName + "_Node" + std::to_string(nodeIndex);
                nodePositions[posKey] = nodeName;
                nodeIndex++;

                auto& nodeEntity = m_entityManager->createEntity(nodeName);
                nodeEntity.addComponent<NodeComponent>(nodePos, false); // Non-fixed so they can fall

                auto& sprite = nodeEntity.addComponent<SpriteComponent>(
                    *m_graphicsDevice,
                    L"DX3D/Assets/Textures/node.png",
                    NODE_SIZE * 0.8f, NODE_SIZE * 0.8f  // Make nodes more visible
                );
                sprite.setPosition(nodePos.x, nodePos.y, 0.0f);
                // Use a more visible color for nodes
                Vec4 nodeColor = square.getColor();
                nodeColor.w = 1.0f; // Make nodes fully opaque
                sprite.setTint(nodeColor);
            }
        }
    }

    return baseName;
}

void JellyTetrisReduxScene::createTetriminoBeams(const JellyTetriminoData& data, const std::string& baseName, int tetriminoId) {
    int beamIndex = 0;
    
    // Create a map to track node positions and their corresponding entity names
    std::map<std::string, std::string> nodePositions; // position string -> nodeName
    Vec2 basePos = Vec2(0, 300); // Same as spawn position
    
    // First, collect all node positions and their names
    int nodeIndex = 0;
    for (size_t squareIndex = 0; squareIndex < data.squarePieces.size(); ++squareIndex) {
        const SquarePiece& square = data.squarePieces[squareIndex];
        Vec2 squareWorldPos = basePos + square.getCenterPosition();

        for (size_t i = 0; i < square.getNodeOffsets().size(); ++i) {
            Vec2 nodePos = squareWorldPos + square.getNodeOffsets()[i];
            
            // Create string key for position
            std::string posKey = std::to_string(static_cast<int>(nodePos.x)) + "," + 
                                std::to_string(static_cast<int>(nodePos.y));
            
            if (nodePositions.find(posKey) == nodePositions.end()) {
                std::string nodeName = baseName + "_Node" + std::to_string(nodeIndex);
                nodePositions[posKey] = nodeName;
                nodeIndex++;
            }
        }
    }

    // Now create beams for each square piece
    for (size_t squareIndex = 0; squareIndex < data.squarePieces.size(); ++squareIndex) {
        const SquarePiece& square = data.squarePieces[squareIndex];
        Vec2 squareWorldPos = basePos + square.getCenterPosition();
        
        for (size_t i = 0; i < square.getBeamConnections().size(); ++i) {
            int node1Idx = square.getBeamConnections()[i].first;
            int node2Idx = square.getBeamConnections()[i].second;

            // Get the actual world positions for these node indices
            Vec2 node1Pos = squareWorldPos + square.getNodeOffsets()[node1Idx];
            Vec2 node2Pos = squareWorldPos + square.getNodeOffsets()[node2Idx];

            // Create string keys for positions
            std::string pos1Key = std::to_string(static_cast<int>(node1Pos.x)) + "," + 
                                 std::to_string(static_cast<int>(node1Pos.y));
            std::string pos2Key = std::to_string(static_cast<int>(node2Pos.x)) + "," + 
                                 std::to_string(static_cast<int>(node2Pos.y));

            // Find the corresponding entity names
            std::string node1Name = nodePositions[pos1Key];
            std::string node2Name = nodePositions[pos2Key];
            std::string beamName = baseName + "_Beam" + std::to_string(beamIndex);

            auto* node1Entity = m_entityManager->findEntity(node1Name);
            auto* node2Entity = m_entityManager->findEntity(node2Name);

            if (!node1Entity || !node2Entity) continue;

            auto& beamEntity = m_entityManager->createEntity(beamName);
            auto& beamComponent = beamEntity.addComponent<BeamComponent>(node1Entity, node2Entity);

            auto& sprite = beamEntity.addComponent<SpriteComponent>(
                *m_graphicsDevice,
                L"DX3D/Assets/Textures/beam.png",
                0.5f, 0.5f  // Make beams smaller and more reasonable
            );

            Vec2 center = beamComponent.getCenterPosition();
            sprite.setPosition(center.x, center.y, 0.0f);
            // Make beams more visible
            Vec4 beamColor = square.getColor();
            beamColor.w = 0.8f; // Make beams more opaque
            sprite.setTint(beamColor);
            beamIndex++;
        }
    }
}


void JellyTetrisReduxScene::update(float dt) {
    // Update FPS counter
    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 1.0f) {
        m_currentFPS = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }
    
    updateCameraMovement(dt);
    
    // Handle node dragging
    updateNodeDragging();
    
    // Handle tetramino input controls
    handleTetraminoInput();
}

void JellyTetrisReduxScene::fixedUpdate(float dt) {
    // Physics step for tetraminos - runs at fixed timestep
    if (m_entityManager) {
        // Clamp delta time to prevent large timesteps that cause explosions
        float clampedDt = std::min(dt, 1.0f / 30.0f); // Max 30 FPS equivalent
        
        // Time physics updates
        auto start = std::chrono::high_resolution_clock::now();
        PhysicsSystem::updateNodes(*m_entityManager, clampedDt);
        PhysicsSystem::updateBeams(*m_entityManager, clampedDt);
        auto physicsEnd = std::chrono::high_resolution_clock::now();
        
        // Add air resistance to reduce wiggling
        addAirResistance();
        
        // Time collision detection
        auto collisionStart = std::chrono::high_resolution_clock::now();
        updateCollisions();
        auto collisionEnd = std::chrono::high_resolution_clock::now();
        
        // Update timing stats
        m_physicsTime = std::chrono::duration<float, std::milli>(physicsEnd - start).count();
        m_collisionTime = std::chrono::duration<float, std::milli>(collisionEnd - collisionStart).count();
    }
}











void JellyTetrisReduxScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    auto& ctx = engine.getContext();

    // Set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

        ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.enableAlphaBlending();

    // Render tetramino visual overlays first (behind nodes)
    renderTetraminoVisualOverlays(ctx);

    // Render beams
    for (auto* entity : m_entityManager->getEntitiesWithComponent<BeamComponent>()) {
        auto* sprite = entity->getComponent<SpriteComponent>();
        if (sprite && sprite->isVisible() && sprite->isValid()) {
            sprite->draw(ctx);
        }
    }

    // Render nodes
    for (auto* entity : m_entityManager->getEntitiesWithComponent<NodeComponent>()) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render other sprites (boundaries, etc.)
    for (auto* entity : m_entityManager->getEntitiesWithComponent<SpriteComponent>()) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid() && 
                !entity->getComponent<BeamComponent>() && 
                !entity->getComponent<NodeComponent>()) {
                sprite->draw(ctx);
            }
        }
    }
}


void JellyTetrisReduxScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::SetNextWindowSize(ImVec2(420, 420), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Jelly Tetris Redux"))
    {
        ImGui::Text("%s", "Tetramino Spawner");
        ImGui::Separator();

        // Controls
        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Tetramino Controls:");
            ImGui::Text("J = Move Left");
            ImGui::Text("K = Move Down");
            ImGui::Text("L = Move Right");
            ImGui::Text("U = Rotate Counter-clockwise");
            ImGui::Text("O = Rotate Clockwise");
            ImGui::Separator();
            
            ImGui::Text("Node Dragging:");
            ImGui::Text("Left Click = Drag individual nodes");
            ImGui::Text("Dragged nodes render on top");
            ImGui::Separator();
            
            ImGui::SliderFloat("Move Speed", &m_tetraminoMoveSpeed, 1.0f, 50.0f, "%.1f");
            ImGui::Text("Base movement speed");
            
            ImGui::SliderFloat("Force Multiplier", &m_tetraminoForceMultiplier, 10.0f, 200.0f, "%.0f");
            ImGui::Text("How strong the movement forces are");
            
            ImGui::Separator();
            ImGui::SliderFloat("Rotation Speed", &m_tetraminoRotationSpeed, 0.5f, 10.0f, "%.1f");
            ImGui::Text("How fast the tetramino rotates");
            
            ImGui::SliderFloat("Rotation Force", &m_tetraminoRotationForceMultiplier, 5.0f, 100.0f, "%.0f");
            ImGui::Text("How strong the rotation forces are");
        }

        // Test Mode Controls
        if (ImGui::CollapsingHeader("Spawn Tetraminos", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Spawn Individual Tetraminos:");
            
            if (ImGui::Button("I-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::I_PIECE);
            if (ImGui::Button("O-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::O_PIECE);
            if (ImGui::Button("T-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::T_PIECE);
            if (ImGui::Button("S-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::S_PIECE);
            if (ImGui::Button("Z-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::Z_PIECE);
            if (ImGui::Button("J-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::J_PIECE);
            if (ImGui::Button("L-Piece", ImVec2(-FLT_MIN, 0))) spawnTestTetramino(TetriminoReduxType::L_PIECE);
            
            ImGui::Separator();
            if (ImGui::Button("Clear All", ImVec2(-FLT_MIN, 0))) clearTestTetraminos();
        }

        // Physics Controls
        if (ImGui::CollapsingHeader("Physics Controls", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Adjust physics parameters in real-time:");
            
            ImGui::SliderFloat("Air Resistance", &m_airResistance, 0.95f, 1.0f, "%.3f");
            ImGui::Text("Higher = more damping, slower falling");
            
            ImGui::Separator();
            ImGui::Text("Collision Settings:");
            
            ImGui::SliderFloat("Collision Bounciness", &m_collisionRestitution, 0.0f, 1.0f, "%.2f");
            ImGui::Text("0 = no bounce, 1 = full bounce");
            
            ImGui::SliderFloat("Collision Damping", &m_collisionDamping, 0.1f, 1.0f, "%.2f");
            ImGui::Text("Lower = more wiggling, Higher = less wiggling");
            
            ImGui::SliderFloat("Collision Speed Threshold", &m_collisionSpeedThreshold, 0.0f, 5.0f, "%.1f");
            ImGui::Text("Higher = fewer micro-collisions");
            
            ImGui::Separator();
            ImGui::Text("Bottom Bounce Settings:");
            
            ImGui::SliderFloat("Bottom Bounce Threshold", &m_bottomBounceThreshold, 1.0f, 20.0f, "%.1f");
            ImGui::Text("Velocity below this stops bouncing");
            
            ImGui::SliderFloat("Bottom Bounce Damping", &m_bottomBounceDamping, 0.0f, 1.0f, "%.2f");
            ImGui::Text("Lower = more bouncing, Higher = less bouncing");

            ImGui::Separator();
            ImGui::Text("Drag Spring Settings:");
            ImGui::SliderFloat("Drag Stiffness", &m_dragSpringStiffness, 10.0f, 200.0f, "%.1f");
            ImGui::SliderFloat("Drag Damping", &m_dragSpringDamping, 1.0f, 40.0f, "%.1f");
            ImGui::SliderFloat("Drag Max Force", &m_dragMaxForce, 100.0f, 3000.0f, "%.0f");
            
            ImGui::Separator();
            ImGui::Text("Collision Optimizations:");
            ImGui::Checkbox("Enable Collisions", &m_enableCollisions);
            ImGui::Text("Same-tetramino collisions are skipped");
            ImGui::Text("Spatial grid reduces collision checks");
        }

        // Status
        if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (m_isDraggingNode) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Dragging Node: %s", m_draggedNode ? m_draggedNode->getName().c_str() : "Unknown");
            } else {
                ImGui::Text("No node being dragged");
            }
            
            // FPS Display
            ImGui::Text("FPS: %.1f", m_currentFPS);
            if (m_currentFPS < 30.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "LOW FPS WARNING!");
            } else if (m_currentFPS < 50.0f) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Moderate FPS");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Good FPS");
            }
            
            // Performance stats
            if (m_entityManager) {
                auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
                auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
                
                ImGui::Separator();
                ImGui::Text("Entity Counts:");
                ImGui::Text("Beams: %zu", beamEntities.size());
                ImGui::Text("Nodes: %zu", nodeEntities.size());
                ImGui::Text("Spatial Cells: %zu", m_spatialGrid.size());
                ImGui::Text("Grid Dirty: %s", m_spatialGridDirty ? "Yes" : "No");
                
                // Calculate theoretical collision checks
                size_t totalBeams = beamEntities.size();
                size_t naiveChecks = totalBeams * (totalBeams - 1) / 2;
                size_t spatialChecks = 0;
                for (const auto& [key, cell] : m_spatialGrid) {
                    size_t cellBeams = cell.beams.size();
                    spatialChecks += cellBeams * (cellBeams - 1) / 2;
                }
                
                ImGui::Separator();
                ImGui::Text("Collision Performance:");
                ImGui::Text("Naive Checks: %zu", naiveChecks);
                ImGui::Text("Spatial Checks: %zu", spatialChecks);
                if (naiveChecks > 0) {
                    float reduction = 100.0f * (1.0f - (float)spatialChecks / (float)naiveChecks);
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Reduction: %.1f%%", reduction);
                }
                
                // Show grid distribution
                ImGui::Separator();
                ImGui::Text("Grid Distribution:");
                int maxBeamsInCell = 0;
                int totalCells = 0;
                for (const auto& [key, cell] : m_spatialGrid) {
                    if (!cell.beams.empty()) {
                        totalCells++;
                        maxBeamsInCell = std::max(maxBeamsInCell, static_cast<int>(cell.beams.size()));
                    }
                }
                ImGui::Text("Active Cells: %d", totalCells);
                ImGui::Text("Max Beams/Cell: %d", maxBeamsInCell);
                
                // Performance timing
                ImGui::Separator();
                ImGui::Text("Timing (ms):");
                ImGui::Text("Physics: %.2f", m_physicsTime);
                ImGui::Text("Collisions: %.2f", m_collisionTime);
                ImGui::Text("Dragging: %.2f", m_dragTime);
                
                float totalTime = m_physicsTime + m_collisionTime + m_dragTime;
                ImGui::Text("Total: %.2f", totalTime);
                
                if (totalTime > 16.67f) { // More than 60 FPS worth
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "BOTTLENECK DETECTED!");
                } else {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Performance: Good");
                }
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
    }
    ImGui::End();
}








void JellyTetrisReduxScene::spawnTestTetramino(TetriminoReduxType type) {
    // Find a good spawn position (center of field, higher up)
    float spawnX = 0.0f; // Center of the field
    float spawnY = 250.0f; // Higher up for better visibility
    
    // Add some random offset to avoid overlapping
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-50.0, 50.0);
    spawnX += dis(gen);
    spawnY += dis(gen);
    
    spawnTetrimino(type, Vec2(spawnX, spawnY));
    std::cout << "Spawned " << m_tetriminoTemplates[static_cast<int>(type)].name << " piece" << std::endl;
}

void JellyTetrisReduxScene::clearTestTetraminos() {
    // Clear all tetramino entities
    const auto& allEntities = m_entityManager->getEntities();
    std::vector<std::string> entitiesToRemove;
    
    for (const auto& entityPtr : allEntities) {
        Entity* entity = entityPtr.get();
        std::string name = entity->getName();
        if (name.find("_Node") != std::string::npos || 
            name.find("_Beam") != std::string::npos) {
            entitiesToRemove.push_back(name);
        }
    }
    
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }
    
    // Reset tetrimino ID counter
    m_nextTetriminoId = 0;
    
    // Mark spatial grid as dirty when entities are removed
    m_spatialGridDirty = true;
    
    std::cout << "Cleared all test tetraminos" << std::endl;
}

void JellyTetrisReduxScene::toggleTestMode() {
    // Simple toggle for test mode
    m_testMode = !m_testMode;
    std::cout << "Test mode: " << (m_testMode ? "enabled" : "disabled") << std::endl;
}


void JellyTetrisReduxScene::updateCameraMovement(float dt) {
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

    // Camera movement
        if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt;
        if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt;
        if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt;
        if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt;

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        camera->move(moveDelta);
    }

    // Zoom controls
    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::Q)) zoomDelta -= zoomSpeed * dt;
    if (input.isKeyDown(Key::E)) zoomDelta += zoomSpeed * dt;
    if (zoomDelta != 0.0f) camera->zoom(zoomDelta);

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(DEFAULT_CAMERA_ZOOM);
    }
}





void JellyTetrisReduxScene::renderTetraminoVisualOverlays(DeviceContext& ctx) {
    // Group nodes by tetramino and render visual overlays
    std::map<std::string, std::vector<Entity*>> tetraminoNodes;
    
    // Group nodes by tetramino prefix
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    for (auto* nodeEntity : nodeEntities) {
        std::string nodeName = nodeEntity->getName();
        
        // Extract tetramino prefix (e.g., "I_0" from "I_0_Node0")
        size_t lastUnderscore = nodeName.find_last_of('_');
        if (lastUnderscore != std::string::npos) {
            std::string prefix = nodeName.substr(0, lastUnderscore);
            tetraminoNodes[prefix].push_back(nodeEntity);
        }
    }
    
    // Render visual overlay for each tetramino
    for (const auto& [prefix, nodes] : tetraminoNodes) {
        if (nodes.empty()) continue;
        
        // Get the first node's color to determine tetramino color
        auto* firstNode = nodes[0]->getComponent<NodeComponent>();
        if (!firstNode) continue;
        
        // Find the corresponding sprite to get color
        auto* firstSprite = nodes[0]->getComponent<SpriteComponent>();
        if (!firstSprite) continue;
        
        Vec4 tetraminoColor = firstSprite->getTint();
        
        // Render individual square overlays for all tetraminos
        renderIndividualSquareOverlays(ctx, nodes, tetraminoColor);
    }
}



void JellyTetrisReduxScene::renderIndividualSquareOverlays(DeviceContext& ctx, const std::vector<Entity*>& nodes, const Vec4& color) {
    // Simple placeholder - no visual overlays for now
    // This method is kept for interface compatibility but does nothing
}

void JellyTetrisReduxScene::renderIndividualSquare(DeviceContext& ctx, const Vec2& minPos, const Vec2& maxPos, const Vec4& color) {
    // Simple placeholder - no visual overlays for now
    // This method is kept for interface compatibility but does nothing
}

void JellyTetrisReduxScene::updateCollisions() {
    checkBoundaryCollisions();
    
    if (!m_enableCollisions) return;
    
    // Update spatial grid for broadphase collision detection
    updateSpatialGrid();
    
    // Use broadphase to check collisions more efficiently
    for (const auto& [cellKey, cell] : m_spatialGrid) {
        checkCollisionsInCell(cellKey);
    }
}

void JellyTetrisReduxScene::checkBoundaryCollisions() {
    if (!m_entityManager) return;
    
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();

    for (auto* beamEntity : beamEntities) {
        auto* beam = beamEntity->getComponent<BeamComponent>();
        if (!beam) continue;

        // Get beam endpoints
        auto* node1 = beam->getNode1Entity()->getComponent<NodeComponent>();
        auto* node2 = beam->getNode2Entity()->getComponent<NodeComponent>();
        if (!node1 || !node2) continue;
        
        // Skip if both nodes are fixed
        if (node1->isPositionFixed() && node2->isPositionFixed()) continue;

        Vec2 pos1 = node1->getPosition();
        Vec2 pos2 = node2->getPosition();
        float beamRadius = NODE_SIZE * 0.2f; // Smaller radius for beams
        
        // Calculate play field boundaries
        float fieldWidth = m_testMode ? PLAY_FIELD_WIDTH * 5.0f : PLAY_FIELD_WIDTH;
        float leftBoundary = -fieldWidth / 2.0f;
        float rightBoundary = fieldWidth / 2.0f;
        float bottomBoundary = -PLAY_FIELD_HEIGHT / 2.0f;
        
        // Check both endpoints for boundary collisions
        for (auto* node : {node1, node2}) {
            if (node->isPositionFixed()) continue;
            
            Vec2 pos = node->getPosition();
            
            // Check left boundary collision
            if (pos.x - beamRadius < leftBoundary) {
                node->setPosition(Vec2(leftBoundary + beamRadius, pos.y));
                Vec2 velocity = node->getVelocity();
                velocity.x = std::abs(velocity.x) * 0.3f; // Bounce with damping
                node->setVelocity(velocity);
            }
            
            // Check right boundary collision
            if (pos.x + beamRadius > rightBoundary) {
                node->setPosition(Vec2(rightBoundary - beamRadius, pos.y));
                Vec2 velocity = node->getVelocity();
                velocity.x = -std::abs(velocity.x) * 0.3f; // Bounce with damping
                node->setVelocity(velocity);
            }
            
            // Check bottom boundary collision
            if (pos.y - beamRadius < bottomBoundary) {
                node->setPosition(Vec2(pos.x, bottomBoundary + beamRadius));
                Vec2 velocity = node->getVelocity();
                
                // If velocity is very small, stop bouncing to prevent jitter
                if (std::abs(velocity.y) < m_bottomBounceThreshold) {
                    velocity.y = 0.0f; // Stop vertical movement
                } else {
                    velocity.y = std::abs(velocity.y) * m_bottomBounceDamping; // Much more damping
                }
                
                // Add horizontal damping to reduce wiggling
                velocity.x *= 0.7f;
                
                node->setVelocity(velocity);
            }
        }
    }
}

void JellyTetrisReduxScene::addAirResistance() {
    if (!m_entityManager) return;
    
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        Vec2 velocity = node->getVelocity();
        
        // Apply air resistance (damping)
        velocity *= m_airResistance;
        
        // Stop very small movements to prevent micro-wiggling
        if (velocity.length() < 0.1f) {
            velocity = Vec2(0.0f, 0.0f);
        }
        
            node->setVelocity(velocity);
    }
}

void JellyTetrisReduxScene::handleTetraminoInput() {
    if (!m_entityManager) return;
    
    auto& input = Input::getInstance();
    float moveSpeed = m_tetraminoMoveSpeed; // Speed for tetramino movement
    
    // Get all nodes and find the most recent tetramino ID
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    std::vector<Entity*> tetraminoNodes;
    
    int highestId = -1;
    
    // First pass: find the highest tetramino ID
    for (auto* nodeEntity : nodeEntities) {
        std::string nodeName = nodeEntity->getName();
        
        // Extract tetramino ID from node name (e.g., "I_5_Node0" -> ID 5)
        size_t firstUnderscore = nodeName.find('_');
        if (firstUnderscore != std::string::npos) {
            size_t secondUnderscore = nodeName.find('_', firstUnderscore + 1);
            if (secondUnderscore != std::string::npos) {
                std::string idStr = nodeName.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
                int tetraminoId = std::stoi(idStr);
                
                if (tetraminoId > highestId) {
                    highestId = tetraminoId;
                }
            }
        }
    }
    
    if (highestId == -1) return;
    
    // Second pass: collect all nodes with the highest ID
    for (auto* nodeEntity : nodeEntities) {
        std::string nodeName = nodeEntity->getName();
        
        // Extract tetramino ID from node name
        size_t firstUnderscore = nodeName.find('_');
        if (firstUnderscore != std::string::npos) {
            size_t secondUnderscore = nodeName.find('_', firstUnderscore + 1);
            if (secondUnderscore != std::string::npos) {
                std::string idStr = nodeName.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
                int tetraminoId = std::stoi(idStr);
                
                if (tetraminoId == highestId) {
                    tetraminoNodes.push_back(nodeEntity);
                }
            }
        }
    }
    
    if (tetraminoNodes.empty()) return;
    
    // Handle input
    Vec2 moveDelta(0.0f, 0.0f);
    float rotationDelta = 0.0f;
    
    if (input.isKeyDown(Key::J)) {
        moveDelta.x = -moveSpeed; // Left
    }
    if (input.isKeyDown(Key::L)) {
        moveDelta.x = moveSpeed; // Right
    }
    if (input.isKeyDown(Key::K)) {
        moveDelta.y = -moveSpeed; // Down (positive Y is down in this coordinate system)
    }
    if (input.isKeyDown(Key::U)) {
        rotationDelta = m_tetraminoRotationSpeed; // Counter-clockwise
    }
    if (input.isKeyDown(Key::O)) {
        rotationDelta = -m_tetraminoRotationSpeed; // Clockwise
    }
    
    // Calculate center of mass for rotation
    Vec2 centerOfMass(0.0f, 0.0f);
    int validNodes = 0;
    for (auto* nodeEntity : tetraminoNodes) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        centerOfMass += node->getPosition();
        validNodes++;
    }
    
    if (validNodes > 0) {
        centerOfMass /= static_cast<float>(validNodes);
    }
    
    // Apply movement forces to all nodes in the tetramino
    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        for (auto* nodeEntity : tetraminoNodes) {
            auto* node = nodeEntity->getComponent<NodeComponent>();
            if (!node || node->isPositionFixed()) continue;
            
            // Apply force instead of directly moving position
            Vec2 force = moveDelta * m_tetraminoForceMultiplier; // Scale up the force
            node->addExternalForce(force);
        }
    }
    
    // Apply rotation forces to all nodes in the tetramino
    if (rotationDelta != 0.0f) {
        for (auto* nodeEntity : tetraminoNodes) {
            auto* node = nodeEntity->getComponent<NodeComponent>();
            if (!node || node->isPositionFixed()) continue;
            
            // Calculate position relative to center of mass
            Vec2 relativePos = node->getPosition() - centerOfMass;
            
            // Calculate perpendicular vector for rotation (90 degrees counter-clockwise)
            Vec2 perpendicular(-relativePos.y, relativePos.x);
            
            // Apply rotational force
            Vec2 rotationForce = perpendicular * rotationDelta * m_tetraminoRotationForceMultiplier;
            node->addExternalForce(rotationForce);
        }
    }
}

Entity* JellyTetrisReduxScene::getMostRecentTetramino() {
    if (!m_entityManager || m_nextTetriminoId == 0) return nullptr;
    
    // Look for the most recently spawned tetramino
    int highestId = -1;
    Entity* mostRecent = nullptr;
    
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    for (auto* nodeEntity : nodeEntities) {
        std::string nodeName = nodeEntity->getName();
        
        // Extract tetramino ID from node name (e.g., "I_5_Node0" -> ID 5)
        size_t firstUnderscore = nodeName.find('_');
        if (firstUnderscore != std::string::npos) {
            size_t secondUnderscore = nodeName.find('_', firstUnderscore + 1);
            if (secondUnderscore != std::string::npos) {
                std::string idStr = nodeName.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
                int tetraminoId = std::stoi(idStr);
                
                if (tetraminoId > highestId) {
                    highestId = tetraminoId;
                    mostRecent = nodeEntity;
                }
            }
        }
    }
    
    return mostRecent;
}

void JellyTetrisReduxScene::checkNodeCollisions() {
    if (!m_entityManager) return;
    
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    // Check all pairs of nodes for collisions
    for (size_t i = 0; i < nodeEntities.size(); ++i) {
        for (size_t j = i + 1; j < nodeEntities.size(); ++j) {
            auto* node1Entity = nodeEntities[i];
            auto* node2Entity = nodeEntities[j];
            
            auto* node1 = node1Entity->getComponent<NodeComponent>();
            auto* node2 = node2Entity->getComponent<NodeComponent>();
            
            if (!node1 || !node2) continue;
            
            // Skip if both nodes are fixed (they can't move)
            if (node1->isPositionFixed() && node2->isPositionFixed()) continue;
            
            // Check if nodes are from the same tetramino (don't collide with themselves)
            std::string name1 = node1Entity->getName();
            std::string name2 = node2Entity->getName();
            
            // Quick check: if names are identical, skip
            if (name1 == name2) continue;
            
            // Extract tetramino ID for faster comparison
            // Format: "I_0_Node0" -> tetramino ID is between first and second underscore
            size_t firstUnderscore1 = name1.find('_');
            size_t firstUnderscore2 = name2.find('_');
            size_t secondUnderscore1 = name1.find('_', firstUnderscore1 + 1);
            size_t secondUnderscore2 = name2.find('_', firstUnderscore2 + 1);
            
            if (firstUnderscore1 != std::string::npos && secondUnderscore1 != std::string::npos &&
                firstUnderscore2 != std::string::npos && secondUnderscore2 != std::string::npos) {
                
                std::string tetraminoId1 = name1.substr(firstUnderscore1 + 1, secondUnderscore1 - firstUnderscore1 - 1);
                std::string tetraminoId2 = name2.substr(firstUnderscore2 + 1, secondUnderscore2 - firstUnderscore2 - 1);
                
                // Skip collision if same tetramino
                if (tetraminoId1 == tetraminoId2) continue;
            }
            
            // Check for collision
            Vec2 pos1 = node1->getPosition();
            Vec2 pos2 = node2->getPosition();
            float nodeRadius = NODE_SIZE * 0.4f;
            float distance = (pos1 - pos2).length();
            
            // Only resolve collision if nodes are close enough and moving fast enough
            if (distance < nodeRadius * 2.0f && distance > 0.0f) {
                Vec2 vel1 = node1->getVelocity();
                Vec2 vel2 = node2->getVelocity();
                float relativeSpeed = (vel1 - vel2).length();
                
                // Only collide if moving fast enough to prevent micro-collisions
                if (relativeSpeed > m_collisionSpeedThreshold) {
                    resolveNodeCollision(*node1, *node2);
                }
            }
        }
    }
}

void JellyTetrisReduxScene::resolveNodeCollision(NodeComponent& node1, NodeComponent& node2) {
    Vec2 pos1 = node1.getPosition();
    Vec2 pos2 = node2.getPosition();
    Vec2 vel1 = node1.getVelocity();
    Vec2 vel2 = node2.getVelocity();
    
    float nodeRadius = NODE_SIZE * 0.4f;
    float distance = (pos1 - pos2).length();
    
    if (distance < nodeRadius * 2.0f && distance > 0.0f) {
        // Calculate collision normal
        Vec2 normal = (pos1 - pos2) / distance;
        
        // Calculate penetration depth
        float penetration = (nodeRadius * 2.0f) - distance;
        
        // Separate the nodes
        Vec2 separation = normal * (penetration * 0.5f);
        
        if (!node1.isPositionFixed()) {
            node1.setPosition(pos1 + separation);
        }
        if (!node2.isPositionFixed()) {
            node2.setPosition(pos2 - separation);
        }
        
        // Calculate relative velocity
        Vec2 relativeVelocity = vel1 - vel2;
        float velocityAlongNormal = relativeVelocity.dot(normal);
        
        // Only resolve if objects are separating
        if (velocityAlongNormal > 0.0f) return;
        
        // Calculate restitution (bounciness) - much less bouncy
        float restitution = m_collisionRestitution;
        
        // Calculate impulse scalar
        float impulseScalar = -(1.0f + restitution) * velocityAlongNormal;
        
        // Apply impulse
        Vec2 impulse = normal * impulseScalar;
        
        if (!node1.isPositionFixed()) {
            Vec2 newVel1 = vel1 + impulse;
            newVel1 *= m_collisionDamping; // Much more damping to reduce wiggling
            node1.setVelocity(newVel1);
        }
        
        if (!node2.isPositionFixed()) {
            Vec2 newVel2 = vel2 - impulse;
            newVel2 *= m_collisionDamping; // Much more damping to reduce wiggling
            node2.setVelocity(newVel2);
        }
    }
}

void JellyTetrisReduxScene::checkNodeBeamCollisions() {
    if (!m_entityManager) return;
    
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    
    // Safety check - limit the number of collision checks to prevent performance issues
    const size_t maxBeams = 100; // Reasonable limit
    if (beamEntities.size() > maxBeams) {
        return; // Skip collision detection if too many beams
    }
    
    // Check all pairs of beams for collisions
    for (size_t i = 0; i < beamEntities.size(); ++i) {
        for (size_t j = i + 1; j < beamEntities.size(); ++j) {
            auto* beam1Entity = beamEntities[i];
            auto* beam2Entity = beamEntities[j];
            
            if (!beam1Entity || !beam2Entity) continue;
            
            auto* beam1 = beam1Entity->getComponent<BeamComponent>();
            auto* beam2 = beam2Entity->getComponent<BeamComponent>();
            
            if (!beam1 || !beam2) continue;
            
            // Get beam endpoints with null checks
            auto* node1aEntity = beam1->getNode1Entity();
            auto* node1bEntity = beam1->getNode2Entity();
            auto* node2aEntity = beam2->getNode1Entity();
            auto* node2bEntity = beam2->getNode2Entity();
            
            if (!node1aEntity || !node1bEntity || !node2aEntity || !node2bEntity) continue;
            
            auto* node1a = node1aEntity->getComponent<NodeComponent>();
            auto* node1b = node1bEntity->getComponent<NodeComponent>();
            auto* node2a = node2aEntity->getComponent<NodeComponent>();
            auto* node2b = node2bEntity->getComponent<NodeComponent>();
            
            if (!node1a || !node1b || !node2a || !node2b) continue;
            
            // Skip if both beams are from the same tetramino (check by name prefix)
            std::string beam1Name = beam1Entity->getName();
            std::string beam2Name = beam2Entity->getName();
            
            // Extract tetramino prefix (e.g., "I_0" from "I_0_Beam0")
            size_t lastUnderscore1 = beam1Name.find_last_of('_');
            size_t lastUnderscore2 = beam2Name.find_last_of('_');
            
            if (lastUnderscore1 != std::string::npos && lastUnderscore2 != std::string::npos) {
                std::string prefix1 = beam1Name.substr(0, lastUnderscore1);
                std::string prefix2 = beam2Name.substr(0, lastUnderscore2);
                
                // Skip collision if they're from the same tetramino
                if (prefix1 == prefix2) continue;
            }
            
            Vec2 beam1Start = node1a->getPosition();
            Vec2 beam1End = node1b->getPosition();
            Vec2 beam2Start = node2a->getPosition();
            Vec2 beam2End = node2b->getPosition();
            
            // Check for beam-to-beam collision with safety checks
            try {
                float distance = distanceLineSegmentToLineSegment(beam1Start, beam1End, beam2Start, beam2End);
                
                // Check for NaN or infinite values
                if (!std::isfinite(distance)) continue;
                
                float beamRadius = NODE_SIZE * 0.2f; // Smaller radius for beams
                float collisionRadius = beamRadius * 2.0f;
                
                if (distance < collisionRadius && distance >= 0.0f) {
                    resolveBeamBeamCollision(*beam1, *beam2);
                }
            } catch (...) {
                // Skip this collision check if it causes an exception
                continue;
            }
        }
    }
}

float JellyTetrisReduxScene::distancePointToLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd) {
    Vec2 line = lineEnd - lineStart;
    float lineLength = line.length();
    
    if (lineLength < 0.001f) {
        // Line is too short, just return distance to start point
        return (point - lineStart).length();
    }
    
    Vec2 lineDir = line / lineLength;
    Vec2 pointToStart = point - lineStart;
    
    // Project point onto line
    float projection = pointToStart.dot(lineDir);
    
    // Clamp to line segment
    projection = std::max(0.0f, std::min(lineLength, projection));
    
    // Get closest point on line segment
    Vec2 closestPoint = lineStart + lineDir * projection;
    
    // Return distance to closest point
    return (point - closestPoint).length();
}

float JellyTetrisReduxScene::distanceLineSegmentToLineSegment(const Vec2& line1Start, const Vec2& line1End, const Vec2& line2Start, const Vec2& line2End) {
    // Simplified and safer line segment distance calculation
    Vec2 d1 = line1End - line1Start;
    Vec2 d2 = line2End - line2Start;
    
    // Check for degenerate line segments
    float len1 = d1.length();
    float len2 = d2.length();
    
    if (len1 < 0.001f && len2 < 0.001f) {
        // Both are points
        return (line1Start - line2Start).length();
    } else if (len1 < 0.001f) {
        // First is a point, second is a line
        return distancePointToLineSegment(line1Start, line2Start, line2End);
    } else if (len2 < 0.001f) {
        // Second is a point, first is a line
        return distancePointToLineSegment(line2Start, line1Start, line1End);
    }
    
    // Both are valid line segments
    Vec2 w0 = line1Start - line2Start;
    float a = d1.dot(d1);
    float b = d1.dot(d2);
    float c = d2.dot(d2);
    float d = d1.dot(w0);
    float e = d2.dot(w0);
    
    float denom = a * c - b * b;
    
    if (denom < 0.0001f) {
        // Lines are parallel - use point-to-line distance
        return distancePointToLineSegment(line1Start, line2Start, line2End);
    }
    
    // Calculate parameters
    float sn = (b * e - c * d);
    float tn = (a * e - b * d);
    
    // Clamp sn to [0,1]
    if (sn < 0.0f) {
        sn = 0.0f;
        tn = e;
    } else if (sn > denom) {
        sn = denom;
        tn = e + b;
    }
    
    // Clamp tn to [0,1]
    if (tn < 0.0f) {
        tn = 0.0f;
        if (-d < 0.0f) {
            sn = 0.0f;
        } else if (-d > a) {
            sn = denom;
        } else {
            sn = -d;
        }
    } else if (tn > denom) {
        tn = denom;
        if ((-d + b) < 0.0f) {
            sn = 0.0f;
        } else if ((-d + b) > a) {
            sn = denom;
        } else {
            sn = (-d + b);
        }
    }
    
    // Calculate closest points
    float sc = (std::abs(sn) < 0.0001f ? 0.0f : sn / denom);
    float tc = (std::abs(tn) < 0.0001f ? 0.0f : tn / denom);
    
    Vec2 p1 = line1Start + d1 * sc;
    Vec2 p2 = line2Start + d2 * tc;
    
    return (p1 - p2).length();
}

void JellyTetrisReduxScene::resolveNodeBeamCollision(NodeComponent& node, BeamComponent& beam) {
    // Get beam endpoints
    auto* node1 = beam.getNode1Entity()->getComponent<NodeComponent>();
    auto* node2 = beam.getNode2Entity()->getComponent<NodeComponent>();
    if (!node1 || !node2) return;
    
    Vec2 nodePos = node.getPosition();
    Vec2 beamStart = node1->getPosition();
    Vec2 beamEnd = node2->getPosition();
    
    // Find closest point on beam
    Vec2 line = beamEnd - beamStart;
    float lineLength = line.length();
    
    if (lineLength < 0.001f) return;
    
    Vec2 lineDir = line / lineLength;
    Vec2 pointToStart = nodePos - beamStart;
    float projection = pointToStart.dot(lineDir);
    projection = std::max(0.0f, std::min(lineLength, projection));
    
    Vec2 closestPoint = beamStart + lineDir * projection;
    
    // Calculate separation vector
    Vec2 separation = nodePos - closestPoint;
    float distance = separation.length();
    
    if (distance < 0.001f) return;
    
    Vec2 normal = separation / distance;
    float nodeRadius = NODE_SIZE * 0.4f;
    float penetration = nodeRadius + 5.0f - distance; // 5.0f tolerance
    
    if (penetration > 0.0f) {
        // Push node away from beam
        Vec2 pushVector = normal * penetration;
        node.setPosition(nodePos + pushVector);
        
        // Apply damping to prevent bouncing
        Vec2 velocity = node.getVelocity();
        float velocityAlongNormal = velocity.dot(normal);
        
        if (velocityAlongNormal < 0.0f) {
            // Remove velocity component along collision normal
            Vec2 newVelocity = velocity - normal * velocityAlongNormal * 0.8f;
            node.setVelocity(newVelocity);
        }
    }
}

void JellyTetrisReduxScene::resolveBeamBeamCollision(BeamComponent& beam1, BeamComponent& beam2) {
    // Get beam endpoints with safety checks
    auto* node1aEntity = beam1.getNode1Entity();
    auto* node1bEntity = beam1.getNode2Entity();
    auto* node2aEntity = beam2.getNode1Entity();
    auto* node2bEntity = beam2.getNode2Entity();
    
    if (!node1aEntity || !node1bEntity || !node2aEntity || !node2bEntity) return;
    
    auto* node1a = node1aEntity->getComponent<NodeComponent>();
    auto* node1b = node1bEntity->getComponent<NodeComponent>();
    auto* node2a = node2aEntity->getComponent<NodeComponent>();
    auto* node2b = node2bEntity->getComponent<NodeComponent>();
    
    if (!node1a || !node1b || !node2a || !node2b) return;
    
    Vec2 beam1Start = node1a->getPosition();
    Vec2 beam1End = node1b->getPosition();
    Vec2 beam2Start = node2a->getPosition();
    Vec2 beam2End = node2b->getPosition();
    
    // Check for valid positions
    if (!std::isfinite(beam1Start.x) || !std::isfinite(beam1Start.y) ||
        !std::isfinite(beam1End.x) || !std::isfinite(beam1End.y) ||
        !std::isfinite(beam2Start.x) || !std::isfinite(beam2Start.y) ||
        !std::isfinite(beam2End.x) || !std::isfinite(beam2End.y)) {
        return;
    }
    
    // Find closest points on both beams
    float distance = distanceLineSegmentToLineSegment(beam1Start, beam1End, beam2Start, beam2End);
    float beamRadius = NODE_SIZE * 0.2f;
    float collisionRadius = beamRadius * 2.0f;
    
    if (distance < collisionRadius && distance >= 0.0f) {
        // Calculate separation vector between closest points
        Vec2 separation = (beam1Start + beam1End) * 0.5f - (beam2Start + beam2End) * 0.5f;
        float separationLength = separation.length();
        
        if (separationLength > 0.001f) {
            Vec2 normal = separation / separationLength;
            float penetration = collisionRadius - distance;
            
            if (penetration > 0.0f) {
                Vec2 pushVector = normal * penetration * 0.5f;
                
                // Check for valid push vector
                if (std::isfinite(pushVector.x) && std::isfinite(pushVector.y)) {
                    // Apply separation to both beams' nodes
                    if (!node1a->isPositionFixed()) {
                        Vec2 newPos = node1a->getPosition() + pushVector;
                        if (std::isfinite(newPos.x) && std::isfinite(newPos.y)) {
                            node1a->setPosition(newPos);
                        }
                    }
                    if (!node1b->isPositionFixed()) {
                        Vec2 newPos = node1b->getPosition() + pushVector;
                        if (std::isfinite(newPos.x) && std::isfinite(newPos.y)) {
                            node1b->setPosition(newPos);
                        }
                    }
                    if (!node2a->isPositionFixed()) {
                        Vec2 newPos = node2a->getPosition() - pushVector;
                        if (std::isfinite(newPos.x) && std::isfinite(newPos.y)) {
                            node2a->setPosition(newPos);
                        }
                    }
                    if (!node2b->isPositionFixed()) {
                        Vec2 newPos = node2b->getPosition() - pushVector;
                        if (std::isfinite(newPos.x) && std::isfinite(newPos.y)) {
                            node2b->setPosition(newPos);
                        }
                    }
                    
                    // Apply damping to reduce bouncing
                    if (!node1a->isPositionFixed()) {
                        Vec2 vel = node1a->getVelocity();
                        vel *= m_collisionDamping;
                        node1a->setVelocity(vel);
                    }
                    if (!node1b->isPositionFixed()) {
                        Vec2 vel = node1b->getVelocity();
                        vel *= m_collisionDamping;
                        node1b->setVelocity(vel);
                    }
                    if (!node2a->isPositionFixed()) {
                        Vec2 vel = node2a->getVelocity();
                        vel *= m_collisionDamping;
                        node2a->setVelocity(vel);
                    }
                    if (!node2b->isPositionFixed()) {
                        Vec2 vel = node2b->getVelocity();
                        vel *= m_collisionDamping;
                        node2b->setVelocity(vel);
                    }
                }
            }
        }
    }
}

// Node dragging implementation
Vec2 JellyTetrisReduxScene::screenToWorldPosition(const Vec2& screenPos) {
    // Use cached camera reference for performance
    if (!m_cameraCacheValid) {
        if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
            m_cachedCamera = cameraEntity->getComponent<Camera2D>();
            m_cameraCacheValid = (m_cachedCamera != nullptr);
        }
    }
    
    if (m_cachedCamera) {
        float screenWidth = GraphicsEngine::getWindowWidth();
        float screenHeight = GraphicsEngine::getWindowHeight();

        // Convert normalized screen coordinates to pixel coordinates
        float pixelX = screenPos.x * screenWidth;
        float pixelY = screenPos.y * screenHeight;

        // Use the camera's built-in screenToWorld method
        Vec2 worldPos = m_cachedCamera->screenToWorld(Vec2(pixelX, pixelY));
        
        // The camera's screenToWorld flips Y, so we need to flip it back
        // to get the correct world coordinates for our game
        return Vec2(worldPos.x, -worldPos.y);
    }
    return Vec2(0, 0);
}

Entity* JellyTetrisReduxScene::findNodeUnderMouse(const Vec2& worldMousePos) {
    if (!m_entityManager) return nullptr;
    
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    
    // Early exit if no nodes
    if (nodeEntities.empty()) return nullptr;
    
    // Find the closest node to the mouse position
    Entity* closestNode = nullptr;
    float closestDistanceSquared = std::numeric_limits<float>::max();
    float nodeRadiusSquared = (NODE_SIZE * 0.5f) * (NODE_SIZE * 0.5f); // Use squared distance for performance
    
    for (auto* nodeEntity : nodeEntities) {
        auto* node = nodeEntity->getComponent<NodeComponent>();
        if (!node || node->isPositionFixed()) continue;
        
        Vec2 nodePos = node->getPosition();
        Vec2 diff = worldMousePos - nodePos;
        float distanceSquared = diff.x * diff.x + diff.y * diff.y; // Avoid sqrt() call
        
        // Check if mouse is within node radius (using squared distance)
        if (distanceSquared < nodeRadiusSquared && distanceSquared < closestDistanceSquared) {
            closestNode = nodeEntity;
            closestDistanceSquared = distanceSquared;
        }
    }
    
    return closestNode;
}

void JellyTetrisReduxScene::updateNodeDragging() {
    if (!m_entityManager) return;
    
    auto& input = Input::getInstance();
    
    // Time dragging updates
    auto dragStart = std::chrono::high_resolution_clock::now();
    
    // Early exit if no mouse interaction
    bool mousePressed = input.wasMouseJustPressed(MouseClick::LeftMouse);
    bool mouseReleased = input.wasMouseJustReleased(MouseClick::LeftMouse);
    bool mouseDown = input.isMouseDown(MouseClick::LeftMouse);
    
    if (!mousePressed && !mouseReleased && !mouseDown && !m_isDraggingNode) {
        return; // No mouse interaction, skip expensive calculations
    }
    
    Vec2 mousePos = input.getMousePositionNDC();
    Vec2 worldMousePos = screenToWorldPosition(mousePos);
    
    // Update mouse position for velocity calculation
    Vec2 mouseDelta = worldMousePos - m_lastMousePosition;
    m_lastMousePosition = worldMousePos;
    
    // Start dragging
    if (mousePressed) {
        Entity* clickedNode = findNodeUnderMouse(worldMousePos);
        if (clickedNode) {
            m_draggedNode = clickedNode;
            m_isDraggingNode = true;
            
            // Cache component references for performance
            m_cachedDraggedNode = clickedNode->getComponent<NodeComponent>();
            m_cachedDraggedSprite = clickedNode->getComponent<SpriteComponent>();
            
            if (m_cachedDraggedNode) {
                Vec2 nodePos = m_cachedDraggedNode->getPosition();
                m_dragOffset = nodePos - worldMousePos;
            }
            
            // Bring dragged node to front by adjusting Z position
            if (m_cachedDraggedSprite) {
                Vec3 pos = m_cachedDraggedSprite->getPosition();
                m_cachedDraggedSprite->setPosition(pos.x, pos.y, 100.0f); // High Z value to render on top
            }
        }
    }
    
    // Update dragging with spring-damper force towards mouse
    if (m_isDraggingNode && m_cachedDraggedNode) {
        Vec2 targetPos = worldMousePos + m_dragOffset;
        Vec2 currentPos = m_cachedDraggedNode->getPosition();
        Vec2 toTarget = targetPos - currentPos;
        Vec2 vel = m_cachedDraggedNode->getVelocity();

        // Spring-damper: F = k * x - c * v
        Vec2 force = toTarget * m_dragSpringStiffness - vel * m_dragSpringDamping;

        // Clamp force
        float mag = force.length();
        if (mag > m_dragMaxForce && mag > 0.0f) {
            force = force * (m_dragMaxForce / mag);
        }

        m_cachedDraggedNode->addExternalForce(force);

        // Keep dragged sprite visually on top; let physics update X/Y
        if (m_cachedDraggedSprite) {
            Vec3 sp = m_cachedDraggedSprite->getPosition();
            // Optionally, nudge sprite toward node for visual sync (no snap)
            m_cachedDraggedSprite->setPosition(currentPos.x, currentPos.y, sp.z);
        }
    }
    
    // Stop dragging
    if (mouseReleased) {
        if (m_isDraggingNode && m_cachedDraggedSprite) {
            // Reset Z position to normal depth
            Vec3 pos = m_cachedDraggedSprite->getPosition();
            m_cachedDraggedSprite->setPosition(pos.x, pos.y, 0.0f); // Reset to normal Z
        }
        
        m_isDraggingNode = false;
        m_draggedNode = nullptr;
        m_cachedDraggedNode = nullptr;
        m_cachedDraggedSprite = nullptr;
    }
    
    // Update drag timing
    auto dragEnd = std::chrono::high_resolution_clock::now();
    m_dragTime = std::chrono::duration<float, std::milli>(dragEnd - dragStart).count();
}

// Broadphase collision detection implementation
void JellyTetrisReduxScene::updateSpatialGrid() {
    if (!m_entityManager) return;
    
    // Only update grid if it's dirty or we have many entities
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    if (!m_spatialGridDirty && beamEntities.size() < 50) {
        return; // Skip update for small numbers of entities
    }
    
    // Clear existing grid
    m_spatialGrid.clear();
    
    // Add beams to spatial grid
    for (auto* beamEntity : beamEntities) {
        auto* beam = beamEntity->getComponent<BeamComponent>();
        if (!beam) continue;
        
        auto* node1 = beam->getNode1Entity()->getComponent<NodeComponent>();
        auto* node2 = beam->getNode2Entity()->getComponent<NodeComponent>();
        if (!node1 || !node2) continue;
        
        Vec2 pos1 = node1->getPosition();
        Vec2 pos2 = node2->getPosition();
        
        // Get bounding box of beam
        Vec2 min = Vec2(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y));
        Vec2 max = Vec2(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y));
        
        // Add padding for collision radius
        float padding = NODE_SIZE * 0.4f;
        min -= Vec2(padding, padding);
        max += Vec2(padding, padding);
        
        // Add beam to all cells it overlaps
        std::vector<std::string> keys;
        getGridKeys(min, max, keys);
        for (const auto& key : keys) {
            m_spatialGrid[key].beams.push_back(beamEntity);
        }
    }
    
    m_spatialGridDirty = false;
}

std::string JellyTetrisReduxScene::getGridKey(const Vec2& position) {
    int gridX = static_cast<int>(std::floor(position.x / GRID_CELL_SIZE));
    int gridY = static_cast<int>(std::floor(position.y / GRID_CELL_SIZE));
    return std::to_string(gridX) + "," + std::to_string(gridY);
}

void JellyTetrisReduxScene::getGridKeys(const Vec2& min, const Vec2& max, std::vector<std::string>& keys) {
    keys.clear();
    
    int minX = static_cast<int>(std::floor(min.x / GRID_CELL_SIZE));
    int minY = static_cast<int>(std::floor(min.y / GRID_CELL_SIZE));
    int maxX = static_cast<int>(std::floor(max.x / GRID_CELL_SIZE));
    int maxY = static_cast<int>(std::floor(max.y / GRID_CELL_SIZE));
    
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            keys.push_back(std::to_string(x) + "," + std::to_string(y));
        }
    }
}

void JellyTetrisReduxScene::checkCollisionsInCell(const std::string& cellKey) {
    auto it = m_spatialGrid.find(cellKey);
    if (it == m_spatialGrid.end()) return;
    
    const auto& cell = it->second;
    
    // Check beam-to-beam collisions within this cell
    for (size_t i = 0; i < cell.beams.size(); ++i) {
        for (size_t j = i + 1; j < cell.beams.size(); ++j) {
            auto* beam1Entity = cell.beams[i];
            auto* beam2Entity = cell.beams[j];
            
            if (!beam1Entity || !beam2Entity) continue;
            
            auto* beam1 = beam1Entity->getComponent<BeamComponent>();
            auto* beam2 = beam2Entity->getComponent<BeamComponent>();
            
            if (!beam1 || !beam2) continue;
            
            // Skip if beams are from same tetramino (optimized check)
            std::string beam1Name = beam1Entity->getName();
            std::string beam2Name = beam2Entity->getName();
            
            // Quick check: if names are identical, skip
            if (beam1Name == beam2Name) continue;
            
            // Extract tetramino ID from beam names for faster comparison
            // Format: "I_0_Beam0" -> tetramino ID is between first and second underscore
            size_t firstUnderscore1 = beam1Name.find('_');
            size_t firstUnderscore2 = beam2Name.find('_');
            size_t secondUnderscore1 = beam1Name.find('_', firstUnderscore1 + 1);
            size_t secondUnderscore2 = beam2Name.find('_', firstUnderscore2 + 1);
            
            if (firstUnderscore1 != std::string::npos && secondUnderscore1 != std::string::npos &&
                firstUnderscore2 != std::string::npos && secondUnderscore2 != std::string::npos) {
                
                std::string tetraminoId1 = beam1Name.substr(firstUnderscore1 + 1, secondUnderscore1 - firstUnderscore1 - 1);
                std::string tetraminoId2 = beam2Name.substr(firstUnderscore2 + 1, secondUnderscore2 - firstUnderscore2 - 1);
                
                // Skip collision if same tetramino
                if (tetraminoId1 == tetraminoId2) continue;
            }
            
            // Get beam endpoints
            auto* node1aEntity = beam1->getNode1Entity();
            auto* node1bEntity = beam1->getNode2Entity();
            auto* node2aEntity = beam2->getNode1Entity();
            auto* node2bEntity = beam2->getNode2Entity();
            
            if (!node1aEntity || !node1bEntity || !node2aEntity || !node2bEntity) continue;
            
            auto* node1a = node1aEntity->getComponent<NodeComponent>();
            auto* node1b = node1bEntity->getComponent<NodeComponent>();
            auto* node2a = node2aEntity->getComponent<NodeComponent>();
            auto* node2b = node2bEntity->getComponent<NodeComponent>();
            
            if (!node1a || !node1b || !node2a || !node2b) continue;
            
            Vec2 beam1Start = node1a->getPosition();
            Vec2 beam1End = node1b->getPosition();
            Vec2 beam2Start = node2a->getPosition();
            Vec2 beam2End = node2b->getPosition();
            
            // Check for beam-to-beam collision
            try {
                float distance = distanceLineSegmentToLineSegment(beam1Start, beam1End, beam2Start, beam2End);
                
                if (!std::isfinite(distance)) continue;
                
                float beamRadius = NODE_SIZE * 0.2f;
                float collisionRadius = beamRadius * 2.0f;
                
                if (distance < collisionRadius && distance >= 0.0f) {
                    resolveBeamBeamCollision(*beam1, *beam2);
                }
            } catch (...) {
                continue;
            }
        }
    }
}

