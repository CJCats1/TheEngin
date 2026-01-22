#include <DX3D/Game/Scenes/MarbleMazeScene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Components/Physics3DComponent.h>
#include <DX3D/Core/Entity.h>
#include <algorithm>
#include <cmath>
#include <windows.h>

using namespace dx3d;

void MarbleMazeScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    
    // Initialize Entity Manager
    m_entityManager = std::unique_ptr<EntityManager>(new EntityManager());
    
    // Load beam texture
    std::wstring beamPath = L"D:/TheEngine/TheEngine/DX3D/Assets/Textures/beam.png";
    m_beamTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), beamPath.c_str());
    
    // Create marble entity with physics
    auto& marbleEntity = m_entityManager->createEntity("Marble");
    auto marbleMesh = Mesh::CreateSphere(device, m_marbleRadius, 16);
    auto& marbleMeshComp = marbleEntity.addComponent<Mesh3DComponent>(marbleMesh);
    auto& marblePhysics = marbleEntity.addComponent<Physics3DComponent>();
    marblePhysics.setMass(m_marbleMass);
    marblePhysics.setRadius(m_marbleRadius);
    marblePhysics.setFriction(m_friction);
    marblePhysics.setGravity(m_gravity);
    marblePhysics.setBounce(m_bounce);
    marbleMeshComp.setMaterial(Vec3(0.2f, 0.6f, 1.0f), 64.0f, 0.3f); // Blue marble
    
    // Create ground plane
    auto& groundEntity = m_entityManager->createEntity("Ground");
    auto groundMesh = Mesh::CreatePlane(device, 12.0f, 12.0f);
    if (m_beamTexture) groundMesh->setTexture(m_beamTexture);
    auto& groundMeshComp = groundEntity.addComponent<Mesh3DComponent>(groundMesh);
    groundMeshComp.setMaterial(Vec3(0.4f, 0.4f, 0.6f), 32.0f, 0.5f);
    
	float wallOffset = 20.0f;
    // Create wall entities
    createWallEntity("Wall1", Vec3(-wallOffset, 0.3f, 0.0f), Vec3(0.2f, 0.6f, 8.0f), device);
    createWallEntity("Wall2", Vec3(wallOffset, 0.3f, 0.0f), Vec3(0.2f, 0.6f, 8.0f), device);
    createWallEntity("Wall3", Vec3(0.0f, 0.3f, -wallOffset), Vec3(8.0f, 0.6f, 0.2f), device);
    createWallEntity("Wall4", Vec3(0.0f, 0.3f, wallOffset), Vec3(8.0f, 0.6f, 0.2f), device);
    
    // Create collision cube
    auto& cubeEntity = m_entityManager->createEntity("CollisionCube");
    auto cubeMesh = Mesh::CreateCube(device, 1.0f);
    if (m_beamTexture) cubeMesh->setTexture(m_beamTexture);
    auto& cubeMeshComp = cubeEntity.addComponent<Mesh3DComponent>(cubeMesh);
    cubeMeshComp.setPosition(Vec3(2.0f, 0.5f, 1.0f));
    cubeMeshComp.setScale(Vec3(0.8f, 1.0f, 0.8f));
    cubeMeshComp.setMaterial(Vec3(0.8f, 0.2f, 0.2f), 32.0f, 0.1f); // Red cube
    
    // Set up camera for debugging
    float w = GraphicsEngine::getWindowWidth();
    float h = GraphicsEngine::getWindowHeight();
    m_camera = Camera3D(1.04719755f, w / h, 0.1f, 1000.0f);
    m_camera.setPosition(Vec3(0.0f, 5.0f, 5.0f)); // Angled view for better debugging
    m_camera.setTarget(Vec3(0.0f, 0.0f, 0.0f));   // Looking at center
}

