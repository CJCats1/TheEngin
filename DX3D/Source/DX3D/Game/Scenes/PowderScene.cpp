#include <DX3D/Game/Scenes/PowderScene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Core/Input.h>
#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>

using namespace dx3d;

void PowderScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();

    // Load texture
    m_nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/node.png");

    // Create camera
    createCamera(engine);

    // Create line renderer for grid debug
    auto& lineEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false);

    if (auto* linePipeline = engine.getLinePipeline())
    {
        m_lineRenderer->setLinePipeline(linePipeline);
    }

    // Initialize grid
    initializeGrid();
    
    // Initialize particle properties
    initializeParticleProperties();
}

void PowderScene::createCamera(GraphicsEngine& engine)
{
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);
}

void PowderScene::initializeGrid()
{
    m_grid.clear();
    m_gridNext.clear();
    m_grid.resize(m_gridWidth * m_gridHeight);
    m_gridNext.resize(m_gridWidth * m_gridHeight);
    
    clearGrid();
}

void PowderScene::clearGrid()
{
    for (auto& cell : m_grid)
    {
        cell.type = ParticleType::Empty;
        cell.updated = false;
    }
}

Vec2 PowderScene::worldToGrid(const Vec2& worldPos) const
{
    Vec2 gridPos = (worldPos - m_gridOrigin) / m_cellSize;
    return gridPos;
}

Vec2 PowderScene::gridToWorld(int x, int y) const
{
    return m_gridOrigin + Vec2(x * m_cellSize, y * m_cellSize);
}

Vec2 PowderScene::getMouseWorldPosition() const
{
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return Vec2(0.0f, 0.0f);
    auto* cam = cameraEntity->getComponent<Camera2D>();
    if (!cam) return Vec2(0.0f, 0.0f);
    Vec2 mouseClient = Input::getInstance().getMousePositionClient();
    return cam->screenToWorld(Vec2(mouseClient.x, mouseClient.y));
}

void PowderScene::update(float dt)
{
    auto& input = Input::getInstance();

    // Toggle pause
    if (input.wasKeyJustPressed(Key::P))
        m_paused = !m_paused;

    // Smooth dt for FPS display
    const float alpha = 0.1f;
    m_smoothDt = (1.0f - alpha) * m_smoothDt + alpha * std::max(1e-6f, dt);

    // Camera controls
    if (auto* camEnt = m_entityManager->findEntity("MainCamera"))
    {
        if (auto* cam = camEnt->getComponent<Camera2D>())
        {
            float zoomDelta = 0.0f;
            const float zoomSpeed = 1.5f;
            if (input.isKeyDown(Key::Q)) zoomDelta += zoomSpeed * dt;
            if (input.isKeyDown(Key::E)) zoomDelta -= zoomSpeed * dt;
            if (zoomDelta != 0.0f) cam->zoom(zoomDelta);

            Vec2 moveDelta(0.0f, 0.0f);
            const float panSpeed = 600.0f;
            if (input.isKeyDown(Key::W)) moveDelta.y += panSpeed * dt;
            if (input.isKeyDown(Key::S)) moveDelta.y -= panSpeed * dt;
            if (input.isKeyDown(Key::A)) moveDelta.x -= panSpeed * dt;
            if (input.isKeyDown(Key::D)) moveDelta.x += panSpeed * dt;
            if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) cam->move(moveDelta);
        }
    }

    // Mouse interaction - add particles
    if (input.isMouseDown(MouseClick::LeftMouse))
    {
        Vec2 mouseWorld = getMouseWorldPosition();
        m_emitAccumulator += m_emitRate * dt;
        int toEmit = (int)m_emitAccumulator;
        if (toEmit > 0)
        {
            addParticlesAt(mouseWorld, m_currentTool, m_brushRadius);
            m_emitAccumulator -= toEmit;
        }
    }
    else
    {
        m_emitAccumulator = 0.0f;
    }

    // Clear particles with right mouse button
    if (input.isMouseDown(MouseClick::RightMouse))
    {
        Vec2 mouseWorld = getMouseWorldPosition();
        Vec2 gridPos = worldToGrid(mouseWorld);
        int gx = (int)std::floor(gridPos.x);
        int gy = (int)std::floor(gridPos.y);
        int radius = (int)(m_brushRadius / m_cellSize);
        
        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dx = -radius; dx <= radius; ++dx)
            {
                int x = gx + dx;
                int y = gy + dy;
                if (isValidGridPos(x, y))
                {
                    float dist2 = (float)(dx * dx + dy * dy);
                    if (dist2 <= radius * radius)
                    {
                        getCell(x, y).type = ParticleType::Empty;
                    }
                }
            }
        }
    }
}

