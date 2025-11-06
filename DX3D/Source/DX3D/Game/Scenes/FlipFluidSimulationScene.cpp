#include <DX3D/Game/Scenes/FlipFluidSimulationScene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <imgui.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstdio>

using namespace dx3d;

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }


void FlipFluidSimulationScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();

    // Preload node texture for Sprites mode
    m_nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/node.png");

    // Camera
    createCamera(engine);

    // Line renderer (for hull rendering)
    auto& lineEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false);
    
    // Set the dedicated line pipeline for optimal line rendering
    if (auto* linePipeline = engine.getLinePipeline())
    {
        m_lineRenderer->setLinePipeline(linePipeline);
        printf("Line renderer created with dedicated line pipeline\n");
    }
    else
    {
        printf("Line renderer created but no line pipeline available\n");
    }

    // Domain from origin/size - make domain smaller than grid so grid extends beyond boundaries
    m_domainWidth = (m_gridWidth - 20) * m_cellSize;   // 20 cells smaller than grid
    m_domainHeight = (m_gridHeight - 20) * m_cellSize; // 20 cells smaller than grid
    
    // Adjust grid origin to center the smaller domain within the larger grid
    m_gridOrigin.x += 10.0f * m_cellSize;  // Offset by 10 cells to center
    m_gridOrigin.y += 10.0f * m_cellSize;  // Offset by 10 cells to center

    // Allocate grid arrays
    m_u.assign((m_gridWidth + 1) * m_gridHeight, 0.0f);
    m_v.assign(m_gridWidth * (m_gridHeight + 1), 0.0f);
    m_uWeight.assign((m_gridWidth + 1) * m_gridHeight, 0.0f);
    m_vWeight.assign(m_gridWidth * (m_gridHeight + 1), 0.0f);
    m_pressure.assign(m_gridWidth * m_gridHeight, 0.0f);
    m_divergence.assign(m_gridWidth * m_gridHeight, 0.0f);
    m_solid.assign(m_gridWidth * m_gridHeight, 0);
    m_cellParticleCount.assign(m_gridWidth * m_gridHeight, 0);

    // spatial hash default cell size ~ 2x radius
    m_hashCellSize = std::max(m_particleRadius * 2.0f, 1.0f);

    // Create boundaries and particles
    createBoundaries();
    spawnParticles();
    ensureWorldAnchor();
    
    // Create interactive ball
    createBall();
    
    // Quad mesh creation removed - not needed anymore
    
    // World anchor is already created by ensureWorldAnchor() above
}

void FlipFluidSimulationScene::createCamera(GraphicsEngine& engine)
{
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(0.8f);
}

std::string FlipFluidSimulationScene::boundaryName(int i) const
{
    switch (i)
    {
    case 0: return "BoundaryLeft";
    case 1: return "BoundaryRight";
    case 2: return "BoundaryBottom";
    default: return "BoundaryTop";
    }
}




void FlipFluidSimulationScene::createBoundaries()
{
    // Create FirmGuy physics boundaries with visual sprites
    auto addBoundary = [&](const std::string& name, const Vec2& pos, float w, float h)
    {
        auto& e = m_entityManager->createEntity(name);
        
        // Add visual sprite
        auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/beam.png", w, h);
        s.setPosition(pos.x, pos.y, 0.0f);
        s.setTint(Vec4(0.3f, 0.3f, 0.3f, 0.8f));
        
        // Add FirmGuy physics component for boundaries
        auto& physics = e.addComponent<FirmGuyComponent>();
        physics.setRectangle(Vec2(w * 0.5f, h * 0.5f)); // half extents
        physics.setPosition(pos);
        physics.setStatic(true); // Static boundaries
        physics.setRestitution(0.8f); // Bouncy boundaries
        physics.setFriction(0.9f); // Some friction
    };

    float left = m_gridOrigin.x;
    float bottom = m_gridOrigin.y;
    float right = m_gridOrigin.x + m_domainWidth;
    float top = m_gridOrigin.y + m_domainHeight;
    float thickness = 20.0f;

    addBoundary(boundaryName(0), Vec2(left - thickness * 0.5f + m_boundaryLeftOffset, (bottom + top) * 0.5f), thickness, m_domainHeight + thickness * 2.0f);
    addBoundary(boundaryName(1), Vec2(right + thickness * 0.5f + m_boundaryRightOffset, (bottom + top) * 0.5f), thickness, m_domainHeight + thickness * 2.0f);
    addBoundary(boundaryName(2), Vec2((left + right) * 0.5f, bottom - thickness * 0.5f + m_boundaryBottomOffset), m_domainWidth + thickness * 2.0f, thickness);
    addBoundary(boundaryName(3), Vec2((left + right) * 0.5f, top + thickness * 0.5f + m_boundaryTopOffset), m_domainWidth + thickness * 2.0f, thickness);

    // Initialize rotated box parameters
    m_boxCenter = Vec2((left + right) * 0.5f, (bottom + top) * 0.5f);
    m_boxHalf   = Vec2(m_domainWidth * 0.5f, m_domainHeight * 0.5f);
}


void FlipFluidSimulationScene::spawnParticles()
{
    // Spawn a blob of fluid in upper-left quadrant
    m_particles.clear();
    int particlesX = 28;
    int particlesY = 18;

    Vec2 start = m_gridOrigin + Vec2(m_domainWidth * 0.15f, m_domainHeight * 0.55f);
    float spacing = m_particleRadius * 2.0f * 0.9f;

    int id = 0;
    for (int j = 0; j < particlesY; ++j)
    {
        for (int i = 0; i < particlesX; ++i)
        {
            Particle p;
            p.position = start + Vec2(i * spacing, j * spacing);
            p.velocity = Vec2(0.0f, 0.0f);
            p.entityName = "Particle_" + std::to_string(id++);

            auto& e = m_entityManager->createEntity(p.entityName);
            auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/MetaballFalloff.png", m_particleRadius * 2.0f, m_particleRadius * 2.0f);
            s.setPosition(p.position.x, p.position.y, 0.0f);
            s.setTint(Vec4(0.2f, 0.6f, 1.0f, 1.0f));
            
            m_particles.push_back(p);
        }
    }
    ensureWorldAnchor();
}

void FlipFluidSimulationScene::update(float dt)
{
    // Handle pause toggle
    auto& input = Input::getInstance();
    if (input.wasKeyJustPressed(Key::P)) m_paused = !m_paused;

    // Smooth dt for FPS display (EMA)
    const float alpha = 0.1f;
    m_smoothDt = (1.0f - alpha) * m_smoothDt + alpha * std::max(1e-6f, dt);

    // Track mouse world for interaction visuals/velocity
    Vec2 mouseWorld = getMouseWorldPosition();
    if (!m_prevMouseWorldValid) { m_prevMouseWorld = mouseWorld; m_prevMouseWorldValid = true; }

    // Emit or apply forces based on tool and mouse buttons
    bool lmb = input.isMouseDown(MouseClick::LeftMouse);
    bool rmb = input.isMouseDown(MouseClick::RightMouse);
    bool lmbJustPressed = input.wasMouseJustPressed(MouseClick::LeftMouse);
    bool lmbJustReleased = input.wasMouseJustReleased(MouseClick::LeftMouse);
    bool rmbJustPressed = input.wasMouseJustPressed(MouseClick::RightMouse);
    bool rmbJustReleased = input.wasMouseJustReleased(MouseClick::RightMouse);
    Vec2 mouseVel = (mouseWorld - m_prevMouseWorld) / std::max(1e-6f, dt);

    // Keyboard rotate box
    const float rotSpeed = 1.0f; // rad/s ~ 57 deg/s
    if (input.isKeyDown(Key::Left))  m_boxAngle += rotSpeed * dt;
    if (input.isKeyDown(Key::Right)) m_boxAngle -= rotSpeed * dt;

    // RMB spring control for ball
    if (rmbJustPressed) m_ballSpringActive = true;
    if (rmbJustReleased) m_ballSpringActive = false;
    if (m_ballSpringActive)
    {
        updateBallSpring(dt, mouseWorld);
    }

    // Camera zoom with Q/E
    if (auto* camEnt = m_entityManager->findEntity("MainCamera"))
    {
        if (auto* cam = camEnt->getComponent<Camera2D>())
        {
            float zoomDelta = 0.0f;
            const float zoomSpeed = 1.5f; // units per second
            if (input.isKeyDown(Key::Q)) zoomDelta += zoomSpeed * dt; // zoom in
            if (input.isKeyDown(Key::E)) zoomDelta -= zoomSpeed * dt; // zoom out
            if (zoomDelta != 0.0f) cam->zoom(zoomDelta);

            // WASD panning
            Vec2 moveDelta(0.0f, 0.0f);
            const float panSpeed = 600.0f; // pixels per second
            if (input.isKeyDown(Key::W)) moveDelta.y += panSpeed * dt;
            if (input.isKeyDown(Key::S)) moveDelta.y -= panSpeed * dt;
            if (input.isKeyDown(Key::A)) moveDelta.x -= panSpeed * dt;
            if (input.isKeyDown(Key::D)) moveDelta.x += panSpeed * dt;
            if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) cam->move(moveDelta);
        }
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
        else if (m_mouseTool == MouseTool::Pickup)
        {
            if (lmbJustPressed)
            {
                pickupParticles(mouseWorld);
            }
            else if (m_isPickingUp)
            {
                movePickedParticles(mouseWorld);
            }
        }
    }
    
    if (lmbJustReleased && m_mouseTool == MouseTool::Pickup)
    {
        releasePickedParticles();
    }
    if (rmb)
    {
        // Right button acts as a suction brush (negative strength)
        applyForceBrush(mouseWorld, mouseVel * -1.0f);
    }

    m_prevMouseWorld = mouseWorld;
}

