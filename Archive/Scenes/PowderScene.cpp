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
    
    // Initialize air system
    initializeAirSystem();
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

void PowderScene::initializeAirSystem()
{
    int gridSize = m_gridWidth * m_gridHeight;
    
    // Initialize air grids - explicitly set to zero
    m_airPressure.resize(gridSize);
    m_airVelocityX.resize(gridSize);
    m_airVelocityY.resize(gridSize);
    m_airHeat.resize(gridSize);
    
    // Initialize double buffering grids - explicitly set to zero
    m_airPressureNext.resize(gridSize);
    m_airVelocityXNext.resize(gridSize);
    m_airVelocityYNext.resize(gridSize);
    m_airHeatNext.resize(gridSize);
    
    // Initialize blocking maps
    m_blockAir.resize(gridSize);
    m_blockAirHeat.resize(gridSize);
    
    // Create Gaussian kernel for smoothing
    makeKernel();
    
    // Clear all air data to ensure zero velocity
    clearAirSystem();
}

void PowderScene::makeKernel()
{
    // Create 3x3 Gaussian kernel for air smoothing
    float s = 0.0f;
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            int idx = (i + 1) + 3 * (j + 1);
            m_airKernel[idx] = std::expf(-2.0f * (i * i + j * j));
            s += m_airKernel[idx];
        }
    }
    
    // Normalize kernel
    s = 1.0f / s;
    for (int i = 0; i < 9; ++i)
    {
        m_airKernel[i] *= s;
    }
}

void PowderScene::clearAirSystem()
{
    // Clear all air grids (both current and next buffers)
    std::fill(m_airPressure.begin(), m_airPressure.end(), 0.0f);
    std::fill(m_airVelocityX.begin(), m_airVelocityX.end(), 0.0f);
    std::fill(m_airVelocityY.begin(), m_airVelocityY.end(), 0.0f);
    std::fill(m_airHeat.begin(), m_airHeat.end(), m_ambientAirTemp);
    
    // Clear double buffering grids
    std::fill(m_airPressureNext.begin(), m_airPressureNext.end(), 0.0f);
    std::fill(m_airVelocityXNext.begin(), m_airVelocityXNext.end(), 0.0f);
    std::fill(m_airVelocityYNext.begin(), m_airVelocityYNext.end(), 0.0f);
    std::fill(m_airHeatNext.begin(), m_airHeatNext.end(), m_ambientAirTemp);
    
    // Clear blocking maps
    std::fill(m_blockAir.begin(), m_blockAir.end(), 0);
    std::fill(m_blockAirHeat.begin(), m_blockAirHeat.end(), 0);
}

float PowderScene::vorticity(int x, int y) const
{
    // Calculate vorticity: dvy/dx - dvx/dy
    if (x > 1 && x < m_gridWidth - 2 && y > 1 && y < m_gridHeight - 2)
    {
        float vx = m_airVelocityX[gridIdx(x, y)];
        float vy = m_airVelocityY[gridIdx(x, y)];
        
        float dvx_dy = m_airVelocityX[gridIdx(x, y + 1)] - m_airVelocityX[gridIdx(x, y - 1)];
        float dvy_dx = m_airVelocityY[gridIdx(x + 1, y)] - m_airVelocityY[gridIdx(x - 1, y)];
        
        return (dvy_dx - dvx_dy) * 0.5f;
    }
    return 0.0f;
}

void PowderScene::updateBlockAirMaps()
{
    // Update blocking maps based on particles
    // Solids block air, some particles block heat
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            const Cell& cell = getCell(x, y);
            bool blocksAir = false;
            bool blocksHeat = false;
            
            if (cell.type != ParticleType::Empty)
            {
                const ParticleProperties& props = getParticleProperties(cell.type);
                
                // Solids block air flow
                if (props.matterState == MatterState::Solid)
                {
                    blocksAir = true;
                }
                
                // Most particles block heat to some degree
                // (In a full implementation, you'd check heat conductivity)
                if (props.matterState == MatterState::Solid)
                {
                    blocksHeat = true;
                }
            }
            
            m_blockAir[gridIdx(x, y)] = blocksAir ? 1 : 0;
            m_blockAirHeat[gridIdx(x, y)] = blocksHeat ? 0x8 : 0;
        }
    }
}

void PowderScene::updateAirSystem(float dt)
{
    // Only update if air is enabled and grids are properly initialized
    if (!m_airEnabled || m_airVelocityX.empty() || m_airVelocityY.empty())
        return;
    
    // Update air pressure and velocity (but don't swap yet)
    updateAirPressure(dt);
    
    // Apply heat convection to velocity (hot air rises)
    // This modifies the next buffers before they're swapped
    updateAirVelocity(dt);
    
    // Now swap pressure/velocity buffers
    m_airPressure.swap(m_airPressureNext);
    m_airVelocityX.swap(m_airVelocityXNext);
    m_airVelocityY.swap(m_airVelocityYNext);
    
    // Update ambient heat (needs velocity for advection)
    updateAirHeat(dt);
}