void PowderScene::fixedUpdate(float dt)
{
    if (m_paused) return;

    const int steps = std::max(1, m_substeps);
    const float h = dt / static_cast<float>(steps);

    for (int s = 0; s < steps; ++s)
    {
        // Update particle positions and interactions
        updateGrid(h);
    }
}

void PowderScene::updateGrid(float dt)
{
    // Copy grid to next frame (for double buffering)
    // We read from m_grid (old state) and write to m_gridNext (new state)
    std::copy(m_grid.begin(), m_grid.end(), m_gridNext.begin());

    // Clear updated flags in new grid
    for (auto& cell : m_gridNext)
    {
        cell.updated = false;
    }

    // Update particles bottom-to-top (or alternating pattern for better flow)
    // In Y-up coordinate system, bottom is Y=0, top is Y=height-1
    int yStart = 0;
    int yEnd = m_gridHeight;
    int yStep = 1;
    
    if (m_alternateUpdate)
    {
        // Alternate between bottom-to-top and top-to-bottom
        static bool alt = false;
        alt = !alt;
        if (alt)
        {
            yStart = m_gridHeight - 1;
            yEnd = -1;
            yStep = -1;
        }
    }

    // Update all particles
    // Read from m_grid, modify m_gridNext
    for (int y = yStart; y != yEnd; y += yStep)
    {
        // Alternate X direction too for better flow
        int xStart = (std::abs(y) % 2 == 0) ? 0 : m_gridWidth - 1;
        int xEnd = (std::abs(y) % 2 == 0) ? m_gridWidth : -1;
        int xStep = (std::abs(y) % 2 == 0) ? 1 : -1;

        for (int x = xStart; x != xEnd; x += xStep)
        {
            // Read from old grid
            const Cell& oldCell = m_grid[gridIdx(x, y)];
            if (oldCell.type == ParticleType::Empty)
                continue;

            // Check if already processed in new grid (might have moved here)
            Cell& newCell = m_gridNext[gridIdx(x, y)];
            if (newCell.updated)
                continue;

            newCell.updated = true;

            // Update particle using unified update function
            updateParticle(x, y, dt);
        }
    }

    // Swap grids
    m_grid.swap(m_gridNext);
}

bool PowderScene::tryMove(int x, int y, int newX, int newY)
{
    if (!isValidGridPos(newX, newY))
        return false;

    // Read source from old grid (where particle currently is)
    const Cell& src = m_grid[gridIdx(x, y)];
    if (src.type == ParticleType::Empty)
        return false;

    // Check destination in new grid
    Cell& dst = m_gridNext[gridIdx(newX, newY)];

    if (dst.type != ParticleType::Empty || dst.updated)
        return false;

    // Move particle from old position to new position
    dst.type = src.type;
    dst.updated = true;

    // Clear source in new grid (particle has moved)
    Cell& srcNew = m_gridNext[gridIdx(x, y)];
    srcNew.type = ParticleType::Empty;

    return true;
}

bool PowderScene::trySwap(int x, int y, int newX, int newY)
{
    if (!isValidGridPos(newX, newY))
        return false;

    // Read from old grid
    const Cell& srcOld = m_grid[gridIdx(x, y)];
    const Cell& dstOld = m_grid[gridIdx(newX, newY)];

    if (srcOld.type == ParticleType::Empty || dstOld.type == ParticleType::Empty)
        return false;

    // Get properties for both particles
    const ParticleProperties& srcProps = getParticleProperties(srcOld.type);
    const ParticleProperties& dstProps = getParticleProperties(dstOld.type);

    // Solids cannot be moved via non-chemical reactions - don't swap with solids
    if (srcProps.matterState == MatterState::Solid || dstProps.matterState == MatterState::Solid)
        return false;

    // Density-based swapping: denser particles sink, lighter particles float
    float srcDensity = srcProps.density;
    float dstDensity = dstProps.density;

    // Only swap if source is denser than destination (denser sinks down)
    if (srcDensity <= dstDensity)
        return false;

    // Check if destination is already updated in new grid (source is being updated now, so ignore its flag)
    Cell& srcNew = m_gridNext[gridIdx(x, y)];
    Cell& dstNew = m_gridNext[gridIdx(newX, newY)];

    // Only check destination - source is currently being processed
    if (dstNew.updated)
        return false;

    // Swap particles in new grid (denser particle sinks, lighter rises)
    srcNew.type = dstOld.type;
    srcNew.updated = true;

    dstNew.type = srcOld.type;
    dstNew.updated = true;

    return true;
}