void FlipFluidSimulationScene::fixedUpdate(float dt)
{
    if (m_paused) return;

    // Update FirmGuy physics (ball + any other dynamic bodies)
    if (m_entityManager)
    {
        FirmGuySystem::update(*m_entityManager, dt);
    }

    const int steps = std::max(1, m_substeps);
    const float h = dt / static_cast<float>(steps);
    for (int s = 0; s < steps; ++s)
    {
        stepFLIP(h);
    }

    // Apply ball constraint to particles after FLIP step
    enforceBallOnParticles();
    
    // Apply buoyancy forces to ball
    applyBallBuoyancy();

    updateParticleSprites();
    
    // Clustered mesh mode removed - not worth keeping
    
    // Fluid surface generation disabled to prevent freezing
    // generateFluidSurface();
    
    // Only sync boundary sprites to rotated box (not position updates)
    auto syncBoundarySprite = [&](const std::string& name, const Vec2& localCenter, float halfW, float halfH)
    {
        float c = cosf(m_boxAngle), s = sinf(m_boxAngle);
        Vec2 worldCenter = m_boxCenter + Vec2(c * localCenter.x - s * localCenter.y, s * localCenter.x + c * localCenter.y);
        if (auto* e = m_entityManager->findEntity(name))
        {
            if (auto* sc = e->getComponent<SpriteComponent>())
            {
                // Only update rotation, not position (position is controlled by offsets)
                sc->setRotation(0.0f, 0.0f, m_boxAngle);
            }
        }
    };
    syncBoundarySprite(boundaryName(0), Vec2(-m_boxHalf.x, 0.0f), 10.0f, m_boxHalf.y);
    syncBoundarySprite(boundaryName(1), Vec2( m_boxHalf.x, 0.0f), 10.0f, m_boxHalf.y);
    syncBoundarySprite(boundaryName(2), Vec2(0.0f, -m_boxHalf.y), m_boxHalf.x, 10.0f);
    syncBoundarySprite(boundaryName(3), Vec2(0.0f,  m_boxHalf.y), m_boxHalf.x, 10.0f);
}

void FlipFluidSimulationScene::render(GraphicsEngine& engine, SwapChain& swapChain)
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

    // Use default pipeline for all rendering modes
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.enableAlphaBlending();

    // Draw particles based on selected rendering mode
    // printf("Rendering mode: %d\n", (int)m_fluidRenderMode);
    
    if (m_fluidRenderMode == FluidRenderMode::Metaballs)
    {
        renderMetaballs(engine, ctx);
    }
    else // Sprites mode
    {
        // Use node.png for particle sprites in Sprites mode
        for (auto* entity : m_entityManager->getEntitiesWithComponent<SpriteComponent>())
        {
            if (auto* sprite = entity->getComponent<SpriteComponent>())
            {
                const std::string& n = entity->getName();
                if (n.rfind("Particle_", 0) == 0 && m_nodeTexture)
                {
                    sprite->setTexture(m_nodeTexture);
                }
                if (sprite->isVisible() && sprite->isValid())
                    sprite->draw(ctx);
            }
        }
    }
    
    // Anchor sprite removed - no longer needed for debugging
    
    // Clear line renderer for grid debug
    if (m_lineRenderer)
    {
        m_lineRenderer->clear();
    }
    
    if (m_showGridDebug && m_lineRenderer)
    {
        // draw grid lines
        Vec4 color = Vec4(1.0f, 1.0f, 1.0f, 0.08f);
        for (int i = 0; i <= m_gridWidth; ++i)
        {
            float x = m_gridOrigin.x + i * m_cellSize;
            m_lineRenderer->addLine(Vec2(x, m_gridOrigin.y), Vec2(x, m_gridOrigin.y + m_domainHeight), color, 1.0f);
        }
        for (int j = 0; j <= m_gridHeight; ++j)
        {
            float y = m_gridOrigin.y + j * m_cellSize;
            m_lineRenderer->addLine(Vec2(m_gridOrigin.x, y), Vec2(m_gridOrigin.x + m_domainWidth, y), color, 1.0f);
        }

        // draw rotated box outline
        float c = cosf(m_boxAngle), s = sinf(m_boxAngle);
        Vec2 hx = Vec2(m_boxHalf.x, 0.0f);
        Vec2 hy = Vec2(0.0f, m_boxHalf.y);
        auto R = [&](const Vec2& v){ return Vec2(c * v.x - s * v.y, s * v.x + c * v.y); };
        Vec2 corners[4] = {
            m_boxCenter + R(Vec2(-m_boxHalf.x, -m_boxHalf.y)),
            m_boxCenter + R(Vec2( m_boxHalf.x, -m_boxHalf.y)),
            m_boxCenter + R(Vec2( m_boxHalf.x,  m_boxHalf.y)),
            m_boxCenter + R(Vec2(-m_boxHalf.x,  m_boxHalf.y))
        };
        Vec4 boxCol = Vec4(0.2f, 1.0f, 0.2f, 0.8f);
        for (int i = 0; i < 4; ++i)
        {
            m_lineRenderer->addLine(corners[i], corners[(i+1)&3], boxCol, 2.0f);
        }
        m_lineRenderer->updateBuffer();
        m_lineRenderer->draw(ctx);
    }
    
    // Clustered mesh rendering removed - not worth keeping
    
    // Fluid surface rendering disabled to prevent freezing
    // renderFluidSurface(engine, ctx);

    // Update FirmGuy boundary positions and rotations to match box angle
    auto updateBoundaryPhysics = [&](const std::string& name, const Vec2& localPos)
    {
        if (auto* e = m_entityManager->findEntity(name))
        {
            if (auto* physics = e->getComponent<FirmGuyComponent>())
            {
                // Calculate rotated position around world center
                float c = cosf(m_boxAngle), s_rot = sinf(m_boxAngle);
                Vec2 worldPos = m_boxCenter + Vec2(c * localPos.x - s_rot * localPos.y, s_rot * localPos.x + c * localPos.y);
                physics->setPosition(worldPos);
                physics->setAngle(m_boxAngle);
                
                // Sync sprite position and rotation with physics
                if (auto* sprite = e->getComponent<SpriteComponent>())
                {
                    sprite->setPosition(worldPos.x, worldPos.y, 0.0f);
                    sprite->setRotation(0.0f, 0.0f, m_boxAngle);
                }
            }
        }
    };
    
    // Update each boundary with proper rotation around world center
    updateBoundaryPhysics(boundaryName(0), Vec2(-m_boxHalf.x + m_boundaryLeftOffset, 0.0f));
    updateBoundaryPhysics(boundaryName(1), Vec2(m_boxHalf.x + m_boundaryRightOffset, 0.0f));
    updateBoundaryPhysics(boundaryName(2), Vec2(0.0f, -m_boxHalf.y + m_boundaryBottomOffset));
    updateBoundaryPhysics(boundaryName(3), Vec2(0.0f, m_boxHalf.y + m_boundaryTopOffset));
}

void FlipFluidSimulationScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::SetNextWindowSize(ImVec2(420, 340), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("FLIP Fluid (2D)"))
    {
        float fps = (m_smoothDt > 0.0f) ? (1.0f / m_smoothDt) : 0.0f;
        ImGui::Text("FPS: %.1f (dt=%.3f ms)", fps, m_smoothDt * 1000.0f);
        ImGui::Checkbox("Paused (P)", &m_paused);
        ImGui::Text("Particles: %d", (int)m_particles.size());
        ImGui::Checkbox("Show Grid", &m_showGridDebug);
        // Grid debug display removed - not needed for simplified rendering modes
        ImGui::SliderFloat("Gravity", &m_gravity, -2000.0f, 0.0f, "%.0f");
        ImGui::SliderFloat("FLIP Blending", &m_flipBlending, 0.0f, 1.0f, "%.2f");
        ImGui::SliderInt("Jacobi Iterations", &m_jacobiIterations, 5, 200);
        ImGui::SliderInt("Substeps", &m_substeps, 1, 8);
        
        ImGui::Separator();
        ImGui::Text("Viscosity");
        ImGui::SliderFloat("Viscosity", &m_viscosity, 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Velocity Damping", &m_velocityDamping, 0.9f, 1.0f, "%.3f");
        
        ImGui::Separator();
        ImGui::Text("Rendering");
        
        // Fluid rendering mode selection
        int renderMode = static_cast<int>(m_fluidRenderMode);
        if (ImGui::RadioButton("Sprites", renderMode == 0)) 
        {
            m_fluidRenderMode = FluidRenderMode::Sprites;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Metaballs", renderMode == 1)) 
        {
            m_fluidRenderMode = FluidRenderMode::Metaballs;
        }
        
        // Update metaball rendering flag for compatibility
        m_useMetaballRendering = (m_fluidRenderMode == FluidRenderMode::Metaballs);
        
        if (m_fluidRenderMode == FluidRenderMode::Metaballs)
        {
            ImGui::Text("Metaball Rendering (john-wigg.dev technique)");
            ImGui::SliderFloat("Particle Scale", &m_metaballRadius, 2.0f, 20.0f, "%.1f");
            ImGui::Text("Scale: %.1fx", m_metaballRadius / m_particleRadius);
            ImGui::SliderFloat("Threshold", &m_metaballThreshold, 0.1f, 2.0f, "%.2f");
            ImGui::SliderFloat("Smoothing", &m_metaballSmoothing, 0.01f, 0.5f, "%.3f");
            ImGui::Text("Note: Uses MetaballFalloff.png with additive blending");
            ImGui::Text("Tip: MetaballFalloff.png should be radial gradient (white center, black edges)");
        }
        
        // Fluid surface controls removed to prevent freezing
        // ImGui::Separator();
        // ImGui::Text("Fluid Surface (Marching Squares) - DISABLED");

        ImGui::Separator();
        int tool = (m_mouseTool == MouseTool::Add) ? 0 : (m_mouseTool == MouseTool::Force) ? 1 : 2;
        if (ImGui::RadioButton("Add Particles (LMB)", tool == 0)) m_mouseTool = MouseTool::Add;
        ImGui::SameLine();
        if (ImGui::RadioButton("Force Brush (LMB)", tool == 1)) m_mouseTool = MouseTool::Force;
        ImGui::SameLine();
        if (ImGui::RadioButton("Pickup Particles (LMB)", tool == 2)) m_mouseTool = MouseTool::Pickup;
        ImGui::SliderFloat("Brush Radius", &m_brushRadius, 5.0f, 120.0f, "%.1f");
        ImGui::SliderFloat("Force Strength", &m_forceStrength, 100.0f, 6000.0f, "%.0f");
        ImGui::SliderFloat("Emit Rate (pps)", &m_emitRate, 0.0f, 2000.0f, "%.0f");
        ImGui::SliderFloat("Emit Jitter", &m_emitJitter, 0.0f, 8.0f, "%.1f");

        ImGui::Separator();
        ImGui::Text("Box Rotation");
        ImGui::SliderAngle("Angle", &m_boxAngle, -90.0f, 90.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset Angle")) m_boxAngle = 0.0f;
        
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
        
        // Update boundary positions if offsets or rotation changed
        static float prevLeftOffset = m_boundaryLeftOffset;
        static float prevRightOffset = m_boundaryRightOffset;
        static float prevBottomOffset = m_boundaryBottomOffset;
        static float prevTopOffset = m_boundaryTopOffset;
        static float prevBoxAngle = m_boxAngle;
        
        if (m_boundaryLeftOffset != prevLeftOffset || m_boundaryRightOffset != prevRightOffset || 
            m_boundaryBottomOffset != prevBottomOffset || m_boundaryTopOffset != prevTopOffset ||
            m_boxAngle != prevBoxAngle)
        {
            // Boundary positions are updated in render() function with proper world center rotation
            prevLeftOffset = m_boundaryLeftOffset;
            prevRightOffset = m_boundaryRightOffset;
            prevBottomOffset = m_boundaryBottomOffset;
            prevTopOffset = m_boundaryTopOffset;
            prevBoxAngle = m_boxAngle;
        }

        ImGui::Separator();
        ImGui::Checkbox("Particle Collisions", &m_enableParticleCollisions);
        ImGui::SliderInt("Collision Iterations", &m_collisionIterations, 1, 6);
        ImGui::SliderFloat("Restitution", &m_collisionRestitution, 0.0f, 0.5f, "%.2f");
        ImGui::Checkbox("Use Spatial Hash", &m_useSpatialHash);
        ImGui::SliderFloat("Hash Cell Size", &m_hashCellSize, m_particleRadius*1.5f, m_particleRadius*4.0f, "%.1f");

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
                // Debug: print visibility state
                static bool lastEnabled = !m_ballEnabled;
                if (m_ballEnabled != lastEnabled)
                {
                    printf("Ball visibility changed to: %s\n", m_ballEnabled ? "VISIBLE" : "HIDDEN");
                    lastEnabled = m_ballEnabled;
                }
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
            printf("Ball entity not found, creating it...\n");
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
        ImGui::Text("Coloring (Velocity Gradient)");
        ImGui::SliderFloat("Speed Min", &m_colorSpeedMin, 0.0f, 400.0f, "%.0f");
        ImGui::SliderFloat("Speed Max", &m_colorSpeedMax, 50.0f, 1200.0f, "%.0f");
        ImGui::Checkbox("Debug Color (Blue→Green→Red)", &m_debugColor);

        if (ImGui::Button("Reset Particles", ImVec2(-FLT_MIN, 0)))
        {
            // Remove existing particle entities
            const auto& all = m_entityManager->getEntities();
            std::vector<std::string> toRemove;
            for (const auto& up : all)
            {
                const std::string& n = up->getName();
                if (n.rfind("Particle_", 0) == 0) toRemove.push_back(n);
            }
            for (const auto& n : toRemove) m_entityManager->removeEntity(n);
            spawnParticles();
        }
        ImGui::Separator();
        int maxThreads = (int)std::max(1u, std::thread::hardware_concurrency());
        ImGui::SliderInt("Threads", &m_threadCount, 1, maxThreads);
        ImGui::SameLine();
        ImGui::Text("(1 = single-thread)");
    }
    ImGui::End();
}

void FlipFluidSimulationScene::updateParticleSprites()
{
    for (const auto& p : m_particles)
    {
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                s->setPosition(p.position.x, p.position.y, 0.0f);
                
                // Reset scale and alpha when not using metaball rendering
                if (!m_useMetaballRendering)
                {
                    s->setScale(1.0f, 1.0f, 1.0f);
                    Vec4 tint = s->getTint();
                    tint.w = 1.0f; // Full opacity
                    s->setTint(tint);
                }
            }
        }
    }
}

// ========================= FLIP Core =========================

void FlipFluidSimulationScene::stepFLIP(float dt)
{
    clearGrid();
    particlesToGrid(dt);
    buildPressureSystem(dt);
    solvePressure();
    applyPressureGradient(dt);
    applyViscosity(dt);
    gridToParticles(dt);
    advectParticles(dt);
    enforceBoundaryOnParticles();
    if (m_enableParticleCollisions)
    {
        if (m_useSpatialHash)
        {
            buildSpatialHash();
            resolveParticleCollisionsHashed();
        }
        else
        {
            resolveParticleCollisions();
        }
    }
    updateParticleColors();
}

void FlipFluidSimulationScene::clearGrid()
{
    std::fill(m_u.begin(), m_u.end(), 0.0f);
    std::fill(m_v.begin(), m_v.end(), 0.0f);
    std::fill(m_uWeight.begin(), m_uWeight.end(), 0.0f);
    std::fill(m_vWeight.begin(), m_vWeight.end(), 0.0f);
    std::fill(m_pressure.begin(), m_pressure.end(), 0.0f);
    std::fill(m_divergence.begin(), m_divergence.end(), 0.0f);

    // mark outermost cells as solid
    std::fill(m_solid.begin(), m_solid.end(), 0);
    for (int j = 0; j < m_gridHeight; ++j)
    {
        for (int i = 0; i < m_gridWidth; ++i)
        {
            bool solid = (i == 0) || (i == m_gridWidth - 1) || (j == 0) || (j == m_gridHeight - 1);
            m_solid[idxP(i, j)] = solid ? 1 : 0;
        }
    }
}

Vec2 FlipFluidSimulationScene::worldToGrid(const Vec2& p) const
{
    Vec2 d = p - m_gridOrigin;
    return Vec2(d.x / m_cellSize, d.y / m_cellSize);
}

Vec2 FlipFluidSimulationScene::gridToWorld(const Vec2& ij) const
{
    return m_gridOrigin + Vec2(ij.x * m_cellSize, ij.y * m_cellSize);
}