void PowderScene::updateAirPressure(float dt)
{
    // Copy current state to next buffer
    // Note: Impulses write to both current and next buffers, so this preserves them
    std::copy(m_airPressure.begin(), m_airPressure.end(), m_airPressureNext.begin());
    std::copy(m_airVelocityX.begin(), m_airVelocityX.end(), m_airVelocityXNext.begin());
    std::copy(m_airVelocityY.begin(), m_airVelocityY.end(), m_airVelocityYNext.begin());
    
    // Set pressure/velocity to zero on absolute edges only (boundary conditions)
    // Only reset the very edges to prevent accumulation, but allow impulses near edges
    for (int i = 0; i < m_gridHeight; ++i)
    {
        m_airPressureNext[gridIdx(0, i)] = 0.0f;
        m_airPressureNext[gridIdx(m_gridWidth - 1, i)] = 0.0f;
        m_airVelocityXNext[gridIdx(0, i)] = 0.0f;
        m_airVelocityXNext[gridIdx(m_gridWidth - 1, i)] = 0.0f;
        m_airVelocityYNext[gridIdx(0, i)] = 0.0f;
        m_airVelocityYNext[gridIdx(m_gridWidth - 1, i)] = 0.0f;
    }
    
    for (int i = 0; i < m_gridWidth; ++i)
    {
        m_airPressureNext[gridIdx(i, 0)] = 0.0f;
        m_airPressureNext[gridIdx(i, m_gridHeight - 1)] = 0.0f;
        m_airVelocityXNext[gridIdx(i, 0)] = 0.0f;
        m_airVelocityXNext[gridIdx(i, m_gridHeight - 1)] = 0.0f;
        m_airVelocityYNext[gridIdx(i, 0)] = 0.0f;
        m_airVelocityYNext[gridIdx(i, m_gridHeight - 1)] = 0.0f;
    }
    
    // Clear velocities near walls
    for (int y = 1; y < m_gridHeight - 1; ++y)
    {
        for (int x = 1; x < m_gridWidth - 1; ++x)
        {
            if (m_blockAir[gridIdx(x, y)])
            {
                m_airVelocityXNext[gridIdx(x, y)] = 0.0f;
                m_airVelocityXNext[gridIdx(x - 1, y)] = 0.0f;
                m_airVelocityXNext[gridIdx(x + 1, y)] = 0.0f;
                m_airVelocityYNext[gridIdx(x, y)] = 0.0f;
                m_airVelocityYNext[gridIdx(x, y - 1)] = 0.0f;
                m_airVelocityYNext[gridIdx(x, y + 1)] = 0.0f;
            }
        }
    }
    
    // Pressure adjustments from velocity divergence
    const float pressureStep = 0.5f;
    for (int y = 1; y < m_gridHeight - 1; ++y)
    {
        for (int x = 1; x < m_gridWidth - 1; ++x)
        {
            float dp = 0.0f;
            dp += m_airVelocityX[gridIdx(x - 1, y)] - m_airVelocityX[gridIdx(x + 1, y)];
            dp += m_airVelocityY[gridIdx(x, y - 1)] - m_airVelocityY[gridIdx(x, y + 1)];
            
            // Apply pressure loss based on magnitude - weak pressures decay faster (spread out)
            // Strong pressures decay slower, weak spread-out pressures decay much faster
            float currentPressure = std::abs(m_airPressureNext[gridIdx(x, y)]);
            float pressureLoss = m_airPressureLoss;
            
            if (currentPressure > 20.0f)
            {
                // Very strong pressures - minimal loss
                pressureLoss = 0.98f;
            }
            else if (currentPressure > 10.0f)
            {
                // Strong pressures - moderate loss
                pressureLoss = 0.85f;
            }
            else if (currentPressure > 2.0f)
            {
                // Medium pressures - use default loss
                pressureLoss = m_airPressureLoss;
            }
            else if (currentPressure > 0.5f)
            {
                // Weak pressures - faster decay
                pressureLoss = 0.3f; // 70% loss per frame
            }
            else
            {
                // Very weak pressures - very fast decay
                pressureLoss = 0.1f; // 90% loss per frame
            }
            
            m_airPressureNext[gridIdx(x, y)] *= pressureLoss;
            m_airPressureNext[gridIdx(x, y)] += dp * pressureStep;
        }
    }
    
    // Velocity adjustments from pressure gradient
    const float velocityStep = 0.5f;
    for (int y = 1; y < m_gridHeight - 1; ++y)
    {
        for (int x = 1; x < m_gridWidth - 1; ++x)
        {
            float dx = 0.0f;
            float dy = 0.0f;
            
            dx += m_airPressure[gridIdx(x - 1, y)] - m_airPressure[gridIdx(x + 1, y)];
            dy += m_airPressure[gridIdx(x, y - 1)] - m_airPressure[gridIdx(x, y + 1)];
            
            // Apply velocity loss based on magnitude - weak velocities decay faster (spread out)
            // Strong impulses decay slower, weak spread-out velocities decay much faster
            float currentVelMag = std::sqrt(m_airVelocityXNext[gridIdx(x, y)] * m_airVelocityXNext[gridIdx(x, y)] + 
                                            m_airVelocityYNext[gridIdx(x, y)] * m_airVelocityYNext[gridIdx(x, y)]);
            float velocityLoss = m_airVelocityLoss;
            
            if (currentVelMag > 10.0f)
            {
                // Very strong impulses - minimal loss
                velocityLoss = 0.98f;
            }
            else if (currentVelMag > 5.0f)
            {
                // Strong velocities - moderate loss
                velocityLoss = 0.85f;
            }
            else if (currentVelMag > 2.0f)
            {
                // Medium velocities - use default loss
                velocityLoss = m_airVelocityLoss;
            }
            else if (currentVelMag > 0.5f)
            {
                // Weak velocities - faster decay
                velocityLoss = 0.3f; // 70% loss per frame
            }
            else
            {
                // Very weak velocities - very fast decay
                velocityLoss = 0.1f; // 90% loss per frame
            }
            
            m_airVelocityXNext[gridIdx(x, y)] *= velocityLoss;
            m_airVelocityYNext[gridIdx(x, y)] *= velocityLoss;
            m_airVelocityXNext[gridIdx(x, y)] += dx * velocityStep;
            m_airVelocityYNext[gridIdx(x, y)] += dy * velocityStep;
            
            // Block velocity at walls
            if (m_blockAir[gridIdx(x - 1, y)] || m_blockAir[gridIdx(x, y)] || m_blockAir[gridIdx(x + 1, y)])
                m_airVelocityXNext[gridIdx(x, y)] = 0.0f;
            if (m_blockAir[gridIdx(x, y - 1)] || m_blockAir[gridIdx(x, y)] || m_blockAir[gridIdx(x, y + 1)])
                m_airVelocityYNext[gridIdx(x, y)] = 0.0f;
        }
    }
    
    // Advection: take values from far away based on velocity
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            if (m_blockAir[gridIdx(x, y)])
                continue;
            
            // Don't skip advection - always process to allow impulses to propagate
            // The advection will naturally handle zero velocities correctly
            
            // Smooth using kernel
            float dx = 0.0f;
            float dy = 0.0f;
            float dp = 0.0f;
            
            for (int j = -1; j <= 1; ++j)
            {
                for (int i = -1; i <= 1; ++i)
                {
                    int nx = x + i;
                    int ny = y + j;
                    
                    if (nx > 0 && nx < m_gridWidth - 1 && ny > 0 && ny < m_gridHeight - 1 && !m_blockAir[gridIdx(nx, ny)])
                    {
                        float f = m_airKernel[(i + 1) + 3 * (j + 1)];
                        dx += m_airVelocityX[gridIdx(nx, ny)] * f;
                        dy += m_airVelocityY[gridIdx(nx, ny)] * f;
                        dp += m_airPressure[gridIdx(nx, ny)] * f;
                    }
                    else
                    {
                        float f = m_airKernel[(i + 1) + 3 * (j + 1)];
                        dx += m_airVelocityX[gridIdx(x, y)] * f;
                        dy += m_airVelocityY[gridIdx(x, y)] * f;
                        dp += m_airPressure[gridIdx(x, y)] * f;
                    }
                }
            }
            
            // Advection: take value from position moved by velocity
            float tx = x - dx * m_airAdvectionMult;
            float ty = y - dy * m_airAdvectionMult;
            
            // Check for walls in path
            if ((std::abs(dx * m_airAdvectionMult) > 1.0f || std::abs(dy * m_airAdvectionMult) > 1.0f) &&
                tx >= 2 && tx < m_gridWidth - 2 && ty >= 2 && ty < m_gridHeight - 2)
            {
                float stepX, stepY;
                int stepLimit;
                
                if (std::abs(dx) > std::abs(dy))
                {
                    stepX = (dx < 0.0f) ? 1.0f : -1.0f;
                    stepY = -dy / std::abs(dx);
                    stepLimit = (int)(std::abs(dx * m_airAdvectionMult));
                }
                else
                {
                    stepY = (dy < 0.0f) ? 1.0f : -1.0f;
                    stepX = -dx / std::abs(dy);
                    stepLimit = (int)(std::abs(dy * m_airAdvectionMult));
                }
                
                float checkX = (float)x;
                float checkY = (float)y;
                
                int step = 0;
                for (; step < stepLimit; ++step)
                {
                    checkX += stepX;
                    checkY += stepY;
                    
                    if (m_blockAir[gridIdx((int)(checkX + 0.5f), (int)(checkY + 0.5f))])
                    {
                        checkX -= stepX;
                        checkY -= stepY;
                        break;
                    }
                }
                
                if (step == stepLimit)
                {
                    tx = x - dx * m_airAdvectionMult;
                    ty = y - dy * m_airAdvectionMult;
                }
                else
                {
                    tx = checkX;
                    ty = checkY;
                }
            }
            
            // Bilinear interpolation
            int i = (int)tx;  // X coordinate
            int j = (int)ty;  // Y coordinate
            tx -= i;
            ty -= j;
            
            if (i >= 2 && i < m_gridWidth - 3 && j >= 2 && j < m_gridHeight - 3)
            {
                const float advWeight = 0.3f; // Advection weight
                dp *= (1.0f - advWeight);
                dp += advWeight * (
                    (1.0f - tx) * (1.0f - ty) * m_airPressure[gridIdx(i, j)] +
                    tx * (1.0f - ty) * m_airPressure[gridIdx(i + 1, j)] +
                    (1.0f - tx) * ty * m_airPressure[gridIdx(i, j + 1)] +
                    tx * ty * m_airPressure[gridIdx(i + 1, j + 1)]
                );
                
                dx *= (1.0f - advWeight);
                dy *= (1.0f - advWeight);
                dx += advWeight * (
                    (1.0f - tx) * (1.0f - ty) * m_airVelocityX[gridIdx(i, j)] +
                    tx * (1.0f - ty) * m_airVelocityX[gridIdx(i + 1, j)] +
                    (1.0f - tx) * ty * m_airVelocityX[gridIdx(i, j + 1)] +
                    tx * ty * m_airVelocityX[gridIdx(i + 1, j + 1)]
                );
                dy += advWeight * (
                    (1.0f - tx) * (1.0f - ty) * m_airVelocityY[gridIdx(i, j)] +
                    tx * (1.0f - ty) * m_airVelocityY[gridIdx(i + 1, j)] +
                    (1.0f - tx) * ty * m_airVelocityY[gridIdx(i, j + 1)] +
                    tx * ty * m_airVelocityY[gridIdx(i + 1, j + 1)]
                );
            }
            
            // Vorticity confinement (adds swirling motion)
            if (m_airVorticityCoeff > 0.0f && x > 1 && x < m_gridWidth - 2 && y > 1 && y < m_gridHeight - 2)
            {
                float dwx = (std::abs(vorticity(x + 1, y)) - std::abs(vorticity(x - 1, y))) * 0.5f;
                float dwy = (std::abs(vorticity(x, y + 1)) - std::abs(vorticity(x, y - 1))) * 0.5f;
                float norm = std::sqrt(dwx * dwx + dwy * dwy);
                float w = vorticity(x, y);
                
                if (norm > 0.001f)
                {
                    dx += m_airVorticityCoeff / 5.0f * dwy / norm * w;
                    dy += m_airVorticityCoeff / 5.0f * (-dwx) / norm * w;
                }
            }
            
            // Clamp values
            const float maxPressure = 256.0f;
            const float minPressure = -256.0f;
            dp = std::clamp(dp, minPressure, maxPressure);
            dx = std::clamp(dx, minPressure, maxPressure);
            dy = std::clamp(dy, minPressure, maxPressure);
            
            // Apply edge damping to prevent velocity accumulation at boundaries
            // Gradually reduce velocity as we approach edges
            const int edgeDampingWidth = 5; // Cells from edge to apply damping
            float edgeDamping = 1.0f;
            
            // Calculate distance to nearest edge
            int distToLeft = x;
            int distToRight = m_gridWidth - 1 - x;
            int distToBottom = y;
            int distToTop = m_gridHeight - 1 - y;
            int minDistToEdge = std::min({distToLeft, distToRight, distToBottom, distToTop});
            
            if (minDistToEdge < edgeDampingWidth)
            {
                // Apply stronger damping near edges (0.0 at edge, 1.0 at edgeDampingWidth)
                edgeDamping = (float)minDistToEdge / (float)edgeDampingWidth;
                edgeDamping = std::max(0.0f, edgeDamping); // Clamp to 0-1
                
                // Apply damping to velocity (pressure can stay, but velocity should be reduced)
                dx *= edgeDamping;
                dy *= edgeDamping;
            }
            
            // Reset absolute edge cells to zero (boundary conditions)
            if (x == 0 || x == m_gridWidth - 1 || y == 0 || y == m_gridHeight - 1)
            {
                dx = 0.0f;
                dy = 0.0f;
                dp = 0.0f;
            }
            
            m_airPressureNext[gridIdx(x, y)] = dp;
            m_airVelocityXNext[gridIdx(x, y)] = dx;
            m_airVelocityYNext[gridIdx(x, y)] = dy;
        }
    }
    
    // Don't swap buffers here - let updateAirSystem handle it after convection
}

void PowderScene::updateAirVelocity(float dt)
{
    // Apply heat convection to velocity (hot air rises, cold air sinks)
    // This is done after pressure update but before swapping
    for (int y = 2; y < m_gridHeight - 2; ++y)
    {
        for (int x = 2; x < m_gridWidth - 2; ++x)
        {
            if (m_blockAir[gridIdx(x, y)])
                continue;
            
            float convGravX = 0.0f;
            float convGravY = -1.0f; // Gravity points down (Y-up, so negative Y)
            
            // Cap gravity magnitude
            float gravMagn = std::sqrt(convGravX * convGravX + convGravY * convGravY);
            if (gravMagn > 10.0f)
            {
                convGravX /= 0.1f * gravMagn;
                convGravY /= 0.1f * gravMagn;
            }
            
            float weight = (m_airHeat[gridIdx(x, y)] - m_ambientAirTemp) / 10000.0f;
            weight = std::clamp(weight, -0.01f, 0.01f); // Cap for stability
            
            // Add convection to velocity (hot air rises)
            m_airVelocityXNext[gridIdx(x, y)] += weight * convGravX * m_airHeatConvection;
            m_airVelocityYNext[gridIdx(x, y)] += weight * convGravY * m_airHeatConvection;
        }
    }
}

