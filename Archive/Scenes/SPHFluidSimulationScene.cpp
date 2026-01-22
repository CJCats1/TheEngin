#include <thread>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Game/Scenes/SPHFluidSimulationScene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Components/FirmGuyComponent.h>
#include <DX3D/Components/FirmGuySystem.h>
#include <imgui.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstdio>

using namespace dx3d;

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Precompute kernel constants if smoothing radius changed
void SPHFluidSimulationScene::updateKernelConstants()
{
    if (m_prevSmoothingRadius == m_sphParams.smoothing_radius) return;
    m_prevSmoothingRadius = m_sphParams.smoothing_radius;
    // values cached for early r^2 checks and kernel constants
    m_h = m_prevSmoothingRadius;
    m_h2 = m_h * m_h;
    m_h3 = m_h2 * m_h;
    m_h4 = m_h2 * m_h2;
    m_h5 = m_h4 * m_h;
    m_h6 = m_h3 * m_h3;
}

void SPHFluidSimulationScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();

    // Preload node texture for Sprites mode
    m_nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/node.png");

    // Camera
    createCamera(engine);

    // Line renderer for debug visualization
    auto& lineEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false);
    
    if (auto* linePipeline = engine.getLinePipeline())
    {
        m_lineRenderer->setLinePipeline(linePipeline);
        printf("SPH Line renderer created with dedicated line pipeline\n");
    }

    // Initialize spatial grid
    m_spatialGrid.initialize(m_domainWidth, m_domainHeight, m_domainMin.x, m_domainMin.y, m_sphParams.smoothing_radius, m_gridCellScale);
    updateKernelConstants();

    // Initialize optimized particle data
    m_optimizedParticles.resize(1000); // Start with capacity for 1000 particles
    m_optimizedParticles.count = 0;

    // Initialize domain parameters

    // Create boundaries, ball, and particles
    createBoundaries();
    createBall();
    spawnParticles();
    m_neighbors.resize(m_particles.size());
    m_neighborsValid = false;
    
    printf("SPH Fluid Simulation Scene loaded with LiquidFun optimizations\n");
}

void SPHFluidSimulationScene::createCamera(GraphicsEngine& engine)
{
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(0.8f);
}

std::string SPHFluidSimulationScene::boundaryName(int i) const
{
    switch (i)
    {
    case 0: return "BoundaryLeft";
    case 1: return "BoundaryRight";
    case 2: return "BoundaryBottom";
    default: return "BoundaryTop";
    }
}

void SPHFluidSimulationScene::createBoundaries()
{
    // Physics boundaries using FirmGuy static bodies
    auto addBoundary = [&](const std::string& name, const Vec2& pos, float w, float h)
    {
        auto& e = m_entityManager->createEntity(name);
        auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/beam.png", w, h);
        s.setPosition(pos.x, pos.y, 0.0f);
        s.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));
        
        // Add physics body
        auto& rb = e.addComponent<FirmGuyComponent>();
        rb.setRectangle(Vec2(w * 0.5f, h * 0.5f));
        rb.setPosition(pos);
        rb.setStatic(true);
        rb.setRestitution(0.1f); // Low bounce for fluid boundaries
        rb.setFriction(0.9f);
    };

    float left = m_domainMin.x;
    float bottom = m_domainMin.y;
    float right = m_domainMax.x;
    float top = m_domainMax.y;
    float thickness = 20.0f;

    // Create boundaries at the exact domain edges for consistent collision detection
    addBoundary(boundaryName(0), Vec2(left - thickness * 0.5f, (bottom + top) * 0.5f), thickness, m_domainHeight + thickness * 2.0f);
    addBoundary(boundaryName(1), Vec2(right + thickness * 0.5f, (bottom + top) * 0.5f), thickness, m_domainHeight + thickness * 2.0f);
    addBoundary(boundaryName(2), Vec2((left + right) * 0.5f, bottom - thickness * 0.5f), m_domainWidth + thickness * 2.0f, thickness);
    addBoundary(boundaryName(3), Vec2((left + right) * 0.5f, top + thickness * 0.5f), m_domainWidth + thickness * 2.0f, thickness);
}

void SPHFluidSimulationScene::createBall()
{
    // Always create the ball entity, but set visibility based on m_ballEnabled
    if (m_entityManager->findEntity(m_ballEntityName)) return;

    Vec2 startPos = Vec2(-m_domainWidth * 0.3f, m_domainHeight * 0.2f);
    auto& e = m_entityManager->createEntity(m_ballEntityName);
    auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/node.png", m_ballRadius*2.0f, m_ballRadius*2.0f);
    s.setPosition(startPos.x, startPos.y, 0.0f);
    s.setTint(Vec4(0.95f, 0.95f, 0.95f, 1.0f));
    s.setVisible(m_ballEnabled); // Set initial visibility

    auto& rb = e.addComponent<FirmGuyComponent>();
    rb.setCircle(m_ballRadius);
    rb.setPosition(startPos);
    rb.setVelocity(Vec2(0.0f, 0.0f));
    rb.setMass(m_ballMass);
    rb.setRestitution(m_ballRestitution);
    rb.setFriction(m_ballFriction);
    
    m_physicsBall = &e;
    printf("SPH Ball created with radius %.1f, mass %.1f\n", m_ballRadius, m_ballMass);
}

void SPHFluidSimulationScene::spawnParticles()
{
    // Spawn a blob of fluid particles
    m_particles.clear();
    int particlesX = 20;
    int particlesY = 15;

    Vec2 start = m_domainMin + Vec2(m_domainWidth * 0.2f, m_domainHeight * 0.6f);
    float spacing = m_particleRadius * 2.0f * 0.9f;

    int id = 0;
    for (int j = 0; j < particlesY; ++j)
    {
        for (int i = 0; i < particlesX; ++i)
        {
            SPHParticle p;
            p.position = start + Vec2(i * spacing, j * spacing);
            p.velocity = Vec2(0.0f, 0.0f);
            p.acceleration = Vec2(0.0f, 0.0f);
            p.density = m_sphParams.rest_density;
            p.pressure = 0.0f;
            p.entityName = "SPHParticle_" + std::to_string(id++);

            auto& e = m_entityManager->createEntity(p.entityName);
            auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/MetaballFalloff.png", m_particleRadius * 2.0f, m_particleRadius * 2.0f);
            s.setPosition(p.position.x, p.position.y, 0.0f);
            s.setTint(Vec4(0.2f, 0.6f, 1.0f, 1.0f));
            m_particles.push_back(p);
        }
    }
    
    printf("Spawned %d SPH particles\n", (int)m_particles.size());
}

void SPHFluidSimulationScene::update(float dt)
{
    // Handle pause toggle
    auto& input = Input::getInstance();
    if (input.wasKeyJustPressed(Key::P)) m_paused = !m_paused;

    // Smooth dt for FPS display
    const float alpha = 0.1f;
    m_smoothDt = (1.0f - alpha) * m_smoothDt + alpha * std::max(1e-6f, dt);

    // Track mouse world for interaction
    Vec2 mouseWorld = getMouseWorldPosition();
    if (!m_prevMouseWorldValid) { m_prevMouseWorld = mouseWorld; m_prevMouseWorldValid = true; }

    // Keyboard controls removed (no rotation)

    // Mouse interaction
    bool lmb = input.isMouseDown(MouseClick::LeftMouse);
    bool rmb = input.isMouseDown(MouseClick::RightMouse);
    bool rmbJustPressed = input.wasMouseJustPressed(MouseClick::RightMouse);
    bool rmbJustReleased = input.wasMouseJustReleased(MouseClick::RightMouse);
    Vec2 mouseVel = (mouseWorld - m_prevMouseWorld) / std::max(1e-6f, dt);

    // RMB spring control for ball (matching FLIP scene)
    if (rmbJustPressed) m_ballSpringActive = true;
    if (rmbJustReleased) m_ballSpringActive = false;
    if (m_ballSpringActive)
    {
        updateBallSpring(dt, mouseWorld);
    }

    if (lmb)
    {
        if (m_mouseTool == MouseTool::Add)
        {
            m_emitAccumulator += m_emitRate * dt;
            int toEmit = (int)m_emitAccumulator;
            if (toEmit > 0)
            {
                addParticlesAt(mouseWorld, toEmit, m_emitJitter);
                m_emitAccumulator -= toEmit;
            }
        }
        else if (m_mouseTool == MouseTool::Force)
        {
            applyForceBrush(mouseWorld, mouseVel);
        }
    }
    
    if (rmb && !m_ballSpringActive)
    {
        // Right button acts as a suction brush when not controlling ball
        applyForceBrush(mouseWorld, mouseVel * -1.0f);
    }

    m_prevMouseWorld = mouseWorld;
}

void SPHFluidSimulationScene::fixedUpdate(float dt)
{
    if (m_paused) return;

    // Rebuild grid if parameters changed
    if (m_prevSmoothingRadius != m_sphParams.smoothing_radius || m_prevGridCellScale != m_gridCellScale)
    {
        m_spatialGrid.initialize(m_domainWidth, m_domainHeight, m_domainMin.x, m_domainMin.y, m_sphParams.smoothing_radius, m_gridCellScale);
        m_prevGridCellScale = m_gridCellScale;
        updateKernelConstants();
    }

    // Apply ball constraint to particles after SPH step
    enforceBallOnParticles();
    
    // Apply buoyancy forces to ball
    applyBallBuoyancy();

    // No rotation - boundaries stay fixed

    // Update physics boundaries and ball
    if (m_entityManager)
    {
        FirmGuySystem::update(*m_entityManager, dt);
    }

    const int steps = 1; // SPH is more stable than FLIP, fewer substeps needed
    const float h = dt / static_cast<float>(steps);
    for (int s = 0; s < steps; ++s)
    {
        stepSPH(h);
        // Resolve ball-particle collisions after each SPH step
        resolveBallParticleCollisions();
    }

    updateParticleSprites();
}