void FlipFluidSimulationScene::particlesToGrid(float dt)
{
    // Scatter particle velocities to face-centered grid with bilinear weights
    for (const auto& p : m_particles)
    {
        // U (faces between i and i+1 at j)
        {
            float gx = (p.position.x - m_gridOrigin.x) / m_cellSize; // i in cell space
            float gy = (p.position.y - m_gridOrigin.y) / m_cellSize - 0.5f; // v-centered shift
            int i0 = (int)std::floor(gx);
            int j0 = (int)std::floor(gy);
            for (int dj = 0; dj <= 1; ++dj)
            {
                for (int di = 0; di <= 1; ++di)
                {
                    int i = i0 + di;
                    int j = j0 + dj;
                    if (i < 0 || i > m_gridWidth || j < 0 || j >= m_gridHeight) continue;
                    float wx = 1.0f - std::abs((gx - i));
                    float wy = 1.0f - std::abs((gy - j));
                    float w = clampf(wx, 0.0f, 1.0f) * clampf(wy, 0.0f, 1.0f);
                    int idx = j * (m_gridWidth + 1) + i;
                    m_u[idx] += p.velocity.x * w;
                    m_uWeight[idx] += w;
                }
            }
        }

        // V (faces between j and j+1 at i)
        {
            float gx = (p.position.x - m_gridOrigin.x) / m_cellSize - 0.5f;
            float gy = (p.position.y - m_gridOrigin.y) / m_cellSize;
            int i0 = (int)std::floor(gx);
            int j0 = (int)std::floor(gy);
            for (int dj = 0; dj <= 1; ++dj)
            {
                for (int di = 0; di <= 1; ++di)
                {
                    int i = i0 + di;
                    int j = j0 + dj;
                    if (i < 0 || i >= m_gridWidth || j < 0 || j > m_gridHeight) continue;
                    float wx = 1.0f - std::abs((gx - i));
                    float wy = 1.0f - std::abs((gy - j));
                    float w = clampf(wx, 0.0f, 1.0f) * clampf(wy, 0.0f, 1.0f);
                    int idx = j * m_gridWidth + i;
                    m_v[idx] += p.velocity.y * w;
                    m_vWeight[idx] += w;
                }
            }
        }
    }

    // Normalize
    for (size_t k = 0; k < m_u.size(); ++k)
        if (m_uWeight[k] > 0.0f) m_u[k] /= m_uWeight[k];
    for (size_t k = 0; k < m_v.size(); ++k)
        if (m_vWeight[k] > 0.0f) m_v[k] /= m_vWeight[k];

    // Add gravity to V faces
    for (int j = 0; j <= m_gridHeight; ++j)
    {
        for (int i = 0; i < m_gridWidth; ++i)
        {
            int idx = j * m_gridWidth + i;
            m_v[idx] += m_gravity * dt;
        }
    }
}

void FlipFluidSimulationScene::buildPressureSystem(float dt)
{
    // Compute divergence at cell centers from face velocities
    parallelFor(0, m_gridHeight, 1, [&](int rowStart, int rowEnd)
    {
        for (int j = rowStart; j < rowEnd; ++j)
        {
            for (int i = 0; i < m_gridWidth; ++i)
            {
                if (m_solid[idxP(i, j)]) { m_divergence[idxP(i, j)] = 0.0f; continue; }

                float uR = m_u[j * (m_gridWidth + 1) + (i + 1)];
                float uL = m_u[j * (m_gridWidth + 1) + i];
                float vT = m_v[(j + 1) * m_gridWidth + i];
                float vB = m_v[j * m_gridWidth + i];

                float div = (uR - uL + vT - vB) / m_cellSize;
                m_divergence[idxP(i, j)] = div;
            }
        }
    });
}

void FlipFluidSimulationScene::solvePressure()
{
    // Jacobi iterations on Poisson: Laplace(p) = divergence
    std::vector<float> pNew(m_pressure.size(), 0.0f);
    for (int it = 0; it < m_jacobiIterations; ++it)
    {
        parallelFor(0, m_gridHeight, 1, [&](int rowStart, int rowEnd)
        {
            for (int j = rowStart; j < rowEnd; ++j)
            {
                for (int i = 0; i < m_gridWidth; ++i)
                {
                    int id = idxP(i, j);
                    if (m_solid[id]) { pNew[id] = 0.0f; continue; }

                    float sum = 0.0f;
                    int count = 0;
                    if (i > 0 && !m_solid[idxP(i - 1, j)]) { sum += m_pressure[idxP(i - 1, j)]; ++count; }
                    if (i < m_gridWidth - 1 && !m_solid[idxP(i + 1, j)]) { sum += m_pressure[idxP(i + 1, j)]; ++count; }
                    if (j > 0 && !m_solid[idxP(i, j - 1)]) { sum += m_pressure[idxP(i, j - 1)]; ++count; }
                    if (j < m_gridHeight - 1 && !m_solid[idxP(i, j + 1)]) { sum += m_pressure[idxP(i, j + 1)]; ++count; }

                    if (count > 0)
                        pNew[id] = (sum - m_divergence[id] * m_cellSize * m_cellSize) / count;
                    else
                        pNew[id] = 0.0f;
                }
            }
        });
        m_pressure.swap(pNew);
    }
}

void FlipFluidSimulationScene::applyPressureGradient(float dt)
{
    // Subtract pressure gradient from face velocities to make flow divergence-free
    parallelFor(0, m_gridHeight, 1, [&](int rowStart, int rowEnd)
    {
        for (int j = rowStart; j < rowEnd; ++j)
        {
            for (int i = 1; i < m_gridWidth; ++i)
            {
                float pR = m_pressure[idxP(i, j)];
                float pL = m_pressure[idxP(i - 1, j)];
                int iu = j * (m_gridWidth + 1) + i;
                m_u[iu] -= (pR - pL) / m_cellSize;
            }
        }
    });
    parallelFor(1, m_gridHeight, 1, [&](int rowStart, int rowEnd)
    {
        for (int j = rowStart; j < rowEnd; ++j)
        {
            for (int i = 0; i < m_gridWidth; ++i)
            {
                float pT = m_pressure[idxP(i, j)];
                float pB = m_pressure[idxP(i, j - 1)];
                int iv = j * m_gridWidth + i;
                m_v[iv] -= (pT - pB) / m_cellSize;
            }
        }
    });
}

float FlipFluidSimulationScene::sampleU(float x, float y) const
{
    // bilinear sample face-centered U
    float gx = (x - m_gridOrigin.x) / m_cellSize;
    float gy = (y - m_gridOrigin.y) / m_cellSize - 0.5f;
    int i0 = (int)std::floor(gx);
    int j0 = (int)std::floor(gy);
    float tx = gx - i0;
    float ty = gy - j0;

    auto at = [&](int i, int j) -> float
    {
        if (i < 0 || i > m_gridWidth || j < 0 || j >= m_gridHeight) return 0.0f;
        return m_u[j * (m_gridWidth + 1) + i];
    };

    float v00 = at(i0, j0);
    float v10 = at(i0 + 1, j0);
    float v01 = at(i0, j0 + 1);
    float v11 = at(i0 + 1, j0 + 1);
    float vx0 = lerp(v00, v10, tx);
    float vx1 = lerp(v01, v11, tx);
    return lerp(vx0, vx1, ty);
}

float FlipFluidSimulationScene::sampleV(float x, float y) const
{
    float gx = (x - m_gridOrigin.x) / m_cellSize - 0.5f;
    float gy = (y - m_gridOrigin.y) / m_cellSize;
    int i0 = (int)std::floor(gx);
    int j0 = (int)std::floor(gy);
    float tx = gx - i0;
    float ty = gy - j0;

    auto at = [&](int i, int j) -> float
    {
        if (i < 0 || i >= m_gridWidth || j < 0 || j > m_gridHeight) return 0.0f;
        return m_v[j * m_gridWidth + i];
    };

    float v00 = at(i0, j0);
    float v10 = at(i0 + 1, j0);
    float v01 = at(i0, j0 + 1);
    float v11 = at(i0 + 1, j0 + 1);
    float vx0 = lerp(v00, v10, tx);
    float vx1 = lerp(v01, v11, tx);
    return lerp(vx0, vx1, ty);
}

void FlipFluidSimulationScene::gridToParticles(float dt)
{
    // FLIP/PIC blend: newVel = FLIP*w + PIC*(1-w)
    for (auto& p : m_particles)
    {
        Vec2 pic(sampleU(p.position.x, p.position.y), sampleV(p.position.x, p.position.y));

        // For a simple FLIP update, compute delta grid velocity at particle position
        // Here we approximate by using the current PIC value as new grid vel and blending toward it
        Vec2 newVel = pic;
        p.velocity = p.velocity * m_flipBlending + newVel * (1.0f - m_flipBlending);
    }
}

void FlipFluidSimulationScene::advectParticles(float dt)
{
    for (auto& p : m_particles)
    {
        // Simple forward Euler. For stability, substeps are already used.
        p.position += p.velocity * dt;
    }
}