void PowderScene::updateAirHeat(float dt)
{
    // Copy current heat to next buffer
    std::copy(m_airHeat.begin(), m_airHeat.end(), m_airHeatNext.begin());
    
    // Set ambient temperature on edges
    for (int i = 0; i < m_gridHeight; ++i)
    {
        m_airHeatNext[gridIdx(0, i)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(1, i)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(m_gridWidth - 2, i)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(m_gridWidth - 1, i)] = m_ambientAirTemp;
    }
    
    for (int i = 0; i < m_gridWidth; ++i)
    {
        m_airHeatNext[gridIdx(i, 0)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(i, 1)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(i, m_gridHeight - 2)] = m_ambientAirTemp;
        m_airHeatNext[gridIdx(i, m_gridHeight - 1)] = m_ambientAirTemp;
    }
    
    // Update heat with advection and convection
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            if (m_blockAirHeat[gridIdx(x, y)] & 0x8)
                continue;
            
            // Smooth using kernel
            float dh = 0.0f;
            float dx = 0.0f;
            float dy = 0.0f;
            
            for (int j = -1; j <= 1; ++j)
            {
                for (int i = -1; i <= 1; ++i)
                {
                    int nx = x + i;
                    int ny = y + j;
                    
                    if (nx > 0 && nx < m_gridWidth - 1 && ny > 0 && ny < m_gridHeight - 1 && !(m_blockAirHeat[gridIdx(nx, ny)] & 0x8))
                    {
                        float f = m_airKernel[(i + 1) + 3 * (j + 1)];
                        dh += m_airHeat[gridIdx(nx, ny)] * f;
                        dx += m_airVelocityX[gridIdx(nx, ny)] * f;
                        dy += m_airVelocityY[gridIdx(nx, ny)] * f;
                    }
                    else
                    {
                        float f = m_airKernel[(i + 1) + 3 * (j + 1)];
                        dh += m_airHeat[gridIdx(x, y)] * f;
                        dx += m_airVelocityX[gridIdx(x, y)] * f;
                        dy += m_airVelocityY[gridIdx(x, y)] * f;
                    }
                }
            }
            
            // Advection: take heat from position moved by velocity
            float tx = x - dx * m_airAdvectionMult;
            float ty = y - dy * m_airAdvectionMult;
            
            // Check for walls in path
            if ((std::abs(dx * m_airAdvectionMult) > 1.0f || std::abs(dy * m_airAdvectionMult) > 1.0f) &&
                tx >= 2 && tx < m_gridWidth - 2 && ty >= 2 && ty < m_gridHeight - 2)
            {
                float stepX, stepY;
                int stepLimit;
                
                if (std::abs(dx) > std::abs(dy))
                {
                    stepX = (dx < 0.0f) ? 1.0f : -1.0f;
                    stepY = -dy / std::abs(dx);
                    stepLimit = (int)(std::abs(dx * m_airAdvectionMult));
                }
                else
                {
                    stepY = (dy < 0.0f) ? 1.0f : -1.0f;
                    stepX = -dx / std::abs(dy);
                    stepLimit = (int)(std::abs(dy * m_airAdvectionMult));
                }
                
                float checkX = (float)x;
                float checkY = (float)y;
                
                int step = 0;
                for (; step < stepLimit; ++step)
                {
                    checkX += stepX;
                    checkY += stepY;
                    
                    if (m_blockAirHeat[gridIdx((int)(checkX + 0.5f), (int)(checkY + 0.5f))] & 0x8)
                    {
                        checkX -= stepX;
                        checkY -= stepY;
                        break;
                    }
                }
                
                if (step == stepLimit)
                {
                    tx = x - dx * m_airAdvectionMult;
                    ty = y - dy * m_airAdvectionMult;
                }
                else
                {
                    tx = checkX;
                    ty = checkY;
                }
            }
            
            // Bilinear interpolation
            int i = (int)tx;  // X coordinate
            int j = (int)ty;  // Y coordinate
            tx -= i;
            ty -= j;
            
            if (i >= 0 && i < m_gridWidth - 1 && j >= 0 && j < m_gridHeight - 1)
            {
                float odh = dh;
                const float advWeight = 0.3f;
                dh *= (1.0f - advWeight);
                dh += advWeight * (
                    (1.0f - tx) * (1.0f - ty) * ((m_blockAirHeat[gridIdx(i, j)] & 0x8) ? odh : m_airHeat[gridIdx(i, j)]) +
                    tx * (1.0f - ty) * ((m_blockAirHeat[gridIdx(i + 1, j)] & 0x8) ? odh : m_airHeat[gridIdx(i + 1, j)]) +
                    (1.0f - tx) * ty * ((m_blockAirHeat[gridIdx(i, j + 1)] & 0x8) ? odh : m_airHeat[gridIdx(i, j + 1)]) +
                    tx * ty * ((m_blockAirHeat[gridIdx(i + 1, j + 1)] & 0x8) ? odh : m_airHeat[gridIdx(i + 1, j + 1)])
                );
            }
            
            // Air convection is handled in updateAirVelocity after heat update
            
            // Clamp temperature
            const float maxTemp = 373.15f + 1000.0f; // Max temp in Kelvin
            const float minTemp = 173.15f; // Min temp in Kelvin
            dh = std::clamp(dh, minTemp, maxTemp);
            
            m_airHeatNext[gridIdx(x, y)] = dh;
        }
    }
    
    // Swap buffers
    m_airHeat.swap(m_airHeatNext);
}

void PowderScene::clearGrid()
{
    for (auto& cell : m_grid)
    {
        cell.type = ParticleType::Empty;
        cell.updated = false;
        cell.life = 0;
        cell.temperature = 273.15f + 22.0f; // Reset to ambient temperature
    }
    
    // Also clear air system
    if (m_airEnabled)
    {
        clearAirSystem();
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

    // Mouse interaction - use current tool
    if (input.isMouseDown(MouseClick::LeftMouse))
    {
        Vec2 mouseWorld = getMouseWorldPosition();
        
        switch (m_currentTool)
        {
        case ToolType::DropParticles:
        {
            // Drop particles
            m_emitAccumulator += m_emitRate * dt;
            int toEmit = (int)m_emitAccumulator;
            if (toEmit > 0)
            {
                addParticlesAt(mouseWorld, m_currentParticleType, m_brushRadius);
                m_emitAccumulator -= toEmit;
            }
            break;
        }
        
        case ToolType::AddImpulse:
        {
            // Add air impulse continuously while holding
            if (m_airEnabled)
            {
                static float impulseAccumulator = 0.0f;
                impulseAccumulator += dt;
                if (impulseAccumulator >= 0.05f) // Limit to 20 impulses per second
                {
                    createAirImpulse(mouseWorld, m_impulseStrength, m_brushRadius);
                    impulseAccumulator = 0.0f;
                }
            }
            break;
        }
        
        case ToolType::Clear:
        {
            // Clear particles
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
            break;
        }
        }
    }
    else
    {
        m_emitAccumulator = 0.0f;
    }

    // Right mouse button always clears particles
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
        // Update blocking maps based on current particle positions
        if (m_airEnabled)
        {
            updateBlockAirMaps();
        }
        
        // Update air system (pressure, velocity, heat)
        if (m_airEnabled)
        {
            updateAirSystem(h);
        }
        
        // Update particle positions and interactions
        updateGrid(h);
    }
}