void SPHFluidSimulationScene::render(GraphicsEngine& engine, SwapChain& swapChain)
{
    auto& ctx = engine.getContext();

    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera"))
    {
        if (auto* camera = cameraEntity->getComponent<Camera2D>())
        {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.enableAlphaBlending();

    // Draw particles
    if (m_fluidRenderMode == FluidRenderMode::Metaballs)
    {
        // Use metaball rendering for smooth fluid surfaces
        renderMetaballs(engine, ctx);
    }
    else // Sprites mode
    {
        // Ensure particle sprites use node.png in Sprites mode
        for (auto* entity : m_entityManager->getEntitiesWithComponent<SpriteComponent>())
        {
            if (auto* sprite = entity->getComponent<SpriteComponent>())
            {
                // Only retarget textures for particle entities to avoid affecting boundaries/ball
                const std::string& n = entity->getName();
                if (n.rfind("SPHParticle_", 0) == 0 && m_nodeTexture)
                {
                    sprite->setTexture(m_nodeTexture);
                }
                if (sprite->isVisible() && sprite->isValid())
                    sprite->draw(ctx);
            }
        }
    }
    
    // Debug grid visualization
    if (m_lineRenderer)
    {
        m_lineRenderer->clear();
    }
    
    if (m_showGridDebug && m_lineRenderer)
    {
        // Draw spatial grid
        Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 0.08f);
        for (int i = 0; i <= m_spatialGrid.grid_width; ++i)
        {
            float x = m_spatialGrid.world_min.x + i * m_spatialGrid.cell_size;
            m_lineRenderer->addLine(Vec2(x, m_spatialGrid.world_min.y), Vec2(x, m_spatialGrid.world_max.y), color, 1.0f);
        }
        for (int j = 0; j <= m_spatialGrid.grid_height; ++j)
        {
            float y = m_spatialGrid.world_min.y + j * m_spatialGrid.cell_size;
            m_lineRenderer->addLine(Vec2(m_spatialGrid.world_min.x, y), Vec2(m_spatialGrid.world_max.x, y), color, 1.0f);
        }
        
        m_lineRenderer->updateBuffer();
        m_lineRenderer->draw(ctx);
    }
    
    // Boundaries are static - no rotation
}

void SPHFluidSimulationScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::SetNextWindowSize(ImVec2(420, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("SPH Fluid Simulation"))
    {
        float fps = (m_smoothDt > 0.0f) ? (1.0f / m_smoothDt) : 0.0f;
        ImGui::Text("FPS: %.1f (dt=%.3f ms)", fps, m_smoothDt * 1000.0f);
        ImGui::Checkbox("Paused (P)", &m_paused);
        ImGui::Text("Particles: %d", (int)m_particles.size());
        ImGui::Checkbox("Show Grid", &m_showGridDebug);
        
        ImGui::Separator();
        ImGui::Text("LiquidFun Optimizations");
        ImGui::Checkbox("Use Optimized Layout (SoA)", &m_useOptimizedLayout);
        ImGui::Checkbox("Enable Island Simulation", &m_enableIslandSimulation);
        ImGui::SliderFloat("Sleep Threshold", &m_sleepThreshold, 0.01f, 1.0f, "%.3f");
        
        ImGui::Separator();
        ImGui::Text("SPH Parameters");
        ImGui::SliderFloat("Rest Density", &m_sphParams.rest_density, 500.0f, 2000.0f, "%.0f");
        ImGui::SliderFloat("Gas Constant", &m_sphParams.gas_constant, 5000.0f, 50000.0f, "%.0f");
        ImGui::SliderFloat("Viscosity", &m_sphParams.viscosity, 0.01f, 2.0f, "%.3f");
        ImGui::SliderFloat("Smoothing Radius", &m_sphParams.smoothing_radius, 10.0f, 40.0f, "%.1f");
        ImGui::SliderFloat("Grid Cell Scale", &m_gridCellScale, 0.5f, 2.0f, "%.2f");
        ImGui::SliderFloat("Gravity", &m_sphParams.gravity, -2000.0f, 0.0f, "%.0f");
        ImGui::SliderFloat("Damping", &m_sphParams.damping, 0.5f, 1.0f, "%.2f");
        
        ImGui::Separator();
        ImGui::Text("Artificial Forces (for incompressibility)");
        ImGui::SliderFloat("Artificial Pressure", &m_sphParams.artificial_pressure, 0.0f, 0.1f, "%.4f");
        ImGui::SliderFloat("Artificial Viscosity", &m_sphParams.artificial_viscosity, 0.0f, 0.5f, "%.4f");
        
        ImGui::Separator();
        ImGui::Text("Visualization");
        ImGui::Checkbox("Color By Speed", &m_colorBySpeed);
        ImGui::Checkbox("Debug Color (Blue\xE2\x86\x92Green\xE2\x86\x92Red)", &m_debugColor);
        ImGui::SliderFloat("Speed Min", &m_colorSpeedMin, 0.0f, 400.0f, "%.0f");
        ImGui::SliderFloat("Speed Max", &m_colorSpeedMax, 50.0f, 1200.0f, "%.0f");
        
        ImGui::Separator();
        ImGui::Text("Rendering Mode");
        int renderMode = (m_fluidRenderMode == FluidRenderMode::Sprites) ? 0 : 1;
        if (ImGui::RadioButton("Sprites", renderMode == 0)) m_fluidRenderMode = FluidRenderMode::Sprites;
        ImGui::SameLine();
        if (ImGui::RadioButton("Metaballs", renderMode == 1)) m_fluidRenderMode = FluidRenderMode::Metaballs;
        
        if (m_fluidRenderMode == FluidRenderMode::Metaballs)
        {
            ImGui::SliderFloat("Metaball Radius", &m_metaballRadius, 10.0f, 50.0f, "%.1f");
            ImGui::SliderFloat("Metaball Threshold", &m_metaballThreshold, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Metaball Smoothing", &m_metaballSmoothing, 0.01f, 0.5f, "%.3f");
        }
        
        ImGui::Separator();
        ImGui::Text("Interactive Ball");
        ImGui::Checkbox("Ball Enabled", &m_ballEnabled);
        
        // Store previous values to detect changes
        static float prevBallRadius = m_ballRadius;
        static float prevBallMass = m_ballMass;
        static float prevBallRestitution = m_ballRestitution;
        static float prevBallFriction = m_ballFriction;
        
        ImGui::SliderFloat("Ball Radius", &m_ballRadius, 5.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Ball Mass", &m_ballMass, 0.5f, 10.0f, "%.1f");
        ImGui::SliderFloat("Ball Restitution", &m_ballRestitution, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Ball Friction", &m_ballFriction, 0.9f, 1.0f, "%.3f");
        
        // Handle ball enabled/disabled
        if (auto* ball = m_entityManager->findEntity(m_ballEntityName))
        {
            if (auto* sprite = ball->getComponent<SpriteComponent>())
            {
                sprite->setVisible(m_ballEnabled);
            }
            if (auto* physics = ball->getComponent<FirmGuyComponent>())
            {
                // When disabled, stop the ball
                if (!m_ballEnabled)
                {
                    physics->setVelocity(Vec2(0.0f, 0.0f));
                }
            }
        }
        else
        {
            // Ball entity doesn't exist - create it
            createBall();
        }
        
        // Update ball physics properties if they changed
        if (m_ballRadius != prevBallRadius || m_ballMass != prevBallMass || 
            m_ballRestitution != prevBallRestitution || m_ballFriction != prevBallFriction)
        {
            if (auto* ball = m_entityManager->findEntity(m_ballEntityName))
            {
                if (auto* physics = ball->getComponent<FirmGuyComponent>())
                {
                    physics->setCircle(m_ballRadius);
                    physics->setMass(m_ballMass);
                    physics->setRestitution(m_ballRestitution);
                    physics->setFriction(m_ballFriction);
                }
                
                // Update sprite size to match new radius
                if (auto* sprite = ball->getComponent<SpriteComponent>())
                {
                    float scale = m_ballRadius / 18.0f; // 18.0f was the original radius
                    sprite->setScale(scale, scale, 1.0f);
                }
            }
            
            prevBallRadius = m_ballRadius;
            prevBallMass = m_ballMass;
            prevBallRestitution = m_ballRestitution;
            prevBallFriction = m_ballFriction;
        }
        
        ImGui::Separator();
        ImGui::Text("Ball Spring (RMB)");
        ImGui::SliderFloat("Spring Stiffness", &m_ballSpringK, 50.0f, 500.0f, "%.0f");
        ImGui::SliderFloat("Spring Damping", &m_ballSpringDamping, 5.0f, 50.0f, "%.1f");
        
        ImGui::Separator();
        ImGui::Text("Ball Buoyancy");
        ImGui::Checkbox("Buoyancy Enabled", &m_ballBuoyancyEnabled);
        ImGui::SliderFloat("Buoyancy Strength", &m_ballBuoyancyStrength, 500.0f, 5000.0f, "%.0f");
        ImGui::SliderFloat("Fluid Damping", &m_ballBuoyancyDamping, 0.8f, 1.0f, "%.3f");
        
        ImGui::Separator();
        ImGui::Text("Performance");
        ImGui::Text("Particles: %d", (int)m_particles.size());
        ImGui::Text("Neighbor Checks: %u", m_neighbor_checks);
        ImGui::Text("Density Calculations: %u", m_density_calculations);
        ImGui::Text("Avg Neighbors: %.1f", m_average_neighbors);
        ImGui::Text("Grid: %dx%d (cell=%.1f)", m_spatialGrid.grid_width, m_spatialGrid.grid_height, m_spatialGrid.cell_size);
        
        ImGui::Separator();
        ImGui::Text("Collision Detection (LiquidFun Style)");
        ImGui::Checkbox("Enable Particle Collisions", &m_enableParticleCollisions);
        ImGui::SliderInt("Collision Iterations", &m_collisionIterations, 1, 5);
        ImGui::SliderFloat("Restitution", &m_collisionRestitution, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Friction", &m_collisionFriction, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Hash Cell Size", &m_collisionHashCellSize, 8.0f, 32.0f, "%.1f");
        ImGui::SliderInt("Max Neighbors (per particle)", &m_maxCollisionNeighbors, 4, 64);
        
        ImGui::Separator();
        ImGui::Text("Contact Sleeping (LiquidFun Style)");
        ImGui::Checkbox("Enable Contact Sleeping", &m_enableContactSleeping);
        ImGui::SliderInt("Sleep Threshold (frames)", &m_contactSleepThreshold, 10, 120);
        ImGui::SliderFloat("Sleep Velocity", &m_contactSleepVelocity, 0.01f, 1.0f, "%.3f");
        
        ImGui::Separator();
        ImGui::Text("Position Constraints (LiquidFun Style)");
        ImGui::Checkbox("Enable Position Constraints", &m_enablePositionConstraints);
        ImGui::SliderFloat("Constraint Strength", &m_positionConstraintStrength, 0.1f, 1.0f, "%.2f");
        ImGui::SliderFloat("Constraint Damping", &m_positionConstraintDamping, 0.5f, 1.0f, "%.2f");
        
        ImGui::Separator();
        ImGui::Text("XSPH Velocity Smoothing (LiquidFun Style)");
        ImGui::Checkbox("Enable XSPH Smoothing", &m_enableXSPHSmoothing);
        ImGui::SliderFloat("Smoothing Factor", &m_xsphSmoothingFactor, 0.0f, 0.2f, "%.3f");
        
        ImGui::Separator();
        ImGui::Text("Low-Speed Stabilization (Anti-Jitter)");
        ImGui::Checkbox("Enable Low-Speed Stabilization", &m_enableLowSpeedStabilization);
        ImGui::SliderFloat("Low Speed Threshold", &m_lowSpeedThreshold, 10.0f, 200.0f, "%.0f");
        ImGui::SliderFloat("Low Speed Damping", &m_lowSpeedDamping, 0.8f, 0.99f, "%.3f");
        ImGui::SliderInt("Stabilization Iterations", &m_lowSpeedStabilizationIterations, 1, 8);
        
        ImGui::Separator();
        int tool = (m_mouseTool == MouseTool::Add) ? 0 : 1;
        if (ImGui::RadioButton("Add Particles (LMB)", tool == 0)) m_mouseTool = MouseTool::Add;
        ImGui::SameLine();
        if (ImGui::RadioButton("Force Brush (LMB)", tool == 1)) m_mouseTool = MouseTool::Force;
        ImGui::SliderFloat("Brush Radius", &m_brushRadius, 5.0f, 120.0f, "%.1f");
        ImGui::SliderFloat("Force Strength", &m_forceStrength, 100.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("Emit Rate (pps)", &m_emitRate, 0.0f, 2000.0f, "%.0f");
        ImGui::SliderFloat("Emit Jitter", &m_emitJitter, 0.0f, 8.0f, "%.1f");

        ImGui::Separator();
        ImGui::Text("Boundaries");
        ImGui::Text("Static boundaries - no rotation");
        
        ImGui::Separator();
        ImGui::Text("Boundary Visualization");
        ImGui::SliderFloat("Left Offset", &m_boundaryLeftOffset, -50.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Right Offset", &m_boundaryRightOffset, -50.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Bottom Offset", &m_boundaryBottomOffset, -50.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Top Offset", &m_boundaryTopOffset, -50.0f, 50.0f, "%.1f");
        if (ImGui::Button("Reset Boundaries"))
        {
            m_boundaryLeftOffset = -15.0f;
            m_boundaryRightOffset = 15.0f;
            m_boundaryBottomOffset = -15.0f;
            m_boundaryTopOffset = 15.0f;
        }

        if (ImGui::Button("Reset Particles", ImVec2(-FLT_MIN, 0)))
        {
            // Remove existing particle entities
            const auto& all = m_entityManager->getEntities();
            std::vector<std::string> toRemove;
            for (const auto& up : all)
            {
                const std::string& n = up->getName();
                if (n.rfind("SPHParticle_", 0) == 0) toRemove.push_back(n);
            }
            for (const auto& n : toRemove) m_entityManager->removeEntity(n);
            spawnParticles();
        }
    }
    ImGui::End();
}

// ========================= SPH Implementation =========================

void SPHFluidSimulationScene::stepSPH(float dt)
{
    updateKernelConstants();
    if (m_useOptimizedLayout)
    {
        stepSPHOptimized(dt);
    }
    else
    {
        // Legacy SPH simulation steps
        updateSpatialGrid();
        buildNeighborLists();
        calculateDensity();
        calculatePressure();
        calculateForces();
        integrateParticles(dt);
        enforceBoundaries();
        
        // LiquidFun-style particle collision detection
        if (m_enableParticleCollisions)
        {
            buildCollisionHash();
            resolveParticleCollisions();
        }
        
        // LiquidFun-style contact sleeping and position constraints
        buildContactList();
        updateContactSleeping();
        applyPositionConstraints();
        applyContactSleeping();
        
        // XSPH velocity smoothing to reduce jitter
        if (m_enableXSPHSmoothing)
        {
            applyXSPHSmoothing();
        }
        
        // Stabilize low-speed particles (bottom layer)
        if (m_enableLowSpeedStabilization)
        {
            stabilizeLowSpeedParticles();
        }
    }
}

void SPHFluidSimulationScene::updateSpatialGrid()
{
    m_spatialGrid.clear();
    
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        const auto& p = m_particles[i];
        m_spatialGrid.addParticle(i, p.position.x, p.position.y);
    }
    if (static_cast<int>(m_neighbors.size()) != static_cast<int>(m_particles.size()))
        m_neighbors.resize(m_particles.size());
    m_neighborsValid = false;
}

void SPHFluidSimulationScene::buildNeighborLists()
{
    if (m_neighborsValid && static_cast<int>(m_neighbors.size()) == static_cast<int>(m_particles.size())) return;
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        m_spatialGrid.findNeighbors(i, m_particles[i].position.x, m_particles[i].position.y, m_sphParams.smoothing_radius, m_neighbors[i]);
    }
    m_neighborsValid = true;
}

void SPHFluidSimulationScene::calculateDensity()
{
    m_density_calculations = 0;
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        float density = 0.0f;
        const auto& neighbors = m_neighbors[i];
        
        for (int j : neighbors)
        {
            if (i == j) continue;
            
            Vec2 r = m_particles[i].position - m_particles[j].position;
            float dx = r.x; float dy = r.y;
            float r2 = dx * dx + dy * dy;
            if (r2 < m_sphParams.smoothing_radius * m_sphParams.smoothing_radius)
            {
                float distance = std::sqrt(std::max(1e-6f, r2));
                density += m_sphParams.mass * poly6Kernel(distance, m_sphParams.smoothing_radius);
                m_density_calculations++;
            }
        }
        
        // Add self-contribution for better density calculation
        density += m_sphParams.mass * poly6Kernel(0.0f, m_sphParams.smoothing_radius);
        
        // Ensure minimum density to prevent division by zero, but allow some compression
        m_particles[i].density = std::max(density, m_sphParams.rest_density * 0.3f);
    }
}

void SPHFluidSimulationScene::calculatePressure()
{
    for (auto& p : m_particles)
    {
        // Ideal gas law: P = k * (ρ - ρ₀)
        p.pressure = m_sphParams.gas_constant * (p.density - m_sphParams.rest_density);
    }
}

void SPHFluidSimulationScene::calculateForces()
{
    m_neighbor_checks = 0;
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        Vec2 pressureForce(0.0f, 0.0f);
        Vec2 viscosityForce(0.0f, 0.0f);
        Vec2 artificialPressureForce(0.0f, 0.0f);
        
        const auto& neighbors = m_neighbors[i];
        
        for (int j : neighbors)
        {
            if (i == j) continue;
            
            Vec2 r = m_particles[i].position - m_particles[j].position;
            float dx = r.x; float dy = r.y;
            float r2 = dx * dx + dy * dy;
            if (r2 < m_sphParams.smoothing_radius * m_sphParams.smoothing_radius && r2 > 1e-12f)
            {
                float distance = std::sqrt(r2);
                m_neighbor_checks++;
                
                // Standard pressure force
                float pressureTerm = (m_particles[i].pressure + m_particles[j].pressure) / (2.0f * m_particles[j].density);
                Vec2 pressureGradient = spikyKernelGradient(r, m_sphParams.smoothing_radius);
                pressureForce -= pressureGradient * (static_cast<f32>(m_sphParams.mass) * static_cast<f32>(pressureTerm));
                
                // Artificial pressure for incompressibility (Monaghan 1994)
                float densityRatio = m_particles[i].density / m_sphParams.rest_density;
                float artificialPressure = m_sphParams.artificial_pressure * (densityRatio * densityRatio * densityRatio * densityRatio - 1.0f);
                artificialPressureForce -= pressureGradient * (static_cast<f32>(m_sphParams.mass) * static_cast<f32>(artificialPressure));
                
                // Viscosity force
                Vec2 velocityDiff = m_particles[j].velocity - m_particles[i].velocity;
                float viscosityTerm = m_sphParams.viscosity * m_sphParams.mass * viscosityKernel(distance, m_sphParams.smoothing_radius) / static_cast<f32>(m_particles[j].density);
                viscosityForce += velocityDiff * static_cast<f32>(viscosityTerm);
                
                // Artificial viscosity for stability
                float artificialViscosityTerm = m_sphParams.artificial_viscosity * m_sphParams.mass * viscosityKernel(distance, m_sphParams.smoothing_radius) / static_cast<f32>(m_particles[j].density);
                viscosityForce += velocityDiff * static_cast<f32>(artificialViscosityTerm);
            }
        }
        
        // Total force = pressure + artificial pressure + viscosity + gravity
        Vec2 totalForce = (pressureForce + artificialPressureForce + viscosityForce) / static_cast<f32>(m_particles[i].density) + Vec2(0.0f, static_cast<f32>(m_sphParams.gravity));
        
        // More reasonable force limiting to allow stronger pressure forces
        float forceMagnitude = totalForce.length();
        if (forceMagnitude > 5000.0f) // Increased from 1000.0f
        {
            totalForce = totalForce * (5000.0f / forceMagnitude);
        }
        
        m_particles[i].acceleration = totalForce;
    }
    
    // Update average neighbors
    if (m_particles.size() > 0)
    {
        m_average_neighbors = static_cast<float>(m_neighbor_checks) / m_particles.size();
    }
}

void SPHFluidSimulationScene::integrateParticles(float dt)
{
    for (auto& p : m_particles)
    {
        // Verlet integration for better stability
        Vec2 oldVelocity = p.velocity;
        p.velocity += p.acceleration * static_cast<f32>(dt);
        
        // Gentle damping to prevent instability while preserving fluid motion
        p.velocity *= 0.995f; // Reduced damping for better fluid behavior
        
        // Position update with velocity averaging for stability
        p.position += (oldVelocity + p.velocity) * 0.5f * static_cast<f32>(dt);
    }
}

void SPHFluidSimulationScene::enforceBoundaries()
{
    // Always use FirmGuy boundary collision detection for consistency
    resolveParticleBoundaryCollisions();
}

void SPHFluidSimulationScene::resolveParticleBoundaryCollisions()
{
    // Simple axis-aligned boundary collision detection
    for (auto& particle : m_particles)
    {
        bool collided = false;
        Vec2 normal(0.0f, 0.0f);
        
        // Check against domain boundaries
        if (particle.position.x < m_domainMin.x + m_particleRadius)
        {
            particle.position.x = m_domainMin.x + m_particleRadius;
            normal = Vec2(1.0f, 0.0f);
            collided = true;
        }
        else if (particle.position.x > m_domainMax.x - m_particleRadius)
        {
            particle.position.x = m_domainMax.x - m_particleRadius;
            normal = Vec2(-1.0f, 0.0f);
            collided = true;
        }
        
        if (particle.position.y < m_domainMin.y + m_particleRadius)
        {
            particle.position.y = m_domainMin.y + m_particleRadius;
            normal = Vec2(0.0f, 1.0f);
            collided = true;
        }
        else if (particle.position.y > m_domainMax.y - m_particleRadius)
        {
            particle.position.y = m_domainMax.y - m_particleRadius;
            normal = Vec2(0.0f, -1.0f);
            collided = true;
        }
        
        // Apply velocity reflection if collided
        if (collided)
        {
            float vN = particle.velocity.dot(normal);
            if (vN < 0.0f)
            {
                float restitution = 0.1f;
                particle.velocity = particle.velocity - normal * vN * (1.0f + restitution);
                particle.velocity *= 0.98f; // Damping
            }
        }
    }
}

void SPHFluidSimulationScene::updateParticleSprites()
{
    for (const auto& p : m_particles)
    {
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                s->setPosition(p.position.x, p.position.y, 0.0f);
                
                Vec4 color;
                if (m_colorBySpeed)
                {
                    float speed = p.velocity.length();
                    float sMin = std::min(m_colorSpeedMin, m_colorSpeedMax - 1.0f);
                    float sMax = std::max(m_colorSpeedMax, sMin + 1.0f);
                    float t = (speed - sMin) / (sMax - sMin);
                    t = clampf(t, 0.0f, 1.0f);

                    if (m_debugColor)
                    {
                        // Debug gradient: Blue -> Green -> Red
                        Vec4 blue  = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
                        Vec4 green = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
                        Vec4 red   = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
                        if (t < 0.5f)
                        {
                            float k = t / 0.5f;
                            color = Vec4(
                                blue.x + (green.x - blue.x) * k,
                                blue.y + (green.y - blue.y) * k,
                                blue.z + (green.z - blue.z) * k,
                                1.0f);
                        }
                        else
                        {
                            float k = (t - 0.5f) / 0.5f;
                            color = Vec4(
                                green.x + (red.x - green.x) * k,
                                green.y + (red.y - green.y) * k,
                                green.z + (red.z - green.z) * k,
                                1.0f);
                        }
                    }
                    else
                    {
                        // Regular gradient: deep blue -> cyan -> white
                        Vec4 slow = Vec4(0.1f, 0.35f, 0.9f, 1.0f);
                        Vec4 mid  = Vec4(0.0f, 1.0f, 1.0f, 1.0f);
                        Vec4 fast = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
                        if (t < 0.5f)
                        {
                            float k = t / 0.5f;
                            color = Vec4(
                                slow.x + (mid.x - slow.x) * k,
                                slow.y + (mid.y - slow.y) * k,
                                slow.z + (mid.z - slow.z) * k,
                                1.0f);
                        }
                        else
                        {
                            float k = (t - 0.5f) / 0.5f;
                            color = Vec4(
                                mid.x + (fast.x - mid.x) * k,
                                mid.y + (fast.y - mid.y) * k,
                                mid.z + (fast.z - mid.z) * k,
                                1.0f);
                        }
                    }
                }
                else
                {
                    // Density-based fallback coloring
                    float densityRatio = p.density / m_sphParams.rest_density;
                    color = Vec4(0.2f, 0.6f, 1.0f, 1.0f);
                    if (densityRatio > 1.0f)
                    {
                        color = Vec4(1.0f, 0.2f, 0.2f, 1.0f);
                    }
                }
                s->setTint(color);
            }
        }
    }
}