void MarbleMazeScene::update(float dt)
{
    auto& input = Input::getInstance();
    
    // Handle marble input (WASD for marble movement)
    m_inputDirection = Vec2(0.0f, 0.0f);
    
    if (input.isKeyDown(Key::W)) m_inputDirection.y += 1.0f;
    if (input.isKeyDown(Key::S)) m_inputDirection.y -= 1.0f;
    if (input.isKeyDown(Key::A)) m_inputDirection.x -= 1.0f;
    if (input.isKeyDown(Key::D)) m_inputDirection.x += 1.0f;
    
    // Normalize input direction
    if (m_inputDirection.lengthSquared() > 0.0f) {
        m_inputDirection.normalize();
    }
    
    // Handle camera controls (Arrow keys for camera movement)
    m_cameraInput = Vec2(0.0f, 0.0f);
    
    if (input.isKeyDown(Key::Up)) m_cameraInput.y += 1.0f;
    if (input.isKeyDown(Key::Down)) m_cameraInput.y -= 1.0f;
    if (input.isKeyDown(Key::Left)) m_cameraInput.x -= 1.0f;
    if (input.isKeyDown(Key::Right)) m_cameraInput.x += 1.0f;
    
    // Mouse look when RMB held
    if (input.isMouseDown(MouseClick::RightMouse))
    {
        Vec2 mouse = input.getMousePositionClient();
        if (!m_mouseCaptured) { m_lastMouse = mouse; m_mouseCaptured = true; }
        Vec2 delta = mouse - m_lastMouse; m_lastMouse = mouse;
        const float sensitivity = 0.0035f;
        m_cameraYaw += delta.x * sensitivity;
        m_cameraPitch += -delta.y * sensitivity;
        m_cameraPitch = std::max(-1.55f, std::min(1.55f, m_cameraPitch));
    }
    else { m_mouseCaptured = false; }
    
    // Update camera position and orientation
    updateCamera(dt);
    
    // Update marble physics using ECS
    if (auto* marbleEntity = m_entityManager->findEntity("Marble")) {
        if (auto* physics = marbleEntity->getComponent<Physics3DComponent>()) {
            if (auto* mesh = marbleEntity->getComponent<Mesh3DComponent>()) {
                // Apply input forces
                Vec3 inputForce(
                    m_inputDirection.x * m_forceStrength,
                    0.0f, // No vertical input force
                    m_inputDirection.y * m_forceStrength
                );
                physics->setInputForce(inputForce);
                
                // Update physics
                physics->update(dt);
                
                // Update mesh position
                Vec3 newPos = physics->getPosition();
                mesh->setPosition(newPos);
                
                // Handle collisions
                handleCollisions();
            }
        }
    }
}

void MarbleMazeScene::fixedUpdate(float dt)
{
    // Fixed timestep physics updates can go here if needed
}


void MarbleMazeScene::handleCollisions()
{
    if (auto* marbleEntity = m_entityManager->findEntity("Marble")) {
        if (auto* marblePhysics = marbleEntity->getComponent<Physics3DComponent>()) {
            Vec3 marblePos = marblePhysics->getPosition();
            float marbleRadius = marblePhysics->getRadius();
            
            // Ground collision (y = 0)
            if (marblePos.y - marbleRadius <= 0.0f) {
                marblePhysics->handleCollision(Vec3(0.0f, 1.0f, 0.0f), marbleRadius - marblePos.y);
            }
            
            // Wall collisions (hardcoded boundaries)
            float wallOffset = 20.0f;
            float wallThickness = 0.2f;
            
            // Left wall
            if (marblePos.x - marbleRadius <= -wallOffset + wallThickness/2) {
                marblePhysics->handleCollision(Vec3(1.0f, 0.0f, 0.0f), marbleRadius - (-wallOffset + wallThickness/2 - marblePos.x));
            }
            
            // Right wall
            if (marblePos.x + marbleRadius >= wallOffset - wallThickness/2) {
                marblePhysics->handleCollision(Vec3(-1.0f, 0.0f, 0.0f), marbleRadius - (marblePos.x - (wallOffset - wallThickness/2)));
            }
            
            // Front wall
            if (marblePos.z - marbleRadius <= -wallOffset + wallThickness/2) {
                marblePhysics->handleCollision(Vec3(0.0f, 0.0f, 1.0f), marbleRadius - (-wallOffset + wallThickness/2 - marblePos.z));
            }
            
            // Back wall
            if (marblePos.z + marbleRadius >= wallOffset - wallThickness/2) {
                marblePhysics->handleCollision(Vec3(0.0f, 0.0f, -1.0f), marbleRadius - (marblePos.z - (wallOffset - wallThickness/2)));
            }
            
            // Cube collision (hardcoded position)
            Vec3 cubePos = Vec3(2.0f, 0.5f, 1.0f);
            Vec3 cubeSize = Vec3(0.8f, 1.0f, 0.8f);
            
            // Simple AABB collision with cube
            if (marblePos.x + marbleRadius > cubePos.x - cubeSize.x/2 &&
                marblePos.x - marbleRadius < cubePos.x + cubeSize.x/2 &&
                marblePos.y + marbleRadius > cubePos.y - cubeSize.y/2 &&
                marblePos.y - marbleRadius < cubePos.y + cubeSize.y/2 &&
                marblePos.z + marbleRadius > cubePos.z - cubeSize.z/2 &&
                marblePos.z - marbleRadius < cubePos.z + cubeSize.z/2) {
                
                // Calculate collision normal (simplified)
                Vec3 collisionNormal = (marblePos - cubePos).normalized();
                if (collisionNormal.lengthSquared() < 0.001f) {
                    collisionNormal = Vec3(1.0f, 0.0f, 0.0f); // Default normal
                }
                marblePhysics->handleCollision(collisionNormal, marbleRadius);
            }
        }
    }
}