void PowderScene::updateGrid(float dt)
{
    // Copy grid to next frame (for double buffering)
    // We read from m_grid (old state) and write to m_gridNext (new state)
    std::copy(m_grid.begin(), m_grid.end(), m_gridNext.begin());

    // Clear updated flags in new grid (preserve life values from copy)
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
    dst.life = src.life; // Preserve life value
    dst.temperature = src.temperature; // Preserve temperature
    dst.updated = true;

    // Clear source in new grid (particle has moved)
    Cell& srcNew = m_gridNext[gridIdx(x, y)];
    srcNew.type = ParticleType::Empty;
    srcNew.life = 0;
    srcNew.temperature = 273.15f + 22.0f; // Reset to ambient temperature

    // Add particle movement to air (particles push air when they move)
    if (m_airEnabled)
    {
        const ParticleProperties& props = getParticleProperties(src.type);
        if (props.matterState != MatterState::Solid)
        {
            addParticleMovementToAir(x, y, newX, newY, props);
        }
    }

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
    srcNew.life = dstOld.life; // Preserve life values
    srcNew.temperature = dstOld.temperature; // Preserve temperature
    srcNew.updated = true;

    dstNew.type = srcOld.type;
    dstNew.life = srcOld.life; // Preserve life values
    dstNew.temperature = srcOld.temperature; // Preserve temperature
    dstNew.updated = true;

    // Add particle swap movement to air (particles push air when they swap)
    if (m_airEnabled)
    {
        // Calculate swap direction (source moves to destination, destination moves to source)
        int srcDx = newX - x;
        int srcDy = newY - y;
        int dstDx = x - newX;
        int dstDy = y - newY;

        const ParticleProperties& srcProps = getParticleProperties(srcOld.type);
        const ParticleProperties& dstProps = getParticleProperties(dstOld.type);
        
        // Add movement for source particle
        if (srcProps.matterState != MatterState::Solid)
        {
            addParticleMovementToAir(x, y, newX, newY, srcProps);
        }
        
        // Add movement for destination particle
        if (dstProps.matterState != MatterState::Solid)
        {
            addParticleMovementToAir(newX, newY, x, y, dstProps);
        }
    }

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

    // Metal: Solid state - does not move via non-chemical reactions (formerly Stone)
    ParticleProperties metal;
    metal.matterState = MatterState::Solid;
    metal.density = 10.0f;
    metal.color = Vec4(0.3f, 0.3f, 0.3f, 1.0f); // Darker gray metal
    m_particleProperties[ParticleType::Metal] = metal;

    // Stone: Powder state - falls down like sand, melts into lava when hot
    ParticleProperties stone;
    stone.matterState = MatterState::Powder;
    stone.density = 4.0f; // Denser than lava (stone sinks in lava)
    stone.color = Vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray stone
    stone.flammable = false; // Stone cannot ignite
    m_particleProperties[ParticleType::Stone] = stone;

    // Lava: Liquid state - hot molten rock, converts other particles
    ParticleProperties lava;
    lava.matterState = MatterState::Liquid;
    lava.density = 3.0f; // Same density as sand (so they don't sink through each other)
    lava.color = Vec4(1.0f, 0.6f, 0.0f, 1.0f); // More orange lava
    lava.movementChance = 0.3f; // More viscous - moves horizontally less often
    lava.flammable = false; // Lava doesn't burn
    lava.burnTemp = 1500.0f; // Lava is very hot
    m_particleProperties[ParticleType::Lava] = lava;

    // Wood: Solid state - does not move via non-chemical reactions
    ParticleProperties wood;
    wood.matterState = MatterState::Solid;
    wood.density = 0.8f;
    wood.color = Vec4(0.4f, 0.25f, 0.1f, 1.0f); // Brown wood
    wood.flammable = true; // Wood can burn
    wood.ignitionTemp = 573.15f; // ~300°C ignition temperature
    wood.burnTemp = 1200.0f; // Hot when burning
    m_particleProperties[ParticleType::Wood] = wood;

    // Gas: Gas state - moves opposite to gravity (upward), repels from other gas particles
    ParticleProperties gas;
    gas.matterState = MatterState::Gas;
    gas.density = 0.1f; // Very light
    gas.color = Vec4(204.0f / 255.0f, 153.0f / 255.0f, 153.0f / 255.0f, 1.0f); // Light pink/rose, fully opaque
    gas.movementChance = 0.3f; // 30% chance to move each frame (makes gas move slower)
    gas.flammable = true; // Gas can burn
    gas.ignitionTemp = 473.15f; // ~200°C ignition temperature
    gas.burnTemp = 1500.0f; // Very hot when burning
    m_particleProperties[ParticleType::Gas] = gas;

    // Acid: Liquid state - corrodes almost everything, has life value
    ParticleProperties acid;
    acid.matterState = MatterState::Liquid;
    acid.density = 1.2f; // Slightly denser than water
    acid.color = Vec4(204.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 1.0f); // CCFF00 - bright yellow-green
    m_particleProperties[ParticleType::Acid] = acid;

    // Fire: Gas state - rises up, heats air, ignites flammable particles
    ParticleProperties fire;
    fire.matterState = MatterState::Gas;
    fire.density = 0.05f; // Very light, rises quickly
    fire.color = Vec4(1.0f, 0.3f, 0.0f, 1.0f); // Orange-red fire
    fire.movementChance = 0.5f; // 50% chance to move
    fire.flammable = false; // Fire doesn't ignite itself
    fire.burnTemp = 1500.0f; // Fire is very hot
    m_particleProperties[ParticleType::Fire] = fire;

    // Smoke: Gas state - rises up, dissipates over time
    ParticleProperties smoke;
    smoke.matterState = MatterState::Gas;
    smoke.density = 0.2f; // Heavier than fire but lighter than air
    smoke.color = Vec4(0.2f, 0.2f, 0.2f, 0.8f); // Dark gray smoke
    smoke.movementChance = 0.4f; // 40% chance to move
    smoke.flammable = false; // Smoke doesn't burn
    m_particleProperties[ParticleType::Smoke] = smoke;

    // Steam: Gas state - rises up, condenses back to water when cool
    ParticleProperties steam;
    steam.matterState = MatterState::Gas;
    steam.density = 0.15f; // Very light, rises quickly
    steam.color = Vec4(0.9f, 0.9f, 0.95f, 0.7f); // Light white/blue steam
    steam.movementChance = 0.5f; // 50% chance to move
    steam.flammable = false; // Steam doesn't burn
    m_particleProperties[ParticleType::Steam] = steam;

    // Mud: Liquid state - very viscous, created from sand and water
    ParticleProperties mud;
    mud.matterState = MatterState::Liquid;
    mud.density = 2.5f; // More dense than water, lava, and acid, but less than powders
    mud.color = Vec4(0.4f, 0.3f, 0.2f, 1.0f); // Brown mud color
    mud.movementChance = 0.15f; // Very viscous - moves horizontally less often than lava
    mud.flammable = false; // Mud doesn't burn
    m_particleProperties[ParticleType::Mud] = mud;

    // Oil: Liquid state - less dense than water, more viscous than water but less than lava, flammable
    ParticleProperties oil;
    oil.matterState = MatterState::Liquid;
    oil.density = 0.8f; // Less dense than water (oil floats on water)
    oil.color = Vec4(0.1f, 0.1f, 0.1f, 1.0f); // Dark/black oil color
    oil.movementChance = 0.5f; // More viscous than water, but less than lava
    oil.flammable = true; // Oil can burn
    oil.ignitionTemp = 473.15f; // ~200°C ignition temperature (similar to gas)
    oil.burnTemp = 1200.0f; // Hot when burning
    m_particleProperties[ParticleType::Oil] = oil;
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

bool PowderScene::canCorrode(ParticleType type) const
{
    // Acid can corrode almost everything except itself and empty
    // In the future, we can add particles that resist corrosion
    if (type == ParticleType::Empty || type == ParticleType::Acid)
        return false;
    
    // For now, acid corrodes everything else
    // TODO: Add corrosion-resistant particles in the future
    return true;
}

void PowderScene::updateFireAndIgnition(int x, int y, float dt)
{
    const Cell& oldCell = m_grid[gridIdx(x, y)];
    Cell& newCell = m_gridNext[gridIdx(x, y)];
    
    // Handle fire particles
    if (oldCell.type == ParticleType::Fire)
    {
        // Fire heats up air and creates upward draft/impulse
        if (m_airEnabled)
        {
            const ParticleProperties& fireProps = getParticleProperties(ParticleType::Fire);
            int idx = gridIdx(x, y);
            
            // Heat up air at fire location
            float heatIncrease = fireProps.burnTemp - m_ambientAirTemp;
            m_airHeatNext[idx] += heatIncrease * 0.1f * dt; // Gradually heat up
            m_airHeatNext[idx] = std::min(m_airHeatNext[idx], fireProps.burnTemp);
            
            // Fire creates strong upward impulse (like a draft) that affects particles
            // Create an upward impulse in a radius around the fire
            const float fireImpulseStrength = 15.0f; // Strong upward impulse
            const int fireImpulseRadius = 3; // Radius in cells
            
            // Add upward impulse to air system in a radius around fire
            for (int dy = -fireImpulseRadius; dy <= fireImpulseRadius; ++dy)
            {
                for (int dx = -fireImpulseRadius; dx <= fireImpulseRadius; ++dx)
                {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (!isValidGridPos(nx, ny))
                        continue;
                    
                    // Skip if blocked by solid
                    if (m_blockAir[gridIdx(nx, ny)])
                        continue;
                    
                    float dist2 = (float)(dx * dx + dy * dy);
                    float r2 = (float)(fireImpulseRadius * fireImpulseRadius);
                    
                    if (dist2 <= r2)
                    {
                        // Calculate distance from fire
                        float dist = std::sqrt(dist2);
                        if (dist < 0.001f) dist = 0.001f; // Avoid division by zero
                        
                        float normalizedDist = dist / (float)fireImpulseRadius;
                        if (normalizedDist > 1.0f) normalizedDist = 1.0f;
                        
                        // Strength falls off with distance (stronger at center, weaker at edges)
                        // But always upward
                        float impulseStrength = fireImpulseStrength * (1.0f - normalizedDist * 0.7f); // Keep some strength even at edges
                        
                        int nIdx = gridIdx(nx, ny);
                        
                        // Add upward velocity impulse (always upward, Y+ direction)
                        // Write to both current and next buffers for immediate effect
                        m_airVelocityY[nIdx] += impulseStrength * dt;
                        m_airVelocityYNext[nIdx] += impulseStrength * dt;
                        
                        // Add some pressure (creates upward push)
                        float pressureStrength = fireImpulseStrength * 0.3f * (1.0f - normalizedDist * 0.7f);
                        m_airPressure[nIdx] += pressureStrength * dt;
                        m_airPressureNext[nIdx] += pressureStrength * dt;
                    }
                }
            }
        }
        
        // Fire tries to ignite flammable neighbors (prefer upward)
        // Check upward neighbors first, then others
        int directions[][2] = {{0, 1}, {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}};
        for (int i = 0; i < 8; ++i)
        {
            int dx = directions[i][0];
            int dy = directions[i][1];
            // Prefer upward directions - check them more often
            if (dy > 0 && rand() % 3 != 0) // 2/3 chance to check upward neighbors
            {
                tryIgniteNeighbor(x, y, dx, dy);
            }
            else if (dy <= 0 && rand() % 5 == 0) // 1/5 chance to check horizontal/downward neighbors
            {
                tryIgniteNeighbor(x, y, dx, dy);
            }
        }
        
        // Fire dries mud -> turns mud into sand
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Mud)
                    {
                        // Convert mud to sand (fire dries mud)
                        Cell& mudNew = m_gridNext[gridIdx(x + dx, y + dy)];
                        mudNew.type = ParticleType::Sand;
                        mudNew.life = 0;
                        mudNew.temperature = neighbor.temperature; // Preserve temperature
                    }
                }
            }
        }
        
        // Fire has a lifetime (decreases over time)
        int currentLife = oldCell.life;
        if (currentLife <= 0)
        {
            currentLife = 100; // Initial fire lifetime (in frames)
        }
        
        currentLife--;
        newCell.life = currentLife;
        
        // Fire dies out when lifetime reaches zero
        if (currentLife <= 0)
        {
            newCell.type = ParticleType::Empty;
            newCell.life = 0;
            newCell.temperature = m_ambientAirTemp;
            return;
        }
        
        // Fire moves like gas but with strong upward preference
        const ParticleProperties& fireProps = getParticleProperties(ParticleType::Fire);
        // Fire has strong upward momentum (preferredDirY = 1 means upward in Y-up coordinate system)
        int preferredDirX = 0;
        int preferredDirY = 1; // Strong upward preference for fire
        if (m_airEnabled)
        {
            // Also check air forces, but fire's upward momentum is stronger
            applyAirForcesToParticle(x, y, fireProps, preferredDirX, preferredDirY);
            // Fire always prefers upward, so ensure Y direction is upward
            if (preferredDirY < 1)
                preferredDirY = 1;
        }
        updateGas(x, y, fireProps, preferredDirX, preferredDirY);
        return;
    }
    
    // Handle smoke particles
    if (oldCell.type == ParticleType::Smoke)
    {
        // Smoke has a lifetime (decreases over time)
        int currentLife = oldCell.life;
        if (currentLife <= 0)
        {
            currentLife = 200; // Initial smoke lifetime (in frames)
        }
        
        currentLife--;
        newCell.life = currentLife;
        
        // Smoke dissipates when lifetime reaches zero
        if (currentLife <= 0)
        {
            newCell.type = ParticleType::Empty;
            newCell.life = 0;
            newCell.temperature = m_ambientAirTemp;
            return;
        }
        
        // Apply air forces to smoke (smoke is very light and should be strongly affected by air)
        const ParticleProperties& smokeProps = getParticleProperties(ParticleType::Smoke);
        int preferredDirX = 0;
        int preferredDirY = 0;
        if (m_airEnabled)
        {
            applyAirForcesToParticle(x, y, smokeProps, preferredDirX, preferredDirY);
        }
        
        // Smoke moves like gas (rises up) but also reacts to air
        updateGas(x, y, smokeProps, preferredDirX, preferredDirY);
        return;
    }
    
    // Handle steam particles
    if (oldCell.type == ParticleType::Steam)
    {
        Cell& newCell = m_gridNext[gridIdx(x, y)];
        
        // Steam tries to reach ambient temperature (cools slowly)
        float& temp = newCell.temperature;
        if (m_airEnabled)
        {
            // Steam temperature moves toward ambient air temperature (slowly)
            float airTemp = m_airHeat[gridIdx(x, y)];
            float tempDiff = airTemp - temp;
            temp += tempDiff * 0.02f * dt; // Cool much slower toward ambient temperature
        }
        else
        {
            // Without air system, move toward default ambient (slowly)
            float tempDiff = m_ambientAirTemp - temp;
            temp += tempDiff * 0.02f * dt; // Cool much slower
        }
        
        // Steam condenses back to water when it cools below boiling point
        const float boilingPoint = 373.15f; // 100°C (boiling point of water)
        if (temp < boilingPoint)
        {
            // Convert steam back to water
            newCell.type = ParticleType::Water;
            newCell.life = 0;
            newCell.temperature = temp; // Preserve temperature
            return; // Don't move as gas, it's now water
        }
        
        // Apply air forces to steam (steam is very light and should be strongly affected by air)
        const ParticleProperties& steamProps = getParticleProperties(ParticleType::Steam);
        int preferredDirX = 0;
        int preferredDirY = 0;
        if (m_airEnabled)
        {
            applyAirForcesToParticle(x, y, steamProps, preferredDirX, preferredDirY);
        }
        
        // Steam moves like gas (rises up) but also reacts to air
        updateGas(x, y, steamProps, preferredDirX, preferredDirY);
        return;
    }
    
    // Check if this particle can be ignited by nearby fire
    const ParticleProperties& props = getParticleProperties(oldCell.type);
    if (props.flammable)
    {
        // Check all 8 adjacent cells for fire
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Fire)
                    {
                        // Fire is nearby, increase temperature
                        float& temp = newCell.temperature;
                        temp += (props.burnTemp - temp) * 0.1f * dt; // Heat up gradually
                        
                        // If temperature reaches ignition point, ignite!
                        if (temp >= props.ignitionTemp)
                        {
                            // This particle is now burning - create fire particles
                            createFireParticle(x, y);
                            
                            // Create smoke particles
                            createSmokeParticle(x, y);
                            
                            // Reduce particle life (it's burning away)
                            // Gas burns much faster than other materials
                            int currentLife = oldCell.life;
                            if (currentLife <= 0)
                            {
                                // Gas burns very quickly, wood burns slower
                                if (oldCell.type == ParticleType::Gas)
                                {
                                    currentLife = 10; // Gas burns quickly (10 frames)
                                }
                                else
                                {
                                    currentLife = 50; // Other materials burn slower
                                }
                            }
                            
                            // Gas burns faster (lose more life per frame)
                            if (oldCell.type == ParticleType::Gas)
                            {
                                currentLife -= 2; // Gas loses 2 life per frame
                            }
                            else
                            {
                                currentLife--; // Other materials lose 1 life per frame
                            }
                            
                            newCell.life = currentLife;
                            
                            // If life reaches zero, particle is consumed
                            if (currentLife <= 0)
                            {
                                newCell.type = ParticleType::Empty;
                                newCell.life = 0;
                                newCell.temperature = m_ambientAirTemp;
                            }
                        }
                        break; // Found fire, no need to check more
                    }
                }
            }
        }
    }
}

