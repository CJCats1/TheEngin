#include <DX3D/Game/Scenes/BridgeScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <iostream>

using namespace dx3d;
void BridgeScene::resetBridge() {
    std::vector<std::string> entitiesToRemove;

    // Collect all nodes except anchors
    auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    for (auto* entity : nodes) {
        if (entity->getName() != "LeftAnchor" && entity->getName() != "RightAnchor") {
            entitiesToRemove.push_back(entity->getName());
        }
    }

    // Collect all beams
    auto beams = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    for (auto* entity : beams) {
        entitiesToRemove.push_back(entity->getName());
    }

    // Remove collected entities
    for (const auto& name : entitiesToRemove) {
        m_entityManager->removeEntity(name);
    }

    // Reset counts
    m_numberOfNodes = 2; // only anchors remain
    m_numberOfBeams = 0;

    // Reset physics state
    PhysicsSystem::resetPhysics(*m_entityManager);
    PhysicsSystem::updateBeams(*m_entityManager, 0.01f);
    PhysicsSystem::updateNodes(*m_entityManager, 0.01f);
}

void BridgeScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_isSimulationRunning = false;
    
    m_entityManager = std::make_unique<EntityManager>();

    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    createBridge(engine);
    createUI(engine);
    PhysicsSystem::updateNodes(*m_entityManager, 0.01f);
    PhysicsSystem::updateBeams(*m_entityManager, 0.01f);
}
void BridgeScene::createUI(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    if (!TextSystem::isInitialized())
        TextSystem::initialize(device);

    float screenWidth = static_cast<float>(GraphicsEngine::getWindowWidth());
    float screenHeight = static_cast<float>(GraphicsEngine::getWindowHeight());

    // --- Top-left status panel ---
    auto statusPanelPos = Vec2(0.14f, 0.95f); // normalized [0,1] screen coords
    auto& statusPanelEntity = m_entityManager->createEntity("StatusPanel");
    auto& statusPanel = statusPanelEntity.addComponent<PanelComponent>(
        device,
        300.0f,   // width (a bit larger than the text for padding)
        40.0f,    // height
        L"Simulation Running: TRUE",
        20.0f
    );
    statusPanel.setScreenPosition(statusPanelPos.x, statusPanelPos.y);
    statusPanel.setTint(Vec4(0.1f, 0.1f, 0.1f, 0.7f));

    // --- Right-side button panel ---
    float buttonWidth = 180.0f;
    float buttonHeight = 40.0f;
    float startX = 0.80f;
    float startY = 0.8f;
    float padding = 0.05f;

    std::vector<std::wstring> buttonLabels = {
        L"Build Mode",
        L"Simulate Mode",
        L"Delete Mode",
        L"Reset Bridge"
    };

    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        auto& btnEntity = m_entityManager->createEntity("Button_" + std::to_string(i));
        auto& button = btnEntity.addComponent<ButtonComponent>(
            device,
            buttonLabels[i],
            22.0f
        );
        button.enableScreenSpace(true);
        button.setScreenPosition(startX, startY - i * (buttonHeight / screenHeight + padding));
        button.setNormalTint(Vec4(0.2f, 0.6f, 0.8f, 0.5f));
        button.setHoveredTint(Vec4(0.4f, 0.8f, 1.0f, 0.5f));
        button.setPressedTint(Vec4(0.1f, 0.4f, 0.6f, 0.5f));
        button.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        button.setFontSize(18.0f);

        // Callback logic
        button.setOnClickCallback([this, i]() {
            switch (i) {
            case 0: // Build mode
                setMode(SceneMode::Build);
                break;
            case 1: // Simulate mode
                setMode(SceneMode::Simulating);
                break;
            case 2: // Delete mode
                if (m_currentMode == SceneMode::Build) {
                    toggleDeleteMode();
                }
				break;
            case 3: // Reset bridge
                resetBridge();
                break;
            }
           
            });
    }

    // --- Bottom-left mini info panel ---
    auto miniPanelPos = Vec2(0.15f, 0.05f);
    auto& infoPanelEntity = m_entityManager->createEntity("InfoPanel");
    auto& infoPanel = infoPanelEntity.addComponent<PanelComponent>(
        device,
        350.0f,  // width
        40.0f,   // height
        L"Click nodes to create new ones!",
        18.0f
    );
    infoPanel.setScreenPosition(miniPanelPos.x, miniPanelPos.y);
    infoPanel.setTint(Vec4(0.1f, 0.1f, 0.1f, 0.6f));


    auto& modePanelEntity = m_entityManager->createEntity("ModePanel");
    auto& modePanel = modePanelEntity.addComponent<PanelComponent>(
        device,
        250.0f,  // width
        40.0f,   // height
        L"Mode: BUILD",
        20.0f
    );
    modePanel.setScreenPosition(0.14f, 0.90f);
    modePanel.setTint(Vec4(0.15f, 0.15f, 0.15f, 0.7f));

}

