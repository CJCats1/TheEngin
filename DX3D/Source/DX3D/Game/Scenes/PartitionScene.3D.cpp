#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Game/Game.h>
#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/FBXLoader.h>
#include <DX3D/Math/Geometry.h>
#include <set>
#include <iostream>
#include <Windows.h>
using namespace dx3d;

// 3D Mode Implementation
void PartitionScene::toggle3DMode() {
    m_is3DMode = !m_is3DMode;
    
    if (m_is3DMode) {
        convertTo3D();
        
        if (m_toggle3DModeButton) {
            m_toggle3DModeButton->setText(L"Toggle 2D Mode");
        }
        // Update UI to 3D layout
        updateUIForMode();
    } else {
        convertTo2D();
        
        
        if (m_toggle3DModeButton) {
            m_toggle3DModeButton->setText(L"Toggle 3D Mode");
            m_toggle3DModeButton->setNormalTint(Vec4(0.8f, 0.2f, 0.8f, 0.8f)); // Purple when in 2D
        }
        // Restore 2D UI
        updateUIForMode();
    }
    
    // Trigger ImGui rebuild when toggling 3D mode
    Game::TriggerImguiRebuild();
}

void PartitionScene::setCameraPreset(CameraPreset preset) {
    m_cameraPreset = preset;
    float aspect = GraphicsEngine::getWindowWidth() / std::max(1.0f, GraphicsEngine::getWindowHeight());
    float fovY = 1.22173048f; // ~70 degrees
    float nearZ = 0.1f;
    float farZ = 5000.0f;
    
    if (preset == CameraPreset::TopDown) {
        m_camera3D.setPosition(Vec3(0.0f, 50.0f, 0.0f));
        m_cameraYaw = 0.0f;
        m_cameraPitch = -3.14159f * 0.5f;
        m_camera3D.setTarget(Vec3(0.0f, 0.0f, 0.0f));
        m_camera3D.setUp(Vec3(0.0f, 0.0f, -1.0f));
        fovY = 1.22173048f;
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    } else if (preset == CameraPreset::FirstPerson) {
        // FPS-style camera position - start at a good height looking forward
        m_camera3D.setPosition(Vec3(0.0f, 5.0f, 15.0f)); // Start at a reasonable height, looking forward
        m_cameraYaw = 0.0f; // Looking forward (0 degrees)
        m_cameraPitch = 0.0f; // Level horizon
        m_camera3D.setTarget(Vec3(0.0f, 5.0f, 0.0f)); // Look at the same height
        m_camera3D.setUp(Vec3(0.0f, 1.0f, 0.0f));
        fovY = 1.22173048f; // ~70 degrees FOV for FPS feel
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    } else if (preset == CameraPreset::Isometric) {
        Vec3 pos = Vec3(80.0f, 60.0f, 80.0f);
        m_camera3D.setPosition(pos);
        m_cameraYaw = 0.785398f; // 45 degrees
        m_cameraPitch = -0.523599f; // -30 degrees
        m_camera3D.setTarget(Vec3(0.0f, 0.0f, 0.0f));
        m_camera3D.setUp(Vec3(0.0f, 0.0f, -1.0f));
        fovY = 0.785398f; // 45 degrees
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    }
}

void PartitionScene::convertTo3D() {
    // Create 3D camera
    float aspect = GraphicsEngine::getWindowWidth() / std::max(1.0f, GraphicsEngine::getWindowHeight());
    m_camera3D.setPerspective(1.22173048f, aspect, 0.1f, 5000.0f);
    setCameraPreset(CameraPreset::FirstPerson); // Start in FPS mode
    
    // Initialize shadow mapping
    auto& device = m_entityManager->findEntity("LineRenderer")->getComponent<LineRenderer>()->getDevice();
    GraphicsResourceDesc gDesc = device.getGraphicsResourceDesc();
    
    m_shadowMap = std::make_unique<ShadowMap>(gDesc, 1024, 1024);
    m_shadowMap2 = std::make_unique<ShadowMap>(gDesc, 1024, 1024);
    m_shadowSampler = ShadowMap::createShadowSampler(device.getD3DDevice());
    
    // Create test 3D entities
    createTest3DEntities(device);
}