// ========================= SPH Kernels =========================

float SPHFluidSimulationScene::poly6Kernel(float distance, float smoothing_radius)
{
    if (distance >= smoothing_radius) return 0.0f;
    
    float q = distance / smoothing_radius;
    float t = 1.0f - q * q;
    float t2 = t * t;
    float t3 = t2 * t;
    float h = smoothing_radius;
    float h3 = h * h * h;
    float h6 = h3 * h3;
    float h9 = h6 * h3;
    return (315.0f / (64.0f * 3.14159f * h9)) * t3;
}

Vec2 SPHFluidSimulationScene::poly6KernelGradient(const Vec2& r, float smoothing_radius)
{
    float distance = r.length();
    if (distance >= smoothing_radius || distance < 1e-6f) return Vec2(0.0f, 0.0f);
    
    float h = smoothing_radius;
    float h3 = h * h * h;
    float h6 = h3 * h3;
    float h9 = h6 * h3;
    float q = distance / h;
    float t = 1.0f - q * q;
    float t2 = t * t;
    float factor = -(945.0f / (32.0f * 3.14159f * h9)) * t2;
    return r * static_cast<f32>(factor / distance);
}

float SPHFluidSimulationScene::spikyKernel(float distance, float smoothing_radius)
{
    if (distance >= smoothing_radius) return 0.0f;
    
    float h = smoothing_radius;
    float h3 = h * h * h;
    float h6 = h3 * h3;
    float q = distance / h;
    float t = 1.0f - q;
    float t2 = t * t;
    float t3 = t2 * t;
    return (15.0f / (3.14159f * h6)) * t3;
}