void FlipFluidSimulationScene::enforceBoundaryOnParticles()
{
    // Use domain boundaries for collision detection (not visual boundary positions)
    // This ensures particles are contained within the actual simulation domain
    float c = cosf(m_boxAngle), s = sinf(m_boxAngle);
    for (auto& p : m_particles)
    {
        // Transform to box-local space
        Vec2 d = p.position - m_boxCenter;
        Vec2 local = Vec2(c * d.x + s * d.y, -s * d.x + c * d.y);

        bool collided = false;
        Vec2 normal(0.0f, 0.0f);

        // Use domain boundaries (m_boxHalf) for collision detection
        if (local.x < -m_boxHalf.x + m_particleRadius)
        {
            local.x = -m_boxHalf.x + m_particleRadius;
            normal = Vec2(-1.0f, 0.0f); collided = true;
        }
        else if (local.x > m_boxHalf.x - m_particleRadius)
        {
            local.x = m_boxHalf.x - m_particleRadius;
            normal = Vec2(1.0f, 0.0f); collided = true;
        }

        if (local.y < -m_boxHalf.y + m_particleRadius)
        {
            local.y = -m_boxHalf.y + m_particleRadius;
            normal = Vec2(0.0f, -1.0f); collided = true;
        }
        else if (local.y > m_boxHalf.y - m_particleRadius)
        {
            local.y = m_boxHalf.y - m_particleRadius;
            normal = Vec2(0.0f, 1.0f); collided = true;
        }

        if (collided)
        {
            // Back to world space
            Vec2 world = Vec2(c * local.x - s * local.y, s * local.x + c * local.y) + m_boxCenter;
            p.position = world;

            // Rotate normal to world space
            Vec2 nWorld = Vec2(c * normal.x - s * normal.y, s * normal.x + c * normal.y);
            float vN = p.velocity.dot(nWorld);
            if (vN < 0.0f)
            {
                p.velocity = p.velocity - nWorld * vN * 1.5f; // reflect + bounce
                p.velocity *= 0.9f; // light damping
            }
        }
    }
}

void FlipFluidSimulationScene::resolveParticleCollisions()
{
    // Simple iterative pairwise collision resolution (naive O(N^2)).
    // For many particles, replace with spatial hashing/GRID later.
    const float targetDist = m_particleRadius * 2.0f * 0.95f; // slightly less to allow packing
    const float targetDist2 = targetDist * targetDist;

    for (int it = 0; it < m_collisionIterations; ++it)
    {
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            for (size_t j = i + 1; j < m_particles.size(); ++j)
            {
                Vec2 dp = m_particles[j].position - m_particles[i].position;
                float dist2 = dp.x * dp.x + dp.y * dp.y;
                if (dist2 < targetDist2)
                {
                    float dist = std::sqrt(std::max(1e-5f, dist2));
                    Vec2 n = (dist > 1e-5f) ? (dp * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                    float overlap = targetDist - dist;

                    // Separate equally
                    m_particles[i].position -= n * (overlap * 0.5f);
                    m_particles[j].position += n * (overlap * 0.5f);

                    // Velocity response (simple restitution along normal)
                    Vec2 relVel = m_particles[j].velocity - m_particles[i].velocity;
                    float relN = relVel.x * n.x + relVel.y * n.y;
                    if (relN < 0.0f)
                    {
                        float impulse = -(1.0f + m_collisionRestitution) * relN * 0.5f;
                        m_particles[i].velocity -= n * impulse;
                        m_particles[j].velocity += n * impulse;
                    }
                }
            }
        }
    }
}

// (quad tree variant removed by request)

void FlipFluidSimulationScene::buildSpatialHash()
{
    m_hash.clear();
    float inv = 1.0f / std::max(1.0f, m_hashCellSize);
    for (int i = 0; i < (int)m_particles.size(); ++i)
    {
        const Vec2 p = m_particles[i].position;
        int ix = (int)std::floor((p.x - m_gridOrigin.x) * inv);
        int iy = (int)std::floor((p.y - m_gridOrigin.y) * inv);
        m_hash[hashKey(ix, iy)].push_back(i);
    }
}

void FlipFluidSimulationScene::resolveParticleCollisionsHashed()
{
    const float targetDist = m_particleRadius * 2.0f * 0.95f;
    const float targetDist2 = targetDist * targetDist;
    float inv = 1.0f / std::max(1.0f, m_hashCellSize);

    for (int it = 0; it < m_collisionIterations; ++it)
    {
        for (int i = 0; i < (int)m_particles.size(); ++i)
        {
            const Vec2 pi = m_particles[i].position;
            int ix = (int)std::floor((pi.x - m_gridOrigin.x) * inv);
            int iy = (int)std::floor((pi.y - m_gridOrigin.y) * inv);

            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    auto itBucket = m_hash.find(hashKey(ix + dx, iy + dy));
                    if (itBucket == m_hash.end()) continue;
                    const auto& bucket = itBucket->second;
                    for (int j : bucket)
                    {
                        if (j <= i) continue;
                        Vec2 dp = m_particles[j].position - m_particles[i].position;
                        float dist2 = dp.x * dp.x + dp.y * dp.y;
                        if (dist2 < targetDist2)
                        {
                            float dist = std::sqrt(std::max(1e-5f, dist2));
                            Vec2 n = (dist > 1e-5f) ? (dp * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                            float overlap = targetDist - dist;
                            m_particles[i].position -= n * (overlap * 0.5f);
                            m_particles[j].position += n * (overlap * 0.5f);

                            Vec2 relVel = m_particles[j].velocity - m_particles[i].velocity;
                            float relN = relVel.x * n.x + relVel.y * n.y;
                            if (relN < 0.0f)
                            {
                                float impulse = -(1.0f + m_collisionRestitution) * relN * 0.5f;
                                m_particles[i].velocity -= n * impulse;
                                m_particles[j].velocity += n * impulse;
                            }
                        }
                    }
                }
            }
        }

        // Rebuild hash after positions changed for next iteration
        if (it + 1 < m_collisionIterations)
            buildSpatialHash();
    }
}

void FlipFluidSimulationScene::updateParticleColors()
{
    // Velocity-based coloring: map speed in [speedMin, speedMax] to blue->white gradient
    float sMin = std::min(m_colorSpeedMin, m_colorSpeedMax - 1.0f);
    float sMax = std::max(m_colorSpeedMax, sMin + 1.0f);
    float invRange = 1.0f / (sMax - sMin);

    for (const auto& p : m_particles)
    {
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                float speed = p.velocity.length();
                float t = (speed - sMin) * invRange;
                t = std::max(0.0f, std::min(1.0f, t));

                Vec4 tint;
                if (m_debugColor)
                {
                    // Debug gradient: Blue (slow) -> Green (medium) -> Red (fast)
                    Vec4 blue  = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
                    Vec4 green = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
                    Vec4 red   = Vec4(1.0f, 0.0f, 0.0f, 1.0f);

                    if (t < 0.5f)
                    {
                        float k = t / 0.5f; // 0..1 from blue to green
                        tint = Vec4(
                            blue.x + (green.x - blue.x) * k,
                            blue.y + (green.y - blue.y) * k,
                            blue.z + (green.z - blue.z) * k,
                            1.0f);
                    }
                    else
                    {
                        float k = (t - 0.5f) / 0.5f; // 0..1 from green to red
                        tint = Vec4(
                            green.x + (red.x - green.x) * k,
                            green.y + (red.y - green.y) * k,
                            green.z + (red.z - green.z) * k,
                            1.0f);
                    }
                }
                else
                {
                    // Original gradient: deep blue (slow) -> cyan -> white (fast)
                    Vec4 slow = Vec4(0.1f, 0.35f, 0.9f, 1.0f);
                    Vec4 fast = Vec4(0.95f, 0.95f, 0.95f, 1.0f);
                    Vec4 mid  = Vec4(0.0f, 1.0f, 1.0f, 1.0f);

                    if (t < 0.5f)
                    {
                        float k = t / 0.5f;
                        tint = Vec4(
                            slow.x + (mid.x - slow.x) * k,
                            slow.y + (mid.y - slow.y) * k,
                            slow.z + (mid.z - slow.z) * k,
                            1.0f);
                    }
                    else
                    {
                        float k = (t - 0.5f) / 0.5f;
                        tint = Vec4(
                            mid.x + (fast.x - mid.x) * k,
                            mid.y + (fast.y - mid.y) * k,
                            mid.z + (fast.z - mid.z) * k,
                            1.0f);
                    }
                }
                s->setTint(tint);
            }
        }
    }
}

Vec2 FlipFluidSimulationScene::getMouseWorldPosition() const
{
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return Vec2(0.0f, 0.0f);
    auto* cam = cameraEntity->getComponent<Camera2D>();
    if (!cam) return Vec2(0.0f, 0.0f);
    Vec2 mouseClient = Input::getInstance().getMousePositionClient();
    return cam->screenToWorld(Vec2(mouseClient.x, mouseClient.y));
}