void BridgeScene::createBridge(GraphicsEngine& engine) {
    // Create bridge nodes (simple bridge example)
    createNode(Vec2(-300.0f, 0.0f), true, "LeftAnchor");
    createNode(Vec2(-200.0f, 0.0f), false, "Node1");
    createNode(Vec2(-100.0f, 0.0f), false, "Node2");
    createNode(Vec2(0.0f, 0.0f), false, "Node3");
    createNode(Vec2(100.0f, 0.0f), false, "Node4");
    createNode(Vec2(200.0f, 0.0f), false, "Node5");
    createNode(Vec2(300.0f, 0.0f), true, "RightAnchor");

    // Create horizontal beams
    createBeam("LeftAnchor", "Node1", "Beam1");
    createBeam("Node1", "Node2", "Beam2");
    createBeam("Node2", "Node3", "Beam3");
    createBeam("Node3", "Node4", "Beam4");
    createBeam("Node4", "Node5", "Beam5");
    createBeam("Node5", "RightAnchor", "Beam6");

    // Create support nodes
    createNode(Vec2(-150.0f, -100.0f), false, "Support1");
    createNode(Vec2(-50.0f, -100.0f), false, "Support2");
    createNode(Vec2(50.0f, -100.0f), false, "Support3");
    createNode(Vec2(150.0f, -100.0f), false, "Support4");

    createBeam("LeftAnchor", "Support1", "Support_Beam9");
    createBeam("RightAnchor", "Support4", "Support_Beam10");

    // Create diagonal support beams
    createBeam("Node1", "Support1", "Support_Beam1");
    createBeam("Node2", "Support1", "Support_Beam2");
    createBeam("Node2", "Support2", "Support_Beam3");
    createBeam("Node3", "Support2", "Support_Beam4");
    createBeam("Node3", "Support3", "Support_Beam5");
    createBeam("Node4", "Support3", "Support_Beam6");
    createBeam("Node4", "Support4", "Support_Beam7");
    createBeam("Node5", "Support4", "Support_Beam8");
}

void BridgeScene::createNode(Vec2 position, bool fixed, const std::string& name) {
    auto& device = *m_graphicsDevice;
    auto& nodeEntity = m_entityManager->createEntity(name);

    // Physics component
    nodeEntity.addComponent<NodeComponent>(position, fixed);

    // Visual component
    auto& sprite = nodeEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png",
        28.0f, 28.0f
    );
    sprite.setPosition(position.x, position.y, 0.0f);
    m_numberOfNodes++;
}

