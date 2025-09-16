#include <DX3D/Game/Scenes/PhysicsTetrisScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace dx3d;

// Tetris piece definitions (relative positions for 4 blocks)

static const Vec4 PIECE_COLORS[7] = {
    {0.0f, 1.0f, 1.0f, 1.0f}, // Cyan - I
    {0.0f, 0.0f, 1.0f, 1.0f}, // Blue - J
    {1.0f, 0.5f, 0.0f, 1.0f}, // Orange - L
    {1.0f, 1.0f, 0.0f, 1.0f}, // Yellow - O
    {0.0f, 1.0f, 0.0f, 1.0f}, // Green - S
    {0.5f, 0.0f, 1.0f, 1.0f}, // Purple - T
    {1.0f, 0.0f, 0.0f, 1.0f}  // Red - Z
};

void PhysicsTetrisScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;

    // Initialize systems
    m_entityManager = std::make_unique<EntityManager>();

    // Game state
    m_gameRunning = true;
    m_currentPiece = -1;
    m_nextPiece = m_random() % 7;
    m_score = 0;
    m_level = 1;
    m_linesCleared = 0;
    m_fallSpeed = 200.0f;
    m_pieceIdCounter = 0;

    // Line clearing state
    m_lineClearTimer = 0.0f;
    m_isClearing = false;

    // Initialize grid tracking
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            m_grid[y][x] = nullptr;
        }
        m_lineAreas[y] = 0.0f;
    }

    // Create camera
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    // Create game boundaries (walls and floor)
    createBoundaries();

    // Create UI
    createUI();

    // Start first piece
    spawnNewPiece();
}

void PhysicsTetrisScene::createBoundaries() {
    auto& device = *m_graphicsDevice;

    // Left wall
    auto& leftWallEntity = m_entityManager->createEntity("LeftWall");
    auto& leftWallSprite = leftWallEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/beam.png", 32.0f, GRID_HEIGHT * BLOCK_SIZE
    );
    leftWallSprite.setPosition(-GRID_WIDTH * BLOCK_SIZE / 2.0f - 16.0f, 0.0f, -1.0f);
    leftWallSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 1.0f));

    // Right wall
    auto& rightWallEntity = m_entityManager->createEntity("RightWall");
    auto& rightWallSprite = rightWallEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/beam.png", 32.0f, GRID_HEIGHT * BLOCK_SIZE
    );
    rightWallSprite.setPosition(GRID_WIDTH * BLOCK_SIZE / 2.0f + 16.0f, 0.0f, -1.0f);
    rightWallSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 1.0f));

    // Floor
    auto& floorEntity = m_entityManager->createEntity("Floor");
    auto& floorSprite = floorEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/beam.png", GRID_WIDTH * BLOCK_SIZE, 32.0f
    );
    floorSprite.setPosition(0.0f, -GRID_HEIGHT * BLOCK_SIZE / 2.0f - 16.0f, -1.0f);
    floorSprite.setTint(Vec4(0.3f, 0.3f, 0.3f, 1.0f));
}

void PhysicsTetrisScene::createUI() {
    auto& device = *m_graphicsDevice;

    if (!TextSystem::isInitialized()) {
        TextSystem::initialize(device);
    }

    // Score display
    auto& scoreEntity = m_entityManager->createEntity("ScoreText");
    auto& scoreText = scoreEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Score: 0", 24.0f
    );
    scoreText.setScreenPosition(0.8f, 0.1f);
    scoreText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Level display
    auto& levelEntity = m_entityManager->createEntity("LevelText");
    auto& levelText = levelEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Level: 1", 20.0f
    );
    levelText.setScreenPosition(0.8f, 0.15f);
    levelText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Lines display
    auto& linesEntity = m_entityManager->createEntity("LinesText");
    auto& linesText = linesEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Lines: 0", 20.0f
    );
    linesText.setScreenPosition(0.8f, 0.2f);
    linesText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Next piece preview background
    auto& previewBgEntity = m_entityManager->createEntity("PreviewBg");
    auto& previewBg = previewBgEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/beam.png", 120.0f, 120.0f
    );
    previewBg.enableScreenSpace(true);
    previewBg.setScreenPosition(0.85f, 0.4f);
    previewBg.setTint(Vec4(0.2f, 0.2f, 0.2f, 0.7f));
}