void MarbleMazeScene::render(GraphicsEngine& engine, SwapChain& swapChain)
{
    auto& ctx = engine.getContext();

    ctx.enableDepthTest();
    ctx.setGraphicsPipelineState(engine.get3DPipeline());
    
    // Set up lighting
    std::vector<Vec3> dirs{ Vec3(-0.4f, -1.0f, -0.3f), Vec3(0.6f, -0.2f, 0.5f) };
    std::vector<Vec3> cols{ Vec3(1.0f, 0.95f, 0.9f), Vec3(0.3f, 0.4f, 1.0f) };
    std::vector<float> intens{ 1.0f, 0.6f };
    ctx.setLights(dirs, cols, intens);
    ctx.setMaterial(Vec3(1.0f, 1.0f, 1.0f), 64.0f, 0.2f);
    ctx.setCameraPosition(m_camera.getPosition());
    ctx.setViewMatrix(m_camera.getViewMatrix());
    ctx.setProjectionMatrix(m_camera.getProjectionMatrix());

    // Render all entities with Mesh3DComponent
    auto meshEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
    for (auto* entity : meshEntities) {
        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
            if (meshComp->isVisible()) {
                meshComp->draw(ctx);
            }
        }
    }

    // frame begin/end handled centrally
}

void MarbleMazeScene::updateCamera(float dt)
{
    // Build forward/right vectors from yaw/pitch
    Vec3 forward(
        std::cos(m_cameraPitch) * std::sin(m_cameraYaw),
        std::sin(m_cameraPitch),
        std::cos(m_cameraPitch) * std::cos(m_cameraYaw)
    );
    forward.normalize();
    
    Vec3 right(forward.z, 0.0f, -forward.x);
    right.normalize();
    
    Vec3 up(0.0f, 1.0f, 0.0f);
    
    // Move camera based on input
    const float moveSpeed = 8.0f;
    Vec3 moveDelta(0, 0, 0);
    
    if (m_cameraInput.x > 0) moveDelta = moveDelta + right * moveSpeed * dt;
    if (m_cameraInput.x < 0) moveDelta = moveDelta - right * moveSpeed * dt;
    if (m_cameraInput.y > 0) moveDelta = moveDelta + forward * moveSpeed * dt;
    if (m_cameraInput.y < 0) moveDelta = moveDelta - forward * moveSpeed * dt;
    
    // Apply vertical movement (Q/E for up/down)
    auto& input = Input::getInstance();
    if (input.isKeyDown(Key::Q)) moveDelta.y += moveSpeed * dt;
    if (input.isKeyDown(Key::E)) moveDelta.y -= moveSpeed * dt;
    
    if (moveDelta.lengthSquared() > 0) {
        Vec3 newPos = m_camera.getPosition() + moveDelta;
        m_camera.setPosition(newPos);
    }
    
    // Update camera target based on yaw/pitch
    Vec3 pos = m_camera.getPosition();
    m_camera.setTarget(pos + forward);
}

void MarbleMazeScene::createWallEntity(const std::string& name, const Vec3& position, const Vec3& scale, GraphicsDevice& device) {
    auto& wallEntity = m_entityManager->createEntity(name);
    auto wallMesh = Mesh::CreateCube(device, 1.0f);
    if (m_beamTexture) wallMesh->setTexture(m_beamTexture);
    auto& wallMeshComp = wallEntity.addComponent<Mesh3DComponent>(wallMesh);
    wallMeshComp.setPosition(position);
    wallMeshComp.setScale(scale);
    wallMeshComp.setMaterial(Vec3(0.4f, 0.4f, 0.4f), 32.0f, 0.1f); // Gray walls
}