void BridgeScene::createBeam(const std::string& node1Name, const std::string& node2Name, const std::string& beamName) {
    auto& device = *m_graphicsDevice;
    auto* node1Entity = m_entityManager->findEntity(node1Name);
    auto* node2Entity = m_entityManager->findEntity(node2Name);

    if (!node1Entity || !node2Entity) return;

    auto& beamEntity = m_entityManager->createEntity(beamName);

    // Physics component
    auto& beamComponent = beamEntity.addComponent<BeamComponent>(node1Entity, node2Entity);

    // Visual component
    auto& sprite = beamEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        1.0f, 1.0f
    );

    Vec2 center = beamComponent.getCenterPosition();
    sprite.setPosition(center.x, center.y, 0.0f);
	m_numberOfBeams++;
}

void BridgeScene::update(float dt) {
    updateCameraMovement(dt);

    auto& input = Input::getInstance();

    if (input.wasKeyJustReleased(Key::Z)) {
        m_isSimulationRunning = !m_isSimulationRunning;
    }

    if (input.wasKeyJustReleased(Key::R)) {
        PhysicsSystem::resetPhysics(*m_entityManager);
    }

    
    // Update UI panels
    updateUIStatus();

    // Handle different modes
    if (m_currentMode == SceneMode::Build && !m_inDeleteMode) {
        handleBuildMode();
    }
    else if (m_inDeleteMode && m_currentMode == SceneMode::Build) {
        handleDeleteMode();
    }

    // Update button interactions
    updateButtonInteractions(dt);
}

void BridgeScene::updateCameraMovement(float dt) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return;
    auto* camera = cameraEntity->getComponent<Camera2D>();
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
    if (zoomDelta != 0.0f) camera->zoom(zoomDelta);

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(1.0f);
    }
}

void BridgeScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();

    float screenWidth = static_cast<float>(GraphicsEngine::getWindowWidth());
    float screenHeight = static_cast<float>(GraphicsEngine::getWindowHeight());

    // WORLD RENDERING
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    ctx.enableDepthTest();
    ctx.disableAlphaBlending();

    // Beams
    for (auto* entity : m_entityManager->getEntitiesWithComponent<BeamComponent>()) {
        auto* beam = entity->getComponent<BeamComponent>();
        auto* sprite = entity->getComponent<SpriteComponent>();
        if (beam && sprite && sprite->isVisible() && sprite->isValid()) {
            float stress = clamp(beam->getStressFactor(), 0.0f, 1.0f);
            Vec4 tintColor = Vec4(1.0f, 0.0f, 0.0f, stress*0.8);
            sprite->setTint(tintColor);
            sprite->draw(ctx);
        }
    }

    // Nodes
    for (auto* entity : m_entityManager->getEntitiesWithComponent<NodeComponent>()) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid() && !sprite->m_useScreenSpace) {
                sprite->draw(ctx);
            }
        }
    }
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();

    for (auto* entity : spriteEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isScreenSpace()) {
                sprite->draw(ctx);
            }
        }
    }

    for (auto* entity : m_entityManager->getEntitiesWithComponent<ButtonComponent>()) {
        if (auto* text = entity->getComponent<ButtonComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }
    for (auto* entity : m_entityManager->getEntitiesWithComponent<PanelComponent>()) {
        if (auto* text = entity->getComponent<PanelComponent>()) {
                text->draw(ctx); // Will respect m_useScreenSpace
        }
    }
    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }
    engine.endFrame(swapChain);
}

void dx3d::BridgeScene::fixedUpdate(float dt)
{
    if (m_isSimulationRunning) {
        PhysicsSystem::updateNodes(*m_entityManager, dt);
        PhysicsSystem::updateBeams(*m_entityManager, dt);
    }
}