void PhysicsTetrisScene::spawnNewPiece() {
    if (!m_gameRunning) return;

    // Remove preview of previous next piece
    removeNextPiecePreview();

    // Current piece becomes the next piece
    int pieceType = m_nextPiece;
    m_nextPiece = m_random() % 7;
    m_currentPiece = pieceType;

    // Create new piece at spawn position
    Vec2 spawnPos(0.0f, -GRID_HEIGHT * BLOCK_SIZE / 2.0f - 100.0f);

    // Create 4 blocks for the tetris piece
    std::vector<TetrisBlock*> pieceBlocks;
    Vec4 color = PIECE_COLORS[pieceType];

    for (int i = 0; i < 4; i++) {
        std::string blockName = "Block_" + std::to_string(m_pieceIdCounter) + "_" + std::to_string(i);
        auto& blockEntity = m_entityManager->createEntity(blockName);

        // Position relative to spawn position
        Vec2 blockPos = spawnPos + TETRIS_SHAPES[pieceType][i] * BLOCK_SIZE;

        // Visual component
        auto& sprite = blockEntity.addComponent<SpriteComponent>(
            *m_graphicsDevice, L"DX3D/Assets/Textures/node.png",
            BLOCK_SIZE - 2.0f, BLOCK_SIZE - 2.0f
        );
        sprite.setPosition(blockPos.x, blockPos.y, -5.0f);
        sprite.setTint(color);

        // Physics component with slightly reduced mass for better spring behavior
        auto& physics = blockEntity.addComponent<TetrisPhysicsComponent>(blockPos, false);
        physics.setVelocity(Vec2(0.0f, m_fallSpeed));
        physics.setMass(0.8f); // Lighter blocks work better with springs
        physics.setFriction(0.6f); // Reduced friction for more dynamic movement

        // Track this block
        TetrisBlock block;
        block.entity = &blockEntity;
        block.pieceId = m_pieceIdCounter;
        block.blockIndex = i;
        block.settled = false;
        m_activeBlocks.push_back(block);

        pieceBlocks.push_back(&m_activeBlocks.back());
    }

    // Create spring constraints between blocks in this piece
    createSpringConstraints(pieceBlocks);

    // Create next piece preview
    createNextPiecePreview();

    m_pieceIdCounter++;
}


void PhysicsTetrisScene::createNextPiecePreview() {
    Vec4 color = PIECE_COLORS[m_nextPiece];
    color.w = 0.7f; // Make it semi-transparent

    for (int i = 0; i < 4; i++) {
        std::string previewName = "NextPiece_" + std::to_string(i);
        auto& previewEntity = m_entityManager->createEntity(previewName);

        auto& sprite = previewEntity.addComponent<SpriteComponent>(
            *m_graphicsDevice, L"DX3D/Assets/Textures/node.png",
            20.0f, 20.0f
        );
        sprite.enableScreenSpace(true);

        // Position in preview area
        Vec2 relativePos = TETRIS_SHAPES[m_nextPiece][i] * 15.0f; // Scaled down
        sprite.setScreenPosition(0.85f + relativePos.x / 400.0f, 0.4f - relativePos.y / 300.0f);
        sprite.setTint(color);
    }
}

void PhysicsTetrisScene::removeNextPiecePreview() {
    for (int i = 0; i < 4; i++) {
        std::string previewName = "NextPiece_" + std::to_string(i);
        m_entityManager->removeEntity(previewName);
    }
}