void PartitionScene::convertTo2D() {
    // Remove 3D entities
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();
    for (const auto& entity : entities) {
        std::string entityName = entity->getName();
        if (entityName.find("3DEntity") == 0 || entityName == "GroundPlane") {
            entitiesToRemove.insert(entityName);
        }
    }
    
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }
    
    m_movingEntities3D.clear();
}

void PartitionScene::update3DCamera(float dt) {
    auto& input = Input::getInstance();
    
    // Only update camera if we're in FPS mode
    if (m_cameraPreset != CameraPreset::FirstPerson) return;
    
    // Mouse look controls - right click and drag
    if (input.isMouseDown(MouseClick::RightMouse)) {
        Vec2 currentMouse = input.getMousePositionNDC();
        
        // Only apply mouse delta if we have a previous mouse position
        if (m_mouseCaptured) {
            Vec2 mouseDelta = currentMouse - m_lastMouse;
            
            // Apply mouse sensitivity
            m_cameraYaw -= mouseDelta.x * m_cameraMouseSensitivity;
            m_cameraPitch -= mouseDelta.y * m_cameraMouseSensitivity;
            
            // Clamp pitch to prevent over-rotation
            const float maxPitch = 1.57f; // 90 degrees
            m_cameraPitch = std::max(-maxPitch, std::min(maxPitch, m_cameraPitch));
        }
        
        m_lastMouse = currentMouse;
        m_mouseCaptured = true;
    } else {
        // Reset mouse capture when not holding right mouse
        m_mouseCaptured = false;
    }
    
    // WASD movement controls
    float moveSpeed = m_cameraMoveSpeed;
    if (input.isKeyDown(Key::Shift)) {
        moveSpeed *= m_cameraRunMultiplier; // Run when holding shift
    }
    
    Vec3 moveDirection(0.0f, 0.0f, 0.0f);
    
    // Calculate forward and right vectors based on yaw
    Vec3 forward = Vec3(
        std::sin(m_cameraYaw),
        0.0f,
        std::cos(m_cameraYaw)
    );
    Vec3 right = Vec3(
        std::cos(m_cameraYaw),
        0.0f,
        -std::sin(m_cameraYaw)
    );
    
    // WASD movement
    if (input.isKeyDown(Key::W)) {
        moveDirection += forward;
    }
    if (input.isKeyDown(Key::S)) {
        moveDirection -= forward;
    }
    if (input.isKeyDown(Key::A)) {
        moveDirection -= right;
    }
    if (input.isKeyDown(Key::D)) {
        moveDirection += right;
    }
    
    // Vertical movement (Space/C for up/down)
    if (input.isKeyDown(Key::Space)) {
        moveDirection.y += 1.0f;
    }
    if (input.isKeyDown(Key::C)) {
        moveDirection.y -= 1.0f;
    }
    
    // Normalize movement direction and apply speed
    if (moveDirection.length() > 0.0f) {
        moveDirection = moveDirection.normalized();
        Vec3 currentPos = m_camera3D.getPosition();
        Vec3 newPos = currentPos + moveDirection * moveSpeed * dt;
        m_camera3D.setPosition(newPos);
    }
    
    // Update camera target based on yaw and pitch
    Vec3 target = m_camera3D.getPosition() + Vec3(
        std::sin(m_cameraYaw) * std::cos(m_cameraPitch),
        std::sin(m_cameraPitch),
        std::cos(m_cameraYaw) * std::cos(m_cameraPitch)
    );
    
    m_camera3D.setTarget(target);
}