Vec2 BridgeScene::screenToWorld(int screenX, int screenY) {
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            return camera->screenToWorld(Vec2((float)screenX, (float)screenY));
        }
    }
    return Vec2((float)screenX, (float)screenY);
}
void BridgeScene::setMode(SceneMode mode) {
    m_currentMode = mode;

    // Update status booleans
    m_isSimulationRunning = (mode == SceneMode::Simulating);
    if (!m_isSimulationRunning)
    {
        PhysicsSystem::resetPhysics(*m_entityManager);
        PhysicsSystem::updateBeams(*m_entityManager, 0.01f);
        PhysicsSystem::updateNodes(*m_entityManager, 0.01f);
    }

    // Update panel
    auto* modeEntity = m_entityManager->findEntity("ModePanel");
    if (modeEntity) {
        auto* panel = modeEntity->getComponent<PanelComponent>();
        if (panel) {
            std::wstring modeText = (mode == SceneMode::Build) ? L"Mode: BUILD" : L"Mode: SIMULATING";
            panel->setTitle(modeText);
        }
    }
}
void BridgeScene::handleBuildMode() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePosition();
    Vec2 mousePosWorld = screenToWorld(mousePos.x, mousePos.y);
    auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();

    // First phase: Check for node clicks to start building
    if (!m_nodeAttachedToMouse) {
        for (auto* entity : nodes) {
            bool isMouseOver = entity->getComponent<NodeComponent>()->mouseInside(mousePosWorld);

            if (isMouseOver) {
                // Highlight node
                auto redTexture = Texture2D::LoadTexture2D(
                    m_graphicsDevice->getD3DDevice(),
                    L"DX3D/Assets/Textures/nodeRed.png"
                );
                entity->getComponent<SpriteComponent>()->setTexture(redTexture);

                // Start building if clicked
                if (input.isMouseDown(MouseClick::LeftMouse)) {
                    startBuildingFromNode(entity, mousePosWorld);
                    break;
                }
            }
            else {
                // Reset to normal texture
                auto normalTexture = Texture2D::LoadTexture2D(
                    m_graphicsDevice->getD3DDevice(),
                    L"DX3D/Assets/Textures/node.png"
                );
                entity->getComponent<SpriteComponent>()->setTexture(normalTexture);
            }
        }
    }

    // Second phase: Handle dragging the temporary node
    if (m_nodeAttachedToMouse && m_tempNode) {
        // Update temp node position to follow mouse
        m_tempNode->getComponent<NodeComponent>()->setPosition(mousePosWorld);
        m_tempNode->getComponent<SpriteComponent>()->setPosition(mousePosWorld.x, mousePosWorld.y, 0.0f);

        // Handle mouse release to complete the beam
        if (input.wasMouseJustReleased(MouseClick::LeftMouse)) {
            completeBuildOperation(mousePosWorld);
        }
    }
}

void BridgeScene::startBuildingFromNode(Entity* sourceNode, Vec2 mousePos) {
    m_savedNode = sourceNode;

    // Create temporary node at mouse position
    std::string tempNodeName = "TempNode_" + std::to_string(m_numberOfNodes + 1);
    createNode(mousePos, false, tempNodeName);
    m_tempNode = m_entityManager->findEntity(tempNodeName);

    // Create temporary beam connecting source to temp node
    std::string tempBeamName = "TempBeam_" + std::to_string(m_numberOfBeams + 1);
    createBeam(sourceNode->getName(), tempNodeName, tempBeamName);
    m_tempBeam = m_entityManager->findEntity(tempBeamName);

    m_nodeAttachedToMouse = true;
}