void PowderScene::tryIgniteNeighbor(int x, int y, int dx, int dy)
{
    if (!isValidGridPos(x + dx, y + dy))
        return;
    
    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
    if (neighbor.type == ParticleType::Empty || neighbor.type == ParticleType::Fire)
        return;
    
    const ParticleProperties& neighborProps = getParticleProperties(neighbor.type);
    if (!neighborProps.flammable)
        return;
    
    // Fire heats up the neighbor
    Cell& neighborNew = m_gridNext[gridIdx(x + dx, y + dy)];
    float& temp = neighborNew.temperature;
    const ParticleProperties& fireProps = getParticleProperties(ParticleType::Fire);
    temp += (fireProps.burnTemp - temp) * 0.2f; // Heat up faster when directly adjacent to fire
    
    // If temperature reaches ignition point, ignite!
    if (temp >= neighborProps.ignitionTemp)
    {
        // Create fire at neighbor location
        createFireParticle(x + dx, y + dy);
        
        // Create smoke
        createSmokeParticle(x + dx, y + dy);
        
        // Gas burns very quickly - reduce its life immediately
        if (neighbor.type == ParticleType::Gas)
        {
            int currentLife = neighborNew.life;
            if (currentLife <= 0)
            {
                currentLife = 10; // Gas burns quickly (10 frames)
            }
            currentLife -= 2; // Gas loses 2 life per frame when burning
            neighborNew.life = currentLife;
            
            // If life reaches zero, gas is consumed
            if (currentLife <= 0)
            {
                neighborNew.type = ParticleType::Empty;
                neighborNew.life = 0;
                neighborNew.temperature = m_ambientAirTemp;
            }
        }
    }
}

void PowderScene::createFireParticle(int x, int y)
{
    if (!isValidGridPos(x, y))
        return;
    
    // Try to create fire in an adjacent empty cell (strongly prefer upward)
    // Order: directly up, diagonal up, then horizontal, then down
    int directions[][2] = {{0, 1}, {-1, 1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}};
    
    // First pass: try upward directions only
    for (int i = 0; i < 3; ++i) // First 3 are upward directions
    {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];
        
        if (isValidGridPos(nx, ny))
        {
            Cell& cell = m_gridNext[gridIdx(nx, ny)];
            if (cell.type == ParticleType::Empty && !cell.updated)
            {
                cell.type = ParticleType::Fire;
                cell.life = 100; // Fire lifetime
                cell.temperature = 1500.0f; // Fire is hot
                cell.updated = true;
                return;
            }
        }
    }
    
    // Second pass: only try horizontal/downward if upward failed (and only rarely)
    if (rand() % 4 == 0) // 1/4 chance to spread horizontally/downward
    {
        for (int i = 3; i < 8; ++i) // Horizontal and downward directions
        {
            int nx = x + directions[i][0];
            int ny = y + directions[i][1];
            
            if (isValidGridPos(nx, ny))
            {
                Cell& cell = m_gridNext[gridIdx(nx, ny)];
                if (cell.type == ParticleType::Empty && !cell.updated)
                {
                    cell.type = ParticleType::Fire;
                    cell.life = 100; // Fire lifetime
                    cell.temperature = 1500.0f; // Fire is hot
                    cell.updated = true;
                    return;
                }
            }
        }
    }
}

void PowderScene::createSmokeParticle(int x, int y)
{
    if (!isValidGridPos(x, y))
        return;
    
    // Try to create smoke in an adjacent empty cell (prefer upward)
    int directions[][2] = {{0, 1}, {-1, 1}, {1, 1}, {-1, 0}, {1, 0}};
    
    // Random chance to create smoke (not every frame)
    if (rand() % 3 == 0) // 1/3 chance
    {
        for (int i = 0; i < 5; ++i)
        {
            int nx = x + directions[i][0];
            int ny = y + directions[i][1];
            
            if (isValidGridPos(nx, ny))
            {
                Cell& cell = m_gridNext[gridIdx(nx, ny)];
                if (cell.type == ParticleType::Empty && !cell.updated)
                {
                    cell.type = ParticleType::Smoke;
                    cell.life = 200; // Smoke lifetime
                    cell.temperature = m_ambientAirTemp;
                    cell.updated = true;
                    return;
                }
            }
        }
    }
}

void PowderScene::updateParticle(int x, int y, float dt)
{
    // Read particle type from old grid
    const Cell& oldCell = m_grid[gridIdx(x, y)];
    if (oldCell.type == ParticleType::Empty)
        return;

    // Get particle properties
    const ParticleProperties& props = getParticleProperties(oldCell.type);

    // Handle fire, smoke, steam, and lava first (they have special behavior)
    if (oldCell.type == ParticleType::Fire || oldCell.type == ParticleType::Smoke || oldCell.type == ParticleType::Steam || oldCell.type == ParticleType::Lava)
    {
        if (oldCell.type == ParticleType::Lava)
        {
            // Lava has special interactions - handle in updateLiquid
            // But we still need to check for fire ignition first
            updateFireAndIgnition(x, y, dt);
        }
        else
        {
            updateFireAndIgnition(x, y, dt);
            return;
        }
    }

    // Apply air forces to particle (ALL particles react to air, but strength depends on density/weight)
    int preferredDirX = 0;
    int preferredDirY = 0;
    if (m_airEnabled)
    {
        applyAirForcesToParticle(x, y, props, preferredDirX, preferredDirY);
    }
    
    // Check for fire ignition (all particles can burn)
    updateFireAndIgnition(x, y, dt);
    
    // Handle Metal heating and melting (solids can still heat up)
    if (oldCell.type == ParticleType::Metal)
    {
        Cell& newCell = m_gridNext[gridIdx(x, y)];
        float& temp = newCell.temperature;
        
        // Metal heats up from nearby fire, lava, or hot air
        if (m_airEnabled)
        {
            // Metal temperature moves toward ambient air temperature
            float airTemp = m_airHeat[gridIdx(x, y)];
            float tempDiff = airTemp - temp;
            temp += tempDiff * 0.05f * dt; // Metal heats up slowly
        }
        
        // Check all 8 adjacent cells for fire or lava (they heat metal quickly)
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Fire || neighbor.type == ParticleType::Lava)
                    {
                        // Fire/lava nearby heats metal quickly
                        const ParticleProperties& heatProps = getParticleProperties(neighbor.type);
                        float tempDiff = heatProps.burnTemp - temp;
                        temp += tempDiff * 0.7f * dt; // Heat up much faster near fire/lava
                    }
                }
            }
        }
        
        // Metal melts into lava at ~1200°C (1473.15K) - lower than lava temp so it can actually melt
        const float metalMeltingPoint = 1473.15f; // ~1200°C (same as stone/sand)
        if (temp >= metalMeltingPoint)
        {
            // Convert metal to lava
            newCell.type = ParticleType::Lava;
            newCell.life = 0;
            newCell.temperature = temp; // Preserve temperature
        }
    }
    
    // Skip solids for normal movement (they don't move via non-chemical reactions)
    if (props.matterState == MatterState::Solid)
    {
        // Solids can still be affected by air, but they don't move normally
        // They can be pushed by very strong air forces (handled separately if needed)
        return;
    }

    // Handle based on state of matter
    switch (props.matterState)
    {
    case MatterState::Powder:
        // Powder: Only fall via non-chemical reactions, but will not spread as liquids do
        // Stays in place unless a force acts upon them (gravity or air)
        updatePowder(x, y, props, dt, preferredDirX, preferredDirY);
        return;

    case MatterState::Liquid:
        // Liquid: Random horizontal movement each frame, then fall down
        // Can float above denser liquids/powders
        updateLiquid(x, y, props, dt, preferredDirX, preferredDirY);
        return;

    case MatterState::Gas:
        // Gas: Moves opposite to gravity (upward), repels from other gas particles
        updateGas(x, y, props, preferredDirX, preferredDirY);
        return;

    default:
        return;
    }
}

void PowderScene::applyAirForcesToParticle(int x, int y, const ParticleProperties& props, int& preferredDirX, int& preferredDirY)
{
    if (!m_airEnabled || !isValidGridPos(x, y))
        return;

    // Get air pressure and velocity at particle location
    float pressure = m_airPressure[gridIdx(x, y)];
    float vx = m_airVelocityX[gridIdx(x, y)];
    float vy = m_airVelocityY[gridIdx(x, y)];

    // Calculate air force strength based on particle density/weight
    // Lighter particles (low density) are affected more by air
    // Heavier particles (high density) are affected less by air
    // Use inverse density: 1.0 / density gives higher values for lighter particles
    const float baseAirSensitivity = 1.0f; // Base sensitivity multiplier
    float airSensitivity = baseAirSensitivity / std::max(props.density, 0.1f); // Inverse density (clamp to avoid division by zero)
    
    // Clamp sensitivity to reasonable range (very light particles get max effect, very heavy get min effect)
    airSensitivity = std::clamp(airSensitivity, 0.1f, 10.0f);
    
    // Adjust thresholds based on density - lighter particles react to smaller forces
    float pressureThreshold = 0.5f / airSensitivity; // Lower threshold for lighter particles
    float velocityThreshold = 0.3f / airSensitivity; // Lower threshold for lighter particles

    // Calculate preferred direction from air pressure gradient
    float pressureX = 0.0f;
    float pressureY = 0.0f;
    
    if (x > 0 && x < m_gridWidth - 1)
    {
        pressureX = m_airPressure[gridIdx(x - 1, y)] - m_airPressure[gridIdx(x + 1, y)];
    }
    if (y > 0 && y < m_gridHeight - 1)
    {
        pressureY = m_airPressure[gridIdx(x, y - 1)] - m_airPressure[gridIdx(x, y + 1)];
    }

    // Combine pressure gradient and velocity to get preferred direction
    // Scale by air sensitivity - lighter particles are pushed more
    float forceX = (pressureX * 0.5f + vx) * airSensitivity;
    float forceY = (pressureY * 0.5f + vy) * airSensitivity;

    // Convert to preferred direction (normalize to -1, 0, or 1)
    // Use the adjusted thresholds
    if (std::abs(forceX) > pressureThreshold || std::abs(vx * airSensitivity) > velocityThreshold)
    {
        preferredDirX = (forceX > 0.0f) ? 1 : (forceX < 0.0f) ? -1 : 0;
    }
    if (std::abs(forceY) > pressureThreshold || std::abs(vy * airSensitivity) > velocityThreshold)
    {
        preferredDirY = (forceY > 0.0f) ? 1 : (forceY < 0.0f) ? -1 : 0;
    }
}

void PowderScene::addParticleMovementToAir(int x, int y, int newX, int newY, const ParticleProperties& props)
{
    if (!m_airEnabled || !isValidGridPos(x, y) || !isValidGridPos(newX, newY))
        return;

    // Calculate movement direction
    int dx = newX - x;
    int dy = newY - y;

    if (dx == 0 && dy == 0)
        return;

    // Particles push air when they move
    // Add velocity to air in the direction of movement
    const float particlePushStrength = 0.3f; // How much particles push air
    const float particlePressureStrength = 0.1f; // How much particles add pressure

    // Add velocity to air at both old and new positions
    if (isValidGridPos(x, y))
    {
        m_airVelocityXNext[gridIdx(x, y)] += dx * particlePushStrength;
        m_airVelocityYNext[gridIdx(x, y)] += dy * particlePushStrength;
        m_airPressureNext[gridIdx(x, y)] += particlePressureStrength;
    }

    if (isValidGridPos(newX, newY))
    {
        m_airVelocityXNext[gridIdx(newX, newY)] += dx * particlePushStrength;
        m_airVelocityYNext[gridIdx(newX, newY)] += dy * particlePushStrength;
        m_airPressureNext[gridIdx(newX, newY)] += particlePressureStrength;
    }
}