Vec2 SPHFluidSimulationScene::spikyKernelGradient(const Vec2& r, float smoothing_radius)
{
    float distance = r.length();
    if (distance >= smoothing_radius || distance < 1e-6f) return Vec2(0.0f, 0.0f);
    
    float h = smoothing_radius;
    float h3 = h * h * h;
    float h6 = h3 * h3;
    float q = distance / h;
    float t = 1.0f - q;
    float t2 = t * t;
    float factor = -(45.0f / (3.14159f * h6)) * t2;
    return r * static_cast<f32>(factor / distance);
}

float SPHFluidSimulationScene::viscosityKernel(float distance, float smoothing_radius)
{
    if (distance >= smoothing_radius) return 0.0f;
    
    float h = smoothing_radius;
    float h3 = h * h * h;
    float h6 = h3 * h3;
    float q = distance / h;
    q = std::max(q, 1e-6f);
    return (15.0f / (2.0f * 3.14159f * h6)) * (-0.5f * q * q * q + q * q + 0.5f / q - 1.0f);
}

// ========================= Spatial Grid Implementation =========================

void SPHFluidSimulationScene::SpatialGrid::initialize(float world_width, float world_height, float world_min_x, float world_min_y, float smoothing_radius, float cell_scale)
{
    this->world_min = Vec2(world_min_x, world_min_y);
    this->world_max = Vec2(world_min_x + world_width, world_min_y + world_height);
    
    // Cell size relative to smoothing radius (default 1.0x). Lower => more cells, fewer candidates
    cell_size = std::max(1.0f, smoothing_radius * cell_scale);
    grid_width = static_cast<int>(std::ceil(world_width / cell_size));
    grid_height = static_cast<int>(std::ceil(world_height / cell_size));
    
    cells.resize(grid_width * grid_height);
    temp_neighbors.reserve(64);
    
    printf("SPH Spatial grid initialized: Grid %dx%d, Cell size %.1f\n", 
           grid_width, grid_height, cell_size);
}

