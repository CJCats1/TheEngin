#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Components/FirmGuyComponent.h>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace dx3d
{
    class Texture2D;
    // SPH (Smoothed Particle Hydrodynamics) fluid simulation scene
    // Based on LiquidFun's approach - pure particle-based, no grid
    class SPHFluidSimulationScene : public Scene
    {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void fixedUpdate(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;

    private:
        struct SPHParticle
        {
            Vec2 position;
            Vec2 velocity;
            Vec2 acceleration;
            float density;
            float pressure;
            std::string entityName;
        };

        // SPH parameters
        struct SPHParameters
        {
            float rest_density = 1000.0f;      // kg/m³
            float gas_constant = 50000.0f;     // Pressure calculation (much higher for incompressibility)
            float viscosity = 1.0f;            // Viscosity coefficient (increased for stability)
            float smoothing_radius = 25.0f;    // Interaction radius (increased for better density calculation)
            float mass = 1.0f;                // Particle mass
            float gravity = -500.0f;          // Gravity acceleration (reduced for stability)
            float damping = 0.8f;             // Boundary damping (increased for stability)
            float artificial_pressure = 0.05f; // Artificial pressure for incompressibility (increased)
            float artificial_viscosity = 0.2f; // Artificial viscosity for stability (increased)
        };

        // Spatial partitioning for neighbor finding
        struct SpatialGrid
        {
            int grid_width;
            int grid_height;
            float cell_size;
            Vec2 world_min;
            Vec2 world_max;
            
            std::vector<std::vector<int>> cells;
            std::vector<int> temp_neighbors;
            
            void initialize(float world_width, float world_height, float world_min_x, float world_min_y, float smoothing_radius, float cell_scale = 1.0f);
            void clear();
            void addParticle(int particle_id, float x, float y);
            void findNeighbors(int particle_id, float x, float y, float radius, std::vector<int>& neighbors);
        };

        // SPH kernels (smoothing functions)
        float poly6Kernel(float distance, float smoothing_radius);
        Vec2 poly6KernelGradient(const Vec2& r, float smoothing_radius);
        float spikyKernel(float distance, float smoothing_radius);
        Vec2 spikyKernelGradient(const Vec2& r, float smoothing_radius);
        float viscosityKernel(float distance, float smoothing_radius);
        void updateKernelConstants();

        // SPH simulation steps
        void stepSPH(float dt);
        void updateSpatialGrid();
        void buildNeighborLists();
        void calculateDensity();
        void calculatePressure();
        void calculateForces();
        void integrateParticles(float dt);
        void enforceBoundaries();
        void updateParticleSprites();
        
        // Collision detection (LiquidFun style)
        void buildCollisionHash();
        void resolveParticleCollisions();
        long long collisionHashKey(int ix, int iy) const;
        
        // LiquidFun-style optimized methods
        void stepSPHOptimized(float dt);
        void calculateDensityOptimized();
        void calculatePressureOptimized();
        void calculateForcesOptimized();
        void integrateParticlesOptimized(float dt);
        void resolveCollisionsOptimized();
        void updateIslands();
        void buildContactCache();
        
        // LiquidFun-style contact sleeping and position constraints
        void buildContactList();
        void updateContactSleeping();
        void applyPositionConstraints();
        void applyContactSleeping();
        
        // XSPH velocity smoothing
        void applyXSPHSmoothing();
        
        // Low-speed particle stabilization
        void stabilizeLowSpeedParticles();

        // Scene setup
        void createCamera(GraphicsEngine& engine);
        void createBoundaries();
        void createPhysicsBall();
        void spawnParticles();
        void addParticlesAt(const Vec2& worldPos, int count, float jitter = 1.0f);
        void applyForceBrush(const Vec2& worldPos, const Vec2& worldVel);
        Vec2 getMouseWorldPosition() const;
        
        // Ball-particle interaction (matching FLIP scene)
        void createBall();
        void updateBallSpring(float dt, const Vec2& target);
        void enforceBallOnParticles();
        void applyBallBuoyancy();
        float calculateFluidDensityAt(const Vec2& worldPos);
        void resolveBallParticleCollisions();

        // ECS
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;
        LineRenderer* m_lineRenderer = nullptr;
        
        // Physics ball (matching FLIP scene)
        Entity* m_physicsBall = nullptr;
        std::string m_ballEntityName = "SPHBall";
        bool m_ballEnabled = false;
        float m_ballRadius = 18.0f;
        float m_ballMass = 3.0f;
        float m_ballRestitution = 0.35f;
        float m_ballFriction = 0.98f;
        
        // Ball mouse interaction (matching FLIP scene)
        bool m_ballSpringActive = false;
        float m_ballSpringK = 120.0f;      // spring stiffness
        float m_ballSpringDamping = 12.0f; // damping factor
        
        // Buoyancy parameters (matching FLIP scene)
        float m_ballBuoyancyStrength = 10000.0f;  // buoyancy force multiplier
        float m_ballBuoyancyDamping = 0.95f;     // velocity damping in fluid
        bool m_ballBuoyancyEnabled = true;

        // SPH data
        std::vector<SPHParticle> m_particles;
        SPHParameters m_sphParams;
        SpatialGrid m_spatialGrid;
        float m_gridCellScale = 1.0f;
        float m_prevGridCellScale = -1.0f;
        std::vector<std::vector<int>> m_neighbors; // per-frame neighbor cache
        bool m_neighborsValid = false;
        // Precomputed kernel constants
        float m_prevSmoothingRadius = -1.0f;
        float m_h = 0.0f;
        float m_h2 = 0.0f;
        float m_h3 = 0.0f;
        float m_h4 = 0.0f;
        float m_h5 = 0.0f;
        float m_h6 = 0.0f;
        float m_poly6Coeff = 0.0f;            // 315 / (64π h^9)
        float m_spikyGradCoeff = 0.0f;        // 45 / (π h^6)
        float m_viscLaplacianCoeff = 0.0f;    // 45 / (π h^6)
        
        // LiquidFun-style optimized data (SoA layout for better cache efficiency)
        struct OptimizedParticleData
        {
            std::vector<f32> positions_x;
            std::vector<f32> positions_y;
            std::vector<f32> velocities_x;
            std::vector<f32> velocities_y;
            std::vector<f32> accelerations_x;
            std::vector<f32> accelerations_y;
            std::vector<f32> densities;
            std::vector<f32> pressures;
            std::vector<f32> masses;
            std::vector<f32> radii;
            std::vector<uint32_t> colors;
            std::vector<uint16_t> entity_ids; // Maps to m_particles array
            std::vector<bool> is_awake; // Island-based simulation
            
            size_t count = 0;
            size_t capacity = 0;
            
            void resize(size_t new_capacity);
            void addParticle(const SPHParticle& p, uint16_t entity_id);
            void removeParticle(size_t index);
            void syncFromParticles(const std::vector<SPHParticle>& particles);
            void syncToParticles(std::vector<SPHParticle>& particles);
        };
        
        OptimizedParticleData m_optimizedParticles;
        bool m_useOptimizedLayout = false;

        // Simulation parameters
        float m_particleRadius = 4.0f;
        bool m_paused = false;
        bool m_showGridDebug = false;
        float m_smoothDt = 0.016f;

        // Mouse interaction
        enum class MouseTool { Add, Force };
        MouseTool m_mouseTool = MouseTool::Add;
        float m_brushRadius = 30.0f;
        float m_forceStrength = 1500.0f;
        float m_emitRate = 400.0f;
        float m_emitJitter = 3.0f;
        Vec2 m_prevMouseWorld = Vec2(0.0f, 0.0f);
        bool m_prevMouseWorldValid = false;
        float m_emitAccumulator = 0.0f;

        // Boundaries
        float m_domainWidth = 600.0f;
        float m_domainHeight = 400.0f;
        Vec2 m_domainMin = Vec2(-300.0f, -200.0f);
        Vec2 m_domainMax = Vec2(300.0f, 200.0f);
        
        // Boundary visualization offsets
        float m_boundaryLeftOffset = -15.0f;
        float m_boundaryRightOffset = 15.0f;
        float m_boundaryBottomOffset = -15.0f;
        float m_boundaryTopOffset = 15.0f;

        // Performance monitoring
        uint32_t m_neighbor_checks = 0;
        uint32_t m_density_calculations = 0;
        float m_average_neighbors = 0.0f;
        
        // Collision detection (LiquidFun style)
        bool m_enableParticleCollisions = true;
        int m_collisionIterations = 2;
        float m_collisionRestitution = 0.3f;
        float m_collisionFriction = 0.1f;
        std::unordered_map<long long, std::vector<int>> m_collisionHash;
        float m_collisionHashCellSize = 16.0f;
        int m_maxCollisionNeighbors = 16;
        
        // LiquidFun-style contact sleeping and position constraints
        struct ContactInfo
        {
            int particle_a, particle_b;
            Vec2 normal;
            float overlap;
            int sleep_counter;
            bool is_active;
        };
        std::vector<ContactInfo> m_contactList;
        bool m_enableContactSleeping = true;
        int m_contactSleepThreshold = 30;
        float m_contactSleepVelocity = 0.1f;
        
        // Position-based constraints for bottom layer
        bool m_enablePositionConstraints = true;
        float m_positionConstraintStrength = 0.8f;
        float m_positionConstraintDamping = 0.9f;
        
        // XSPH velocity smoothing (LiquidFun style)
        bool m_enableXSPHSmoothing = true;
        float m_xsphSmoothingFactor = 0.05f;
        
        // Low-speed particle stabilization
        bool m_enableLowSpeedStabilization = true;
        float m_lowSpeedThreshold = 50.0f;
        float m_lowSpeedDamping = 0.95f;
        int m_lowSpeedStabilizationIterations = 3;
        
        // LiquidFun-style contact caching
        struct ContactCache
        {
            std::vector<std::pair<int, int>> contacts;
            std::vector<f32> contact_distances;
            std::vector<Vec2> contact_normals;
            int frame_count = 0;
            
            void clear() { contacts.clear(); contact_distances.clear(); contact_normals.clear(); }
            void addContact(int i, int j, f32 dist, const Vec2& normal);
            bool hasContact(int i, int j) const;
        };
        ContactCache m_contactCache;
        
        // Island-based simulation (LiquidFun style)
        struct ParticleIsland
        {
            std::vector<int> particle_indices;
            Vec2 center_of_mass;
            Vec2 linear_velocity;
            f32 angular_velocity;
            f32 moment_of_inertia;
            bool is_awake;
            int sleep_counter;
            
            void update(const OptimizedParticleData& particles);
            bool shouldSleep() const;
        };
        std::vector<ParticleIsland> m_particleIslands;
        bool m_enableIslandSimulation = true;
        f32 m_sleepThreshold = 0.1f;
        int m_sleepCounterThreshold = 60;

        // Rendering
        enum class FluidRenderMode { Sprites, Metaballs };
        FluidRenderMode m_fluidRenderMode = FluidRenderMode::Metaballs;
        bool m_useMetaballRendering = false;
        float m_metaballRadius = 20.0f;
        float m_metaballThreshold = 0.5f;
        float m_metaballSmoothing = 0.1f;
        bool m_colorBySpeed = true;
        float m_speedColorMax = 300.0f;
        // Velocity-based coloring controls (match FLIP scene)
        bool m_debugColor = false; // Blue→Green→Red when true, Blue→White when false
        float m_colorSpeedMin = 0.0f;
        float m_colorSpeedMax = 400.0f;
        
        // Metaball rendering data
        std::vector<Vec2> m_metaballPositions;
        std::vector<Vec4> m_metaballColors;
        std::vector<float> m_metaballRadii;
        std::shared_ptr<Texture2D> m_nodeTexture; // sprite texture for Sprites mode

        // Helper functions
        std::string boundaryName(int i) const;
        float clampf(float v, float a, float b) const { return v < a ? a : (v > b ? b : v); }
        
        // Boundary collision detection
        void resolveParticleBoundaryCollisions();
        
        // Metaball rendering
        void updateMetaballData();
        void renderMetaballs(GraphicsEngine& engine, DeviceContext& ctx);
        void renderMetaballField(GraphicsEngine& engine, DeviceContext& ctx);
        float calculateMetaballField(const Vec2& worldPos);
        Vec4 calculateMetaballColor(const Vec2& worldPos);
    };
}