void BridgeScene::completeBuildOperation(Vec2 mousePos) {
    auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    bool connectedToExistingNode = false;
    bool releasedOnSourceNode = false;

    // Check if mouse is over an existing node
    for (auto* entity : nodes) {
        if (entity == m_tempNode) continue; // Skip the temp node

        if (entity->getComponent<NodeComponent>()->mouseInside(mousePos)) {
            if (entity == m_savedNode) {
                // Released on the source node - cancel the operation
                releasedOnSourceNode = true;
            }
            else {
                // Found a different existing node to connect to

                // Remove the temporary beam first
                if (m_tempBeam) {
                    m_entityManager->removeEntity(m_tempBeam->getName());
                    m_numberOfBeams--; // Decrement since we incremented when creating temp beam
                    m_tempBeam = nullptr;
                }

                // Remove the temporary node
                if (m_tempNode) {
                    m_entityManager->removeEntity(m_tempNode->getName());
                    m_numberOfNodes--; // Decrement since we incremented when creating temp node
                    m_tempNode = nullptr;
                }

                // Create a new beam connecting the saved node to the existing node
                std::string newBeamName = "Beam_" + std::to_string(m_numberOfBeams + 1);
                createBeam(m_savedNode->getName(), entity->getName(), newBeamName);

                connectedToExistingNode = true;
            }
            break;
        }
    }

    // Handle cancellation (released on source node)
    if (releasedOnSourceNode) {
        // Remove temporary objects without creating anything permanent
        if (m_tempBeam) {
            m_entityManager->removeEntity(m_tempBeam->getName());
            m_numberOfBeams--; // Decrement since we incremented when creating temp beam
            m_tempBeam = nullptr;
        }

        if (m_tempNode) {
            m_entityManager->removeEntity(m_tempNode->getName());
            m_numberOfNodes--; // Decrement since we incremented when creating temp node
            m_tempNode = nullptr;
        }
    }
    // If not connected to existing node and not cancelled, finalize the temp node position
    else if (!connectedToExistingNode && m_tempNode) {
        // The temp node becomes permanent - just update its starting position
        m_tempNode->getComponent<NodeComponent>()->setStartingPosition(mousePos);
        // No need to decrement counters since the temp node and beam become permanent
    }

    // Update physics
    PhysicsSystem::resetPhysics(*m_entityManager);
    PhysicsSystem::updateBeams(*m_entityManager, 0.01f);
    PhysicsSystem::updateNodes(*m_entityManager, 0.01f);

    // Reset state
    m_nodeAttachedToMouse = false;
    m_savedNode = nullptr;
    m_tempNode = nullptr;
    m_tempBeam = nullptr;
}
void BridgeScene::handleDeleteMode() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePosition();
    Vec2 mousePosWorld = screenToWorld(mousePos.x, mousePos.y);
    auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    auto beams = m_entityManager->getEntitiesWithComponent<BeamComponent>();

    // Reset all visuals first
    resetAllNodeAndBeamTextures();

    bool foundNodeToDelete = false;
    Entity* nodeToDelete = nullptr;

    // Find node under mouse
    for (auto* entity : nodes) {
        if (!entity->getComponent<NodeComponent>()->isFixed() &&
            entity->getComponent<NodeComponent>()->mouseInside(mousePosWorld)) {

            foundNodeToDelete = true;
            nodeToDelete = entity;

            // Highlight node in red
            auto redTexture = Texture2D::LoadTexture2D(
                m_graphicsDevice->getD3DDevice(),
                L"DX3D/Assets/Textures/nodeRed.png"
            );
            entity->getComponent<SpriteComponent>()->setTexture(redTexture);

            // Highlight connected beams in red
            for (auto* beamEntity : beams) {
                auto* beam = beamEntity->getComponent<BeamComponent>();
                if (beam && (beam->isConnectedToNode(entity))) {
                    // Set beam to red texture/tint
                    if (auto* sprite = beamEntity->getComponent<SpriteComponent>()) {
                        sprite->setTint(Vec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                }
            }

            // Handle deletion on click
            if (input.wasMouseJustPressed(MouseClick::LeftMouse)) {
                m_deleteBeamsAndNodes = true;
            }
            break;
        }
    }

    // Execute deletion
    if (m_deleteBeamsAndNodes && nodeToDelete) {
        deleteNodeAndConnectedBeams(nodeToDelete);
        m_deleteBeamsAndNodes = false;

        // Exit delete mode unless shift is held
        if (!input.isKeyDown(Key::Shift)) {
            m_inDeleteMode = false;
        }
    }

    // Exit delete mode on shift release
    if (input.wasKeyJustReleased(Key::Shift)) {
        m_inDeleteMode = false;
    }
}

void BridgeScene::deleteNodeAndConnectedBeams(Entity* nodeToDelete) {
    auto beams = m_entityManager->getEntitiesWithComponent<BeamComponent>();
    std::vector<std::string> beamsToDelete;

    // Find all beams connected to this node
    for (auto* beamEntity : beams) {
        auto* beam = beamEntity->getComponent<BeamComponent>();
        if (beam && beam->isConnectedToNode(nodeToDelete)) {
            beamsToDelete.push_back(beamEntity->getName());
        }
    }

    // Delete connected beams
    for (const auto& beamName : beamsToDelete) {
        m_entityManager->removeEntity(beamName);
        m_numberOfBeams--;
    }

    // Delete the node
    m_entityManager->removeEntity(nodeToDelete->getName());
    m_numberOfNodes--;
}

void BridgeScene::resetAllNodeAndBeamTextures() {
    auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
    auto beams = m_entityManager->getEntitiesWithComponent<BeamComponent>();

    // Reset node textures
    for (auto* entity : nodes) {
        auto normalTexture = Texture2D::LoadTexture2D(
            m_graphicsDevice->getD3DDevice(),
            L"DX3D/Assets/Textures/node.png"
        );
        entity->getComponent<SpriteComponent>()->setTexture(normalTexture);
    }

    // Reset beam tints
    for (auto* entity : beams) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            sprite->setTint(Vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Normal tint
        }
    }
}

