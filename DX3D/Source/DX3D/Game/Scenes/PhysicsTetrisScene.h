#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <vector>
#include <random>
#include <memory>
#include <DX3D/Core/EntityManager.h>
#include<DX3D/Systems/AdvancedSlicingSystem.h>
#include <DX3D/Components/TetrisPhysicsComponent.h>

namespace dx3d
{
    static const Vec2 TETRIS_SHAPES[7][4] = {
        // I-piece
        {{-1.5f, 0.0f}, {-0.5f, 0.0f}, {0.5f, 0.0f}, {1.5f, 0.0f}},
        // J-piece  
        {{-1.0f, -0.5f}, {0.0f, -0.5f}, {1.0f, -0.5f}, {1.0f, 0.5f}},
        // L-piece
        {{-1.0f, -0.5f}, {0.0f, -0.5f}, {1.0f, -0.5f}, {-1.0f, 0.5f}},
        // O-piece
        {{-0.5f, -0.5f}, {0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f}},
        // S-piece
        {{-1.0f, 0.5f}, {0.0f, -0.5f}, {1.0f, -0.5f}, {0.0f, 0.5f}},
        // T-piece
        {{-1.0f, -0.5f}, {0.0f, -0.5f}, {1.0f, -0.5f}, {0.0f, 0.5f}},
        // Z-piece
        {{0.0f, 0.5f}, {0.0f, -0.5f}, {1.0f, 0.5f}, {-1.0f, -0.5f}}
    };

    class PhysicsTetrisScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;
    private:

        std::vector<std::unique_ptr<SpringConstraint>> m_springConstraints;
        float m_springStrength = 800.0f;  // Adjustable spring strength
        float m_springDamping = 0.8f;     // Damping factor
        float m_maxSpringLength = 2.5f;   // Max stretch before breaking (multiplier of rest length)

        // Constants
        static constexpr int GRID_WIDTH = 10;
        static constexpr int GRID_HEIGHT = 20;
        static constexpr float BLOCK_SIZE = 32.0f;
        static constexpr float LINE_CLEAR_DURATION = 1.0f;

        // Core systems
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;
        std::random_device m_randomDevice;
        std::mt19937 m_random{ m_randomDevice() };

        // Game state
        bool m_gameRunning;
        int m_currentPiece;
        int m_nextPiece;
        int m_score;
        int m_level;
        int m_linesCleared;
        float m_fallSpeed;
        int m_pieceIdCounter;

        // Line clearing state
        float m_lineClearTimer;
        bool m_isClearing;
        std::vector<int> m_completedLines;

        // Grid and block tracking
        struct TetrisBlock {
            Entity* entity;
            int pieceId;
            int blockIndex;
            bool settled;
            std::vector<std::unique_ptr<SpringConstraint>> springs; // Springs connected to this block

            TetrisBlock() : entity(nullptr), pieceId(-1), blockIndex(-1), settled(false) {}
        };


        // Methods for spring constraint system
        void createSpringConstraints(const std::vector<TetrisBlock*>& pieceBlocks);
        void updateSpringConstraints(float dt);
        void removeSpringConstraints(int pieceId);
        void removeAllSpringConstraints();
        float calculateRestLength(const Vec2& posA, const Vec2& posB);
        bool areBlocksAdjacent(int blockIndexA, int blockIndexB, int pieceType);


        std::vector<TetrisBlock> m_activeBlocks;
        std::vector<TetrisBlock*> m_blocksToRemove;
        Entity* m_grid[GRID_HEIGHT][GRID_WIDTH];
        float m_lineAreas[GRID_HEIGHT];

        // Private methods
        void createBoundaries();
        void createUI();
        void spawnNewPiece();
        void createNextPiecePreview();
        void removeNextPiecePreview();

        void handleInput(float dt);
        void updatePhysics(float dt);
        void updateLineAreas();
        void checkForCompletedLines();