void PowderScene::createAirImpulse(const Vec2& worldPos, float strength, float radius)
{
    if (!m_airEnabled)
        return;

    Vec2 gridPos = worldToGrid(worldPos);
    int centerX = (int)std::floor(gridPos.x);
    int centerY = (int)std::floor(gridPos.y);
    int radiusCells = (int)(radius / m_cellSize);
    
    // Ensure minimum radius
    if (radiusCells < 1)
        radiusCells = 1;

    // Create a radial outward impulse (like an explosion)
    for (int dy = -radiusCells; dy <= radiusCells; ++dy)
    {
        for (int dx = -radiusCells; dx <= radiusCells; ++dx)
        {
            int x = centerX + dx;
            int y = centerY + dy;

            if (!isValidGridPos(x, y))
                continue;

            // Skip if blocked by solid
            if (m_blockAir[gridIdx(x, y)])
                continue;

            float dist2 = (float)(dx * dx + dy * dy);
            float r2 = (float)(radiusCells * radiusCells);

            if (dist2 <= r2 && dist2 > 0.0f)
            {
                // Calculate distance from center
                float dist = std::sqrt(dist2);
                if (dist < 0.001f) continue; // Avoid division by zero
                
                float normalizedDist = dist / (float)radiusCells;
                if (normalizedDist > 1.0f) normalizedDist = 1.0f;

                // Create radial outward velocity (particles pushed away from center)
                float dirX = dx / dist;
                float dirY = dy / dist;

                // Strength falls off with distance (stronger at center, weaker at edges)
                float impulseStrength = strength * (1.0f - normalizedDist);

                // Add velocity impulse (outward) - write to current buffers
                // These will be copied to next buffers in updateAirPressure, then updated
                int idx = gridIdx(x, y);
                m_airVelocityX[idx] += dirX * impulseStrength;
                m_airVelocityY[idx] += dirY * impulseStrength;

                // Add pressure impulse (positive pressure at center)
                float pressureStrength = strength * 0.5f * (1.0f - normalizedDist);
                m_airPressure[idx] += pressureStrength;
                
                // Also update next buffers directly so it's visible immediately
                // (in case we're paused or between updates)
                m_airVelocityXNext[idx] += dirX * impulseStrength;
                m_airVelocityYNext[idx] += dirY * impulseStrength;
                m_airPressureNext[idx] += pressureStrength;
            }
        }
    }
}

void PowderScene::updatePowder(int x, int y, const ParticleProperties& props, float dt, int preferredDirX, int preferredDirY)
{
    // Read particle type from old grid
    const Cell& oldCell = m_grid[gridIdx(x, y)];
    Cell& newCell = m_gridNext[gridIdx(x, y)];
    
    // Handle Sand+Water -> Mud conversion
    if (oldCell.type == ParticleType::Sand)
    {
        // Check all 8 adjacent cells for water
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Water)
                    {
                        // Convert sand to mud
                        newCell.type = ParticleType::Mud;
                        newCell.life = 0;
                        newCell.temperature = oldCell.temperature; // Preserve temperature
                        
                        // Convert water to mud as well
                        Cell& waterNew = m_gridNext[gridIdx(x + dx, y + dy)];
                        waterNew.type = ParticleType::Mud;
                        waterNew.life = 0;
                        waterNew.temperature = neighbor.temperature; // Preserve temperature
                        return; // Don't move as powder, it's now mud
                    }
                }
            }
        }
    }
    
    // Handle Stone->Lava conversion (stone melts when hot)
    if (oldCell.type == ParticleType::Stone)
    {
        float& temp = newCell.temperature;
        
        // Stone heats up from nearby fire, lava, or hot air
        if (m_airEnabled)
        {
            // Stone temperature moves toward ambient air temperature
            float airTemp = m_airHeat[gridIdx(x, y)];
            float tempDiff = airTemp - temp;
            temp += tempDiff * 0.05f * dt; // Stone heats up slowly
        }
        
        // Check all 8 adjacent cells for fire or lava (they heat stone quickly)
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Fire || neighbor.type == ParticleType::Lava)
                    {
                        // Fire/lava nearby heats stone quickly
                        const ParticleProperties& heatProps = getParticleProperties(neighbor.type);
                        float tempDiff = heatProps.burnTemp - temp;
                        temp += tempDiff * 0.3f * dt; // Heat up faster near fire/lava
                    }
                }
            }
        }
        
        // Stone melts into lava at ~1200°C (1473.15K)
        const float meltingPoint = 1473.15f; // ~1200°C
        if (temp >= meltingPoint)
        {
            // Convert stone to lava
            newCell.type = ParticleType::Lava;
            newCell.life = 0;
            newCell.temperature = temp; // Preserve temperature
            return; // Don't move as powder, it's now lava
        }
    }
    
    // Powder falls down due to gravity (Y-up, so down is Y-1)
    // But can also be pushed by air
    
    // If air is pushing, try to move in that direction first (if not too strong against gravity)
    if (preferredDirX != 0 && std::abs(preferredDirY) <= 1)
    {
        // Try horizontal movement from air
        if (tryMove(x, y, x + preferredDirX, y))
            return;
    }
    
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

