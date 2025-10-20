#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <vector>
#include <random>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <map>
#include <DX3D/Graphics/LineRenderer.h>
namespace dx3d
{
    enum class TetriminoType {
        I_PIECE,
        O_PIECE,
        T_PIECE,
        S_PIECE,
        Z_PIECE,
        J_PIECE,
        L_PIECE
    };

    // Frame component for rigid body reference
    class FrameComponent {
    public:
        FrameComponent(Vec2 position = Vec2(0.0f, 0.0f), float rotation = 0.0f)
            : m_position(position), m_rotation(rotation), m_angularVelocity(0.0f), m_velocity(0.0f, 0.0f), m_startPosition(position), m_startRotation(rotation) {
        }

        Vec2 getPosition() const { return m_position; }
        void setPosition(const Vec2& pos) { m_position = pos; }
        float getRotation() const { return m_rotation; }
        void setRotation(float rot) { m_rotation = rot; }
        
        float getAngularVelocity() const { return m_angularVelocity; }
        void setAngularVelocity(float angularVel) { m_angularVelocity = angularVel; }
        void addAngularVelocity(float deltaAngularVel) { m_angularVelocity += deltaAngularVel; }
        
        Vec2 getVelocity() const { return m_velocity; }
        void setVelocity(const Vec2& vel) { m_velocity = vel; }
        void addVelocity(const Vec2& deltaVel) { m_velocity += deltaVel; }

        Vec2 getStartPosition() const { return m_startPosition; }
        float getStartRotation() const { return m_startRotation; }

        void reset() {
            m_position = m_startPosition;
            m_rotation = m_startRotation;
            m_angularVelocity = 0.0f;
            m_velocity = Vec2(0.0f, 0.0f);
        }

    private:
        Vec2 m_position;
        float m_rotation;
        float m_angularVelocity;
        Vec2 m_velocity;
        Vec2 m_startPosition;
        float m_startRotation;
    };

    struct TetriminoData {
        std::vector<Vec2> nodeOffsets;
        std::vector<std::pair<int, int>> beamConnections; // pairs of node indices to connect
        Vec4 color;
        std::string name;
    };

    class PhysicsTetrisScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
        void fixedUpdate(float dt) override;
        void updateCameraMovement(float dt);
        void updateUI();
        void renderFrameDebug(DeviceContext& ctx);
        void updateFrameDebugVisualization(); 
        void updateFrameDebug(float dt);  
        Vec2 calculateCenterOfMass(const TetriminoData& data);
        float calculateRotationFromNodes(const std::vector<Vec2>& nodePositions, const std::string& tetriminoPrefix);
        float calculateAngularVelocityFromNodes(const std::vector<Vec2>& nodePositions, const std::vector<Vec2>& nodeVelocities, const Vec2& centerOfMass);
        void applyAngularImpulseToFrame(Entity* nodeEntity, const Vec2& nodePosition, const Vec2& impulse);
    private:
        LineRenderer* m_lineRenderer = nullptr;
        bool m_showFrameDebug = false;
        
        // Debug physics parameters
        float m_debugFrameGravity = 0.1f;
        float m_debugNodeGravityColliding = 0.2f;
        float m_debugCollisionThreshold = 0.8f;


        // Helper functions for debug rendering
        float calculateTotalSpringForce(const std::string& tetriminoPrefix);
        void renderDebugTetrimino(const std::string& tetriminoPrefix, const Vec2& screenPosition, float scale);
        