        void startLineClear(const std::vector<int>& lines);
        void updateLineClearAnimation(float dt);
        void applyGravityAfterLineClear();
        void updateUI();
        void checkGameOver();
        void updateCameraMovement(float dt);
        void updateSpritePositions();
        void performAdvancedLineClear(const std::vector<int>& completedLines) {
            m_isClearing = true;
            m_lineClearTimer = 0.0f;
            m_completedLines = completedLines;

            // Collect all blocks that will be affected
            std::vector<Entity*> affectedBlocks;
            std::vector<Entity*> blocksToDestroy;

            for (auto& block : m_activeBlocks) {
                if (!block.settled) continue;

                if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
                    Vec2 pos = physics->getPosition();
                    float blockTop = pos.y - 16.0f; // Half block size
                    float blockBottom = pos.y + 16.0f;

                    bool affectedByLine = false;
                    for (int line : completedLines) {
                        float lineY = (line - GRID_HEIGHT / 2) * BLOCK_SIZE;
                        float lineTop = lineY - BLOCK_SIZE / 2.0f;
                        float lineBottom = lineY + BLOCK_SIZE / 2.0f;

                        // Check if block intersects with any completed line
                        if (!(blockBottom < lineTop || blockTop > lineBottom)) {
                            affectedByLine = true;
                            break;
                        }
                    }

                    if (affectedByLine) {
                        affectedBlocks.push_back(block.entity);
                        blocksToDestroy.push_back(block.entity);
                    }
                }
            }

            // Perform slicing for each completed line
            for (int line : completedLines) {
                float lineY = (line - GRID_HEIGHT / 2) * BLOCK_SIZE;

                // Slice all affected blocks at this line
                auto slicedParts = AdvancedSlicingSystem::sliceBlocksAtLine(
                    affectedBlocks, lineY, BLOCK_SIZE
                );

                // Create new entities for sliced parts
                for (const auto& part : slicedParts) {
                    if (part.vertices.size() >= 3) { // Valid polygon
                        createSlicedBlock(part);
                    }
                }
            }

            // Remove original blocks that were sliced
            for (Entity* blockEntity : blocksToDestroy) {
                // Remove from active blocks list
                m_activeBlocks.erase(
                    std::remove_if(m_activeBlocks.begin(), m_activeBlocks.end(),
                        [blockEntity](const TetrisBlock& b) { return b.entity == blockEntity; }),
                    m_activeBlocks.end()
                );

                // Remove entity
                m_entityManager->removeEntity(blockEntity->getName());
            }

