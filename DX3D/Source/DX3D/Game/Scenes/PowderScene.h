#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Math/Geometry.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace dx3d
{
    class Texture2D;

    // Grid-based powder/sand simulation inspired by The Powder Toy
    // Uses cellular automata approach for performance
    class PowderScene : public Scene
    {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void fixedUpdate(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;

    private:
        // States of matter - base behaviors for particles
        enum class MatterState : uint8_t
        {
            Solid = 0,   // Do not move via non-chemical reactions
            Powder = 1,  // Only fall via non-chemical reactions, but will not spread as liquids do
            Liquid = 2,  // Random horizontal movement each frame, then fall down. Can float above denser liquids/powders
            Gas = 3      // Repels from other gas particles and reacts to other particles (no gravity movement)
        };

        // Particle types
        enum class ParticleType : uint8_t
        {
            Empty = 0,
            Sand = 1,
            Water = 2,
            Stone = 3,
            Wood = 4,
            Gas = 5,
            // More types can be added later
        };

        // Particle behavior properties (data-driven)
        struct ParticleProperties
        {
            MatterState matterState = MatterState::Solid;  // Base state of matter
            float density = 1.0f;
            Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            float movementChance = 1.0f; // Chance for the particle to attempt movement each frame (0.0 - 1.0)
        };

        // Grid cell data
        struct Cell
        {
            ParticleType type = ParticleType::Empty;
            bool updated = false; // Prevent double-updating in same frame
        };

        // Helper functions
        void createCamera(GraphicsEngine& engine);
        void initializeGrid();
        void clearGrid();
        void updateGrid(float dt);
        void renderParticles(GraphicsEngine& engine, DeviceContext& ctx);
        Vec2 getMouseWorldPosition() const;

        // Grid access
        inline int gridIdx(int x, int y) const { return y * m_gridWidth + x; }
        inline bool isValidGridPos(int x, int y) const 
        { 
            return x >= 0 && x < m_gridWidth && y >= 0 && y < m_gridHeight; 
        }
        inline Cell& getCell(int x, int y) { return m_grid[gridIdx(x, y)]; }
        inline const Cell& getCell(int x, int y) const { return m_grid[gridIdx(x, y)]; }

        // Unified particle update function (uses matter state)
        void updateParticle(int x, int y, float dt);
        
        // Matter state update functions
        void updatePowder(int x, int y, const ParticleProperties& props);
        void updateLiquid(int x, int y, const ParticleProperties& props);
        void updateGas(int x, int y, const ParticleProperties& props);
        
        // Helper for particle movement
        bool tryMove(int x, int y, int newX, int newY);
        bool trySwap(int x, int y, int newX, int newY);
        
        // Particle properties management
        void initializeParticleProperties();
        const ParticleProperties& getParticleProperties(ParticleType type) const;
        
        // Mouse interaction
        void addParticlesAt(const Vec2& worldPos, ParticleType type, float radius);
        Vec2 worldToGrid(const Vec2& worldPos) const;
        Vec2 gridToWorld(int x, int y) const;

        // ECS
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;
        LineRenderer* m_lineRenderer = nullptr;

        // Grid simulation
        std::vector<Cell> m_grid;
        std::vector<Cell> m_gridNext; // Double buffering for parallel updates
        
        int m_gridWidth = 200;
        int m_gridHeight = 150;
        float m_cellSize = 4.0f; // world units per cell
        Vec2 m_gridOrigin = Vec2(-400.0f, -300.0f); // bottom-left of domain

        // Simulation parameters
        bool m_paused = false;
        int m_substeps = 1;
        bool m_alternateUpdate = true; // Alternate grid traversal for better flow

        // Mouse interaction
        ParticleType m_currentTool = ParticleType::Sand;
        float m_brushRadius = 20.0f;
        float m_emitRate = 50.0f; // particles per second
        float m_emitAccumulator = 0.0f;

        // Rendering
        std::shared_ptr<Texture2D> m_nodeTexture;
        bool m_showGrid = false;
        
        // Performance tracking
        float m_smoothDt = 0.016f;
        
        // Particle properties registry
        std::unordered_map<ParticleType, ParticleProperties> m_particleProperties;
    };
}