void PowderScene::initializeParticleProperties()
{
    // Sand: Powder state - falls down, can swap with less dense particles, but doesn't spread horizontally
    ParticleProperties sand;
    sand.matterState = MatterState::Powder;
    sand.density = 3.0f;
    sand.color = Vec4(0.9f, 0.8f, 0.5f, 1.0f); // Yellowish sand
    m_particleProperties[ParticleType::Sand] = sand;

    // Water: Liquid state - random horizontal movement, falls down, can float above denser liquids/powders
    ParticleProperties water;
    water.matterState = MatterState::Liquid;
    water.density = 1.0f;
    water.color = Vec4(0.2f, 0.6f, 1.0f, 0.8f); // Blue water
    m_particleProperties[ParticleType::Water] = water;

    // Stone: Solid state - does not move via non-chemical reactions
    ParticleProperties stone;
    stone.matterState = MatterState::Solid;
    stone.density = 10.0f;
    stone.color = Vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray stone
    m_particleProperties[ParticleType::Stone] = stone;

    // Wood: Solid state - does not move via non-chemical reactions
    ParticleProperties wood;
    wood.matterState = MatterState::Solid;
    wood.density = 0.8f;
    wood.color = Vec4(0.4f, 0.25f, 0.1f, 1.0f); // Brown wood
    m_particleProperties[ParticleType::Wood] = wood;

    // Gas: Gas state - moves opposite to gravity (upward), repels from other gas particles
    ParticleProperties gas;
    gas.matterState = MatterState::Gas;
    gas.density = 0.1f; // Very light
    gas.color = Vec4(204.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f, 1.0f); // Light pink/rose, fully opaque
    gas.movementChance = 0.3f; // 30% chance to move each frame (makes gas move slower)
    m_particleProperties[ParticleType::Gas] = gas;
}

const PowderScene::ParticleProperties& PowderScene::getParticleProperties(ParticleType type) const
{
    auto it = m_particleProperties.find(type);
    if (it != m_particleProperties.end())
    {
        return it->second;
    }
    
    // Return default properties if not found
    static ParticleProperties defaultProps;
    return defaultProps;
}

void PowderScene::updateParticle(int x, int y, float dt)
{
    // Read particle type from old grid
    const Cell& oldCell = m_grid[gridIdx(x, y)];
    if (oldCell.type == ParticleType::Empty)
        return;

    // Get particle properties
    const ParticleProperties& props = getParticleProperties(oldCell.type);

    // Handle based on state of matter
    switch (props.matterState)
    {
    case MatterState::Solid:
        // Solids do not move via non-chemical reactions
        return;

    case MatterState::Powder:
        // Powder: Only fall via non-chemical reactions, but will not spread as liquids do
        // Stays in place unless a force acts upon them (gravity)
        updatePowder(x, y, props);
        return;

    case MatterState::Liquid:
        // Liquid: Random horizontal movement each frame, then fall down
        // Can float above denser liquids/powders
        updateLiquid(x, y, props);
        return;

    case MatterState::Gas:
        // Gas: Moves opposite to gravity (upward), repels from other gas particles
        updateGas(x, y, props);
        return;

    default:
        return;
    }
}