void FlipFluidSimulationScene::addParticlesAt(const Vec2& worldPos, int count, float jitter)
{
    int startIdx = (int)m_particles.size();
    for (int i = 0; i < count; ++i)
    {
        Particle p;
        float rx = ((rand() % 2000) / 1000.0f - 1.0f) * jitter;
        float ry = ((rand() % 2000) / 1000.0f - 1.0f) * jitter;
        p.position = worldPos + Vec2(rx, ry);
        p.velocity = Vec2(0.0f, 0.0f);
        p.entityName = "Particle_" + std::to_string(startIdx + i);

        auto& e = m_entityManager->createEntity(p.entityName);
        auto& s = e.addComponent<SpriteComponent>(*m_graphicsDevice, L"DX3D/Assets/Textures/MetaballFalloff.png", m_particleRadius * 2.0f, m_particleRadius * 2.0f);
        s.setPosition(p.position.x, p.position.y, 0.0f);
        s.setTint(Vec4(0.2f, 0.6f, 1.0f, 1.0f));
        
        m_particles.push_back(p);
    }
    ensureWorldAnchor();
}

void FlipFluidSimulationScene::applyForceBrush(const Vec2& worldPos, const Vec2& worldVel)
{
    float r2 = m_brushRadius * m_brushRadius;
    for (auto& p : m_particles)
    {
        Vec2 d = p.position - worldPos;
        float dist2 = d.x * d.x + d.y * d.y;
        if (dist2 <= r2)
        {
            float dist = std::sqrt(std::max(1e-4f, dist2));
            float falloff = 1.0f - dist / m_brushRadius; // linear falloff
            Vec2 dir = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(0.0f, 0.0f);
            // Push along mouse motion plus radial push away from center slightly
            Vec2 impulse = worldVel * 0.5f + dir * 100.0f;
            p.velocity += impulse * (m_forceStrength * falloff) * (1.0f / 6000.0f);
        }
    }
}

void FlipFluidSimulationScene::ensureWorldAnchor()
{
    // Create or refresh a tiny invisible sprite at the world origin to anchor transforms
    if (auto* old = m_entityManager->findEntity("WorldOriginAnchor"))
    {
        m_entityManager->removeEntity("WorldOriginAnchor");
    }
    auto& anchor = m_entityManager->createEntity("WorldOriginAnchor");
    auto& sprite = anchor.addComponent<SpriteComponent>(
        *m_graphicsDevice,
        L"DX3D/Assets/Textures/node.png",
        1.0f, 1.0f);
    sprite.setPosition(0.0f, 0.0f, 0.0f);
    sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f));
}

void FlipFluidSimulationScene::pickupParticles(const Vec2& worldPos)
{
    m_pickedParticles.clear();
    float r2 = m_brushRadius * m_brushRadius;
    
    // Find all particles within brush radius
    for (int i = 0; i < (int)m_particles.size(); ++i)
    {
        Vec2 d = m_particles[i].position - worldPos;
        float dist2 = d.x * d.x + d.y * d.y;
        if (dist2 <= r2)
        {
            m_pickedParticles.push_back(i);
        }
    }
    
    if (!m_pickedParticles.empty())
    {
        m_isPickingUp = true;
        // Calculate average offset from mouse to particle center
        Vec2 avgPos(0.0f, 0.0f);
        for (int idx : m_pickedParticles)
        {
            avgPos += m_particles[idx].position;
        }
        avgPos *= (1.0f / m_pickedParticles.size());
        m_pickupOffset = avgPos - worldPos;
    }
}

void FlipFluidSimulationScene::movePickedParticles(const Vec2& worldPos)
{
    if (!m_isPickingUp || m_pickedParticles.empty()) return;
    
    Vec2 targetCenter = worldPos + m_pickupOffset;
    Vec2 currentCenter(0.0f, 0.0f);
    
    // Calculate current center of picked particles
    for (int idx : m_pickedParticles)
    {
        currentCenter += m_particles[idx].position;
    }
    currentCenter *= (1.0f / m_pickedParticles.size());
    
    // Move all particles by the same offset
    Vec2 offset = targetCenter - currentCenter;
    for (int idx : m_pickedParticles)
    {
        m_particles[idx].position += offset;
        // Dampen velocity to prevent particles from flying away
        m_particles[idx].velocity *= 0.8f;
    }
}

void FlipFluidSimulationScene::releasePickedParticles()
{
    m_pickedParticles.clear();
    m_isPickingUp = false;
    m_pickupOffset = Vec2(0.0f, 0.0f);
}

void FlipFluidSimulationScene::applyViscosity(float dt)
{
    if (m_viscosity <= 0.0f && m_velocityDamping >= 1.0f) return;
    
    // Apply viscosity by smoothing the velocity field
    if (m_viscosity > 0.0f)
    {
        std::vector<float> uNew = m_u;
        std::vector<float> vNew = m_v;
        
        // Viscous diffusion on U faces
        for (int j = 0; j < m_gridHeight; ++j)
        {
            for (int i = 1; i < m_gridWidth; ++i)
            {
                int idx = j * (m_gridWidth + 1) + i;
                if (m_uWeight[idx] > 0.0f)
                {
                    float laplacian = 0.0f;
                    int count = 0;
                    
                    // Check neighbors
                    if (i > 0 && m_uWeight[idx - 1] > 0.0f) { laplacian += m_u[idx - 1]; ++count; }
                    if (i < m_gridWidth && m_uWeight[idx + 1] > 0.0f) { laplacian += m_u[idx + 1]; ++count; }
                    if (j > 0 && m_uWeight[idx - (m_gridWidth + 1)] > 0.0f) { laplacian += m_u[idx - (m_gridWidth + 1)]; ++count; }
                    if (j < m_gridHeight - 1 && m_uWeight[idx + (m_gridWidth + 1)] > 0.0f) { laplacian += m_u[idx + (m_gridWidth + 1)]; ++count; }
                    
                    if (count > 0)
                    {
                        laplacian /= count;
                        uNew[idx] = m_u[idx] + m_viscosity * dt * (laplacian - m_u[idx]);
                    }
                }
            }
        }
        
        // Viscous diffusion on V faces
        for (int j = 1; j < m_gridHeight; ++j)
        {
            for (int i = 0; i < m_gridWidth; ++i)
            {
                int idx = j * m_gridWidth + i;
                if (m_vWeight[idx] > 0.0f)
                {
                    float laplacian = 0.0f;
                    int count = 0;
                    
                    // Check neighbors
                    if (i > 0 && m_vWeight[idx - 1] > 0.0f) { laplacian += m_v[idx - 1]; ++count; }
                    if (i < m_gridWidth - 1 && m_vWeight[idx + 1] > 0.0f) { laplacian += m_v[idx + 1]; ++count; }
                    if (j > 0 && m_vWeight[idx - m_gridWidth] > 0.0f) { laplacian += m_v[idx - m_gridWidth]; ++count; }
                    if (j < m_gridHeight && m_vWeight[idx + m_gridWidth] > 0.0f) { laplacian += m_v[idx + m_gridWidth]; ++count; }
                    
                    if (count > 0)
                    {
                        laplacian /= count;
                        vNew[idx] = m_v[idx] + m_viscosity * dt * (laplacian - m_v[idx]);
                    }
                }
            }
        }
        
        m_u = uNew;
        m_v = vNew;
    }
    
    // Apply global velocity damping to reduce "jelly" effect
    if (m_velocityDamping < 1.0f)
    {
        for (auto& p : m_particles)
        {
            p.velocity *= m_velocityDamping;
        }
    }
}

void FlipFluidSimulationScene::renderMetaballs(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Implement john-wigg.dev approach:
    // 1. Render radial gradient textures with additive blending to accumulate field
    // 2. Apply threshold to create smooth metaball surfaces
    
    // Step 1: Render field accumulation using MetaballFalloff.png
    renderMetaballField(engine, ctx);
    
    // Step 2: Apply threshold to create surfaces (this would be done with a shader in a real implementation)
    // For now, we'll use the existing particle sprites with proper scaling
    
    // Draw boundary sprites (synced with FirmGuy physics)
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

    // Always draw the interactive ball sprite on top so node.png is visible
    if (auto* ball = m_entityManager->findEntity(m_ballEntityName))
    {
        if (auto* s = ball->getComponent<SpriteComponent>())
        {
            if (s->isVisible() && s->isValid()) s->draw(ctx);
        }
    }
}

void FlipFluidSimulationScene::updateMetaballData()
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

void FlipFluidSimulationScene::createMetaballQuad()
{
    // Create a fullscreen quad for metaball rendering
    auto& metaballEntity = m_entityManager->createEntity(m_metaballQuadEntity);
    auto& sprite = metaballEntity.addComponent<SpriteComponent>(
        *m_graphicsDevice,
        L"DX3D/Assets/Textures/node.png", // We'll use a simple texture as base
        GraphicsEngine::getWindowWidth(),
        GraphicsEngine::getWindowHeight()
    );
    sprite.setPosition(0.0f, 0.0f, 0.0f);
    sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    sprite.setVisible(true);
}