void PowderScene::updateLiquid(int x, int y, const ParticleProperties& props, float dt, int preferredDirX, int preferredDirY)
{
    // Liquid: Each individual dot randomly moves left, right, or stays in place horizontally every frame
    // Then falls down. In cellular automata, each particle can only move once per frame,
    // so we prioritize horizontal movement first, then falling if horizontal didn't happen.
    
    // Read particle type from old grid
    const Cell& oldCell = m_grid[gridIdx(x, y)];
    Cell& newCell = m_gridNext[gridIdx(x, y)];
    
    // Handle Lava interactions (converts other particles)
    if (oldCell.type == ParticleType::Lava)
    {
        // Lava heats up air
        if (m_airEnabled)
        {
            const ParticleProperties& lavaProps = getParticleProperties(ParticleType::Lava);
            int idx = gridIdx(x, y);
            float heatIncrease = lavaProps.burnTemp - m_ambientAirTemp;
            m_airHeatNext[idx] += heatIncrease * 0.1f * dt; // Gradually heat up air
            m_airHeatNext[idx] = std::min(m_airHeatNext[idx], lavaProps.burnTemp);
        }
        
        // Check all 8 adjacent cells for particles to convert/ignite
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    Cell& neighborNew = m_gridNext[gridIdx(x + dx, y + dy)];
                    
                    // Sand touches lava -> heats up (converts to lava over time)
                    if (neighbor.type == ParticleType::Sand)
                    {
                        float& temp = neighborNew.temperature;
                        // Heat up sand when touching lava
                        const ParticleProperties& lavaProps = getParticleProperties(ParticleType::Lava);
                        float tempDiff = lavaProps.burnTemp - temp;
                        temp += tempDiff * 0.4f * dt; // Heat up much faster
                        
                        // Sand melts into lava at ~1200°C (1473.15K) - same as stone
                        const float meltingPoint = 1473.15f; // ~1200°C
                        if (temp >= meltingPoint)
                        {
                            neighborNew.type = ParticleType::Lava;
                            neighborNew.life = 0;
                            neighborNew.temperature = temp; // Preserve temperature
                        }
                        continue;
                    }
                    
                    // Metal touches lava -> melts to lava
                    if (neighbor.type == ParticleType::Metal)
                    {
                        float& temp = neighborNew.temperature;
                        // Heat up metal quickly when touching lava
                        const ParticleProperties& lavaProps = getParticleProperties(ParticleType::Lava);
                        float tempDiff = lavaProps.burnTemp - temp;
                        temp += tempDiff * 0.8f * dt; // Heat up much faster
                        
                        // Metal melts into lava at ~1200°C (1473.15K) - lower than lava temp so it can actually melt
                        const float metalMeltingPoint = 1473.15f; // ~1200°C (same as stone/sand)
                        if (temp >= metalMeltingPoint)
                        {
                            neighborNew.type = ParticleType::Lava;
                            neighborNew.life = 0;
                            neighborNew.temperature = temp; // Preserve temperature
                        }
                        continue;
                    }
                    
                    // Water touches lava -> water turns to steam, lava turns to stone
                    if (neighbor.type == ParticleType::Water)
                    {
                        neighborNew.type = ParticleType::Steam;
                        neighborNew.life = 0;
                        neighborNew.temperature = 373.15f + 50.0f; // Hot steam
                        
                        // Convert the lava that touched water into stone
                        Cell& lavaNew = m_gridNext[gridIdx(x, y)];
                        lavaNew.type = ParticleType::Stone;
                        lavaNew.life = 0;
                        lavaNew.temperature = 273.15f + 22.0f; // Reset to ambient temperature
                        continue;
                    }
                    
                    // Mud touches lava -> mud turns to sand
                    if (neighbor.type == ParticleType::Mud)
                    {
                        neighborNew.type = ParticleType::Sand;
                        neighborNew.life = 0;
                        neighborNew.temperature = neighbor.temperature; // Preserve temperature
                        continue;
                    }
                    
                    // Oil touches lava -> ignites immediately
                    if (neighbor.type == ParticleType::Oil)
                    {
                        const ParticleProperties& oilProps = getParticleProperties(ParticleType::Oil);
                        float& temp = neighborNew.temperature;
                        temp = oilProps.ignitionTemp + 10.0f; // Set above ignition temperature to ensure ignition
                        
                        // Immediately create fire at oil location
                        createFireParticle(x + dx, y + dy);
                        
                        // Oil burns quickly
                        int currentLife = neighbor.life;
                        if (currentLife <= 0)
                        {
                            currentLife = 30; // Oil burns faster than wood
                        }
                        currentLife -= 2; // Oil loses 2 life per frame when burning
                        neighborNew.life = currentLife;
                        
                        // If life reaches zero, oil is consumed
                        if (currentLife <= 0)
                        {
                            neighborNew.type = ParticleType::Empty;
                            neighborNew.life = 0;
                            neighborNew.temperature = m_ambientAirTemp;
                        }
                        continue;
                    }
                    
                    // Wood touches lava -> ignites immediately
                    if (neighbor.type == ParticleType::Wood)
                    {
                        const ParticleProperties& woodProps = getParticleProperties(ParticleType::Wood);
                        float& temp = neighborNew.temperature;
                        temp = woodProps.ignitionTemp + 10.0f; // Set above ignition temperature to ensure ignition
                        
                        // Immediately create fire and smoke at wood location
                        createFireParticle(x + dx, y + dy);
                        createSmokeParticle(x + dx, y + dy);
                        
                        // Start burning process
                        int currentLife = neighbor.life;
                        if (currentLife <= 0)
                        {
                            currentLife = 50; // Wood burns slower
                        }
                        currentLife--; // Start consuming wood
                        neighborNew.life = currentLife;
                        
                        // If life reaches zero, wood is consumed
                        if (currentLife <= 0)
                        {
                            neighborNew.type = ParticleType::Empty;
                            neighborNew.life = 0;
                            neighborNew.temperature = m_ambientAirTemp;
                        }
                        continue;
                    }
                    
                    // Gas touches lava -> ignites immediately
                    if (neighbor.type == ParticleType::Gas)
                    {
                        const ParticleProperties& gasProps = getParticleProperties(ParticleType::Gas);
                        float& temp = neighborNew.temperature;
                        temp = gasProps.ignitionTemp + 10.0f; // Set above ignition temperature to ensure ignition
                        
                        // Immediately create fire at gas location
                        createFireParticle(x + dx, y + dy);
                        
                        // Gas burns very quickly
                        int currentLife = neighbor.life;
                        if (currentLife <= 0)
                        {
                            currentLife = 10; // Gas burns quickly
                        }
                        currentLife -= 2; // Gas loses 2 life per frame when burning
                        neighborNew.life = currentLife;
                        
                        // If life reaches zero, gas is consumed
                        if (currentLife <= 0)
                        {
                            neighborNew.type = ParticleType::Empty;
                            neighborNew.life = 0;
                            neighborNew.temperature = m_ambientAirTemp;
                        }
                        continue;
                    }
                }
            }
        }
    }
    
    // Handle water->steam conversion (water boils when hot)
    if (oldCell.type == ParticleType::Water)
    {
        float& temp = newCell.temperature;
        
        // Water heats up from nearby fire or hot air
        if (m_airEnabled)
        {
            // Water temperature moves toward ambient air temperature
            float airTemp = m_airHeat[gridIdx(x, y)];
            float tempDiff = airTemp - temp;
            temp += tempDiff * 0.05f * dt; // Water heats up slower than steam cools
        }
        
        // Check all 8 adjacent cells for fire (fire heats water quickly)
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type == ParticleType::Fire)
                    {
                        // Fire nearby heats water quickly
                        const ParticleProperties& fireProps = getParticleProperties(ParticleType::Fire);
                        float tempDiff = fireProps.burnTemp - temp;
                        temp += tempDiff * 0.2f * dt; // Heat up faster near fire
                    }
                }
            }
        }
        
        // Water boils and turns to steam at 100°C (373.15K)
        const float boilingPoint = 373.15f; // 100°C
        if (temp >= boilingPoint)
        {
            // Convert water to steam
            newCell.type = ParticleType::Steam;
            newCell.life = 0;
            newCell.temperature = temp; // Preserve temperature
            return; // Don't move as liquid, it's now steam
        }
    }
    
    // Handle acid corrosion before movement
    if (oldCell.type == ParticleType::Acid)
    {
        // Get acid cell in new grid and track life
        Cell& acidNew = m_gridNext[gridIdx(x, y)];
        int currentLife = oldCell.life;
        
        // Corrosion chance per frame (35% chance to corrode each adjacent particle)
        // This makes corrosion fast but not instant
        const float corrosionChance = 0.15f;
        
        // Check all 8 adjacent cells (including diagonals) for particles to corrode
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue; // Skip self
                
                if (isValidGridPos(x + dx, y + dy))
                {
                    const Cell& neighbor = m_grid[gridIdx(x + dx, y + dy)];
                    if (neighbor.type != ParticleType::Empty && canCorrode(neighbor.type))
                    {
                        // Random chance to corrode this frame (makes it slower but still fast)
                        float randomValue = (float)rand() / (float)RAND_MAX;
                        if (randomValue <= corrosionChance)
                        {
                            // Corrode the neighbor particle
                            Cell& neighborNew = m_gridNext[gridIdx(x + dx, y + dy)];
                            neighborNew.type = ParticleType::Empty;
                            neighborNew.life = 0;
                            
                            // Decrement acid's life for each particle corroded
                            currentLife--;
                            
                            // If acid's life reaches 0, remove it and stop
                            if (currentLife <= 0)
                            {
                                acidNew.type = ParticleType::Empty;
                                acidNew.life = 0;
                                return; // Acid is gone, no movement
                            }
                        }
                    }
                }
            }
        }
        
        // Update acid's life in new grid
        acidNew.life = currentLife;
    }
    
    // First, try horizontal movement (random left, right, or stay, but prefer air direction)
    // Use movementChance to control viscosity (lower = more viscous, moves less often)
    float randomValue = (float)rand() / (float)RAND_MAX;
    if (randomValue > props.movementChance)
    {
        // Skip horizontal movement this frame (viscous liquids move less often)
        // Still allow falling down
    }
    else
    {
        int horizontalDir = rand() % 3 - 1; // -1 (left), 0 (stay), 1 (right)
        
        // If air is pushing, prefer that direction (but still allow randomness)
        if (preferredDirX != 0 && rand() % 3 == 0) // 1/3 chance to follow air
        {
            horizontalDir = preferredDirX;
        }
        
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

void PowderScene::updateGas(int x, int y, const ParticleProperties& props, int preferredDirX, int preferredDirY)
{
    // Gas repels from other gas particles and reacts to other particles
    // Gas does NOT move opposite to gravity - it only moves due to repulsion and reactions
    // Gas also reacts to air pressure/velocity

    // Apply movement chance - gas moves slower by randomly skipping movement attempts
    float randomValue = (float)rand() / (float)RAND_MAX;
    if (randomValue > props.movementChance)
        return; // Skip movement this frame

    // Combine air forces with particle repulsion
    int airDirX = preferredDirX;
    int airDirY = preferredDirY;

    // Check all 8 adjacent cells (including diagonals) for particles to react to
    int gasNeighbors = 0;
    int otherParticleNeighbors = 0;
    int repelDirX = 0;
    int repelDirY = 0;
    
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
                        repelDirX -= dx;
                        repelDirY -= dy;
                    }
                    // Gas reacts to other particles (non-gas) - try to move away from them
                    else
                    {
                        otherParticleNeighbors++;
                        // React by moving away from denser particles
                        repelDirX -= dx;
                        repelDirY -= dy;
                    }
                }
            }
        }
    }

    // Combine air forces with particle repulsion
    // Air forces are stronger for gas (gas is very light and easily pushed by air)
    // Fire has even stronger upward preference (check if this is fire)
    bool isFire = (m_grid[gridIdx(x, y)].type == ParticleType::Fire);
    int airWeight = isFire ? 5 : 2; // Fire has 5x weight for upward movement
    int combinedDirX = repelDirX + airDirX * airWeight;
    int combinedDirY = repelDirY + airDirY * airWeight;
    
    // Fire always has very strong upward bias and resists horizontal movement
    if (isFire)
    {
        // Strong upward preference - always add upward bias
        combinedDirY += 5; // Extra upward push for fire
        
        // Reduce horizontal movement tendency - fire should go up, not sideways
        if (combinedDirY > 0)
        {
            // If moving up, don't move horizontally
            combinedDirX = 0;
        }
        else if (combinedDirY <= 0)
        {
            // If not moving up, reduce horizontal movement
            combinedDirX = combinedDirX / 2; // Halve horizontal movement
        }
    }
    
    // If there are neighbors (gas or other particles) or air is pushing, try to move
    if (gasNeighbors > 0 || otherParticleNeighbors > 0 || airDirX != 0 || airDirY != 0)
    {
        // Normalize direction (prefer stronger direction)
        int repelX = (combinedDirX > 0) ? 1 : (combinedDirX < 0) ? -1 : 0;
        int repelY = (combinedDirY > 0) ? 1 : (combinedDirY < 0) ? -1 : 0;
        
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
                    // Set initial life value for acid particles
                    if (type == ParticleType::Acid)
                    {
                        cell.life = 5;
                    }
                    else if (type == ParticleType::Fire)
                    {
                        cell.life = 100; // Fire lifetime
                    }
                    else if (type == ParticleType::Smoke)
                    {
                        cell.life = 200; // Smoke lifetime
                    }
                    else if (type == ParticleType::Steam)
                    {
                        cell.life = 0; // Steam doesn't have a lifetime (it condenses based on temperature)
                    }
                    else if (type == ParticleType::Lava)
                    {
                        cell.life = 0; // Lava doesn't have a lifetime
                    }
                    else
                    {
                        cell.life = 0;
                    }
                    // Initialize temperature
                    const ParticleProperties& props = getParticleProperties(type);
                    if (type == ParticleType::Fire)
                    {
                        cell.temperature = props.burnTemp; // Fire is hot
                    }
                    else if (type == ParticleType::Steam)
                    {
                        cell.temperature = 373.15f + 10.0f; // Steam starts above boiling point (110°C)
                    }
                    else if (type == ParticleType::Lava)
                    {
                        cell.temperature = props.burnTemp; // Lava is hot (1500K)
                    }
                    else
                    {
                        cell.temperature = 273.15f + 22.0f; // Ambient temperature
                    }
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

    // Render air velocity visualization first (behind particles)
    if (m_airEnabled && m_showAirVelocity)
    {
        renderAirVelocity(engine, ctx);
    }
    
    // Render air pressure visualization (behind particles)
    if (m_airEnabled && m_showAirPressure)
    {
        renderAirPressure(engine, ctx);
    }

    // Render particles on top
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
                        sprite->setPosition(worldPos.x, worldPos.y, 0.1f); // Slightly in front
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

void PowderScene::renderAirVelocity(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Create or update sprite entities for air velocity visualization
    int totalCells = m_gridWidth * m_gridHeight;

    // Create sprites on demand (simplified approach - could be optimized with pooling)
    static std::vector<std::string> airEntityNames;

    // Create new entities if needed
    if (totalCells > airEntityNames.size())
    {
        int needed = totalCells - (int)airEntityNames.size();
        for (int i = 0; i < needed; ++i)
        {
            std::string name = "AirVelocity_" + std::to_string(airEntityNames.size());
            airEntityNames.push_back(name);
            auto& e = m_entityManager->createEntity(name);
            auto& s = e.addComponent<SpriteComponent>(
                *m_graphicsDevice, 
                L"DX3D/Assets/Textures/node.png", 
                m_cellSize, 
                m_cellSize);
            s.setVisible(false);
        }
    }

    // Render air velocity as color overlay
    int spriteIndex = 0;
    const float maxVelocity = 20.0f; // Maximum velocity for color mapping
    
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            // Skip if blocked by solid (no air here)
            if (m_blockAir[gridIdx(x, y)])
            {
                continue;
            }

            // Get air velocity (use current buffer, not next buffer for visualization)
            float vx = m_airVelocityX[gridIdx(x, y)];
            float vy = m_airVelocityY[gridIdx(x, y)];
            float velocityMagnitude = std::sqrt(vx * vx + vy * vy);
            
            // Make sure we're not showing NaN or invalid values
            if (std::isnan(velocityMagnitude) || std::isinf(velocityMagnitude))
            {
                velocityMagnitude = 0.0f;
            }

            // Skip if no velocity (transparent) - lower threshold to show small velocities
            if (velocityMagnitude < 0.01f)
            {
                continue;
            }

            // Map velocity to color: clear (0) -> blue (medium) -> red (fast)
            // Normalize velocity (0.0 to 1.0)
            float normalizedVel = std::min(velocityMagnitude / maxVelocity, 1.0f);

            Vec4 velocityColor;
            if (normalizedVel < 0.5f)
            {
                // Clear to blue: 0.0 -> 0.5
                float t = normalizedVel * 2.0f; // 0.0 to 1.0
                float alpha = t * 0.6f; // Fade in from transparent
                velocityColor = Vec4(0.0f, 0.0f, t, alpha); // Blue
            }
            else
            {
                // Blue to red: 0.5 -> 1.0
                float t = (normalizedVel - 0.5f) * 2.0f; // 0.0 to 1.0
                float alpha = 0.6f + t * 0.4f; // 0.6 to 1.0
                velocityColor = Vec4(t, 0.0f, 1.0f - t, alpha); // Blue -> Red
            }

            // Render velocity overlay
            if (spriteIndex >= (int)airEntityNames.size())
                break;

            Vec2 worldPos = gridToWorld(x, y);
            std::string& name = airEntityNames[spriteIndex];
            auto* entity = m_entityManager->findEntity(name);
            if (entity)
            {
                auto* sprite = entity->getComponent<SpriteComponent>();
                if (sprite)
                {
                    sprite->setPosition(worldPos.x, worldPos.y, -0.1f); // Behind particles
                    sprite->setTint(velocityColor);
                    sprite->setVisible(true);
                    sprite->draw(ctx);
                }
            }
            spriteIndex++;
        }
    }

    // Hide unused sprites
    for (int i = spriteIndex; i < (int)airEntityNames.size(); ++i)
    {
        auto* entity = m_entityManager->findEntity(airEntityNames[i]);
        if (entity)
        {
            auto* sprite = entity->getComponent<SpriteComponent>();
            if (sprite)
                sprite->setVisible(false);
        }
    }
}