void PowderScene::updatePowder(int x, int y, const ParticleProperties& props)
{
    // Powder falls down due to gravity (Y-up, so down is Y-1)
    // Try to move down into empty space first
    if (tryMove(x, y, x, y - 1))
        return;

    // Density-based: if powder is denser than particle below, swap (powder sinks)
    // Powders can fall through gases and liquids (but not solids)
    if (isValidGridPos(x, y - 1))
    {
        const Cell& cellBelow = m_grid[gridIdx(x, y - 1)];
        if (cellBelow.type != ParticleType::Empty)
        {
            const ParticleProperties& belowProps = getParticleProperties(cellBelow.type);
            // Powder can sink through less dense particles (liquids and gases, but not solids)
            // Gases are always less dense, so powder always falls through them
            if (belowProps.matterState == MatterState::Gas || 
                (belowProps.matterState != MatterState::Solid && props.density > belowProps.density))
            {
                if (trySwap(x, y, x, y - 1))
                    return;
            }
        }
    }

    // If can't move down, try diagonal down (left or right)
    int dir = (rand() % 2 == 0) ? -1 : 1;
    
    // Try diagonal into empty space
    if (tryMove(x, y, x + dir, y - 1))
        return;

    // Try other diagonal into empty space
    if (tryMove(x, y, x - dir, y - 1))
        return;

    // Try diagonal swapping with less dense particles (but not solids)
    // Powders can fall through gases and liquids diagonally
    for (int dx : {dir, -dir})
    {
        if (isValidGridPos(x + dx, y - 1))
        {
            const Cell& cellDiag = m_grid[gridIdx(x + dx, y - 1)];
            if (cellDiag.type != ParticleType::Empty)
            {
                const ParticleProperties& diagProps = getParticleProperties(cellDiag.type);
                // Skip solids - they cannot be moved
                // Gases are always less dense, so powder always falls through them
                if (diagProps.matterState == MatterState::Gas ||
                    (diagProps.matterState != MatterState::Solid && props.density > diagProps.density))
                {
                    if (trySwap(x, y, x + dx, y - 1))
                        return;
                }
            }
        }
    }

    // Powder is at rest (can't fall further, doesn't spread horizontally)
}

void PowderScene::updateLiquid(int x, int y, const ParticleProperties& props)
{
    // Liquid: Each individual dot randomly moves left, right, or stays in place horizontally every frame
    // Then falls down. In cellular automata, each particle can only move once per frame,
    // so we prioritize horizontal movement first, then falling if horizontal didn't happen.
    
    // First, try horizontal movement (random left, right, or stay)
    int horizontalDir = rand() % 3 - 1; // -1 (left), 0 (stay), 1 (right)
    
    if (horizontalDir != 0)
    {
        if (isValidGridPos(x + horizontalDir, y))
        {
            Cell& dst = m_gridNext[gridIdx(x + horizontalDir, y)];
            if (dst.type == ParticleType::Empty && !dst.updated)
            {
                if (tryMove(x, y, x + horizontalDir, y))
                {
                    // Moved horizontally, done for this frame (particle can only move once)
                    return;
                }
            }
        }
    }

    // If horizontal movement didn't happen, try to fall down (Y-up, so down is Y-1)
    if (tryMove(x, y, x, y - 1))
        return;

    // Liquids can fall through gases (gases are always less dense)
    // Try to swap with gas below
    if (isValidGridPos(x, y - 1))
    {
        const Cell& cellBelow = m_grid[gridIdx(x, y - 1)];
        if (cellBelow.type != ParticleType::Empty)
        {
            const ParticleProperties& belowProps = getParticleProperties(cellBelow.type);
            // Liquids always fall through gases
            if (belowProps.matterState == MatterState::Gas)
            {
                if (trySwap(x, y, x, y - 1))
                    return;
            }
            // Also try to fall through less dense liquids and powders
            else if (belowProps.matterState != MatterState::Solid && props.density > belowProps.density)
            {
                if (trySwap(x, y, x, y - 1))
                    return;
            }
        }
    }

    // Try diagonal falling through gases
    int dir = (rand() % 2 == 0) ? -1 : 1;
    for (int dx : {dir, -dir})
    {
        if (isValidGridPos(x + dx, y - 1))
        {
            const Cell& cellDiag = m_grid[gridIdx(x + dx, y - 1)];
            if (cellDiag.type != ParticleType::Empty)
            {
                const ParticleProperties& diagProps = getParticleProperties(cellDiag.type);
                // Liquids always fall through gases
                if (diagProps.matterState == MatterState::Gas)
                {
                    if (trySwap(x, y, x + dx, y - 1))
                        return;
                }
                // Also try to fall through less dense liquids and powders
                else if (diagProps.matterState != MatterState::Solid && props.density > diagProps.density)
                {
                    if (trySwap(x, y, x + dx, y - 1))
                        return;
                }
            }
        }
    }

    // Liquid can float upward through denser particles (to escape being trapped)
    // Can float above denser liquids and powders (but not solids)
    float particleDensity = props.density;
    if (isValidGridPos(x, y + 1))
    {
        const Cell& cellAbove = m_grid[gridIdx(x, y + 1)];
        if (cellAbove.type != ParticleType::Empty)
        {
            const ParticleProperties& aboveProps = getParticleProperties(cellAbove.type);
            // If what's above is denser than this liquid, swap (denser sinks, lighter floats up)
            // Liquids can float above denser liquids and powders (but not solids)
            if (aboveProps.matterState != MatterState::Solid &&
                aboveProps.density > particleDensity && 
                (aboveProps.matterState == MatterState::Liquid || aboveProps.matterState == MatterState::Powder))
            {
                if (trySwap(x, y + 1, x, y))
                    return;
            }
        }
    }

    // Try diagonal upward swaps (but not with solids)
    // Reuse dir variable from above
    for (int dx : {dir, -dir})
    {
        if (isValidGridPos(x + dx, y + 1))
        {
            const Cell& cellDiag = m_grid[gridIdx(x + dx, y + 1)];
            if (cellDiag.type != ParticleType::Empty)
            {
                const ParticleProperties& diagProps = getParticleProperties(cellDiag.type);
                // Can float above denser liquids and powders (but not solids)
                if (diagProps.matterState != MatterState::Solid &&
                    diagProps.density > particleDensity && 
                    (diagProps.matterState == MatterState::Liquid || diagProps.matterState == MatterState::Powder))
                {
                    if (trySwap(x + dx, y + 1, x, y))
                        return;
                }
            }
        }
    }

    // Can fall down diagonally if there's a gap (already handled above with gas swapping)
    // This is just for empty spaces
    if (tryMove(x, y, x + dir, y - 1))
        return;
    if (tryMove(x, y, x - dir, y - 1))
        return;

    // Liquid is at rest or pooling
}