void PhysicsTetrisScene::update(float dt) {
    if (!m_gameRunning) {
        // Handle game over input
        auto& input = Input::getInstance();
        if (input.wasKeyJustPressed(Key::R)) {
            resetGame();
        }
        return;
    }

    updateCameraMovement(dt);

    // Use enhanced input handling
    handleEnhancedInput(dt);

    // Handle line clearing animation
    if (m_isClearing) {
        updateLineClearAnimation(dt);
        return; // Don't update physics during line clearing
    }

    

    // Update sprite positions to match physics
    updateSpritePositions();

    // Check for line completion
    //updateLineAreas();
    //checkForCompletedLines();

    // Update UI
    updateUI();

    // Check for game over condition
    checkGameOver();

    // Update high score
    updateHighScore();
}
void PhysicsTetrisScene::updateSpritePositions() {
    for (auto& block : m_activeBlocks) {
        if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
            if (auto* sprite = block.entity->getComponent<SpriteComponent>()) {
                Vec2 pos = physics->getPosition();
                // Try flipping the Y coordinate
                sprite->setPosition(pos.x, -pos.y, -5.0f); // Note the negative Y
            }
        }
    }
}
void PhysicsTetrisScene::handleInput(float dt) {
    auto& input = Input::getInstance();

    if (m_activeBlocks.empty()) return;

    // Find current piece blocks (same pieceId and not settled)
    std::vector<TetrisBlock*> currentPieceBlocks;
    int currentPieceId = -1;

    for (auto& block : m_activeBlocks) {
        if (!block.settled) {
            if (currentPieceId == -1) {
                currentPieceId = block.pieceId;
            }
            if (block.pieceId == currentPieceId) {
                currentPieceBlocks.push_back(&block);
            }
        }
    }

    if (currentPieceBlocks.empty()) return;

    // Apply forces to current piece (reduced force for spring system)
    Vec2 force(0.0f, 0.0f);
    float torque = 0.0f;

    // Reduced forces to work better with springs
    if (input.isKeyDown(Key::Left)) {
        force.x = -200.0f; // Reduced from -300.0f
    }
    if (input.isKeyDown(Key::Right)) {
        force.x = 200.0f;  // Reduced from 300.0f
    }
    if (input.isKeyDown(Key::Down)) {
        force.y = 300.0f;  // Reduced from 400.0f (soft drop)
    }
    if (input.isKeyDown(Key::Up)) {
        torque = 150.0f;   // Reduced from 200.0f (rotate)
    }

    // Apply forces to all blocks in the current piece
    for (auto* block : currentPieceBlocks) {
        if (auto* physics = block->entity->getComponent<TetrisPhysicsComponent>()) {
            physics->addForce(force);
            if (torque != 0.0f) {
                physics->addTorque(torque * (block->blockIndex % 2 == 0 ? 1.0f : -1.0f)); // Alternate torque direction
            }
        }
    }
}

