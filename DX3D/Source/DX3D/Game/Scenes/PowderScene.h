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
            Acid = 6,
            Fire = 7,
            Smoke = 8,
            Steam = 9,
            Metal = 10,
            Lava = 11,
            Mud = 12,
            Oil = 13,
            // More types can be added later
        };

        // Particle behavior properties (data-driven)
        struct ParticleProperties
        {
            MatterState matterState = MatterState::Solid;  // Base state of matter
            float density = 1.0f;
            Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            float movementChance = 1.0f; // Chance for the particle to attempt movement each frame (0.0 - 1.0)
            bool flammable = false; // Can this particle be ignited by fire?
            float ignitionTemp = 500.0f; // Temperature (Kelvin) at which this particle ignites
            float burnTemp = 1000.0f; // Temperature (Kelvin) this particle produces when burning
        };

        // Grid cell data
        struct Cell
        {
            ParticleType type = ParticleType::Empty;
            bool updated = false; // Prevent double-updating in same frame
            int life = 0; // Life value for particles that need it (e.g., acid, fire lifetime, smoke lifetime)
            float temperature = 273.15f + 22.0f; // Temperature in Kelvin (default ~22°C)
        };

        // Helper functions
        void createCamera(GraphicsEngine& engine);
        void initializeGrid();
        void clearGrid();
        void updateGrid(float dt);
        void renderParticles(GraphicsEngine& engine, DeviceContext& ctx);
        void renderAirVelocity(GraphicsEngine& engine, DeviceContext& ctx);
        void renderAirPressure(GraphicsEngine& engine, DeviceContext& ctx);
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
        void updatePowder(int x, int y, const ParticleProperties& props, float dt, int preferredDirX = 0, int preferredDirY = 0);
        void updateLiquid(int x, int y, const ParticleProperties& props, float dt, int preferredDirX = 0, int preferredDirY = 0);
        void updateGas(int x, int y, const ParticleProperties& props, int preferredDirX = 0, int preferredDirY = 0);
        
        // Helper for particle movement
        bool tryMove(int x, int y, int newX, int newY);
        bool trySwap(int x, int y, int newX, int newY);
        
        // Particle-air interaction
        void applyAirForcesToParticle(int x, int y, const ParticleProperties& props, int& preferredDirX, int& preferredDirY);
        void addParticleMovementToAir(int x, int y, int newX, int newY, const ParticleProperties& props);
        
        // Air test functions
        void createAirImpulse(const Vec2& worldPos, float strength, float radius);
        
        // Particle properties management
        void initializeParticleProperties();
        const ParticleProperties& getParticleProperties(ParticleType type) const;
        
        // Corrosion helper
        bool canCorrode(ParticleType type) const;
        
        // Fire system helpers
        void updateFireAndIgnition(int x, int y, float dt);
        void tryIgniteNeighbor(int x, int y, int dx, int dy);
        void createFireParticle(int x, int y);
        void createSmokeParticle(int x, int y);
        
        // Mouse interaction
        void addParticlesAt(const Vec2& worldPos, ParticleType type, float radius);
        Vec2 worldToGrid(const Vec2& worldPos) const;
        Vec2 gridToWorld(int x, int y) const;

        // Air system functions
        void initializeAirSystem();
        void clearAirSystem();
        void updateAirSystem(float dt);
        void updateAirPressure(float dt);
        void updateAirVelocity(float dt);
        void updateAirHeat(float dt);
        void updateBlockAirMaps();
        void makeKernel(); // Gaussian kernel for air smoothing
        float vorticity(int x, int y) const; // Calculate vorticity for velocity field
        
        // Air system access
        inline float& getAirPressure(int x, int y) { return m_airPressure[gridIdx(x, y)]; }
        inline float& getAirVelocityX(int x, int y) { return m_airVelocityX[gridIdx(x, y)]; }
        inline float& getAirVelocityY(int x, int y) { return m_airVelocityY[gridIdx(x, y)]; }
        inline float& getAirHeat(int x, int y) { return m_airHeat[gridIdx(x, y)]; }

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
        enum class ToolType
        {
            DropParticles = 0,
            AddImpulse = 1,
            Clear = 2
        };
        
        ToolType m_currentTool = ToolType::DropParticles;
        ParticleType m_currentParticleType = ParticleType::Sand;
        float m_brushRadius = 20.0f;
        float m_emitRate = 50.0f; // particles per second
        float m_emitAccumulator = 0.0f;
        float m_impulseStrength = 50.0f; // Air impulse strength

        // Rendering
        std::shared_ptr<Texture2D> m_nodeTexture;
        bool m_showGrid = false;
        bool m_showAirVelocity = false; // Show air velocity as color overlay
        bool m_showAirPressure = false; // Show air pressure as color overlay
        
        // Performance tracking
        float m_smoothDt = 0.016f;
        
        // Particle properties registry
        std::unordered_map<ParticleType, ParticleProperties> m_particleProperties;

        // Air system
        std::vector<float> m_airPressure;      // Air pressure grid
        std::vector<float> m_airVelocityX;    // Air velocity X component
        std::vector<float> m_airVelocityY;    // Air velocity Y component
        std::vector<float> m_airHeat;          // Ambient air temperature
        std::vector<float> m_airPressureNext;  // Double buffering for pressure
        std::vector<float> m_airVelocityXNext; // Double buffering for velocity X
        std::vector<float> m_airVelocityYNext; // Double buffering for velocity Y
        std::vector<float> m_airHeatNext;      // Double buffering for heat
        std::vector<uint8_t> m_blockAir;       // Block air flow map
        std::vector<uint8_t> m_blockAirHeat;   // Block air heat map
        float m_airKernel[9];                  // Gaussian kernel for smoothing (3x3)
        
        // Air system parameters
        float m_ambientAirTemp = 273.15f + 22.0f; // ~22°C in Kelvin
        float m_airPressureLoss = 0.6f;         // Pressure loss per frame (0.6 = 40% loss, keeping 60% per frame)
        float m_airVelocityLoss = 0.6f;         // Velocity loss per frame (0.6 = 40% loss, keeping 60% per frame)
        float m_airAdvectionMult = 0.7f;         // Advection multiplier (taking values from far away)
        float m_airVorticityCoeff = 0.0f;        // Vorticity confinement coefficient
        float m_airHeatConvection = 0.0001f;      // Heat convection strength
        bool m_airEnabled = true;                 // Enable/disable air system
    };
}
