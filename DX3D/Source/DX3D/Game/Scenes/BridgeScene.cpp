#include <DX3D/Game/Scenes/BridgeScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <DX3D/Core/Input.h>

using namespace dx3d;

void BridgeScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_isSimulationRunning = true;
    m_entityManager = std::make_unique<EntityManager>();

    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    createBridge(engine);
}

void BridgeScene::createBridge(GraphicsEngine& engine) {

    // Create bridge nodes (simple bridge example)
    createNode(Vec2(-300.0f, 0.0f), true, "LeftAnchor", engine);    // Fixed left anchor
    createNode(Vec2(-200.0f, 0.0f), false, "Node1", engine);        // Movable nodes
    createNode(Vec2(-100.0f, 0.0f), false, "Node2", engine);
    createNode(Vec2(0.0f, 0.0f), false, "Node3", engine);
    createNode(Vec2(100.0f, 0.0f), false, "Node4", engine);
    createNode(Vec2(200.0f, 0.0f), false, "Node5", engine);
    createNode(Vec2(300.0f, 0.0f), true, "RightAnchor", engine);    // Fixed right anchor

    // Create horizontal beams
    createBeam("LeftAnchor", "Node1", "Beam1", engine);
    createBeam("Node1", "Node2", "Beam2", engine);
    createBeam("Node2", "Node3", "Beam3", engine);
    createBeam("Node3", "Node4", "Beam4", engine);
    createBeam("Node4", "Node5", "Beam5", engine);
    createBeam("Node5", "RightAnchor", "Beam6", engine);

    // Create support nodes
    createNode(Vec2(-150.0f, -100.0f), false, "Support1", engine);
    createNode(Vec2(-50.0f, -100.0f), false, "Support2", engine);
    createNode(Vec2(50.0f, -100.0f), false, "Support3", engine);
    createNode(Vec2(150.0f, -100.0f), false, "Support4", engine);

    // Create diagonal support beams
    createBeam("Node1", "Support1", "Support_Beam1", engine);
    createBeam("Node2", "Support1", "Support_Beam2", engine);
    createBeam("Node2", "Support2", "Support_Beam3", engine);
    createBeam("Node3", "Support2", "Support_Beam4", engine);
    createBeam("Node3", "Support3", "Support_Beam5", engine);
    createBeam("Node4", "Support3", "Support_Beam6", engine);
    createBeam("Node4", "Support4", "Support_Beam7", engine);
    createBeam("Node5", "Support4", "Support_Beam8", engine);
}

void BridgeScene::createNode(Vec2 position, bool fixed, const std::string& name, GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    auto& nodeEntity = m_entityManager->createEntity(name);

    // Add physics component
    auto& nodeComponent = nodeEntity.addComponent<NodeComponent>(position, fixed);

    // Add visual component
    // Note: You'll need to create appropriate textures for nodes
    auto& sprite = nodeEntity.addComponent<SpriteComponent>(
        device,
         L"DX3D/Assets/Textures/node.png",
        28.0f, 28.0f
    );
    sprite.setPosition(position.x, position.y, 0.0f);
}