void PhysicsTetrisScene::updatePhysics(float dt) {
    // Update spring constraints first (this applies forces to blocks)
    updateSpringConstraints(dt);

    // Update physics for all active blocks
    for (auto& block : m_activeBlocks) {
        if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
            physics->update(dt);

            // Check if block has settled (low velocity and in contact with something)
            Vec2 velocity = physics->getVelocity();
            float velocityThreshold = 30.0f; // Lowered threshold for spring system

            if (velocity.length() < velocityThreshold && physics->isGrounded()) {
                if (!block.settled) {
                    block.settled = true;
                    // Remove springs for this piece when it settles
                    removeSpringConstraints(block.pieceId);
                }
            }
        }
    }

    // Check if we need to spawn a new piece
    bool allSettled = true;
    int activePieceId = -1;

    for (const auto& block : m_activeBlocks) {
        if (!block.settled) {
            allSettled = false;
            if (activePieceId == -1) {
                activePieceId = block.pieceId;
            }
        }
    }

    // Only spawn new piece if current piece has settled
    if (allSettled && !m_activeBlocks.empty()) {
        spawnNewPiece();
    }
}
void PhysicsTetrisScene::updateLineAreas() {
    // Reset line areas
    for (int y = 0; y < GRID_HEIGHT; y++) {
        m_lineAreas[y] = 0.0f;
    }

    // Calculate area covered by blocks in each line using more precise method
    for (const auto& block : m_activeBlocks) {
        if (!block.settled) continue;

        if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
            Vec2 pos = physics->getPosition();

            // Get block bounds
            float blockTop = pos.y - BLOCK_SIZE / 2.0f;
            float blockBottom = pos.y + BLOCK_SIZE / 2.0f;

            // Check which lines this block intersects
            for (int y = 0; y < GRID_HEIGHT; y++) {
                float lineTop = (y - GRID_HEIGHT / 2.0f) * BLOCK_SIZE - BLOCK_SIZE / 2.0f;
                float lineBottom = lineTop + BLOCK_SIZE;

                // Calculate intersection area
                float intersectionTop = std::max(blockTop, lineTop);
                float intersectionBottom = std::min(blockBottom, lineBottom);

                if (intersectionTop < intersectionBottom) {
                    float intersectionHeight = intersectionBottom - intersectionTop;
                    float intersectionArea = BLOCK_SIZE * intersectionHeight;
                    m_lineAreas[y] += intersectionArea;
                }
            }
        }
    }
}

void PhysicsTetrisScene::checkForCompletedLines() {
    std::vector<int> completedLines;
    float threshold = GRID_WIDTH * BLOCK_SIZE * BLOCK_SIZE * 0.75f; // 75% filled threshold

    for (int y = 0; y < GRID_HEIGHT; y++) {
        if (m_lineAreas[y] >= threshold) {
            completedLines.push_back(y);
        }
    }

    if (!completedLines.empty()) {
        // Use advanced slicing instead of simple removal
        performAdvancedLineClear(completedLines);

        // Play sound effect
        playSound("line_clear");
    }
}

void PhysicsTetrisScene::startLineClear(const std::vector<int>& lines) {
    m_isClearing = true;
    m_lineClearTimer = 0.0f;
    m_completedLines = lines;

    // Mark blocks in completed lines for removal
    m_blocksToRemove.clear();

    for (auto& block : m_activeBlocks) {
        if (!block.settled) continue;

        if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
            Vec2 pos = physics->getPosition();
            int gridY = static_cast<int>((pos.y + GRID_HEIGHT * BLOCK_SIZE / 2.0f) / BLOCK_SIZE);

            for (int line : lines) {
                if (gridY == line) {
                    m_blocksToRemove.push_back(&block);
                    break;
                }
            }
        }
    }

    // Start blinking effect
    for (auto* block : m_blocksToRemove) {
        if (auto* sprite = block->entity->getComponent<SpriteComponent>()) {
            // Add blinking animation component
            auto& animation = block->entity->addComponent<AnimationComponent>();
            animation.setUpdateFunction([](Entity& entity, float dt) {
                static float timer = 0.0f;
                timer += dt;

                if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                    Vec4 tint = sprite->getTint();
                    tint.w = 0.5f + 0.5f * sin(timer * 10.0f); // Blink effect
                    sprite->setTint(tint);
                }
                });
        }
    }

    // Update score
    int linesCount = static_cast<int>(lines.size());
    int scoreAdd = 0;
    switch (linesCount) {
    case 1: scoreAdd = 100; break;
    case 2: scoreAdd = 300; break;
    case 3: scoreAdd = 500; break;
    case 4: scoreAdd = 800; break;
    default: scoreAdd = linesCount * 200; break;
    }

    m_score += scoreAdd;
    m_linesCleared += linesCount;

    // Update level
    m_level = (m_linesCleared / 10) + 1;
    m_fallSpeed = 200.0f + m_level * 20.0f;
}

