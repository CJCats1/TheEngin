#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/Mesh.h>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <algorithm>

namespace dx3d
{
    // Simple 2D FLIP fluid simulation scene
    // Particles are rendered using node.png; boundaries use beam.png
    class FlipFluidSimulationScene : public Scene
    {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void fixedUpdate(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;

    private:
        struct Particle
        {
            Vec2 position;   // world space
            Vec2 velocity;   // world space
            std::string entityName; // for ECS sprite lookup
        };

        // Grid indexing helpers (MAC grid: u on vertical faces, v on horizontal faces)
        inline int idxP(int x, int y) const { return y * m_gridWidth + x; }
        inline int idxU(int x, int y) const { return y * (m_gridWidth + 1) + x; }
        inline int idxV(int x, int y) const { return y * m_gridWidth + x; }

        // Scene setup helpers
        void createCamera(GraphicsEngine& engine);
        void createBoundaries();
        void updateBoundaryPositions();
        void spawnParticles();
        void updateParticleSprites();
        void updateBoundarySprites();
        void addParticlesAt(const Vec2& worldPos, int count, float jitter = 1.0f);
        void applyForceBrush(const Vec2& worldPos, const Vec2& worldVel);
        void pickupParticles(const Vec2& worldPos);
        void movePickedParticles(const Vec2& worldPos);
        void releasePickedParticles();
        void ensureWorldAnchor();
        // Rotated box helpers
        Vec2 worldToBoxLocal(const Vec2& p) const;
        Vec2 boxLocalToWorld(const Vec2& p) const;

        // FLIP pipeline
        void stepFLIP(float dt);
        void clearGrid();
        void particlesToGrid(float dt);
        void buildPressureSystem(float dt);
        void solvePressure();
        void applyPressureGradient(float dt);
        void gridToParticles(float dt);
        void advectParticles(float dt);
        void enforceBoundaryOnParticles();
        void resolveParticleCollisions();
        void updateParticleColors();
        void resolveParticleCollisionsHashed();
        void buildSpatialHash();
        void applyViscosity(float dt);
        void renderMetaballs(GraphicsEngine& engine, DeviceContext& ctx);
        void updateMetaballData();
        void createMetaballQuad();
        void renderFullscreenMetaball(GraphicsEngine& engine, DeviceContext& ctx);
        void generateMetaballMesh();
        float calculateMetaballField(const Vec2& worldPos);
        Vec4 calculateMetaballColor(const Vec2& worldPos);
        void renderMetaballMesh(GraphicsEngine& engine, DeviceContext& ctx);
        
        // Metaball rendering functions
        void renderMetaballsAsSprites(DeviceContext& ctx);
        
        // Texture-based metaball rendering (john-wigg.dev approach)
        void initializeMetaballTextures(GraphicsEngine& engine);
        void createMetaballFalloffTexture();
        void createMetaballGradientTexture();
        void renderMetaballField(GraphicsEngine& engine, DeviceContext& ctx);
        void renderMetaballSurface(GraphicsEngine& engine, DeviceContext& ctx);
        
        // Marching squares for fluid surface generation
        void generateFluidSurface();
        void renderFluidSurface(GraphicsEngine& engine, DeviceContext& ctx);
        float calculateDensityAt(const Vec2& worldPos);
        Vec2 interpolateEdge(const Vec2& p1, const Vec2& p2, float val1, float val2, float threshold);
        
        // Convex Hull for Fluid Surface
        std::vector<std::vector<Vec2>> simpleClusterPoints(const std::vector<Vec2>& points);
        std::vector<Vec2> getConvexHull(const std::vector<Vec2>& points);
        float crossProduct(const Vec2& O, const Vec2& A, const Vec2& B);
        
        // Clustered mesh rendering removed - not worth keeping

        // Utility
        Vec2 worldToGrid(const Vec2& p) const;
        Vec2 gridToWorld(const Vec2& ij) const;
        float sampleU(float x, float y) const;
        float sampleV(float x, float y) const;
        float clampf(float v, float a, float b) const { return v < a ? a : (v > b ? b : v); }

        // ECS
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;
        LineRenderer* m_lineRenderer = nullptr;

        // Particles
        std::vector<Particle> m_particles;

        // Grid (cell-centered pressure; face-centered velocities)
        int m_gridWidth = 60;
        int m_gridHeight = 40;
        float m_cellSize = 10.0f; // world units per cell
        Vec2 m_gridOrigin = Vec2(-300.0f, -200.0f); // bottom-left of domain in world space

        std::vector<float> m_u;        // size: (W+1)*H
        std::vector<float> m_v;        // size: W*(H+1)
        std::vector<float> m_uWeight;  // same as m_u
        std::vector<float> m_vWeight;  // same as m_v
        std::vector<float> m_pressure; // size: W*H
        std::vector<float> m_divergence;// size: W*H
        std::vector<uint8_t> m_solid;  // size: W*H (0 fluid/air, 1 solid)

        // Simulation parameters (editable via ImGui)
        float m_gravity = -980.0f;          // px/s^2 downward in world Y
        float m_flipBlending = 0.8f;        // 1.0 = pure FLIP, 0.0 = pure PIC
        int   m_jacobiIterations = 5;       // pressure solve
        int   m_substeps = 1;               // simulation substeps per fixedUpdate
        float m_particleRadius = 4.0f;      // for rendering and boundary padding
        bool  m_paused = false;
        bool  m_showGridDebug = false;
        float m_smoothDt = 0.016f; // smoothed frame time for FPS
        
        // Viscosity parameters
        float m_viscosity = 0.0f;           // viscosity coefficient (0 = inviscid, higher = more viscous)
        float m_velocityDamping = 1.0f;     // global velocity damping per frame (1.0 = no damping)
        
        // Fluid rendering modes
        enum class FluidRenderMode { Sprites, Metaballs };
        FluidRenderMode m_fluidRenderMode = FluidRenderMode::Metaballs;
        
        // Metaball rendering
        bool m_useMetaballRendering = true;
        float m_metaballThreshold = 0.5f;
        float m_metaballSmoothing = 0.1f;
        float m_metaballRadius = 20.0f;
        
        // Metaball rendering resources
        std::vector<Vec2> m_metaballPositions;
        std::vector<Vec4> m_metaballColors;
        std::vector<float> m_metaballRadii;
        
        // Texture-based metaball rendering (like john-wigg.dev approach)
        std::shared_ptr<Texture2D> m_metaballFalloffTexture; // Radial gradient texture
        std::shared_ptr<Texture2D> m_metaballFieldTexture;  // Accumulated field texture
        std::shared_ptr<Texture2D> m_metaballGradientTexture; // Color gradient for final rendering
        
        // Metaball mesh generation (CPU-based for now)
        std::vector<Vec2> m_metaballVertices;
        std::vector<Vec4> m_metaballVertexColors;
        std::vector<uint32_t> m_metaballIndices;
        
        // Metaball rendering parameters
        int m_metaballTextureSize = 256;       // Size of the field texture
        
        // Metaball rendering entities
        std::string m_metaballQuadEntity = "MetaballQuad";
        bool m_metaballQuadCreated = false;

        // Mouse interaction
        enum class MouseTool { Add, Force, Pickup };
        MouseTool m_mouseTool = MouseTool::Add;
        float m_brushRadius = 30.0f;
        float m_forceStrength = 1500.0f; // impulse per second
        float m_emitRate = 400.0f;        // particles per second when adding
        float m_emitJitter = 3.0f;
        Vec2  m_prevMouseWorld = Vec2(0.0f, 0.0f);
        bool  m_prevMouseWorldValid = false;
        float m_emitAccumulator = 0.0f;
        
        // Particle pickup
        std::vector<int> m_pickedParticles; // indices of particles being held
        bool m_isPickingUp = false;
        Vec2 m_pickupOffset = Vec2(0.0f, 0.0f); // offset from mouse to particle center

        // Particle-particle collisions
        bool  m_enableParticleCollisions = true;
        int   m_collisionIterations = 1;
        float m_collisionRestitution = 0.1f; // 0..1

        // Coloring
        std::vector<int> m_cellParticleCount; // size W*H (not needed for velocity coloring, kept for future)
        int   m_colorFoamThreshold = 2;   // legacy
        float m_colorSpeedThreshold = 200.0f; // legacy
        float m_colorSpeedMin = 0.0f;     // speed for darkest blue
        float m_colorSpeedMax = 400.0f;   // speed for white spray

        // Multithreading
        int   m_threadCount = 1; // default single-thread; enable in UI if beneficial
        template<typename F>
        void parallelFor(int start, int end, int grain, F&& fn)
        {
            int n = std::max(1, m_threadCount);
            if (n <= 1)
            {
                fn(start, end);
                return;
            }
            grain = std::max(1, grain);
            int total = end - start;
            int chunk = std::max(grain, (total + n - 1) / n);
            std::vector<std::thread> threads;
            threads.reserve(n);
            for (int t = 0; t < n; ++t)
            {
                int s = start + t * chunk;
                int e = std::min(end, s + chunk);
                if (s >= e) break;
                threads.emplace_back([=]() { fn(s, e); });
            }
            for (auto& th : threads) th.join();
        }

        // Spatial hashing for particle neighbors
        bool  m_useSpatialHash = true;
        float m_hashCellSize = 16.0f; // default ~ 4x radius for better performance
        std::unordered_map<long long, std::vector<int>> m_hash; // key = (ix<<32)|iy
        inline long long hashKey(int ix, int iy) const { return (static_cast<long long>(ix) << 32) ^ static_cast<unsigned long long>(iy); }

        // Boundaries in world space (axis-aligned box)
        float m_domainWidth = 600.0f;
        float m_domainHeight = 400.0f;
        // Rotated collision box (visual boundaries)
        Vec2  m_boxCenter = Vec2(0.0f, 0.0f);
        Vec2  m_boxHalf   = Vec2(m_domainWidth * 0.5f, m_domainHeight * 0.5f);
        float m_boxAngle  = 0.0f; // radians
        
        // Boundary visualization offsets
        float m_boundaryLeftOffset = -10.0f;
        float m_boundaryRightOffset = 10.0f;
        float m_boundaryBottomOffset = -10.0f;
        float m_boundaryTopOffset = 10.0f;
        
        // Marching squares fluid surface - DISABLED
        bool m_showFluidSurface = false; // Disabled to prevent freezing
        float m_fluidSurfaceThreshold = 0.5f;
        float m_fluidSurfaceResolution = 12.0f;
        Vec4 m_fluidSurfaceColor = Vec4(0.2f, 0.6f, 1.0f, 0.8f);
        std::vector<Vec2> m_fluidSurfaceLines;
        int m_fluidBodyCount = 0;
        
        // Performance optimization
        int m_fluidSurfaceUpdateRate = 0; // Always update every frame for highest quality
        int m_fluidSurfaceFrameCounter = 0;
        bool m_fluidSurfaceDirty = true;
        
        // Clustered mesh rendering removed - not worth keeping

        // Helpers to name entities
        std::string boundaryName(int i) const;
        Vec2 getMouseWorldPosition() const;
    };
}