void PartitionScene::update3DMovingEntities(float dt) {
    if (!m_entitiesMoving) return;
    
    float effectiveDt = dt * m_simulationSpeedMultiplier;
    if (effectiveDt <= 0.0f) return;
    
    for (auto& entity3D : m_movingEntities3D) {
        if (!entity3D.active) continue;
        
        // Update position
        entity3D.position.x += entity3D.velocity.x * effectiveDt * m_entitySpeedMultiplier;
        entity3D.position.y += entity3D.velocity.y * effectiveDt * m_entitySpeedMultiplier;
        entity3D.position.z += entity3D.velocity.z * effectiveDt * m_entitySpeedMultiplier;
        
        // Boundary collision detection and bounce
        if (entity3D.position.x - entity3D.size.x * 0.5f <= -entity3D.bounds.x ||
            entity3D.position.x + entity3D.size.x * 0.5f >= entity3D.bounds.x) {
            entity3D.velocity.x = -entity3D.velocity.x;
            entity3D.position.x = std::max(-entity3D.bounds.x + entity3D.size.x * 0.5f,
                                         std::min(entity3D.bounds.x - entity3D.size.x * 0.5f, entity3D.position.x));
        }
        
        if (entity3D.position.y - entity3D.size.y * 0.5f <= -entity3D.bounds.y ||
            entity3D.position.y + entity3D.size.y * 0.5f >= entity3D.bounds.y) {
            entity3D.velocity.y = -entity3D.velocity.y;
            entity3D.position.y = std::max(-entity3D.bounds.y + entity3D.size.y * 0.5f,
                                         std::min(entity3D.bounds.y - entity3D.size.y * 0.5f, entity3D.position.y));
        }
        
        if (entity3D.position.z - entity3D.size.z * 0.5f <= -entity3D.bounds.z ||
            entity3D.position.z + entity3D.size.z * 0.5f >= entity3D.bounds.z) {
            entity3D.velocity.z = -entity3D.velocity.z;
            entity3D.position.z = std::max(-entity3D.bounds.z + entity3D.size.z * 0.5f,
                                         std::min(entity3D.bounds.z - entity3D.size.z * 0.5f, entity3D.position.z));
        }
        
        // Update mesh position
        if (auto* entity = m_entityManager->findEntity(entity3D.name)) {
            if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                meshComp->setPosition(entity3D.position);
            }
        }
    }
}

Vec3 PartitionScene::screenToWorldPosition3D(const Vec2& screenPos) {
    // Convert screen position to 3D world position
    float ndcX = screenPos.x * 2.0f - 1.0f;
    float ndcY = screenPos.y * 2.0f - 1.0f;
    
    // For now, just project onto the ground plane at y=0
    Vec3 worldPos;
    worldPos.x = ndcX * 10.0f;
    worldPos.y = 0.5f;
    worldPos.z = ndcY * 10.0f;
    
    return worldPos;
}

