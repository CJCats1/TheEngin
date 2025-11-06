#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <vector>
#include <random>
#include <memory>
#include <string>
#include <map>

namespace dx3d
{
    // Tetrimino types
    enum class TetriminoReduxType {
        I_PIECE = 0,
        O_PIECE = 1,
        T_PIECE = 2,
        S_PIECE = 3,
        Z_PIECE = 4,
        J_PIECE = 5,
        L_PIECE = 6
    };

    // Square piece component - represents a single square made of 4 nodes
    class SquarePiece {
    public:
        SquarePiece(Vec2 centerPosition, Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f))
            : m_centerPosition(centerPosition), m_color(color) {
            // Create 4 nodes in a square pattern around the center (25x25 square)
            m_nodeOffsets = {
                Vec2(-12.5f, -12.5f), Vec2(12.5f, -12.5f), 
                Vec2(12.5f, 12.5f), Vec2(-12.5f, 12.5f)
            };
            m_beamConnections = { {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 2}, {1, 3} };
        }

        Vec2 getCenterPosition() const { return m_centerPosition; }
        void setCenterPosition(const Vec2& pos) { m_centerPosition = pos; }
        Vec4 getColor() const { return m_color; }
        void setColor(const Vec4& color) { m_color = color; }
        
        const std::vector<Vec2>& getNodeOffsets() const { return m_nodeOffsets; }
        const std::vector<std::pair<int, int>>& getBeamConnections() const { return m_beamConnections; }

    private:
        Vec2 m_centerPosition;
        Vec4 m_color;
        std::vector<Vec2> m_nodeOffsets;
        std::vector<std::pair<int, int>> m_beamConnections;
    };

    // Jelly Tetris specific data structure
    struct JellyTetriminoData {
        std::vector<SquarePiece> squarePieces; // Each tetrimino is made of connected squares
        Vec4 color;
        std::string name;
    };

    class JellyTetrisReduxScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void fixedUpdate(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
        void updateCameraMovement(float dt);
        void renderTetraminoVisualOverlays(DeviceContext& ctx);
        void renderIndividualSquareOverlays(DeviceContext& ctx, const std::vector<Entity*>& nodes, const Vec4& color);
        void renderIndividualSquare(DeviceContext& ctx, const Vec2& minPos, const Vec2& maxPos, const Vec4& color);
        
        // Collision handling
        void updateCollisions();
        void checkBoundaryCollisions();
        void checkNodeCollisions();
        void checkNodeBeamCollisions();
        void resolveNodeCollision(NodeComponent& node1, NodeComponent& node2);
        void resolveNodeBeamCollision(NodeComponent& node, BeamComponent& beam);
        void resolveBeamBeamCollision(BeamComponent& beam1, BeamComponent& beam2);
        float distancePointToLineSegment(const Vec2& point, const Vec2& lineStart, const Vec2& lineEnd);
        float distanceLineSegmentToLineSegment(const Vec2& line1Start, const Vec2& line1End, const Vec2& line2Start, const Vec2& line2End);
        void addAirResistance();
    private:
        
        // Tetrimino management
        void initializeTetriminoTemplates();
        void spawnTetrimino(TetriminoReduxType type, Vec2 position);
        std::string createTetriminoNodes(const JellyTetriminoData& data, Vec2 basePosition, int tetriminoId);
        void createTetriminoBeams(const JellyTetriminoData& data, const std::string& baseName, int tetriminoId);

        // Play field
        void createPlayField();
        
        // Test mode
        void spawnTestTetramino(TetriminoReduxType type);
        void clearTestTetraminos();
        void toggleTestMode();
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;

        // Game state
        std::vector<JellyTetriminoData> m_tetriminoTemplates;
        std::random_device m_randomDevice;
        std::mt19937 m_randomGen;
        std::uniform_int_distribution<> m_tetriminoDist;

        int m_nextTetriminoId = 0;
        bool m_testMode = false;
        
        // Input control
        void handleTetraminoInput();
        Entity* getMostRecentTetramino();
        
        // Node dragging functionality
        void updateNodeDragging();
        Entity* findNodeUnderMouse(const Vec2& worldMousePos);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
        
        // Physics parameters for ImGui controls
        float m_airResistance = 0.995f;
        float m_collisionRestitution = 0.1f;
        float m_collisionDamping = 0.3f;
        float m_collisionSpeedThreshold = 1.0f;
        float m_bottomBounceThreshold = 5.0f;
        float m_bottomBounceDamping = 0.2f;
        float m_tetraminoMoveSpeed = 3.0f; // Much slower movement
        float m_tetraminoForceMultiplier = 20.0f; // Reduced to prevent explosions
        float m_tetraminoRotationSpeed = 2.0f; // Rotation speed
        float m_tetraminoRotationForceMultiplier = 1.0f; // Rotation force strength
        
        // Node dragging state
        Entity* m_draggedNode = nullptr;
        bool m_isDraggingNode = false;
        Vec2 m_dragOffset;
        Vec2 m_lastMousePosition;
        // Spring dragging params
        float m_dragSpringStiffness = 60.0f; // k
        float m_dragSpringDamping = 10.0f;   // c
        float m_dragMaxForce = 800.0f;       // clamp force
        
        // Cached references for performance
        Camera2D* m_cachedCamera = nullptr;
        bool m_cameraCacheValid = false;
        
        // Cached component references for dragged node
        NodeComponent* m_cachedDraggedNode = nullptr;
        SpriteComponent* m_cachedDraggedSprite = nullptr;
        
        // FPS tracking
        float m_fpsTimer = 0.0f;
        int m_frameCount = 0;
        float m_currentFPS = 0.0f;
        
        // Performance timing
        float m_collisionTime = 0.0f;
        float m_physicsTime = 0.0f;
        float m_dragTime = 0.0f;
        
        // Broadphase collision detection
        struct SpatialCell {
            std::vector<Entity*> beams;
            std::vector<Entity*> nodes;
        };
        
        // Spatial grid for broadphase collision detection
        static constexpr float GRID_CELL_SIZE = 50.0f; // Smaller cells for better distribution
        std::unordered_map<std::string, SpatialCell> m_spatialGrid;
        bool m_spatialGridDirty = true;
        
        // Collision optimization settings
        bool m_enableCollisions = true;
        int m_maxCollisionChecksPerFrame = 1000; // Limit collision checks per frame
        int m_collisionCheckCounter = 0;
        
        // Broadphase methods
        void updateSpatialGrid();
        std::string getGridKey(const Vec2& position);
        void getGridKeys(const Vec2& min, const Vec2& max, std::vector<std::string>& keys);
        void checkCollisionsInCell(const std::string& cellKey);

        // Play field constants
        static constexpr float PLAY_FIELD_WIDTH = 300.0f;
        static constexpr float PLAY_FIELD_HEIGHT = 600.0f;
        static constexpr float WALL_THICKNESS = 20.0f;
        static constexpr float NODE_SIZE = 20.0f;
        
        // Camera constants
        static constexpr float CAMERA_BASE_SPEED = 300.0f;
        static constexpr float CAMERA_FAST_SPEED = 600.0f;
        static constexpr float CAMERA_ZOOM_SPEED = 2.0f;
        static constexpr float DEFAULT_CAMERA_ZOOM = 0.8f;
    };
}