void PowderScene::updateGas(int x, int y, const ParticleProperties& props)
{
    // Gas repels from other gas particles and reacts to other particles
    // Gas does NOT move opposite to gravity - it only moves due to repulsion and reactions

    // Apply movement chance - gas moves slower by randomly skipping movement attempts
    float randomValue = (float)rand() / (float)RAND_MAX;
    if (randomValue > props.movementChance)
        return; // Skip movement this frame

    // Check all 8 adjacent cells (including diagonals) for particles to react to
    int gasNeighbors = 0;
    int otherParticleNeighbors = 0;
    int preferredDirX = 0;
    int preferredDirY = 0;
    
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0) continue; // Skip self
            
            if (isValidGridPos(x + dx, y + dy))
            {
                const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                if (neighbor.type != ParticleType::Empty)
                {
                    const ParticleProperties& neighborProps = getParticleProperties(neighbor.type);
                    
                    // Gas repels from other gas particles
                    if (neighborProps.matterState == MatterState::Gas)
                    {
                        gasNeighbors++;
                        // Repel: move in opposite direction
                        preferredDirX -= dx;
                        preferredDirY -= dy;
                    }
                    // Gas reacts to other particles (non-gas) - try to move away from them
                    else
                    {
                        otherParticleNeighbors++;
                        // React by moving away from denser particles
                        preferredDirX -= dx;
                        preferredDirY -= dy;
                    }
                }
            }
        }
    }

    // If there are neighbors (gas or other particles), try to move away from them
    if (gasNeighbors > 0 || otherParticleNeighbors > 0)
    {
        // Normalize direction (prefer stronger direction)
        int repelX = (preferredDirX > 0) ? 1 : (preferredDirX < 0) ? -1 : 0;
        int repelY = (preferredDirY > 0) ? 1 : (preferredDirY < 0) ? -1 : 0;
        
        // Try to move in the repel/react direction (horizontal first, then vertical)
        if (repelX != 0)
        {
            if (isValidGridPos(x + repelX, y))
            {
                Cell& dst = m_gridNext[gridIdx(x + repelX, y)];
                if (dst.type == ParticleType::Empty && !dst.updated)
                {
                    if (tryMove(x, y, x + repelX, y))
                        return; // Moved horizontally away from neighbors
                }
            }
        }
        
        // Try vertical movement
        if (repelY != 0)
        {
            if (isValidGridPos(x, y + repelY))
            {
                Cell& dst = m_gridNext[gridIdx(x, y + repelY)];
                if (dst.type == ParticleType::Empty && !dst.updated)
                {
                    if (tryMove(x, y, x, y + repelY))
                        return; // Moved vertically away from neighbors
                }
            }
        }
        
        // Try diagonal repel/react movement
        if (repelX != 0 && repelY != 0)
        {
            if (isValidGridPos(x + repelX, y + repelY))
            {
                Cell& dst = m_gridNext[gridIdx(x + repelX, y + repelY)];
                if (dst.type == ParticleType::Empty && !dst.updated)
                {
                    if (tryMove(x, y, x + repelX, y + repelY))
                        return; // Moved diagonally away from neighbors
                }
            }
        }
    }

    // If no neighbors to react to, try random horizontal movement (gas expands/diffuses)
    if (rand() % 3 == 0) // 1/3 chance to move horizontally
    {
        int horizontalDir = (rand() % 2 == 0) ? -1 : 1;
        if (isValidGridPos(x + horizontalDir, y))
        {
            Cell& dst = m_gridNext[gridIdx(x + horizontalDir, y)];
            if (dst.type == ParticleType::Empty && !dst.updated)
            {
                if (tryMove(x, y, x + horizontalDir, y))
                    return;
            }
        }
    }

    // Gas is at rest (no neighbors to react to and no random movement)
}