void BridgeScene::createBeam(const std::string& node1Name, const std::string& node2Name, const std::string& beamName, GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    auto* node1Entity = m_entityManager->findEntity(node1Name);
    auto* node2Entity = m_entityManager->findEntity(node2Name);

    if (!node1Entity || !node2Entity) return;

    auto& beamEntity = m_entityManager->createEntity(beamName);

    // Add physics component
    auto& beamComponent = beamEntity.addComponent<BeamComponent>(node1Entity, node2Entity);

    // Add visual component
    auto& sprite = beamEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        1.0f, 1.0f  // Will be scaled by beam component
    );

    // Initial positioning will be handled by the physics system
    Vec2 center = beamComponent.getCenterPosition();
    sprite.setPosition(center.x, center.y, 0.0f);
}
#include <iostream>
void BridgeScene::update(float dt) {
    // Update camera movement
    updateCameraMovement(dt);
    
    auto& input = Input::getInstance();

    if (input.wasKeyJustReleased(Key::Z))
    {
        m_isSimulationRunning = !m_isSimulationRunning;
    }
    if (input.isKeyDown(Key::R)) {
        PhysicsSystem::resetPhysics(*m_entityManager);
        m_isSimulationRunning = false;
    }
    // Update physics if simulation is running
    if (m_isSimulationRunning) {
        PhysicsSystem::updateNodes(*m_entityManager, dt);
        PhysicsSystem::updateBeams(*m_entityManager, dt);
    }
    if (input.isMouseDown(MouseClick::LeftMouse))
    {
        if (!m_draggedNode)
        {


            //and if mouse is colliding a node
            //std::cout << "mouse clicked down" << std::endl;
            Vec2 mousePos = input.getMousePosition();
            Vec2 mousePosWorld = screenToWorld(mousePos.x, mousePos.y);
            auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
            for (auto* entity : nodes) {
                if (entity->getComponent<NodeComponent>()->mouseInside(mousePosWorld))
                {
                    m_draggedNode = entity;
                    break;
                }
            }
        }
    }
    else
    {
        //std::cout << "mouse released" << std::endl;
        m_draggedNode = nullptr;
    }
    // Handle dragged node
    if (m_draggedNode) {
        // You'll need to implement mouse position getting in your input system
        // Vec2 mouseWorld = screenToWorld(input.getMouseX(), input.getMouseY());
        // if (auto* nodeComp = m_draggedNode->getComponent<NodeComponent>()) {
        //     nodeComp->setPosition(mouseWorld - m_dragOffset);
        // }
    }
}

void BridgeScene::updateCameraMovement(float dt) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return;

    auto* camera = cameraEntity->getComponent<Camera>();
    if (!camera) return;

    auto& input = Input::getInstance();

    float baseSpeed = 300.0f;
    float fastSpeed = 600.0f;
    float zoomSpeed = 2.0f;

    float currentSpeed = input.isKeyDown(Key::Shift) ? fastSpeed : baseSpeed;

    Vec2 moveDelta(0.0f, 0.0f);
    if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt;
    if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt;
    if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt;
    if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt;

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        camera->move(moveDelta);
    }

    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::Q)) zoomDelta -= zoomSpeed * dt;
    if (input.isKeyDown(Key::E)) zoomDelta += zoomSpeed * dt;

    if (zoomDelta != 0.0f) {
        camera->zoom(zoomDelta);
    }

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(1.0f);
    }
}

void BridgeScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();

    // Find and set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // Render all beams first (so they appear behind nodes)
    auto beamEntities = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    for (auto* entity : beamEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render all nodes on top
    auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    for (auto* entity : nodeEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    engine.endFrame(swapChain);
}

void BridgeScene::onMouseClick(int button, int x, int y) {
    if (button == 0) { // Left mouse button
        Vec2 mouseWorld = screenToWorld(x, y);

        // Check if clicking on a node
        auto nodeEntities = m_entityManager->getEntitiesWithComponent<NodeComponent>();
        for (auto* entity : nodeEntities) {
            if (auto* node = entity->getComponent<NodeComponent>()) {
                if (node->mouseInside(mouseWorld) && !node->isPositionFixed()) {
                    m_draggedNode = entity;
                    m_dragOffset = mouseWorld - node->getPosition();
                    break;
                }
            }
        }
    }
}

void BridgeScene::onMouseRelease(int button, int x, int y) {
    if (button == 0) { // Left mouse button
        m_draggedNode = nullptr;
    }
}

void BridgeScene::onMouseMove(int x, int y) {
    if (m_draggedNode) {
        Vec2 mouseWorld = screenToWorld(x, y);
        if (auto* nodeComp = m_draggedNode->getComponent<NodeComponent>()) {
            nodeComp->setPosition(mouseWorld - m_dragOffset);
        }
    }
}


Vec2 BridgeScene::screenToWorld(int screenX, int screenY) {
    // You'll need to implement this based on your camera system
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera>()) {
            return camera->screenToWorld(Vec2(static_cast<float>(screenX), static_cast<float>(screenY)));
        }
    }
    return Vec2(static_cast<float>(screenX), static_cast<float>(screenY));
}