void SPHFluidSimulationScene::SpatialGrid::clear()
{
    for (size_t i = 0; i < cells.size(); ++i)
    {
        cells[i].clear();
    }
}

void SPHFluidSimulationScene::SpatialGrid::addParticle(int particle_id, float x, float y)
{
    int grid_x = static_cast<int>((x - world_min.x) / cell_size);
    int grid_y = static_cast<int>((y - world_min.y) / cell_size);
    
    grid_x = std::max(0, std::min(grid_x, grid_width - 1));
    grid_y = std::max(0, std::min(grid_y, grid_height - 1));
    
    int cell_index = grid_y * grid_width + grid_x;
    cells[cell_index].push_back(particle_id);
}

void SPHFluidSimulationScene::SpatialGrid::findNeighbors(int particle_id, float x, float y, float radius, std::vector<int>& neighbors)
{
    neighbors.clear();
    
    int grid_x = static_cast<int>((x - world_min.x) / cell_size);
    int grid_y = static_cast<int>((y - world_min.y) / cell_size);
    
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int check_x = grid_x + dx;
            int check_y = grid_y + dy;
            
            if (check_x < 0 || check_x >= grid_width ||
                check_y < 0 || check_y >= grid_height)
                continue;
            
            int cell_index = check_y * grid_width + check_x;
            const auto& cell = cells[cell_index];
            
            for (int other_id : cell)
            {
                if (other_id == particle_id) continue;
                neighbors.push_back(other_id);
            }
        }
    }
}

// ========================= Mouse Interaction =========================

Vec2 SPHFluidSimulationScene::getMouseWorldPosition() const
{
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return Vec2(0.0f, 0.0f);
    auto* cam = cameraEntity->getComponent<Camera2D>();
    if (!cam) return Vec2(0.0f, 0.0f);
    Vec2 mouseClient = Input::getInstance().getMousePositionClient();
    return cam->screenToWorld(Vec2(mouseClient.x, mouseClient.y));
}

void SPHFluidSimulationScene::addParticlesAt(const Vec2& worldPos, int count, float jitter)
{
    int startIdx = (int)m_particles.size();
    for (int i = 0; i < count; ++i)
    {
        SPHParticle p;
        float rx = ((rand() % 2000) / 1000.0f - 1.0f) * jitter;
        float ry = ((rand() % 2000) / 1000.0f - 1.0f) * jitter;
        p.position = worldPos + Vec2(rx, ry);
        p.velocity = Vec2(0.0f, 0.0f);
        p.acceleration = Vec2(0.0f, 0.0f);
        p.density = m_sphParams.rest_density;
        p.pressure = 0.0f;
        p.entityName = "SPHParticle_" + std::to_string(startIdx + i);

        auto& e = m_entityManager->createEntity(p.entityName);
        auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/MetaballFalloff.png", m_particleRadius * 2.0f, m_particleRadius * 2.0f);
        s.setPosition(p.position.x, p.position.y, 0.0f);
        s.setTint(Vec4(0.2f, 0.6f, 1.0f, 1.0f));
        m_particles.push_back(p);
    }
}

void SPHFluidSimulationScene::applyForceBrush(const Vec2& worldPos, const Vec2& worldVel)
{
    float r2 = m_brushRadius * m_brushRadius;
    for (auto& p : m_particles)
    {
        Vec2 d = p.position - worldPos;
        float dist2 = d.x * d.x + d.y * d.y;
        if (dist2 <= r2)
        {
            float dist = std::sqrt(std::max(1e-4f, dist2));
            float falloff = 1.0f - dist / m_brushRadius;
            p.velocity += worldVel * falloff * (m_forceStrength / 10000.0f);
        }
    }
}

// ========================= Collision Detection (LiquidFun Style) =========================

void SPHFluidSimulationScene::buildCollisionHash()
{
    m_collisionHash.clear();
    float inv = 1.0f / std::max(1.0f, m_collisionHashCellSize);
    
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        const Vec2& pos = m_particles[i].position;
        int ix = static_cast<int>(std::floor((pos.x - m_domainMin.x) * inv));
        int iy = static_cast<int>(std::floor((pos.y - m_domainMin.y) * inv));
        m_collisionHash[collisionHashKey(ix, iy)].push_back(i);
    }
}

void SPHFluidSimulationScene::resolveParticleCollisions()
{
    const float particleDiameter = m_particleRadius * 2.0f;
    const float targetDistance = particleDiameter * 0.95f; // Slightly less for packing
    const float targetDistance2 = targetDistance * targetDistance;
    const float slop = particleDiameter * 0.1f; // penetration allowance to reduce jitter
    const float smallRelVel = 5.0f; // threshold to damp tiny bouncing
    std::vector<int> neighbors;
    neighbors.reserve(128);
    for (int iteration = 0; iteration < m_collisionIterations; ++iteration)
    {
        // Shock propagation: process from bottom to top
        std::vector<int> order(m_particles.size());
        for (int k = 0; k < static_cast<int>(m_particles.size()); ++k) order[k] = k;
        std::sort(order.begin(), order.end(), [&](int a, int b) { return m_particles[a].position.y < m_particles[b].position.y; });

        for (int idx = 0; idx < static_cast<int>(order.size()); ++idx)
        {
            int i = order[idx];
            const Vec2& pos_i = m_particles[i].position;
            int ix = static_cast<int>(std::floor((pos_i.x - m_domainMin.x) / m_collisionHashCellSize));
            int iy = static_cast<int>(std::floor((pos_i.y - m_domainMin.y) / m_collisionHashCellSize));
            
            // Check 3x3 grid around particle
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    auto it = m_collisionHash.find(collisionHashKey(ix + dx, iy + dy));
                    if (it == m_collisionHash.end()) continue;
                    
                    const auto& bucket = it->second;
                    int processed = 0;
                    for (int j : bucket)
                    {
                        if (j <= i) continue; // Avoid duplicate pairs
                        if (processed++ >= m_maxCollisionNeighbors) break; // Cap work per particle
                        
                        Vec2 pos_j = m_particles[j].position;
                        Vec2 dp = pos_j - pos_i;
                        float dist2 = dp.x * dp.x + dp.y * dp.y;
                        
                        if (dist2 < targetDistance2 && dist2 > 1e-6f)
                        {
                            float dist = std::sqrt(dist2);
                            Vec2 normal = dp * (1.0f / dist);
                            float overlap = targetDistance - dist;
                            
                            // Skip tiny penetrations (slop) to avoid jitter
                            if (overlap <= slop) continue;
                            
                            // Split impulse: position correction WITHOUT velocity injection
                            float separation = (overlap - slop) * 0.4f;
                            m_particles[i].position -= normal * separation;
                            m_particles[j].position += normal * separation;
                            
                            // Only apply velocity response for significant relative motion
                            Vec2 relVel = m_particles[j].velocity - m_particles[i].velocity;
                            float relVelN = relVel.x * normal.x + relVel.y * normal.y;
                            
                            if (relVelN < -smallRelVel) // Only for significant approaching motion
                            {
                                // Gentle velocity correction (no bouncing)
                                float correction = -relVelN * 0.1f; // Very gentle
                                Vec2 velCorrection = normal * correction;
                                m_particles[i].velocity += velCorrection;
                                m_particles[j].velocity -= velCorrection;
                            }
                        }
                    }
                }
            }
        }
        
        // Rebuild hash after positions changed
        if (iteration + 1 < m_collisionIterations)
        {
            buildCollisionHash();
        }
    }
}

long long SPHFluidSimulationScene::collisionHashKey(int ix, int iy) const
{
    return (static_cast<long long>(ix) << 32) ^ static_cast<unsigned long long>(iy);
}

// ========================= LiquidFun-Style Optimized Implementation =========================

void SPHFluidSimulationScene::OptimizedParticleData::resize(size_t new_capacity)
{
    capacity = new_capacity;
    positions_x.resize(capacity);
    positions_y.resize(capacity);
    velocities_x.resize(capacity);
    velocities_y.resize(capacity);
    accelerations_x.resize(capacity);
    accelerations_y.resize(capacity);
    densities.resize(capacity);
    pressures.resize(capacity);
    masses.resize(capacity);
    radii.resize(capacity);
    colors.resize(capacity);
    entity_ids.resize(capacity);
    is_awake.resize(capacity);
}

void SPHFluidSimulationScene::OptimizedParticleData::addParticle(const SPHParticle& p, uint16_t entity_id)
{
    if (count >= capacity) resize(capacity * 2);
    
    positions_x[count] = p.position.x;
    positions_y[count] = p.position.y;
    velocities_x[count] = p.velocity.x;
    velocities_y[count] = p.velocity.y;
    accelerations_x[count] = p.acceleration.x;
    accelerations_y[count] = p.acceleration.y;
    densities[count] = p.density;
    pressures[count] = p.pressure;
    masses[count] = 1.0f; // Default mass
    radii[count] = 4.0f; // Default radius
    colors[count] = 0xFF4080FF; // Default blue
    entity_ids[count] = entity_id;
    is_awake[count] = true;
    
    count++;
}

void SPHFluidSimulationScene::OptimizedParticleData::syncFromParticles(const std::vector<SPHParticle>& particles)
{
    if (particles.size() != count)
    {
        count = particles.size();
        if (count > capacity) resize(count);
    }
    
    for (size_t i = 0; i < count; ++i)
    {
        positions_x[i] = particles[i].position.x;
        positions_y[i] = particles[i].position.y;
        velocities_x[i] = particles[i].velocity.x;
        velocities_y[i] = particles[i].velocity.y;
        accelerations_x[i] = particles[i].acceleration.x;
        accelerations_y[i] = particles[i].acceleration.y;
        densities[i] = particles[i].density;
        pressures[i] = particles[i].pressure;
        is_awake[i] = true; // ensure active after sync
    }
}