void PhysicsTetrisScene::updateLineClearAnimation(float dt) {
    m_lineClearTimer += dt;

    if (m_lineClearTimer >= LINE_CLEAR_DURATION) {
        // Remove completed lines
        for (auto* block : m_blocksToRemove) {
            // Remove from active blocks list
            m_activeBlocks.erase(
                std::remove_if(m_activeBlocks.begin(), m_activeBlocks.end(),
                    [block](const TetrisBlock& b) { return &b == block; }),
                m_activeBlocks.end()
            );

            // Remove entity
            m_entityManager->removeEntity(block->entity->getName());
        }

        m_blocksToRemove.clear();
        m_completedLines.clear();
        m_isClearing = false;
        m_lineClearTimer = 0.0f;

        // Apply gravity to remaining blocks
        applyGravityAfterLineClear();
    }
}

void PhysicsTetrisScene::applyGravityAfterLineClear() {
    // Give all settled blocks a downward velocity to fill gaps
    for (auto& block : m_activeBlocks) {
        if (block.settled) {
            if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
                physics->setVelocity(Vec2(0.0f, 200.0f));
                block.settled = false; // Allow them to settle again
            }
        }
    }
}

void PhysicsTetrisScene::updateUI() {
    // Update score
    if (auto* scoreEntity = m_entityManager->findEntity("ScoreText")) {
        if (auto* scoreText = scoreEntity->getComponent<TextComponent>()) {
            scoreText->setText(L"Score: " + std::to_wstring(m_score));
        }
    }

    // Update level
    if (auto* levelEntity = m_entityManager->findEntity("LevelText")) {
        if (auto* levelText = levelEntity->getComponent<TextComponent>()) {
            levelText->setText(L"Level: " + std::to_wstring(m_level));
        }
    }

    // Update lines
    if (auto* linesEntity = m_entityManager->findEntity("LinesText")) {
        if (auto* linesText = linesEntity->getComponent<TextComponent>()) {
            linesText->setText(L"Lines: " + std::to_wstring(m_linesCleared));
        }
    }
}

void PhysicsTetrisScene::checkGameOver() {
    // Check if any settled blocks are above the danger line
    float dangerLine = -GRID_HEIGHT * BLOCK_SIZE / 2.0f + BLOCK_SIZE * 2; // 2 blocks from top

    for (const auto& block : m_activeBlocks) {
        if (block.settled) {
            if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
                Vec2 pos = physics->getPosition();
                if (pos.y < dangerLine) {
                    m_gameRunning = false;
                    playSound("game_over");
                    showGameOverScreen();
                    break;
                }
            }
        }
    }
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

void PhysicsTetrisScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();

    // Set camera for world rendering
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());

    // Render world objects (boundaries, blocks)
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid() && !sprite->isScreenSpace()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render screen space UI
    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isScreenSpace()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render text components
    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx);
            }
        }
    }

    // Render buttons
    for (auto* entity : m_entityManager->getEntitiesWithComponent<ButtonComponent>()) {
        if (auto* button = entity->getComponent<ButtonComponent>()) {
            if (button->isVisible()) {
                button->draw(ctx);
            }
        }
    }

    engine.endFrame(swapChain);
}

void dx3d::PhysicsTetrisScene::fixedUpdate(float dt)
{
    // Update physics for all active blocks
    updatePhysics(dt);
}
void PhysicsTetrisScene::createSpringConstraints(const std::vector<TetrisBlock*>& pieceBlocks) {
    if (pieceBlocks.size() != 4) return;

    int pieceType = m_currentPiece;

    // Create springs between adjacent blocks in the tetromino
    for (size_t i = 0; i < pieceBlocks.size(); i++) {
        for (size_t j = i + 1; j < pieceBlocks.size(); j++) {
            auto* blockA = pieceBlocks[i];
            auto* blockB = pieceBlocks[j];

            auto* physicsA = blockA->entity->getComponent<TetrisPhysicsComponent>();
            auto* physicsB = blockB->entity->getComponent<TetrisPhysicsComponent>();

            if (!physicsA || !physicsB) continue;

            // Check if these blocks should be connected based on tetromino shape
            if (areBlocksAdjacent(blockA->blockIndex, blockB->blockIndex, pieceType)) {
                Vec2 posA = physicsA->getPosition();
                Vec2 posB = physicsB->getPosition();
                float restLength = calculateRestLength(posA, posB);

                auto spring = std::make_unique<SpringConstraint>(
                    physicsA, physicsB, restLength, m_springStrength, m_springDamping);

                m_springConstraints.push_back(std::move(spring));
            }
        }
    }
}