void PartitionScene::createTest3DEntities(GraphicsDevice& device) {
    
    // Clear existing 3D entities
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();
    for (const auto& entity : entities) {
        std::string entityName = entity->getName();
        if (entityName.find("3DEntity") == 0) {
            entitiesToRemove.insert(entityName);
        }
    }
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }
    m_movingEntities3D.clear();
    
    // Create some test 3D entities
    std::random_device rd;
    std::mt19937 gen(rd());
    // Wider spawn area so entities start farther apart
    std::uniform_real_distribution<float> posDist(-100.0f, 100.0f);
    std::uniform_real_distribution<float> heightDist(-5.0f, 5.0f); // Height above ground
    std::uniform_real_distribution<float> sizeDist(0.15f, 0.35f);
    // Faster initial velocities so they travel further
    std::uniform_real_distribution<float> velDist(-20.0f, 20.0f);

    // Enforce a minimum spacing between spawns to avoid overlap
    const float minSpawnDistance = 6.0f; // world units
    std::vector<Vec3> acceptedPositions;
    
    for (int i = 0; i < 10; i++) {
        auto& entity = m_entityManager->createEntity("3DEntity" + std::to_string(i));
        
        // Rejection sampling for spaced-out spawns
        Vec3 position;
        int attempts = 0;
        do {
            position = Vec3(posDist(gen), heightDist(gen), posDist(gen));
            attempts++;
            if (attempts > 200) break; // safety
        } while (std::any_of(acceptedPositions.begin(), acceptedPositions.end(), [&](const Vec3& p){
            Vec3 d = p - position;
            float dist2 = d.x*d.x + d.y*d.y + d.z*d.z;
            return dist2 < (minSpawnDistance * minSpawnDistance);
        }));
        acceptedPositions.push_back(position);
        Vec3 size(sizeDist(gen), sizeDist(gen), sizeDist(gen));
        Vec3 velocity(velDist(gen), velDist(gen), velDist(gen));
        
        // Create a sphere mesh from FBX and apply beam texture
        auto mesh = Mesh::CreateFromFBX(device, "DX3D/Assets/Models/Sphere.fbx");
        if (!mesh) {
            // Fallback to cube if sphere loading fails
            mesh = Mesh::CreateCube(device, 2.0f);
        }
        
        // Apply beam texture to the mesh
        std::wstring beamTexturePath;
        beamTexturePath.assign(L"DX3D/Assets/Textures/beam.png");
        auto beamTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), beamTexturePath.c_str());
        if (beamTexture) {
            mesh->setTexture(beamTexture);
        }
        
        auto& meshComp = entity.addComponent<Mesh3DComponent>(mesh);
        meshComp.setPosition(position);
        meshComp.setScale(size);
        meshComp.setVisible(true);
        
        // Set material properties for proper 3D rendering
        meshComp.setMaterial(Vec3(0.2f, 0.6f, 1.0f), 64.0f, 0.3f); // Blue material with some shine
        
        // Create moving entity data
        MovingEntity3D movingEntity3D;
        movingEntity3D.name = "3DEntity" + std::to_string(i);
        movingEntity3D.velocity = velocity;
        // Expanded bounds so they can travel much farther before bouncing
        movingEntity3D.bounds = Vec3(150.0f, 100.0f, 150.0f);
        movingEntity3D.position = position;
        movingEntity3D.size = size;
        movingEntity3D.id = i;
        movingEntity3D.active = true;
        
        m_movingEntities3D.push_back(movingEntity3D);
    }
    
    // Create a ground plane
    auto& groundEntity = m_entityManager->createEntity("GroundPlane");
    auto groundMesh = Mesh::CreatePlane(device, 100.0f, 100.0f); // 100x100 unit plane
    if (groundMesh) {
        // Apply a simple texture or color to the ground
        std::wstring groundTexturePath;
        groundTexturePath.assign(L"DX3D/Assets/Textures/beam.png"); // Reuse beam texture for now
        auto groundTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), groundTexturePath.c_str());
        if (groundTexture) {
            groundMesh->setTexture(groundTexture);
        }
        
        auto& groundMeshComp = groundEntity.addComponent<Mesh3DComponent>(groundMesh);
        groundMeshComp.setPosition(Vec3(0.0f, -50.0f, 0.0f)); // Position below the spheres
        groundMeshComp.setScale(Vec3(1.0f, 1.0f, 1.0f));
        groundMeshComp.setVisible(true);
        
        // Set material for ground plane
        groundMeshComp.setMaterial(Vec3(0.4f, 0.4f, 0.6f), 32.0f, 0.5f); // Gray material
    }
    
    // Create a background plane with dotted pattern
    auto& backgroundEntity = m_entityManager->createEntity("BackgroundPlane");
    auto backgroundMesh = Mesh::CreatePlane(device, 200.0f, 200.0f); // Large background plane
    if (backgroundMesh) {
        // You could load a dotted texture here if you have one
        // For now, we'll use a simple color
        auto& backgroundMeshComp = backgroundEntity.addComponent<Mesh3DComponent>(backgroundMesh);
        backgroundMeshComp.setPosition(Vec3(0.0f, -100.0f, 0.0f)); // Far behind everything
        backgroundMeshComp.setScale(Vec3(2.0f, 2.0f, 1.0f)); // Make it large
        backgroundMeshComp.setVisible(true);
        
        // Set material for background - dark blue to match dotted pattern
        backgroundMeshComp.setMaterial(Vec3(0.1f, 0.1f, 0.2f), 1.0f, 0.0f); // Dark blue, no shine
    }
}