void SPHFluidSimulationScene::OptimizedParticleData::syncToParticles(std::vector<SPHParticle>& particles)
{
    for (size_t i = 0; i < count && i < particles.size(); ++i)
    {
        particles[i].position.x = positions_x[i];
        particles[i].position.y = positions_y[i];
        particles[i].velocity.x = velocities_x[i];
        particles[i].velocity.y = velocities_y[i];
        particles[i].acceleration.x = accelerations_x[i];
        particles[i].acceleration.y = accelerations_y[i];
        particles[i].density = densities[i];
        particles[i].pressure = pressures[i];
    }
}

void SPHFluidSimulationScene::stepSPHOptimized(float dt)
{
    // Sync from particles to optimized layout
    m_optimizedParticles.syncFromParticles(m_particles);
    
    // Update spatial grid with optimized data
    m_spatialGrid.clear();
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        m_spatialGrid.addParticle(static_cast<int>(i), m_optimizedParticles.positions_x[i], m_optimizedParticles.positions_y[i]);
    }
    
    // Optimized SPH steps
    calculateDensityOptimized();
    calculatePressureOptimized();
    calculateForcesOptimized();
    integrateParticlesOptimized(dt);
    
    // Rebuild spatial grid for collision phase (positions updated during integration)
    m_spatialGrid.clear();
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        m_spatialGrid.addParticle(static_cast<int>(i), m_optimizedParticles.positions_x[i], m_optimizedParticles.positions_y[i]);
    }
    
    // Optimized collision resolution
    if (m_enableParticleCollisions)
    {
        resolveCollisionsOptimized();
    }
    
    // Update islands for sleeping particles
    if (m_enableIslandSimulation)
    {
        updateIslands();
    }
    
    // Sync back to particles
    m_optimizedParticles.syncToParticles(m_particles);
    
    // Enforce boundaries
    enforceBoundaries();
}

void SPHFluidSimulationScene::calculateDensityOptimized()
{
    m_density_calculations = 0;
    std::vector<int> neighbors;
    neighbors.reserve(64);
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        if (!m_optimizedParticles.is_awake[i]) continue;
        
        f32 density = 0.0f;
        m_spatialGrid.findNeighbors(static_cast<int>(i), m_optimizedParticles.positions_x[i], m_optimizedParticles.positions_y[i], m_sphParams.smoothing_radius, neighbors);
        
        for (int j : neighbors)
        {
            if (static_cast<size_t>(j) == i) continue;
            
            f32 dx = m_optimizedParticles.positions_x[i] - m_optimizedParticles.positions_x[j];
            f32 dy = m_optimizedParticles.positions_y[i] - m_optimizedParticles.positions_y[j];
            f32 distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < m_sphParams.smoothing_radius)
            {
                density += m_sphParams.mass * poly6Kernel(distance, m_sphParams.smoothing_radius);
                m_density_calculations++;
            }
        }
        
        m_optimizedParticles.densities[i] = std::max(density, m_sphParams.rest_density * 0.1f);
    }
}

void SPHFluidSimulationScene::calculateForcesOptimized()
{
    m_neighbor_checks = 0;
    std::vector<int> neighbors;
    neighbors.reserve(64);
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        if (!m_optimizedParticles.is_awake[i]) continue;
        
        f32 pressureForceX = 0.0f;
        f32 pressureForceY = 0.0f;
        f32 viscosityForceX = 0.0f;
        f32 viscosityForceY = 0.0f;
        f32 artificialPressureForceX = 0.0f;
        f32 artificialPressureForceY = 0.0f;
        
        m_spatialGrid.findNeighbors(static_cast<int>(i), m_optimizedParticles.positions_x[i], m_optimizedParticles.positions_y[i], m_sphParams.smoothing_radius, neighbors);
        
        for (int j : neighbors)
        {
            if (static_cast<size_t>(j) == i) continue;
            
            f32 dx = m_optimizedParticles.positions_x[i] - m_optimizedParticles.positions_x[j];
            f32 dy = m_optimizedParticles.positions_y[i] - m_optimizedParticles.positions_y[j];
            f32 distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < m_sphParams.smoothing_radius && distance > 1e-6f)
            {
                m_neighbor_checks++;
                
                // Standard pressure force
                f32 pressureTerm = (m_optimizedParticles.pressures[i] + m_optimizedParticles.pressures[j]) / (2.0f * std::max(1e-3f, m_optimizedParticles.densities[j]));
                Vec2 pressureGradient = spikyKernelGradient(Vec2(dx, dy), m_sphParams.smoothing_radius);
                pressureForceX -= pressureGradient.x * (m_sphParams.mass * pressureTerm);
                pressureForceY -= pressureGradient.y * (m_sphParams.mass * pressureTerm);
                
                // Artificial pressure for incompressibility
                f32 densityRatio = m_optimizedParticles.densities[i] / m_sphParams.rest_density;
                f32 artificialPressure = m_sphParams.artificial_pressure * (densityRatio * densityRatio * densityRatio * densityRatio - 1.0f);
                artificialPressureForceX -= pressureGradient.x * (m_sphParams.mass * artificialPressure);
                artificialPressureForceY -= pressureGradient.y * (m_sphParams.mass * artificialPressure);
                
                // Viscosity force
                f32 velocityDiffX = m_optimizedParticles.velocities_x[j] - m_optimizedParticles.velocities_x[i];
                f32 velocityDiffY = m_optimizedParticles.velocities_y[j] - m_optimizedParticles.velocities_y[i];
                f32 viscosityTerm = m_sphParams.viscosity * m_sphParams.mass * viscosityKernel(distance, m_sphParams.smoothing_radius) / std::max(1e-3f, m_optimizedParticles.densities[j]);
                viscosityForceX += velocityDiffX * viscosityTerm;
                viscosityForceY += velocityDiffY * viscosityTerm;
                
                // Artificial viscosity for stability
                f32 artificialViscosityTerm = m_sphParams.artificial_viscosity * m_sphParams.mass * viscosityKernel(distance, m_sphParams.smoothing_radius) / std::max(1e-3f, m_optimizedParticles.densities[j]);
                viscosityForceX += velocityDiffX * artificialViscosityTerm;
                viscosityForceY += velocityDiffY * artificialViscosityTerm;
            }
        }
        
        // Total force with improved magnitude limiting
        f32 denom = std::max(1e-3f, m_optimizedParticles.densities[i]);
        f32 totalForceX = (pressureForceX + artificialPressureForceX + viscosityForceX) / denom;
        f32 totalForceY = (pressureForceY + artificialPressureForceY + viscosityForceY) / denom + m_sphParams.gravity;
        
        f32 forceMagnitude = std::sqrt(totalForceX * totalForceX + totalForceY * totalForceY);
        if (forceMagnitude > 5000.0f) // Increased from 1000.0f
        {
            f32 scale = 5000.0f / forceMagnitude;
            totalForceX *= scale;
            totalForceY *= scale;
        }
        
        m_optimizedParticles.accelerations_x[i] = totalForceX;
        m_optimizedParticles.accelerations_y[i] = totalForceY;
    }
    
    // Update average neighbors
    if (m_optimizedParticles.count > 0)
    {
        m_average_neighbors = static_cast<float>(m_neighbor_checks) / m_optimizedParticles.count;
    }
}

void SPHFluidSimulationScene::calculatePressureOptimized()
{
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        m_optimizedParticles.pressures[i] = m_sphParams.gas_constant * (m_optimizedParticles.densities[i] - m_sphParams.rest_density);
    }
}

void SPHFluidSimulationScene::integrateParticlesOptimized(float dt)
{
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        if (!m_optimizedParticles.is_awake[i]) continue;
        
        // Verlet integration with damping
        m_optimizedParticles.velocities_x[i] += m_optimizedParticles.accelerations_x[i] * dt;
        m_optimizedParticles.velocities_y[i] += m_optimizedParticles.accelerations_y[i] * dt;
        m_optimizedParticles.velocities_x[i] *= 0.99f; // Damping
        m_optimizedParticles.velocities_y[i] *= 0.99f;
        
        m_optimizedParticles.positions_x[i] += m_optimizedParticles.velocities_x[i] * dt;
        m_optimizedParticles.positions_y[i] += m_optimizedParticles.velocities_y[i] * dt;
    }
}