void BridgeScene::updateUIStatus() {
    // Update simulation status panel
    auto* uiEntity = m_entityManager->findEntity("StatusPanel");
    if (uiEntity) {
        uiEntity->getComponent<PanelComponent>()->setTitle(
            m_isSimulationRunning ? L"Simulation Running: TRUE" : L"Simulation Running: FALSE"
        );
    }

    // Update mode panel
    auto* modeEntity = m_entityManager->findEntity("ModePanel");
    if (modeEntity) {
        auto* panel = modeEntity->getComponent<PanelComponent>();
        if (panel) {
            std::wstring modeText;
            if (m_currentMode == SceneMode::Build) {
                if (m_inDeleteMode) {
                    modeText = L"Mode: DELETE";
                }
                else {
                    modeText = L"Mode: BUILD";
                }
            }
            else {
                modeText = L"Mode: SIMULATING";
            }
            panel->setTitle(modeText);
        }
    }
    if (auto* infoEntity = m_entityManager->findEntity("InfoPanel")) {
        auto* panel = infoEntity->getComponent<PanelComponent>();
        if (panel) {
            std::wstring infoText;

            if (m_currentMode == SceneMode::Build) {
                if (m_inDeleteMode)
                    infoText = L"Click nodes to delete them (hold Shift for multi-delete).";
                else if (m_nodeAttachedToMouse)
                    infoText = L"Drag to place a new node, release to connect.";
                else
                    infoText = L"Click a node to start building.";
            }
            else if (m_currentMode == SceneMode::Simulating) {
                infoText = L"Simulation is running";
            }

            panel->setTitle(infoText);
        }
    }
}

void BridgeScene::updateButtonInteractions(float dt) {
    auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
    for (auto* entity : buttonEntities) {
        if (auto* button = entity->getComponent<ButtonComponent>()) {
            button->update(dt);
        }
    }
}

// Add this method to toggle delete mode (call from a button or key press)
void BridgeScene::toggleDeleteMode() {
    if (m_currentMode == SceneMode::Simulating) {
        return; // Can't delete in simulation mode
    }
    m_inDeleteMode = !m_inDeleteMode;

    // Reset any ongoing build operation
    if (m_nodeAttachedToMouse) {
        if (m_tempNode) {
            m_entityManager->removeEntity(m_tempNode->getName());
        }
        if (m_tempBeam) {
            m_entityManager->removeEntity(m_tempBeam->getName());
        }
        m_nodeAttachedToMouse = false;
        m_savedNode = nullptr;
        m_tempNode = nullptr;
        m_tempBeam = nullptr;
    }
}