void PartitionScene::addRandom3DEntities(GraphicsDevice& device, int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-40.0f, 40.0f);
    std::uniform_real_distribution<float> heightDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> sizeDist(0.15f, 0.35f);
    std::uniform_real_distribution<float> velDist(-20.0f, 20.0f);

    for (int i = 0; i < count; ++i) {
        auto& entity = m_entityManager->createEntity("3DEntity" + std::to_string(static_cast<int>(m_movingEntities3D.size())));

        Vec3 position(posDist(gen), heightDist(gen), posDist(gen));
        Vec3 size(sizeDist(gen), sizeDist(gen), sizeDist(gen));
        Vec3 velocity(velDist(gen), velDist(gen), velDist(gen));

        auto mesh = Mesh::CreateFromFBX(device, "DX3D/Assets/Models/Sphere.fbx");
        if (!mesh) mesh = Mesh::CreateCube(device, 2.0f);
        std::wstring beamTexturePath; beamTexturePath.assign(L"DX3D/Assets/Textures/beam.png");
        auto beamTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), beamTexturePath.c_str());
        if (beamTexture) mesh->setTexture(beamTexture);

        auto& meshComp = entity.addComponent<Mesh3DComponent>(mesh);
        meshComp.setPosition(position);
        meshComp.setScale(size);
        meshComp.setVisible(true);
        
        // Set material properties for proper 3D rendering
        meshComp.setMaterial(Vec3(0.2f, 0.6f, 1.0f), 64.0f, 0.3f); // Blue material with some shine

        MovingEntity3D movingEntity3D;
        movingEntity3D.name = entity.getName();
        movingEntity3D.velocity = velocity;
        movingEntity3D.bounds = Vec3(80.0f, 80.0f, 80.0f);
        movingEntity3D.position = position;
        movingEntity3D.size = size;
        movingEntity3D.id = static_cast<int>(m_movingEntities3D.size());
        movingEntity3D.active = true;
        m_movingEntities3D.push_back(movingEntity3D);
    }
}

void PartitionScene::clearAllEntities3D() {
    std::set<std::string> entitiesToRemove;
    const auto& entities = m_entityManager->getEntities();
    for (const auto& entity : entities) {
        std::string entityName = entity->getName();
        if (entityName.find("3DEntity") == 0) {
            entitiesToRemove.insert(entityName);
        }
    }
    for (const std::string& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }
    m_movingEntities3D.clear();
}

void PartitionScene::calculateLightViewProj() {
    // Light 1: top-down
    Vec3 lightPos(0.0f, 50.0f, 0.0f);
    Vec3 lightTarget(0.0f, 0.0f, 0.0f);
    Vec3 lightUp(0.0f, 0.0f, 1.0f);
    Mat4 lightView = Mat4::lookAt(lightPos, lightTarget, lightUp);
    Mat4 lightProj = Mat4::orthographic(100.0f, 100.0f, 0.1f, 200.0f);
    m_lightViewProj = lightView * lightProj;

    // Light 2: angled
    Vec3 lightPos2(60.0f, 60.0f, 60.0f);
    Vec3 lightTarget2(0.0f, 0.0f, 0.0f);
    Vec3 lightUp2(0.0f, 1.0f, 0.0f);
    Mat4 lightView2 = Mat4::lookAt(lightPos2, lightTarget2, lightUp2);
    Mat4 lightProj2 = Mat4::orthographic(120.0f, 120.0f, 0.1f, 250.0f);
    m_lightViewProj2 = lightView2 * lightProj2;
}