void SPHFluidSimulationScene::resolveCollisionsOptimized()
{
    const f32 particleDiameter = m_particleRadius * 2.0f;
    const f32 targetDistance = particleDiameter * 0.95f;
    const f32 targetDistance2 = targetDistance * targetDistance;
    const f32 slop = particleDiameter * 0.1f;
    const f32 smallRelVel = 5.0f;
    
    for (int iteration = 0; iteration < m_collisionIterations; ++iteration)
    {
        std::vector<int> neighbors;
        neighbors.reserve(128);
        // Shock propagation order: bottom to top
        std::vector<int> order(static_cast<int>(m_optimizedParticles.count));
        for (int k = 0; k < static_cast<int>(m_optimizedParticles.count); ++k) order[k] = k;
        std::sort(order.begin(), order.end(), [&](int a, int b) { return m_optimizedParticles.positions_y[a] < m_optimizedParticles.positions_y[b]; });
        for (size_t ord = 0; ord < order.size(); ++ord)
        {
            int i = order[ord];
            if (!m_optimizedParticles.is_awake[i]) continue;
            
            f32 posX = m_optimizedParticles.positions_x[i];
            f32 posY = m_optimizedParticles.positions_y[i];
            
            m_spatialGrid.findNeighbors(static_cast<int>(i), posX, posY, m_sphParams.smoothing_radius, neighbors);
            
            int processed = 0;
            for (int j : neighbors)
            {
                if (static_cast<size_t>(j) <= i || !m_optimizedParticles.is_awake[j]) continue;
                if (processed++ >= m_maxCollisionNeighbors) break;
                
                f32 dx = m_optimizedParticles.positions_x[j] - posX;
                f32 dy = m_optimizedParticles.positions_y[j] - posY;
                f32 dist2 = dx * dx + dy * dy;
                
                if (dist2 < targetDistance2 && dist2 > 1e-6f)
                {
                    f32 dist = std::sqrt(dist2);
                    f32 invDist = 1.0f / dist;
                    f32 normalX = dx * invDist;
                    f32 normalY = dy * invDist;
                    f32 overlap = targetDistance - dist;
                    
                    if (overlap <= slop) continue;
                    // Split impulse: position correction WITHOUT velocity injection
                    f32 separation = (overlap - slop) * 0.4f;
                    m_optimizedParticles.positions_x[i] -= normalX * separation;
                    m_optimizedParticles.positions_y[i] -= normalY * separation;
                    m_optimizedParticles.positions_x[j] += normalX * separation;
                    m_optimizedParticles.positions_y[j] += normalY * separation;
                    
                    // Only apply velocity response for significant relative motion
                    f32 relVelX = m_optimizedParticles.velocities_x[j] - m_optimizedParticles.velocities_x[i];
                    f32 relVelY = m_optimizedParticles.velocities_y[j] - m_optimizedParticles.velocities_y[i];
                    f32 relVelN = relVelX * normalX + relVelY * normalY;
                    
                    if (relVelN < -smallRelVel) // Only for significant approaching motion
                    {
                        // Gentle velocity correction (no bouncing)
                        f32 correction = -relVelN * 0.1f; // Very gentle
                        f32 velCorrectionX = normalX * correction;
                        f32 velCorrectionY = normalY * correction;
                        m_optimizedParticles.velocities_x[i] += velCorrectionX;
                        m_optimizedParticles.velocities_y[i] += velCorrectionY;
                        m_optimizedParticles.velocities_x[j] -= velCorrectionX;
                        m_optimizedParticles.velocities_y[j] -= velCorrectionY;
                    }
                }
            }
        }
    }
}

void SPHFluidSimulationScene::updateIslands()
{
    // Simple island update - mark particles as awake if they're moving fast enough
    for (size_t i = 0; i < m_optimizedParticles.count; ++i)
    {
        f32 speed = std::sqrt(m_optimizedParticles.velocities_x[i] * m_optimizedParticles.velocities_x[i] + 
                             m_optimizedParticles.velocities_y[i] * m_optimizedParticles.velocities_y[i]);
        
        if (speed > m_sleepThreshold)
        {
            m_optimizedParticles.is_awake[i] = true;
        }
        else
        {
            // Could implement sleep counter here
            m_optimizedParticles.is_awake[i] = true; // Keep awake for now
        }
    }
}

void SPHFluidSimulationScene::buildContactCache()
{
    // Contact caching implementation - for future optimization
    m_contactCache.clear();
}

// ========================= LiquidFun-Style Contact Sleeping & Position Constraints =========================

void SPHFluidSimulationScene::buildContactList()
{
    m_contactList.clear();
    const float particleDiameter = m_particleRadius * 2.0f;
    const float contactDistance = particleDiameter * 0.98f;
    const float contactDistance2 = contactDistance * contactDistance;
    
    // Build contact list using spatial grid
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        std::vector<int> neighbors;
        m_spatialGrid.findNeighbors(i, m_particles[i].position.x, m_particles[i].position.y, m_sphParams.smoothing_radius, neighbors);
        
        for (int j : neighbors)
        {
            if (j <= i) continue;
            
            Vec2 dp = m_particles[j].position - m_particles[i].position;
            float dist2 = dp.x * dp.x + dp.y * dp.y;
            
            if (dist2 < contactDistance2 && dist2 > 1e-6f)
            {
                float dist = std::sqrt(dist2);
                Vec2 normal = dp * (1.0f / dist);
                float overlap = contactDistance - dist;
                
                ContactInfo contact;
                contact.particle_a = i;
                contact.particle_b = j;
                contact.normal = normal;
                contact.overlap = overlap;
                contact.sleep_counter = 0;
                contact.is_active = true;
                m_contactList.push_back(contact);
            }
        }
    }
}

void SPHFluidSimulationScene::updateContactSleeping()
{
    if (!m_enableContactSleeping) return;
    
    for (auto& contact : m_contactList)
    {
        if (!contact.is_active) continue;
        
        Vec2& pos_a = m_particles[contact.particle_a].position;
        Vec2& pos_b = m_particles[contact.particle_b].position;
        Vec2& vel_a = m_particles[contact.particle_a].velocity;
        Vec2& vel_b = m_particles[contact.particle_b].velocity;
        
        // Check if particles are moving slowly relative to each other
        Vec2 rel_vel = vel_b - vel_a;
        float rel_vel_mag = rel_vel.length();
        
        if (rel_vel_mag < m_contactSleepVelocity)
        {
            contact.sleep_counter++;
        }
        else
        {
            contact.sleep_counter = 0;
        }
        
        // Put contact to sleep if it's been stable long enough
        if (contact.sleep_counter > m_contactSleepThreshold)
        {
            contact.is_active = false;
        }
    }
}

void SPHFluidSimulationScene::applyPositionConstraints()
{
    if (!m_enablePositionConstraints) return;
    
    const float particleDiameter = m_particleRadius * 2.0f;
    const float targetDistance = particleDiameter * 0.95f;
    
    // Apply position-based constraints (LiquidFun style)
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        for (auto& contact : m_contactList)
        {
            if (!contact.is_active) continue;
            
            int i = contact.particle_a;
            int j = contact.particle_b;
            
            Vec2& pos_a = m_particles[i].position;
            Vec2& pos_b = m_particles[j].position;
            Vec2& vel_a = m_particles[i].velocity;
            Vec2& vel_b = m_particles[j].velocity;
            
            Vec2 dp = pos_b - pos_a;
            float dist = dp.length();
            
            if (dist < targetDistance && dist > 1e-6f)
            {
                Vec2 normal = dp * (1.0f / dist);
                float overlap = targetDistance - dist;
                
                // Position correction (LiquidFun style)
                float correction = overlap * m_positionConstraintStrength;
                Vec2 correction_vec = normal * correction;
                
                pos_a -= correction_vec * 0.5f;
                pos_b += correction_vec * 0.5f;
                
                // Velocity damping for stability
                Vec2 rel_vel = vel_b - vel_a;
                float rel_vel_n = rel_vel.x * normal.x + rel_vel.y * normal.y;
                
                if (rel_vel_n < 0.0f) // Approaching
                {
                    Vec2 damp_impulse = normal * (rel_vel_n * m_positionConstraintDamping * 0.5f);
                    vel_a += damp_impulse;
                    vel_b -= damp_impulse;
                }
            }
        }
    }
}

void SPHFluidSimulationScene::applyContactSleeping()
{
    if (!m_enableContactSleeping) return;
    
    // Skip velocity updates for sleeping contacts
    for (auto& contact : m_contactList)
    {
        if (!contact.is_active)
        {
            // Apply gentle damping to sleeping particles
            m_particles[contact.particle_a].velocity *= 0.99f;
            m_particles[contact.particle_b].velocity *= 0.99f;
        }
    }
}

void SPHFluidSimulationScene::applyXSPHSmoothing()
{
    if (!m_enableXSPHSmoothing) return;
    
    // XSPH velocity smoothing: v_i += c * Σ_j (m_j/ρ_j) * (v_j - v_i) * W_ij
    std::vector<Vec2> smoothed_velocities(m_particles.size());
    
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        Vec2 smoothing_velocity(0.0f, 0.0f);
        const auto& neighbors = m_neighbors[i];
        
        for (int j : neighbors)
        {
            if (i == j) continue;
            
            Vec2 r = m_particles[i].position - m_particles[j].position;
            float dx = r.x; float dy = r.y;
            float r2 = dx * dx + dy * dy;
            if (r2 < m_sphParams.smoothing_radius * m_sphParams.smoothing_radius && r2 > 1e-12f)
            {
                float distance = std::sqrt(r2);
                // XSPH smoothing term
                float kernel_value = poly6Kernel(distance, m_sphParams.smoothing_radius);
                Vec2 velocity_diff = m_particles[j].velocity - m_particles[i].velocity;
                float mass_density_ratio = m_sphParams.mass / std::max(1e-3f, m_particles[j].density);
                
                smoothing_velocity += velocity_diff * (mass_density_ratio * kernel_value);
            }
        }
        
        // Apply smoothing with factor
        smoothed_velocities[i] = m_particles[i].velocity + smoothing_velocity * m_xsphSmoothingFactor;
    }
    
    // Update velocities with smoothed values
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        m_particles[i].velocity = smoothed_velocities[i];
    }
}