void FlipFluidSimulationScene::generateMetaballMesh()
{
    // Simplified metaball rendering - just use scaled sprites
    // The complex mesh generation was causing performance issues
    // This function is kept for compatibility but does minimal work
    m_metaballVertices.clear();
    m_metaballVertexColors.clear();
    m_metaballIndices.clear();
}

float FlipFluidSimulationScene::calculateMetaballField(const Vec2& worldPos)
{
    float field = 0.0f;
    
    for (size_t i = 0; i < m_metaballPositions.size(); ++i)
    {
        Vec2 toParticle = worldPos - m_metaballPositions[i];
        float dist = toParticle.length();
        float radius = m_metaballRadii[i];
        
        if (dist < radius)
        {
            // Smooth falloff function
            float t = dist / radius;
            float influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t); // Smooth step
            field += influence;
        }
    }
    
    return field;
}

Vec4 FlipFluidSimulationScene::calculateMetaballColor(const Vec2& worldPos)
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

void FlipFluidSimulationScene::renderMetaballMesh(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Simplified metaball rendering - the actual rendering is now done in renderMetaballs()
    // using scaled sprites instead of complex mesh generation
    // This function is kept for compatibility but does nothing
}



void FlipFluidSimulationScene::renderMetaballSurface(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Apply threshold to create smooth metaball surfaces
    // This is the key step that converts the accumulated field into organic shapes
    
    // For now, we'll use a CPU-based approach to create metaball surfaces
    // In a real implementation, this would use a fragment shader with thresholding
    
    if (m_lineRenderer)
    {
        m_lineRenderer->clear();
        
        // Create metaball surface by sampling the field and applying threshold
        Vec4 surfaceColor = Vec4(0.2f, 0.6f, 1.0f, 0.8f);
        
        // Sample points around each particle to create the surface
        for (const auto& p : m_particles)
        {
            // Create a more organic surface by sampling multiple points
            const int samples = 32;
            const float radius = m_metaballRadius;
            
            for (int i = 0; i < samples; ++i)
            {
                float angle1 = (float)i / samples * 2.0f * 3.14159f;
                float angle2 = (float)(i + 1) / samples * 2.0f * 3.14159f;
                
                Vec2 p1 = p.position + Vec2(cosf(angle1), sinf(angle1)) * radius;
                Vec2 p2 = p.position + Vec2(cosf(angle2), sinf(angle2)) * radius;
                
                // Apply threshold effect by varying line thickness based on field strength
                float fieldStrength = calculateMetaballField(p.position);
                float lineWidth = (fieldStrength > m_metaballThreshold) ? 3.0f : 1.0f;
                
                m_lineRenderer->addLine(p1, p2, surfaceColor, lineWidth);
            }
        }
        
        m_lineRenderer->updateBuffer();
        m_lineRenderer->draw(ctx);
    }
}

void FlipFluidSimulationScene::initializeMetaballTextures(GraphicsEngine& engine)
{
    // Create the radial gradient texture for metaball falloff
    createMetaballFalloffTexture();
    
    // Create color gradient texture for final rendering
    createMetaballGradientTexture();
    
    // Create field texture for accumulating metaball contributions
    // This would be a render target texture in a real implementation
}

void FlipFluidSimulationScene::createMetaballFalloffTexture()
{
    // Create a radial gradient texture where alpha decreases with distance from center
    // This is the key insight from john-wigg.dev - use texture-based falloff instead of math
    
    // Load the MetaballFalloff.png texture you created
    // This should be a radial gradient texture with white center fading to transparent edges
    if (m_graphicsDevice)
    {
        m_metaballFalloffTexture = Texture2D::LoadTexture2D(
            m_graphicsDevice->getD3DDevice(),
            L"DX3D/Assets/Textures/MetaballFalloff.png"
        );
    }
}

void FlipFluidSimulationScene::createMetaballGradientTexture()
{
    // Create a gradient texture for color mapping based on field strength
    // This allows for smooth color transitions in the final metaball surface
}

void FlipFluidSimulationScene::renderMetaballField(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Implement john-wigg.dev technique with velocity-based coloring:
    // 1. Render radial gradient textures with additive blending to accumulate field
    // 2. Use particle colors for velocity-based coloring
    
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

void FlipFluidSimulationScene::renderMetaballsAsSprites(DeviceContext& ctx)
{
    // Improved sprite-based metaball rendering
    // Create much larger, more transparent sprites for better blending
    
    for (const auto& p : m_particles)
    {
        if (auto* e = m_entityManager->findEntity(p.entityName))
        {
            if (auto* s = e->getComponent<SpriteComponent>())
            {
                // Use a much larger scale for better metaball effect
                float scale = (m_metaballRadius * 2.0f) / m_particleRadius; // Double the radius
                s->setScale(scale, scale, 1.0f);
                
                // Make particles transparent for better blending
                Vec4 tint = s->getTint();
                tint.w = m_metaballSmoothing; // Use the alpha control from UI
                s->setTint(tint);
                
                if (s->isVisible() && s->isValid())
                    s->draw(ctx);
            }
        }
    }
}

// ========================= Marching Squares Fluid Surface =========================

float FlipFluidSimulationScene::calculateDensityAt(const Vec2& worldPos)
{
    float density = 0.0f;
    float influenceRadius = m_particleRadius * 2.0f;
    
    // Use spatial hashing to only check nearby particles
    float invCellSize = 1.0f / std::max(1.0f, m_hashCellSize);
    int gridX = (int)std::floor((worldPos.x - m_gridOrigin.x) * invCellSize);
    int gridY = (int)std::floor((worldPos.y - m_gridOrigin.y) * invCellSize);
    
    // Check particles in nearby grid cells
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            auto it = m_hash.find(hashKey(gridX + dx, gridY + dy));
            if (it != m_hash.end())
            {
                for (int particleIdx : it->second)
                {
                    const auto& p = m_particles[particleIdx];
                    Vec2 toParticle = worldPos - p.position;
                    float dist = toParticle.length();
                    
                    if (dist < influenceRadius)
                    {
                        // Smooth falloff function (similar to metaball)
                        float t = dist / influenceRadius;
                        float influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t); // Smooth step
                        density += influence;
                    }
                }
            }
        }
    }
    
    return density;
}

Vec2 FlipFluidSimulationScene::interpolateEdge(const Vec2& p1, const Vec2& p2, float val1, float val2, float threshold)
{
    if (std::abs(val1 - val2) < 1e-6f) return p1;
    float t = (threshold - val1) / (val2 - val1);
    return p1 + (p2 - p1) * t;
}