void PartitionScene::renderShadowMap(GraphicsEngine& engine) {
    if (!m_shadowMap || !m_is3DMode) return;
    
    auto& ctx = engine.getContext();
    auto* d3dContext = ctx.getD3DDeviceContext();
    
    // Ensure the shadow map SRVs are unbound before binding as DSVs
    ID3D11ShaderResourceView* nullSRVs[10] = {};
    d3dContext->PSSetShaderResources(1, 10, nullSRVs);
    
    // Calculate light matrices
    calculateLightViewProj();
    
    // Render Light 1 shadow map
    m_shadowMap->clear(d3dContext);
    m_shadowMap->setAsRenderTarget(d3dContext);
    m_shadowMap->setViewport(d3dContext);
    
    // Set light 1 matrices
    Vec3 lightPos(0.0f, 50.0f, 0.0f);
    Vec3 lightTarget(0.0f, 0.0f, 0.0f);
    Vec3 lightUp(0.0f, 0.0f, 1.0f);
    ctx.setViewMatrix(Mat4::lookAt(lightPos, lightTarget, lightUp));
    ctx.setProjectionMatrix(Mat4::orthographic(100.0f, 100.0f, 0.1f, 200.0f));
    
    // Use shadow map pipeline
    ctx.setGraphicsPipelineState(engine.getShadowMapPipeline());
    
    // Render 3D meshes to shadow map
    auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
    for (auto* entity : mesh3DEntities) {
        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
            if (meshComp->isVisible()) {
                // Set world matrix
                Mat4 worldMatrix = Mat4::identity();
                worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                ctx.setWorldMatrix(worldMatrix);
                
                meshComp->draw(ctx);
            }
        }
    }
    
    // Unbind
    d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Render Light 2 shadow map
    m_shadowMap2->clear(d3dContext);
    m_shadowMap2->setAsRenderTarget(d3dContext);
    m_shadowMap2->setViewport(d3dContext);

    // Set light 2 matrices
    Vec3 lightPosB(60.0f, 60.0f, 60.0f);
    Vec3 lightTargetB(0.0f, 0.0f, 0.0f);
    Vec3 lightUpB(0.0f, 1.0f, 0.0f);
    ctx.setViewMatrix(Mat4::lookAt(lightPosB, lightTargetB, lightUpB));
    ctx.setProjectionMatrix(Mat4::orthographic(120.0f, 120.0f, 0.1f, 250.0f));

    ctx.setGraphicsPipelineState(engine.getShadowMapPipeline());

    auto mesh3DEntities2 = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
    for (auto* entity : mesh3DEntities2) {
        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
            if (meshComp->isVisible()) {
                Mat4 worldMatrix = Mat4::identity();
                worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                ctx.setWorldMatrix(worldMatrix);
                meshComp->draw(ctx);
            }
        }
    }

    // Unbind
    d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void PartitionScene::renderShadowMapDebug(GraphicsEngine& engine) {
    if (!m_shadowMap || !m_is3DMode || !m_showShadowMapDebug) return;
    
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    
    // Switch to screen space for debug overlay
    if (auto* debugPipeline = engine.getShadowMapDebugPipeline()) {
        ctx.setGraphicsPipelineState(*debugPipeline);
    } else {
        ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    }
    ctx.setScreenSpaceMatrices(screenWidth, screenHeight);
    
    // Create debug quad in top-right corner
    float quadSize = 200.0f;
    float margin = 20.0f;
    float x = screenWidth - quadSize - margin;
    float y = margin;
    
    // Convert to NDC coordinates (-1 to 1)
    float ndcX = (x / screenWidth) * 2.0f - 1.0f;
    float ndcY = 1.0f - (y / screenHeight) * 2.0f; // Flip Y for DirectX
    float ndcX2 = ((x + quadSize) / screenWidth) * 2.0f - 1.0f;
    float ndcY2 = 1.0f - ((y + quadSize) / screenHeight) * 2.0f;
    
    struct DebugVertex {
        float x, y, z;
        float u, v;
    };
    
    DebugVertex vertices[] = {
        { ndcX, ndcY, 0.0f, 0.0f, 0.0f },
        { ndcX2, ndcY, 0.0f, 1.0f, 0.0f },
        { ndcX, ndcY2, 0.0f, 0.0f, 1.0f },
        { ndcX2, ndcY2, 0.0f, 1.0f, 1.0f }
    };
    
    
    // Create vertex buffer
    static Microsoft::WRL::ComPtr<ID3D11Buffer> debugVertexBuffer;
    if (!debugVertexBuffer) {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(vertices);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertices;
        
        auto device = engine.getGraphicsDevice().getD3DDevice();
        device->CreateBuffer(&bufferDesc, &initData, debugVertexBuffer.GetAddressOf());
    }
    
    // Set up rendering
    auto* d3dContext = ctx.getD3DDeviceContext();
    UINT stride = sizeof(DebugVertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, debugVertexBuffer.GetAddressOf(), &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // Bind shadow map
    ID3D11ShaderResourceView* shadowSRV = m_shadowMap->getDepthSRV();
    if (shadowSRV) {
        d3dContext->PSSetShaderResources(0, 1, &shadowSRV);
    }
    
    // Create simple sampler
    static Microsoft::WRL::ComPtr<ID3D11SamplerState> debugSampler;
    if (!debugSampler) {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        
        auto device = engine.getGraphicsDevice().getD3DDevice();
        device->CreateSamplerState(&samplerDesc, debugSampler.GetAddressOf());
    }
    
    d3dContext->PSSetSamplers(0, 1, debugSampler.GetAddressOf());
    ctx.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    // Draw debug quad
    d3dContext->Draw(4, 0);
    
    // Unbind
    ID3D11ShaderResourceView* nullSRV = nullptr;
    d3dContext->PSSetShaderResources(0, 1, &nullSRV);
}