void SPHFluidSimulationScene::stabilizeLowSpeedParticles()
{
    if (!m_enableLowSpeedStabilization) return;
    
    const float particleDiameter = m_particleRadius * 2.0f;
    const float targetDistance = particleDiameter * 0.98f; // Slightly tighter for stability
    const float targetDistance2 = targetDistance * targetDistance;
    
    std::vector<int> neighbors;
    neighbors.reserve(64);
    
    // Multiple iterations for better stability
    for (int iteration = 0; iteration < m_lowSpeedStabilizationIterations; ++iteration)
    {
        for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
        {
            float speed = m_particles[i].velocity.length();
            
            // Only stabilize slow particles
            if (speed > m_lowSpeedThreshold) continue;
            
            m_spatialGrid.findNeighbors(i, m_particles[i].position.x, m_particles[i].position.y, m_sphParams.smoothing_radius, neighbors);
            
            for (int j : neighbors)
            {
                if (i == j) continue;
                
                Vec2 dp = m_particles[j].position - m_particles[i].position;
                float dist2 = dp.x * dp.x + dp.y * dp.y;
                
                if (dist2 < targetDistance2 && dist2 > 1e-6f)
                {
                    float dist = std::sqrt(dist2);
                    Vec2 normal = dp * (1.0f / dist);
                    float overlap = targetDistance - dist;
                    
                    if (overlap > 0.0f)
                    {
                        // Gentle position correction
                        float correction = overlap * 0.2f; // Very gentle
                        m_particles[i].position -= normal * correction;
                        m_particles[j].position += normal * correction;
                        
                        // Strong velocity damping for low-speed particles
                        Vec2 relVel = m_particles[j].velocity - m_particles[i].velocity;
                        float relVelN = relVel.x * normal.x + relVel.y * normal.y;
                        
                        if (std::fabs(relVelN) > 1.0f) // Only damp significant relative motion
                        {
                            Vec2 damp = normal * (relVelN * m_lowSpeedDamping);
                            m_particles[i].velocity += damp;
                            m_particles[j].velocity -= damp;
                        }
                    }
                }
            }
            
            // Additional damping for very slow particles
            if (speed < m_lowSpeedThreshold * 0.5f)
            {
                m_particles[i].velocity *= m_lowSpeedDamping;
            }
        }
    }
}

void SPHFluidSimulationScene::updateBallSpring(float dt, const Vec2& target)
{
    if (!m_ballEnabled) return;
    auto* e = m_entityManager->findEntity(m_ballEntityName);
    if (!e) return;
    auto* rb = e->getComponent<FirmGuyComponent>();
    if (!rb) return;

    Vec2 pos = rb->getPosition();
    Vec2 vel = rb->getVelocity();
    Vec2 toTarget = target - pos;
    Vec2 springForce = toTarget * m_ballSpringK - vel * m_ballSpringDamping;
    // simple explicit integration on velocity only; FirmGuySystem will integrate positions
    vel += springForce * dt / std::max(0.001f, rb->getMass());
    rb->setVelocity(vel);
}

void SPHFluidSimulationScene::enforceBallOnParticles()
{
    if (!m_ballEnabled) return;
    auto* e = m_entityManager->findEntity(m_ballEntityName);
    if (!e) return;
    auto* rb = e->getComponent<FirmGuyComponent>();
    if (!rb) return;

    Vec2 c = rb->getPosition();
    float r = rb->getRadius();

    for (auto& p : m_particles)
    {
        Vec2 d = p.position - c;
        float dist2 = d.x*d.x + d.y*d.y;
        float minDist = r + m_particleRadius * 0.9f;
        if (dist2 < minDist*minDist)
        {
            float dist = std::sqrt(std::max(1e-6f, dist2));
            Vec2 n = (dist > 1e-6f) ? d * (1.0f/dist) : Vec2(1.0f, 0.0f);
            float penetration = minDist - dist;
            // push particle out
            p.position += n * penetration;
            // reflect/bounce velocity along normal
            float vn = p.velocity.dot(n);
            if (vn < 0.0f) p.velocity -= n * (1.0f + m_collisionRestitution) * vn;
        }
    }
}

float SPHFluidSimulationScene::calculateFluidDensityAt(const Vec2& worldPos)
{
    float density = 0.0f;
    float influenceRadius = m_particleRadius * 3.0f; // larger influence radius for smoother density
    
    for (const auto& p : m_particles)
    {
        Vec2 toParticle = worldPos - p.position;
        float dist = toParticle.length();
        
        if (dist < influenceRadius)
        {
            // Smooth falloff function (similar to SPH)
            float t = dist / influenceRadius;
            float influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t); // Smooth step
            density += influence;
        }
    }
    
    return density;
}

void SPHFluidSimulationScene::applyBallBuoyancy()
{
    if (!m_ballEnabled || !m_ballBuoyancyEnabled) return;
    auto* e = m_entityManager->findEntity(m_ballEntityName);
    if (!e) return;
    auto* rb = e->getComponent<FirmGuyComponent>();
    if (!rb) return;

    Vec2 ballPos = rb->getPosition();
    float ballRadius = rb->getRadius();
    
    // Calculate fluid density at ball center
    float fluidDensity = calculateFluidDensityAt(ballPos);
    
    // Always apply some buoyancy if there's any fluid nearby
    if (fluidDensity > 0.01f)
    {
        // Much stronger buoyancy force
        float buoyancyForce = m_ballBuoyancyStrength * fluidDensity;
        
        // Apply buoyancy as a direct velocity change (much stronger)
        Vec2 currentVel = rb->getVelocity();
        currentVel.y += buoyancyForce * 0.1f; // Much stronger force multiplier
        
        // Apply fluid damping
        currentVel *= m_ballBuoyancyDamping;
        
        rb->setVelocity(currentVel);
        
        // Debug output
        static int debugCounter = 0;
        if (debugCounter++ % 60 == 0) // Print every 60 frames
        {
            printf("SPH Buoyancy: density=%.3f, force=%.1f, vel.y=%.2f\n", 
                   fluidDensity, buoyancyForce, currentVel.y);
        }
    }
}

void SPHFluidSimulationScene::resolveBallParticleCollisions()
{
    // This function is now handled by enforceBallOnParticles() in the FLIP-style approach
    // Keeping this for compatibility but it's not used
}

// Old ball functions removed - now using FLIP-style ball controls

// Rotation functionality removed - boundaries are now static

// ========================= Metaball Rendering =========================

void SPHFluidSimulationScene::updateMetaballData()
{
    m_metaballPositions.clear();
    m_metaballColors.clear();
    m_metaballRadii.clear();
    
    for (const auto& p : m_particles)
    {
        m_metaballPositions.push_back(p.position);
        m_metaballRadii.push_back(m_metaballRadius);
        
        // Get color from particle's sprite
        Vec4 color(0.2f, 0.6f, 1.0f, 1.0f); // default blue
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                color = s->getTint();
            }
        }
        m_metaballColors.push_back(color);
    }
}

void SPHFluidSimulationScene::renderMetaballs(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Update metaball data from current particles
    updateMetaballData();
    
    // Render metaball field using scaled sprites with additive blending
    renderMetaballField(engine, ctx);
    
    // Draw boundary sprites
    for (int i = 0; i < 4; ++i)
    {
        if (auto* e = m_entityManager->findEntity(boundaryName(i)))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                if (s->isVisible() && s->isValid())
                    s->draw(ctx);
            }
        }
    }

    // Always draw the interactive ball sprite on top
    if (auto* ball = m_entityManager->findEntity(m_ballEntityName))
    {
        if (auto* s = ball->getComponent<SpriteComponent>())
        {
            if (s->isVisible() && s->isValid()) s->draw(ctx);
        }
    }
}

void SPHFluidSimulationScene::renderMetaballField(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Enable additive blending for field accumulation
    ctx.enableAlphaBlending();
    
    // Load metaball falloff texture if not already loaded
    static std::shared_ptr<Texture2D> metaballTexture = nullptr;
    if (!metaballTexture)
    {
        metaballTexture = Texture2D::LoadTexture2D(engine.getGraphicsDevice().getD3DDevice(), L"DX3D/Assets/Textures/MetaballFalloff.png");
    }
    
    // Render each particle using the MetaballFalloff.png texture with velocity colors
    for (const auto& p : m_particles)
    {
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                // Store original scale, color, and texture
                Vec3 originalScale = s->getScale();
                Vec4 originalColor = s->getTint();
                auto originalTexture = s->getTexture();
                
                // Set metaball texture and scale up for metaball effect
                if (metaballTexture)
                {
                    s->setTexture(metaballTexture);
                }
                float scale = m_metaballRadius / m_particleRadius;
                s->setScale(scale, scale, 1.0f);
                
                // Use the particle's velocity-based color for metaball rendering
                s->setTint(originalColor);
                
                if (s->isVisible() && s->isValid())
                {
                    s->draw(ctx);
                }
                
                // Restore original scale, color, and texture
                s->setScale(originalScale.x, originalScale.y, originalScale.z);
                s->setTint(originalColor);
                s->setTexture(originalTexture);
            }
        }
    }
}

float SPHFluidSimulationScene::calculateMetaballField(const Vec2& worldPos)
{
    float field = 0.0f;
    
    for (size_t i = 0; i < m_metaballPositions.size(); ++i)
    {
        Vec2 toParticle = worldPos - m_metaballPositions[i];
        float dist = toParticle.length();
        float radius = m_metaballRadii[i];
        
        if (dist < radius)
        {
            float t = dist / radius;
            // Smooth step function for organic metaball shapes
            float influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t);
            field += influence;
        }
    }
    
    return field;
}

Vec4 SPHFluidSimulationScene::calculateMetaballColor(const Vec2& worldPos)
{
    Vec4 color(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    for (size_t i = 0; i < m_metaballPositions.size(); ++i)
    {
        Vec2 toParticle = worldPos - m_metaballPositions[i];
        float dist = toParticle.length();
        float radius = m_metaballRadii[i];
        
        if (dist < radius)
        {
            float t = dist / radius;
            float influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t);
            
            color = color + (m_metaballColors[i] * influence);
            totalWeight += influence;
        }
    }
    
    if (totalWeight > 0.0f)
    {
        color = color * (1.0f / totalWeight);
    }
    
    return color;
}