        bool isNodePartOfBeam(Entity* nodeEntity, Entity* beamEntity);
        bool checkNodeBeamCollision(Entity* nodeEntity, Entity* beamEntity);
        float distancePointToLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd);
        void resolveNodeBeamCollision(Entity* nodeEntity, Entity* beamEntity);
        Vec2 getClosestPointOnLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd);
        // Tetrimino management
        void initializeTetriminoTemplates();
        void spawnTetrimino(TetriminoType type, Vec2 position);
        void spawnNextTetrimino();
        std::string createTetriminoNodes(const TetriminoData& data, Vec2 basePosition, int tetriminoId);
        void createTetriminoBeams(const TetriminoData& data, const std::string& baseName, int tetriminoId);
        void createTetriminoFrame(const std::string& baseName, Vec2 position, int tetriminoId);

        // Play field
        void createPlayField();
        void createBoundaryWalls();
        void createDebugToggleButton();
        void createGameOverPanel();

        // Collision detection
        void updateCollisions();
        void updateFrameBasedPhysics(float dt);
        bool checkAABBCollision(const Vec2& pos1, float size1, const Vec2& pos2, float size2);
        void resolveNodeCollision(Entity* node1, Entity* node2);

        // Game logic
        void handleTetriminoInput(float dt);
        void checkTetriminoLanded();
        bool isTetriminoActive(int tetriminoId);
        void lockTetrimino(int tetriminoId);
        void checkCompletedRows();
        void removeCompletedRow(int row);
        bool isRowComplete(int row);
        void checkAndClearLines();
        bool isLineComplete(float y, float tolerance = 10.0f);
        void clearLine(float y, float tolerance = 10.0f);
        void renderLineScanDebug();
        void renderLineProgressBars();
        float calculateLineProgress(float y, float tolerance = 10.0f);
        void applyGravityToOrphanedNodes(const std::set<std::string>& affectedTetriminos, const std::vector<std::string>& removedFrames);
        bool isNodeOrphaned(const std::string& nodeName);
        void applyGravityToAllOrphanedNodes();
        void applyGravityToUnstableTetriminos();
        void checkGameOverCondition();
        void handleGameOver();
        void restartGame();
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;

        // Game state
        std::vector<TetriminoData> m_tetriminoTemplates;
        std::random_device m_randomDevice;
        std::mt19937 m_randomGen;
        std::uniform_int_distribution<> m_tetriminoDist;

        float m_spawnTimer = 0.0f;
        float m_spawnInterval = 3.0f; // seconds between spawns
        int m_nextTetriminoId = 0;
        int m_currentTetriminoId = -1; // Track which tetrimino is currently controllable
        bool m_hasActiveTetrimino = false; // Only spawn when no active tetrimino
        
        // Game state
        bool m_gameOver = false;
        float m_gameOverTimer = 0.0f;

        // Play field constants
        static constexpr float PLAY_FIELD_WIDTH = 300.0f;
        static constexpr float PLAY_FIELD_HEIGHT = 600.0f;
        static constexpr float WALL_THICKNESS = 20.0f;
        static constexpr float NODE_SIZE = 20.0f;

        // Row completion tracking
        static constexpr int GRID_WIDTH = 12;
        static constexpr int GRID_HEIGHT = 20;
        static constexpr float CELL_SIZE = 40.0f;
        
        // Line clearing
        static constexpr int LINE_CLEAR_NODE_THRESHOLD = 12;
        static constexpr float LINE_SCAN_SPACING = 20.0f;
        static constexpr float LINE_SCAN_TOLERANCE = 10.0f;
        
        // Physics constants
        static constexpr float GRAVITY_ACCELERATION = 300.0f;
        static constexpr float FRAME_TIME_ESTIMATE = 0.016f; // ~60fps
        static constexpr float AIR_RESISTANCE = 0.995f;
        static constexpr float FRAME_STIFFNESS_ACTIVE = 5000.0f;
        static constexpr float FRAME_STIFFNESS_LOCKED = 2000.0f;
        static constexpr float DAMPING_COEFF_ACTIVE = 80.0f;
        static constexpr float DAMPING_COEFF_LOCKED = 40.0f;
        static constexpr float DAMPING_FACTOR_ACTIVE = 0.99f;
        static constexpr float DAMPING_FACTOR_LOCKED = 0.95f;
        static constexpr float MASS_FACTOR = 0.02f;
        static constexpr float BEAM_THICKNESS = 8.0f;
        static constexpr float COLLISION_BOUNCE_FACTOR = 1.2f;
        static constexpr float COLLISION_DAMPING = 0.8f;
        static constexpr float VELOCITY_REFLECTION = 1.5f;
        static constexpr float BOUNDARY_BOUNCE = 0.2f;
        
        // Movement and input constants
        static constexpr float HORIZONTAL_MOVEMENT = 20.0f;
        static constexpr float FRAME_BOUNDARY_PADDING = 30.0f;  // Reduced from 60.0f
        static constexpr float NODE_BOUNDARY_PADDING = 5.0f;    // Reduced from NODE_SIZE/2
        static constexpr float FAST_DROP_MOVEMENT = 10.0f;
        static constexpr float MOVEMENT_IMPULSE = 8.0f;
        static constexpr float DROP_IMPULSE = 5.0f;
        static constexpr float ROTATION_ANGLE = 1.5708f; // 90 degrees in radians
        static constexpr float ROTATION_IMPULSE = 3.0f;
        static constexpr float MOVE_TIMER_INTERVAL = 0.15f;
        static constexpr float FAST_DROP_TIMER_INTERVAL = 0.03f;
        static constexpr float ROTATE_TIMER_INTERVAL = 0.25f;
        
        // Game timing constants
        static constexpr float DEFAULT_SPAWN_INTERVAL = 3.0f;
        static constexpr float GRAVITY_DROP_RATE = 9.8f;
        static constexpr float LOCKED_PIECE_GRAVITY_FACTOR = 0.2f;
        static constexpr float SETTLED_VELOCITY_THRESHOLD = 30.0f;
        static constexpr float ORPHAN_GRAVITY_IMPULSE = 200.0f;
        static constexpr float ORPHAN_HORIZONTAL_REDUCTION = 0.5f;
        
        // Camera constants
        static constexpr float CAMERA_BASE_SPEED = 300.0f;
        static constexpr float CAMERA_FAST_SPEED = 600.0f;
        static constexpr float CAMERA_ZOOM_SPEED = 2.0f;
        static constexpr float DEFAULT_CAMERA_ZOOM = 0.8f;
        
        // Game over constants
        static constexpr float GAME_OVER_THRESHOLD = 50.0f;
        
        // UI constants
        static constexpr float DEBUG_BUTTON_FONT_SIZE = 16.0f;
        static constexpr float DEBUG_BUTTON_PADDING_X = 15.0f;
        static constexpr float DEBUG_BUTTON_PADDING_Y = 8.0f;
        static constexpr float DEBUG_BUTTON_SCREEN_X = 0.8f;
        static constexpr float DEBUG_BUTTON_SCREEN_Y = 0.8f;
        static constexpr float GAME_OVER_PANEL_WIDTH = 400.0f;
        static constexpr float GAME_OVER_PANEL_HEIGHT = 200.0f;
        static constexpr float GAME_OVER_FONT_SIZE = 32.0f;
        static constexpr float GAME_OVER_SCREEN_X = 0.5f;
        static constexpr float GAME_OVER_SCREEN_Y = 0.5f;
        
        // Debug visualization constants
        static constexpr float DEBUG_CROSS_SIZE = 20.0f;
        static constexpr float DEBUG_CENTER_SIZE = 10.0f;
        static constexpr float DEBUG_ORIENTATION_LENGTH = 25.0f;
        static constexpr float DEBUG_TARGET_CIRCLE_RADIUS = 3.0f;
        static constexpr float DEBUG_LINE_THICKNESS = 1.0f;
        static constexpr float DEBUG_THICK_LINE = 2.0f;
        static constexpr float DEBUG_VERY_THICK_LINE = 3.0f;
        static constexpr int DEBUG_CIRCLE_SEGMENTS = 8;

        // Cached entity collections (update only when entities are added/removed)
		//need to implement entity cache invalidation when entities are added/removed
        std::vector<Entity*> m_nodeEntities;
        std::vector<Entity*> m_beamEntities;
        std::vector<Entity*> m_frameEntities;
        bool m_entitiesCacheDirty = true;
        
        
        void updateEntityCache();
        void markEntityCacheDirty() { m_entitiesCacheDirty = true; }
		//maybe some spatial partiioning for optimization, but string concatenation is probably my main bottleneck

    };
}