            // Add particle effects for dramatic slicing
            addSliceEffects(completedLines);
        }

        void createSlicedBlock(const SliceData& sliceData) {
            if (sliceData.vertices.empty()) return;

            std::string blockName = "SlicedBlock_" + std::to_string(m_pieceIdCounter++);
            auto& blockEntity = m_entityManager->createEntity(blockName);

            // Calculate size and position from vertices
            Vec2 minPos = sliceData.vertices[0];
            Vec2 maxPos = sliceData.vertices[0];

            for (const Vec2& vertex : sliceData.vertices) {
                minPos.x = std::min(minPos.x, vertex.x);
                minPos.y = std::min(minPos.y, vertex.y);
                maxPos.x = std::max(maxPos.x, vertex.x);
                maxPos.y = std::max(maxPos.y, vertex.y);
            }

            Vec2 size = maxPos - minPos;
            Vec2 center = sliceData.centerOfMass;

            // Visual component
            auto& sprite = blockEntity.addComponent<SpriteComponent>(
                *m_graphicsDevice, L"DX3D/Assets/Textures/node.png",
                size.x, size.y
            );
            sprite.setPosition(center.x, center.y, -5.0f);
            sprite.setTint(sliceData.color);

            // Physics component
            auto& physics = blockEntity.addComponent<TetrisPhysicsComponent>(center, false);
            physics.setMass(sliceData.mass);
            physics.setVelocity(Vec2((rand() % 200) - 100.0f, -50.0f)); // Random scatter velocity

            // Add to active blocks
            TetrisBlock newBlock;
            newBlock.entity = &blockEntity;
            newBlock.pieceId = m_pieceIdCounter;
            newBlock.blockIndex = 0;
            newBlock.settled = false;
            m_activeBlocks.push_back(newBlock);
        }

        void addSliceEffects(const std::vector<int>& lines) {
            // Create particle effects or screen shake for dramatic slicing
            for (int line : lines) {
                float lineY = (line - GRID_HEIGHT / 2) * BLOCK_SIZE;

                // Create temporary visual effects
                for (int i = 0; i < 10; i++) {
                    std::string effectName = "SliceEffect_" + std::to_string(line) + "_" + std::to_string(i);
                    auto& effectEntity = m_entityManager->createEntity(effectName);

                    auto& sprite = effectEntity.addComponent<SpriteComponent>(
                        *m_graphicsDevice, L"DX3D/Assets/Textures/node.png",
                        8.0f, 8.0f
                    );

                    float x = (rand() % static_cast<int>(GRID_WIDTH * BLOCK_SIZE)) - GRID_WIDTH * BLOCK_SIZE / 2.0f;
                    sprite.setPosition(x, lineY, -10.0f);
                    sprite.setTint(Vec4(1.0f, 0.5f, 0.0f, 0.8f)); // Orange particles

                    // Add animation to make particles fade and move
                    auto& animation = effectEntity.addComponent<AnimationComponent>();
                    animation.setUpdateFunction([](Entity& entity, float dt) {
                        static float timer = 0.0f;
                        timer += dt;

                        if (auto* sprite = entity.getComponent<SpriteComponent>()) {
                            Vec4 tint = sprite->getTint();
                            tint.w = std::max(0.0f, tint.w - dt * 2.0f); // Fade out
                            sprite->setTint(tint);

                            Vec3 pos = sprite->getPosition();
                            pos.y -= 100.0f * dt; // Move up
                            pos.x += (rand() % 20 - 10) * dt; // Random drift
                            sprite->setPosition(pos.x, pos.y, pos.z);

                            // Remove when fully transparent
                            //if (tint.w <= 0.0f) {
                            //    // Mark for removal (you'd need to implement this)
                            //}
                        }
                        });
                }
            }
        }
        // Enhanced collision detection between blocks
        bool checkBlockCollision(Entity* block1, Entity* block2) {
            if (!block1 || !block2) return false;

            auto* physics1 = block1->getComponent<TetrisPhysicsComponent>();
            auto* physics2 = block2->getComponent<TetrisPhysicsComponent>();

            if (!physics1 || !physics2) return false;

            Vec2 pos1 = physics1->getPosition();
            Vec2 pos2 = physics2->getPosition();

            float distance = (pos1 - pos2).length();
            float minDistance = BLOCK_SIZE; // Minimum distance for collision

            return distance < minDistance;
        }

        // Check if a piece can be placed at a specific position (for spawn checking)
        bool canPlacePieceAt(int pieceType, Vec2 position) {
            for (int i = 0; i < 4; i++) {
                Vec2 blockPos = position + TETRIS_SHAPES[pieceType][i] * BLOCK_SIZE;

                // Check boundaries
                if (blockPos.x < -GRID_WIDTH * BLOCK_SIZE / 2.0f + BLOCK_SIZE / 2 ||
                    blockPos.x > GRID_WIDTH * BLOCK_SIZE / 2.0f - BLOCK_SIZE / 2 ||
                    blockPos.y > GRID_HEIGHT * BLOCK_SIZE / 2.0f - BLOCK_SIZE / 2) {
                    return false;
                }

                // Check collision with existing settled blocks
                for (const auto& block : m_activeBlocks) {
                    if (block.settled) {
                        if (auto* physics = block.entity->getComponent<TetrisPhysicsComponent>()) {
                            Vec2 existingPos = physics->getPosition();
                            if ((blockPos - existingPos).length() < BLOCK_SIZE * 0.9f) {
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        // Hard drop functionality
        void hardDropCurrentPiece() {
            // Find current piece blocks
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

            // Apply strong downward force
            for (auto* block : currentPieceBlocks) {
                if (auto* physics = block->entity->getComponent<TetrisPhysicsComponent>()) {
                    physics->setVelocity(Vec2(0.0f, 800.0f)); // Fast drop
                }
            }
        }

        // Rotate current piece (simplified rotation)
        void rotateCurrentPiece() {
            // Find current piece blocks
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

            // Calculate center of piece
            Vec2 center(0.0f, 0.0f);
            for (auto* block : currentPieceBlocks) {
                if (auto* physics = block->entity->getComponent<TetrisPhysicsComponent>()) {
                    center += physics->getPosition();
                }
            }
            center /= static_cast<f32>(currentPieceBlocks.size());

            // Apply rotation torque to all blocks
            for (auto* block : currentPieceBlocks) {
                if (auto* physics = block->entity->getComponent<TetrisPhysicsComponent>()) {
                    physics->addTorque(500.0f);
                }
            }
        }

        // Enhanced input handling with more controls
        void handleEnhancedInput(float dt) {
            auto& input = Input::getInstance();

            if (m_activeBlocks.empty()) return;

            // Find current piece blocks
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

            // Movement forces
            Vec2 force(0.0f, 0.0f);

            if (input.isKeyDown(Key::Left) || input.isKeyDown(Key::A)) {
                force.x = -400.0f;
            }
            if (input.isKeyDown(Key::Right) || input.isKeyDown(Key::D)) {
                force.x = 400.0f;
            }
            if (input.isKeyDown(Key::Down) || input.isKeyDown(Key::S)) {
                force.y = 600.0f; // Soft drop
            }

            // Rotation (on key press, not hold)
            if (input.wasKeyJustPressed(Key::Up) || input.wasKeyJustPressed(Key::W)) {
                rotateCurrentPiece();
            }

            // Hard drop
            if (input.wasKeyJustPressed(Key::Space)) {
                hardDropCurrentPiece();
            }

            // Apply forces to current piece
            for (auto* block : currentPieceBlocks) {
                if (auto* physics = block->entity->getComponent<TetrisPhysicsComponent>()) {
                    physics->addForce(force);
                }
            }
        }

       

        // Sound effects integration (if you have an audio system)
        void playSound(const std::string& soundName) {
            // Integrate with your audio system
            // Examples:
            // - "piece_drop" when piece lands
            // - "line_clear" when lines are cleared
            // - "piece_rotate" when piece rotates
            // - "game_over" when game ends
        }

        // Save/load high scores
        void updateHighScore() {
            // Implement high score saving/loading
            static int highScore = 0;
            if (m_score > highScore) {
                highScore = m_score;
                // Save to file or registry
            }
        }

        // Enhanced game over screen
        void showGameOverScreen() {
            // Create game over UI elements
            auto& gameOverEntity = m_entityManager->createEntity("GameOverText");
            auto& gameOverText = gameOverEntity.addComponent<TextComponent>(
                *m_graphicsDevice, TextSystem::getRenderer(),
                L"GAME OVER", 48.0f
            );
            gameOverText.setScreenPosition(0.5f, 0.4f);
            gameOverText.setColor(Vec4(1.0f, 0.0f, 0.0f, 1.0f));

            auto& finalScoreEntity = m_entityManager->createEntity("FinalScoreText");
            auto& finalScoreText = finalScoreEntity.addComponent<TextComponent>(
                *m_graphicsDevice, TextSystem::getRenderer(),
                L"Final Score: " + std::to_wstring(m_score), 24.0f
            );
            finalScoreText.setScreenPosition(0.5f, 0.5f);
            finalScoreText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));

            // Add restart button
            auto& restartButtonEntity = m_entityManager->createEntity("RestartButton");
            auto& restartButton = restartButtonEntity.addComponent<ButtonComponent>(
                *m_graphicsDevice, L"Restart", 32.0f
            );
            restartButton.setScreenPosition(0.5f, 0.6f);
            restartButton.setOnClickCallback([this]() {
                resetGame();
                });
        }

        // Reset game state
        void resetGame() {
            // Clear all blocks
            m_activeBlocks.clear();

            // Remove all block entities
            //auto allEntities = m_entityManager->getEntities();
            //for (auto entity : allEntities) {
            //    if (entity->getName().find("Block_") == 0 ||
            //        entity->getName().find("SlicedBlock_") == 0) {
            //        m_entityManager->removeEntity(entity->getName());
            //    }
            //}

            // Reset game state
            m_gameRunning = true;
            m_currentPiece = -1;
            m_nextPiece = m_random() % 7;
            m_score = 0;
            m_level = 1;
            m_linesCleared = 0;
            m_fallSpeed = 200.0f;
            m_pieceIdCounter = 0;
            m_isClearing = false;
            m_lineClearTimer = 0.0f;

            // Clear grid
            for (int y = 0; y < GRID_HEIGHT; y++) {
                for (int x = 0; x < GRID_WIDTH; x++) {
                    m_grid[y][x] = nullptr;
                }
                m_lineAreas[y] = 0.0f;
            }

            // Remove game over UI
            m_entityManager->removeEntity("GameOverText");
            m_entityManager->removeEntity("FinalScoreText");
            m_entityManager->removeEntity("RestartButton");

            // Spawn new piece
            spawnNewPiece();
        }

/*
USAGE NOTES AND INTEGRATION STEPS:

1. Make sure you have these texture files in your Assets/Textures folder:
   - node.png (for tetris blocks)
   - wall.png (for boundaries)
   - beam.png (for UI backgrounds)

2. Update your input system to handle the Key enum values used:
   - Key::Left, Key::Right, Key::Up, Key::Down
   - Key::A, Key::S, Key::W, Key::D
   - Key::Space, Key::Shift

3. Implement the PhysicsComponent class with the methods used:
   - getPosition(), setPosition()
   - getVelocity(), setVelocity()
   - addForce(), addTorque()
   - update(dt)
   - Basic collision detection

4. Make sure your Vec2 and Vec4 classes support:
   - Basic arithmetic operations (+, -, *, /)
   - length() method for Vec2
   - Component access (x, y for Vec2; x, y, z, w or r, g, b, a for Vec4)

5. Your SpriteComponent should support:
   - setPosition(x, y, z)
   - setTint(Vec4)
   - getTint()
   - enableScreenSpace(bool)
   - setScreenPosition(x, y)

6. TextSystem integration:
   - Make sure TextSystem::initialize() is called
   - TextComponent should support setText() and setScreenPosition()

7. Optional enhancements you can add:
   - Sound effects system integration
   - Particle effects for line clears
   - More sophisticated collision detection
   - Save/load system for high scores
   - Different difficulty levels
   - Hold piece functionality
   - Ghost piece preview

8. Physics tuning parameters you may want to adjust:
   - GRAVITY constant in PhysicsComponent
   - Fall speed progression (m_fallSpeed)
   - Force magnitudes for movement and rotation
   - Collision damping and restitution
   - Line clear threshold (currently 80% filled)

9. Performance considerations:
   - Consider object pooling for blocks if creating/destroying many
   - Limit number of active physics objects
   - Use spatial partitioning for collision detection if needed

10. Debug features to help development:
    - Grid line rendering
    - Physics body visualization
    - Line area visualization
    - Performance metrics display
*/


    };
}