void PowderScene::renderAirPressure(GraphicsEngine& engine, DeviceContext& ctx)
{
    // Create or update sprite entities for air pressure visualization
    int totalCells = m_gridWidth * m_gridHeight;

    // Create sprites on demand (simplified approach - could be optimized with pooling)
    static std::vector<std::string> airPressureEntityNames;

    // Create new entities if needed
    if (totalCells > airPressureEntityNames.size())
    {
        int needed = totalCells - (int)airPressureEntityNames.size();
        for (int i = 0; i < needed; ++i)
        {
            std::string name = "AirPressure_" + std::to_string(airPressureEntityNames.size());
            airPressureEntityNames.push_back(name);
            auto& e = m_entityManager->createEntity(name);
            auto& s = e.addComponent<SpriteComponent>(
                *m_graphicsDevice, 
                L"DX3D/Assets/Textures/node.png", 
                m_cellSize, 
                m_cellSize);
            s.setVisible(false);
        }
    }

    // Render air pressure as color overlay
    int spriteIndex = 0;
    const float maxPressure = 50.0f; // Maximum pressure for color mapping
    
    for (int y = 0; y < m_gridHeight; ++y)
    {
        for (int x = 0; x < m_gridWidth; ++x)
        {
            // Skip if blocked by solid (no air here)
            if (m_blockAir[gridIdx(x, y)])
            {
                continue;
            }

            // Get air pressure (use current buffer, not next buffer for visualization)
            float pressure = m_airPressure[gridIdx(x, y)];
            
            // Make sure we're not showing NaN or invalid values
            if (std::isnan(pressure) || std::isinf(pressure))
            {
                pressure = 0.0f;
            }

            // Skip if no pressure (transparent) - lower threshold to show small pressures
            if (std::abs(pressure) < 0.01f)
            {
                continue;
            }

            // Map pressure to color: blue (negative/low) -> green (zero) -> red (positive/high)
            // Normalize pressure (-1.0 to 1.0)
            float normalizedPressure = std::clamp(pressure / maxPressure, -1.0f, 1.0f);

            Vec4 pressureColor;
            if (normalizedPressure < 0.0f)
            {
                // Negative pressure: blue (darker = more negative)
                float t = -normalizedPressure; // 0.0 to 1.0
                float alpha = t * 0.6f; // Fade in from transparent
                pressureColor = Vec4(0.0f, 0.0f, t, alpha); // Blue
            }
            else if (normalizedPressure > 0.0f)
            {
                // Positive pressure: red (brighter = more positive)
                float t = normalizedPressure; // 0.0 to 1.0
                float alpha = t * 0.6f; // Fade in from transparent
                pressureColor = Vec4(t, 0.0f, 0.0f, alpha); // Red
            }
            else
            {
                // Zero pressure: green
                pressureColor = Vec4(0.0f, 0.5f, 0.0f, 0.3f); // Green, semi-transparent
            }

            // Render pressure overlay
            if (spriteIndex >= (int)airPressureEntityNames.size())
                break;

            Vec2 worldPos = gridToWorld(x, y);
            std::string& name = airPressureEntityNames[spriteIndex];
            auto* entity = m_entityManager->findEntity(name);
            if (entity)
            {
                auto* sprite = entity->getComponent<SpriteComponent>();
                if (sprite)
                {
                    sprite->setPosition(worldPos.x, worldPos.y, -0.1f); // Behind particles
                    sprite->setTint(pressureColor);
                    sprite->setVisible(true);
                    sprite->draw(ctx);
                }
            }
            spriteIndex++;
        }
    }

    // Hide unused sprites
    for (int i = spriteIndex; i < (int)airPressureEntityNames.size(); ++i)
    {
        auto* entity = m_entityManager->findEntity(airPressureEntityNames[i]);
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
        int sandCount = 0, waterCount = 0, stoneCount = 0, woodCount = 0, gasCount = 0, acidCount = 0, fireCount = 0, smokeCount = 0, steamCount = 0, metalCount = 0, lavaCount = 0, mudCount = 0, oilCount = 0;
        for (const auto& cell : m_grid)
        {
            if (cell.type == ParticleType::Sand) sandCount++;
            else if (cell.type == ParticleType::Water) waterCount++;
            else if (cell.type == ParticleType::Stone) stoneCount++;
            else if (cell.type == ParticleType::Wood) woodCount++;
            else if (cell.type == ParticleType::Gas) gasCount++;
            else if (cell.type == ParticleType::Acid) acidCount++;
            else if (cell.type == ParticleType::Fire) fireCount++;
            else if (cell.type == ParticleType::Smoke) smokeCount++;
            else if (cell.type == ParticleType::Steam) steamCount++;
            else if (cell.type == ParticleType::Metal) metalCount++;
            else if (cell.type == ParticleType::Lava) lavaCount++;
            else if (cell.type == ParticleType::Mud) mudCount++;
            else if (cell.type == ParticleType::Oil) oilCount++;
        }
        ImGui::Text("Particles: Sand=%d, Water=%d, Stone=%d, Wood=%d, Gas=%d, Acid=%d, Fire=%d, Smoke=%d, Steam=%d, Metal=%d, Lava=%d, Mud=%d, Oil=%d", 
                    sandCount, waterCount, stoneCount, woodCount, gasCount, acidCount, fireCount, smokeCount, steamCount, metalCount, lavaCount, mudCount, oilCount);

        ImGui::Separator();
        ImGui::Text("Simulation");
        ImGui::SliderInt("Substeps", &m_substeps, 1, 8);
        ImGui::Checkbox("Alternate Update", &m_alternateUpdate);
        ImGui::Text("(Alternating improves flow)");

        ImGui::Separator();
        ImGui::Text("Tools");
        
        // Tool type dropdown
        int toolType = (int)m_currentTool;
        const char* toolTypes[] = { "Drop Particles", "Add Impulse", "Clear" };
        if (ImGui::Combo("Tool Type", &toolType, toolTypes, 3))
        {
            m_currentTool = (ToolType)toolType;
        }
        
        // Tool-specific settings
        if (m_currentTool == ToolType::DropParticles)
        {
            int particleType = (int)m_currentParticleType - 1;
            const char* particles[] = { "Sand", "Water", "Stone", "Wood", "Gas", "Acid", "Fire", "Smoke", "Steam", "Metal", "Lava", "Mud", "Oil" };
            if (ImGui::Combo("Particle Type", &particleType, particles, 13))
            {
                m_currentParticleType = (ParticleType)(particleType + 1);
            }
            ImGui::SliderFloat("Emit Rate", &m_emitRate, 10.0f, 500.0f, "%.0f");
            ImGui::Text("LMB: Drop particles");
        }
        else if (m_currentTool == ToolType::AddImpulse)
        {
            ImGui::SliderFloat("Impulse Strength", &m_impulseStrength, 10.0f, 200.0f, "%.1f");
            ImGui::Text("LMB: Create air impulse");
            if (!m_airEnabled)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Air system must be enabled!");
            }
        }
        else if (m_currentTool == ToolType::Clear)
        {
            ImGui::Text("LMB: Clear particles");
        }
        
        ImGui::SliderFloat("Brush Radius", &m_brushRadius, 5.0f, 100.0f, "%.1f");
        ImGui::Text("RMB: Always clears particles");

        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::Text("WASD: Pan");
        ImGui::Text("Q/E: Zoom");
        
        ImGui::Separator();
        ImGui::Text("Visualization");
        ImGui::Checkbox("Show Grid", &m_showGrid);
        if (m_airEnabled)
        {
            ImGui::Checkbox("Show Air Velocity", &m_showAirVelocity);
            ImGui::Text("(Clear=No velocity, Blue=Medium, Red=Fast)");
            ImGui::Checkbox("Show Air Pressure", &m_showAirPressure);
            ImGui::Text("(Blue=Negative, Green=Zero, Red=Positive)");
        }

        ImGui::Separator();
        ImGui::Text("Air System");
        ImGui::Checkbox("Enable Air", &m_airEnabled);
        if (m_airEnabled)
        {
            ImGui::SliderFloat("Ambient Temp (K)", &m_ambientAirTemp, 173.15f, 373.15f + 500.0f, "%.1f");
            ImGui::SliderFloat("Pressure Loss", &m_airPressureLoss, 0.9f, 1.0f, "%.3f");
            ImGui::SliderFloat("Velocity Loss", &m_airVelocityLoss, 0.9f, 1.0f, "%.3f");
            ImGui::SliderFloat("Advection Mult", &m_airAdvectionMult, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Vorticity Coeff", &m_airVorticityCoeff, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Heat Convection", &m_airHeatConvection, 0.0f, 0.001f, "%.5f");
            
            // Display some air stats
            float avgPressure = 0.0f;
            float avgVelocity = 0.0f;
            float avgHeat = 0.0f;
            int count = 0;
            for (int y = 0; y < m_gridHeight; ++y)
            {
                for (int x = 0; x < m_gridWidth; ++x)
                {
                    if (!m_blockAir[gridIdx(x, y)])
                    {
                        float p = m_airPressure[gridIdx(x, y)];
                        float vx = m_airVelocityX[gridIdx(x, y)];
                        float vy = m_airVelocityY[gridIdx(x, y)];
                        float h = m_airHeat[gridIdx(x, y)];
                        
                        // Check for NaN/infinity and skip invalid values
                        if (std::isnan(p) || std::isinf(p)) p = 0.0f;
                        if (std::isnan(vx) || std::isinf(vx)) vx = 0.0f;
                        if (std::isnan(vy) || std::isinf(vy)) vy = 0.0f;
                        if (std::isnan(h) || std::isinf(h)) h = m_ambientAirTemp;
                        
                        avgPressure += std::abs(p);
                        float velMag = std::sqrt(vx * vx + vy * vy);
                        if (std::isnan(velMag) || std::isinf(velMag)) velMag = 0.0f;
                        avgVelocity += velMag;
                        avgHeat += h;
                        count++;
                    }
                }
            }
            if (count > 0)
            {
                avgPressure /= count;
                avgVelocity /= count;
                avgHeat /= count;
                
                // Final NaN check
                if (std::isnan(avgPressure) || std::isinf(avgPressure)) avgPressure = 0.0f;
                if (std::isnan(avgVelocity) || std::isinf(avgVelocity)) avgVelocity = 0.0f;
                if (std::isnan(avgHeat) || std::isinf(avgHeat)) avgHeat = m_ambientAirTemp;
                
                ImGui::Text("Avg Pressure: %.2f", avgPressure);
                ImGui::Text("Avg Velocity: %.2f", avgVelocity);
                ImGui::Text("Avg Heat: %.1f K (%.1f C)", avgHeat, avgHeat - 273.15f);
            }
        }

        if (ImGui::Button("Clear All", ImVec2(-FLT_MIN, 0)))
        {
            clearGrid();
        }
    }
    ImGui::End();
}