void FlipFluidSimulationScene::generateFluidSurface()
{
    m_fluidSurfaceLines.clear();
    
    if (!m_showFluidSurface) return;
    
    if (m_particles.empty()) 
    {
        m_fluidBodyCount = 0;
        return;
    }
    
    // Safety check: limit particle count to prevent performance issues
    if (m_particles.size() > 1000)
    {
        m_fluidBodyCount = 0;
        return; // Skip fluid surface generation for too many particles
    }
    
    // Performance optimization: Skip updates based on frame rate
    m_fluidSurfaceFrameCounter++;
    if (m_fluidSurfaceUpdateRate > 0 && m_fluidSurfaceFrameCounter < m_fluidSurfaceUpdateRate)
    {
        return; // Skip this frame
    }
    m_fluidSurfaceFrameCounter = 0;
    
    // Proper marching squares implementation for smooth fluid contours
    float cellSize = m_fluidSurfaceResolution;
    
    // Find the bounding area of particles to limit our grid
    Vec2 minPos = m_particles[0].position;
    Vec2 maxPos = m_particles[0].position;
    
    for (const auto& p : m_particles)
    {
        minPos.x = std::min(minPos.x, p.position.x);
        minPos.y = std::min(minPos.y, p.position.y);
        maxPos.x = std::max(maxPos.x, p.position.x);
        maxPos.y = std::max(maxPos.y, p.position.y);
    }
    
    // Add padding and create grid bounds
    float padding = m_particleRadius * 3.0f;
    Vec2 gridMin = minPos - Vec2(padding, padding);
    Vec2 gridMax = maxPos + Vec2(padding, padding);
    
    int gridWidth = (int)((gridMax.x - gridMin.x) / cellSize) + 1;
    int gridHeight = (int)((gridMax.y - gridMin.y) / cellSize) + 1;
    
    // Sample density values at grid points
    std::vector<std::vector<float>> densityGrid(gridHeight, std::vector<float>(gridWidth));
    for (int j = 0; j < gridHeight; ++j)
    {
        for (int i = 0; i < gridWidth; ++i)
        {
            Vec2 worldPos = gridMin + Vec2(i * cellSize, j * cellSize);
            densityGrid[j][i] = calculateDensityAt(worldPos);
        }
    }
    
    // Marching squares algorithm with proper contour tracing
    std::vector<Vec2> contourPoints;
    
    for (int j = 0; j < gridHeight - 1; ++j)
    {
        for (int i = 0; i < gridWidth - 1; ++i)
        {
            // Get corner values
            float val00 = densityGrid[j][i];
            float val10 = densityGrid[j][i + 1];
            float val01 = densityGrid[j + 1][i];
            float val11 = densityGrid[j + 1][i + 1];
            
            // Get corner positions
            Vec2 p00 = gridMin + Vec2(i * cellSize, j * cellSize);
            Vec2 p10 = gridMin + Vec2((i + 1) * cellSize, j * cellSize);
            Vec2 p01 = gridMin + Vec2(i * cellSize, (j + 1) * cellSize);
            Vec2 p11 = gridMin + Vec2((i + 1) * cellSize, (j + 1) * cellSize);
            
            // Determine which edges have crossings
            bool edge0 = (val00 >= m_fluidSurfaceThreshold) != (val10 >= m_fluidSurfaceThreshold);
            bool edge1 = (val10 >= m_fluidSurfaceThreshold) != (val11 >= m_fluidSurfaceThreshold);
            bool edge2 = (val11 >= m_fluidSurfaceThreshold) != (val01 >= m_fluidSurfaceThreshold);
            bool edge3 = (val01 >= m_fluidSurfaceThreshold) != (val00 >= m_fluidSurfaceThreshold);
            
            // Store edge crossings
            if (edge0)
            {
                Vec2 point = interpolateEdge(p00, p10, val00, val10, m_fluidSurfaceThreshold);
                contourPoints.push_back(point);
            }
            if (edge1)
            {
                Vec2 point = interpolateEdge(p10, p11, val10, val11, m_fluidSurfaceThreshold);
                contourPoints.push_back(point);
            }
            if (edge2)
            {
                Vec2 point = interpolateEdge(p11, p01, val11, val01, m_fluidSurfaceThreshold);
                contourPoints.push_back(point);
            }
            if (edge3)
            {
                Vec2 point = interpolateEdge(p01, p00, val01, val00, m_fluidSurfaceThreshold);
                contourPoints.push_back(point);
            }
        }
    }
    
    // Simple approach: Just use single convex hull for now (no clustering)
    if (contourPoints.size() >= 3)
    {
        // Limit points for performance
        std::vector<Vec2> workingPoints;
        if (contourPoints.size() <= 50)
        {
            workingPoints = contourPoints;
        }
        else
        {
            // Sample points evenly
            int step = static_cast<int>(contourPoints.size()) / 50;
            for (size_t i = 0; i < contourPoints.size(); i += step)
            {
                workingPoints.push_back(contourPoints[i]);
            }
        }
        
        // Single convex hull (no clustering for now)
        std::vector<Vec2> hull = getConvexHull(workingPoints);
        m_fluidBodyCount = 1;
        
        // Create connected line segments from convex hull
        for (size_t i = 0; i < hull.size(); ++i)
        {
            size_t next = (i + 1) % hull.size();
            m_fluidSurfaceLines.push_back(hull[i]);
            m_fluidSurfaceLines.push_back(hull[next]);
        }
    }
    else
    {
        m_fluidBodyCount = 0;
    }
}

void FlipFluidSimulationScene::renderFluidSurface(GraphicsEngine& engine, DeviceContext& ctx)
{
    if (!m_showFluidSurface || m_fluidSurfaceLines.empty()) return;
    
    if (m_lineRenderer)
    {
        // Add fluid surface lines to the line renderer
        // Draw fluid surface lines - connect adjacent points to form contours
        for (size_t i = 0; i < m_fluidSurfaceLines.size(); i += 2)
        {
            if (i + 1 < m_fluidSurfaceLines.size())
            {
                m_lineRenderer->addLine(
                    m_fluidSurfaceLines[i],
                    m_fluidSurfaceLines[i + 1],
                    m_fluidSurfaceColor,
                    4.0f  // Make lines thicker and more visible
                );
            }
        }
        
        // If we have odd number of points, connect the last point to the first
        if (m_fluidSurfaceLines.size() > 2 && m_fluidSurfaceLines.size() % 2 == 1)
        {
            m_lineRenderer->addLine(
                m_fluidSurfaceLines[m_fluidSurfaceLines.size() - 1],
                m_fluidSurfaceLines[0],
                m_fluidSurfaceColor,
                4.0f
            );
        }
        
        // Update and draw the line renderer with all accumulated lines
        m_lineRenderer->updateBuffer();
        m_lineRenderer->draw(ctx);
    }
}

// ========================= Convex Hull for Fluid Surface =========================

std::vector<std::vector<Vec2>> FlipFluidSimulationScene::simpleClusterPoints(const std::vector<Vec2>& points)
{
    std::vector<std::vector<Vec2>> clusters;
    if (points.empty()) return clusters;
    
    // Limit points for performance and prevent infinite loops
    const size_t maxPoints = 100;
    std::vector<Vec2> workingPoints;
    if (points.size() <= maxPoints)
    {
        workingPoints = points;
    }
    else
    {
        // Sample points evenly
        int step = static_cast<int>(points.size()) / maxPoints;
        for (size_t i = 0; i < points.size(); i += step)
        {
            workingPoints.push_back(points[i]);
        }
    }
    
    // Simple distance-based clustering with safety limits
    const float clusterDistance = m_fluidSurfaceResolution * 3.0f; // Larger distance for fewer clusters
    std::vector<bool> used(workingPoints.size(), false);
    
    for (size_t i = 0; i < workingPoints.size(); ++i)
    {
        if (used[i]) continue;
        
        // Start new cluster
        std::vector<Vec2> cluster;
        std::vector<size_t> toProcess;
        toProcess.push_back(i);
        used[i] = true;
        
        int iterations = 0;
        const int maxIterations = 1000; // Safety limit
        
        // Flood fill to find all nearby points
        while (!toProcess.empty() && iterations < maxIterations)
        {
            size_t current = toProcess.back();
            toProcess.pop_back();
            cluster.push_back(workingPoints[current]);
            iterations++;
            
            // Find nearby points
            for (size_t j = 0; j < workingPoints.size(); ++j)
            {
                if (used[j]) continue;
                
                float dist = (workingPoints[current] - workingPoints[j]).length();
                if (dist <= clusterDistance)
                {
                    toProcess.push_back(j);
                    used[j] = true;
                }
            }
        }
        
        if (cluster.size() >= 3) // Only keep clusters with enough points
        {
            clusters.push_back(cluster);
        }
    }
    
    return clusters;
}

std::vector<Vec2> FlipFluidSimulationScene::getConvexHull(const std::vector<Vec2>& points)
{
    if (points.size() < 3) return points;
    
    // Limit the number of points for performance (take every nth point)
    const int maxPoints = 50; // Limit to 50 points for performance
    std::vector<Vec2> workingPoints;
    
    if (points.size() <= maxPoints)
    {
        workingPoints = points;
    }
    else
    {
        // Sample points evenly
        int step = static_cast<int>(points.size()) / maxPoints;
        for (size_t i = 0; i < points.size(); i += step)
        {
            workingPoints.push_back(points[i]);
        }
    }
    
    // Find the bottom-most point (and leftmost in case of tie)
    int start = 0;
    for (size_t i = 1; i < workingPoints.size(); ++i)
    {
        if (workingPoints[i].y < workingPoints[start].y || 
            (workingPoints[i].y == workingPoints[start].y && workingPoints[i].x < workingPoints[start].x))
        {
            start = static_cast<int>(i);
        }
    }
    
    std::vector<Vec2> hull;
    int current = start;
    
    do
    {
        hull.push_back(workingPoints[current]);
        int next = (current + 1) % workingPoints.size();
        
        for (size_t i = 0; i < workingPoints.size(); ++i)
        {
            if (crossProduct(workingPoints[current], workingPoints[i], workingPoints[next]) > 0)
            {
                next = static_cast<int>(i);
            }
        }
        
        current = next;
    } while (current != start);
    
    return hull;
}

float FlipFluidSimulationScene::crossProduct(const Vec2& O, const Vec2& A, const Vec2& B)
{
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// ========================= Clustered Mesh Fluid Rendering - REMOVED =========================
// Clustered mesh rendering has been removed as it was causing performance issues
// and was not worth keeping in the codebase.

void FlipFluidSimulationScene::createBall()
{
    // Always create the ball entity, but set visibility based on m_ballEnabled
    if (m_entityManager->findEntity(m_ballEntityName)) return;

    Vec2 startPos = m_boxCenter + Vec2(-m_boxHalf.x * 0.3f, m_boxHalf.y * 0.2f);
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
}

void FlipFluidSimulationScene::updateBallSpring(float dt, const Vec2& target)
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

void FlipFluidSimulationScene::enforceBallOnParticles()
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

float FlipFluidSimulationScene::calculateFluidDensityAt(const Vec2& worldPos)
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

void FlipFluidSimulationScene::applyBallBuoyancy()
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
            printf("Buoyancy: density=%.3f, force=%.1f, vel.y=%.2f\n", 
                   fluidDensity, buoyancyForce, currentVel.y);
        }
    }
}