void PowderScene::addParticlesAt(const Vec2& worldPos, ParticleType type, float radius)
{
    Vec2 gridPos = worldToGrid(worldPos);
    int gx = (int)std::floor(gridPos.x);
    int gy = (int)std::floor(gridPos.y);
    int radiusCells = (int)(radius / m_cellSize);
    
    int count = 0;
    const int maxParticlesPerFrame = 200; // Limit for performance

    for (int dy = -radiusCells; dy <= radiusCells && count < maxParticlesPerFrame; ++dy)
    {
        for (int dx = -radiusCells; dx <= radiusCells && count < maxParticlesPerFrame; ++dx)
        {
            int x = gx + dx;
            int y = gy + dy;
            
            if (!isValidGridPos(x, y))
                continue;

            float dist2 = (float)(dx * dx + dy * dy);
            float r2 = radiusCells * radiusCells;
            
            // Random placement for better distribution
            if (dist2 <= r2 && rand() % 3 == 0) // 1/3 chance per cell
            {
                Cell& cell = getCell(x, y);
                if (cell.type == ParticleType::Empty)
                {
                    cell.type = type;
                    count++;
                }
            }
        }
    }
}


void PowderScene::render(GraphicsEngine& engine, SwapChain& swapChain)
{
    auto& ctx = engine.getContext();

    // Set up camera
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera"))
    {
        if (auto* camera = cameraEntity->getComponent<Camera2D>())
        {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // Set up rendering
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.enableAlphaBlending();

    // Render particles
    renderParticles(engine, ctx);

    // Debug grid
    if (m_lineRenderer)
    {
        m_lineRenderer->clear();
    }

    if (m_showGrid && m_lineRenderer)
    {
        Vec4 gridColor = Vec4(1.0f, 1.0f, 1.0f, 0.05f);
        for (int x = 0; x <= m_gridWidth; ++x)
        {
            Vec2 start = gridToWorld(x, 0);
            Vec2 end = gridToWorld(x, m_gridHeight);
            m_lineRenderer->addLine(start, end, gridColor, 1.0f);
        }
        for (int y = 0; y <= m_gridHeight; ++y)
        {
            Vec2 start = gridToWorld(0, y);
            Vec2 end = gridToWorld(m_gridWidth, y);
            m_lineRenderer->addLine(start, end, gridColor, 1.0f);
        }
        m_lineRenderer->updateBuffer();
        m_lineRenderer->draw(ctx);
    }
}

void PowderScene::renderParticles(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Create or update sprite entities for visible particles
    int totalCells = m_gridWidth * m_gridHeight;

    // Create sprites on demand (simplified approach - could be optimized with pooling)
    static int lastCellCount = 0;
    static std::vector<std::string> entityNames;

    // Create new entities if needed
    if (totalCells > entityNames.size())
    {
        int needed = totalCells - (int)entityNames.size();
        for (int i = 0; i < needed; ++i)
        {
            std::string name = "PowderParticle_" + std::to_string(entityNames.size());
            entityNames.push_back(name);
            auto& e = m_entityManager->createEntity(name);
            auto& s = e.addComponent<SpriteComponent>(
                *m_graphicsDevice, 
                L"DX3D/Assets/Textures/node.png", 
                m_cellSize, 
                m_cellSize);
            s.setVisible(false);
        }
    }

    // Render all cells (particles)
    int spriteIndex = 0;
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            const Cell& cell = getCell(x, y);
            Vec2 worldPos = gridToWorld(x, y);
            
            // Get particle color from properties
            Vec4 particleColor = Vec4(0.0f, 0.0f, 0.0f, 0.0f); // Empty cells are transparent
            if (cell.type != ParticleType::Empty)
            {
                particleColor = getParticleProperties(cell.type).color;
            }
            
            // Render if there's a particle
            if (cell.type != ParticleType::Empty)
            {
                if (spriteIndex >= (int)entityNames.size())
                    break;

                std::string& name = entityNames[spriteIndex];
                auto* entity = m_entityManager->findEntity(name);
                if (entity)
                {
                    auto* sprite = entity->getComponent<SpriteComponent>();
                    if (sprite)
                    {
                        sprite->setPosition(worldPos.x, worldPos.y, 0.0f);
                        sprite->setTint(particleColor);
                        sprite->setVisible(true);
                        sprite->draw(ctx);
                    }
                }
                spriteIndex++;
            }
        }
    }

    // Hide unused sprites
    for (int i = spriteIndex; i < (int)entityNames.size(); ++i)
    {
        auto* entity = m_entityManager->findEntity(entityNames[i]);
        if (entity)
        {
            auto* sprite = entity->getComponent<SpriteComponent>();
            if (sprite)
                sprite->setVisible(false);
        }
    }
}

void PowderScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::SetNextWindowSize(ImVec2(320, 280), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Powder Toy Simulation"))
    {
        float fps = (m_smoothDt > 0.0f) ? (1.0f / m_smoothDt) : 0.0f;
        ImGui::Text("FPS: %.1f (dt=%.3f ms)", fps, m_smoothDt * 1000.0f);
        ImGui::Checkbox("Paused (P)", &m_paused);

        // Count particles
        int sandCount = 0, waterCount = 0, stoneCount = 0, woodCount = 0, gasCount = 0;
        for (const auto& cell : m_grid)
        {
            if (cell.type == ParticleType::Sand) sandCount++;
            else if (cell.type == ParticleType::Water) waterCount++;
            else if (cell.type == ParticleType::Stone) stoneCount++;
            else if (cell.type == ParticleType::Wood) woodCount++;
            else if (cell.type == ParticleType::Gas) gasCount++;
        }
        ImGui::Text("Particles: Sand=%d, Water=%d, Stone=%d, Wood=%d, Gas=%d", sandCount, waterCount, stoneCount, woodCount, gasCount);

        ImGui::Separator();
        ImGui::Text("Simulation");
        ImGui::SliderInt("Substeps", &m_substeps, 1, 8);
        ImGui::Checkbox("Alternate Update", &m_alternateUpdate);
        ImGui::Text("(Alternating improves flow)");

        ImGui::Separator();
        ImGui::Text("Tools");
        int tool = (int)m_currentTool - 1;
        const char* tools[] = { "Sand", "Water", "Stone", "Wood", "Gas" };
        if (ImGui::Combo("Tool", &tool, tools, 5))
        {
            m_currentTool = (ParticleType)(tool + 1);
        }
        ImGui::SliderFloat("Brush Radius", &m_brushRadius, 5.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Emit Rate", &m_emitRate, 10.0f, 500.0f, "%.0f");
        ImGui::Text("LMB: Add particles");
        ImGui::Text("RMB: Clear particles");

        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::Text("WASD: Pan");
        ImGui::Text("Q/E: Zoom");
        
        ImGui::Separator();
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Grid", &m_showGrid);

        if (ImGui::Button("Clear All", ImVec2(-FLT_MIN, 0)))
        {
            clearGrid();
        }
    }
    ImGui::End();
}