void PhysicsTetrisScene::updateSpringConstraints(float dt) {
    // Update all spring constraints
    for (auto& spring : m_springConstraints) {
        if (spring->getActive()) {
            spring->updateSpring(dt);

            // Check if spring should break due to excessive stretching
            if (spring->shouldBreak(m_maxSpringLength)) {
                spring->setActive(false);
            }
        }
    }

    // Remove inactive springs
    m_springConstraints.erase(
        std::remove_if(m_springConstraints.begin(), m_springConstraints.end(),
            [](const std::unique_ptr<SpringConstraint>& spring) {
                return !spring->getActive();
            }),
        m_springConstraints.end()
    );
}

void PhysicsTetrisScene::removeSpringConstraints(int pieceId) {
    // Remove springs for a specific piece when it settles
    m_springConstraints.erase(
        std::remove_if(m_springConstraints.begin(), m_springConstraints.end(),
            [pieceId, this](const std::unique_ptr<SpringConstraint>& spring) {
                // Check if either block belongs to the settled piece
                for (const auto& block : m_activeBlocks) {
                    if (block.pieceId == pieceId) {
                        auto* physics = block.entity->getComponent<TetrisPhysicsComponent>();
                        if (physics == spring->blockA || physics == spring->blockB) {
                            return true;
                        }
                    }
                }
                return false;
            }),
        m_springConstraints.end()
    );
}

void PhysicsTetrisScene::removeAllSpringConstraints() {
    m_springConstraints.clear();
}

float PhysicsTetrisScene::calculateRestLength(const Vec2& posA, const Vec2& posB) {
    return (posB - posA).length();
}

bool PhysicsTetrisScene::areBlocksAdjacent(int blockIndexA, int blockIndexB, int pieceType) {
    // This determines which blocks in a tetromino should be connected by springs
    // You'll need to define adjacency based on your TETRIS_SHAPES array

    // For now, let's create a simple adjacency map for standard tetrominoes
    static const std::vector<std::vector<std::vector<int>>> adjacencyMap = {
        // I-piece (0): linear connection 0-1-2-3
        {{1}, {0, 2}, {1, 3}, {2}},

        // J-piece (1): L-shaped
        {{1}, {0, 2}, {1, 3}, {2}},

        // L-piece (2): L-shaped (mirrored)
        {{1}, {0, 2}, {1, 3}, {2}},

        // O-piece (3): square - all connected to neighbors
        {{1, 2}, {0, 3}, {0, 3}, {1, 2}},

        // S-piece (4): Z-shaped
        {{1, 2}, {0, 3}, {0, 3}, {1, 2}},

        // T-piece (5): T-shaped
        {{1, 2, 3}, {0}, {0}, {0}},

        // Z-piece (6): Z-shaped (opposite of S)
        {{1, 2}, {0, 3}, {0, 3}, {1, 2}}
    };

    if (pieceType < 0 || pieceType >= adjacencyMap.size()) return false;
    if (blockIndexA < 0 || blockIndexA >= 4 || blockIndexB < 0 || blockIndexB >= 4) return false;

    const auto& adjacentBlocks = adjacencyMap[pieceType][blockIndexA];
    return std::find(adjacentBlocks.begin(), adjacentBlocks.end(), blockIndexB) != adjacentBlocks.end();
}