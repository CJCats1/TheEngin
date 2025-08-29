#include <DX3D/Game/Scenes/BridgeScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/PhysicsComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <iostream>

using namespace dx3d;

void BridgeScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_isSimulationRunning = true;
    m_entityManager = std::make_unique<EntityManager>();

    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    createBridge(engine);
    createUI(engine);
}
void BridgeScene::createUI(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    if (!TextSystem::isInitialized())
        TextSystem::initialize(device);
    float screenWidth = static_cast<float>(GraphicsEngine::getWindowWidth());
    float screenHeight = static_cast<float>(GraphicsEngine::getWindowHeight());
    // --- Top-left status panel ---
	auto StatusPanelPosition = Vec2(0.14f, 0.95f); // normalized [0,1] screen coords

    // Status text
    auto& statusTextEntity = m_entityManager->createEntity("UI_StatusText");
    auto& statusText = statusTextEntity.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"Simulation Running: TRUE",
        20.0f
    );
    statusText.setFontFamily(L"Consolas");
    statusText.setColor(Vec4(1.0f, 1.0f, 0.0f, 1.0f));
    statusText.setScreenPosition(StatusPanelPosition.x, StatusPanelPosition.y);
    auto textSize = statusText.getTextSize();
    auto& statusPanelEntity = m_entityManager->createEntity("StatusPanel");
    auto& statusPanel = statusPanelEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png", // A simple semi-transparent panel texture
        textSize.x, textSize.y
    );
    statusPanel.enableScreenSpace(true);
    statusPanel.setScreenPosition(StatusPanelPosition.x, StatusPanelPosition.y); // top-left
    statusPanel.setTint(Vec4(0.1f, 0.1f, 0.1f, 0.7f)); // dark semi-transparent

    // --- Right-side button panel ---
    float buttonWidth = 180.0f;
    float buttonHeight = 40.0f;
    float startX = 0.80f;
    float startY = 0.8f;
    float padding = 0.05f;

    std::vector<std::wstring> buttonLabels = {
        L"Toggle Simulation",
        L"Reset Bridge",
        L"Add Weight",
        L"Remove Weight"
    };

    for (size_t i = 0; i < buttonLabels.size(); ++i) {
        auto& btnEntity = m_entityManager->createEntity("Button_" + std::to_string(i));
        auto& button = btnEntity.addComponent<ButtonComponent>(
            device,
            buttonLabels[i],
            22.0f
        );

        button.setScreenPosition(startX, startY - i * (buttonHeight / screenHeight + padding));
        button.setNormalTint(Vec4(0.2f, 0.6f, 0.8f, 1.0f));
        button.setHoveredTint(Vec4(0.4f, 0.8f, 1.0f, 1.0f));
        button.setPressedTint(Vec4(0.1f, 0.4f, 0.6f, 1.0f));
        button.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        button.setFontSize(18.0f);

        // Callback logic
        button.setOnClickCallback([this, i]() {
            switch (i) {
            case 0: // Toggle simulation
                m_isSimulationRunning = !m_isSimulationRunning;
                break;
            case 1: // Reset bridge
                PhysicsSystem::resetPhysics(*m_entityManager);
                break;
            case 2: // Add weight (example)
                std::cout << "Add weight clicked!\n";
                break;
            case 3: // Remove weight (example)
                std::cout << "Remove weight clicked!\n";
                break;
            }
            });
    }

    // --- Bottom-left mini-panel (example info panel) ---
    auto MiniPanelPosition = Vec2(0.15f, 0.05f);
    

    auto& infoTextEntity = m_entityManager->createEntity("UI_InfoText");
    auto& infoText = infoTextEntity.addComponent<TextComponent>(
        device,
        TextSystem::getRenderer(),
        L"Click nodes to drag them around!",
        18.0f
    );
    infoText.setFontFamily(L"Consolas");
    infoText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    infoText.setScreenPosition(MiniPanelPosition.x, MiniPanelPosition.y);
    
    textSize = infoText.getTextSize();
    auto& infoPanelEntity = m_entityManager->createEntity("InfoPanel");
    auto& infoPanel = infoPanelEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        textSize.x, textSize.y
    );
    infoPanel.enableScreenSpace(true);
    infoPanel.setScreenPosition(MiniPanelPosition.x, MiniPanelPosition.y);
    infoPanel.setTint(Vec4(0.1f, 0.1f, 0.1f, 0.6f));
}
void BridgeScene::createBridge(GraphicsEngine& engine) {
    // Create bridge nodes (simple bridge example)
    createNode(Vec2(-300.0f, 0.0f), true, "LeftAnchor", engine);
    createNode(Vec2(-200.0f, 0.0f), false, "Node1", engine);
    createNode(Vec2(-100.0f, 0.0f), false, "Node2", engine);
    createNode(Vec2(0.0f, 0.0f), false, "Node3", engine);
    createNode(Vec2(100.0f, 0.0f), false, "Node4", engine);
    createNode(Vec2(200.0f, 0.0f), false, "Node5", engine);
    createNode(Vec2(300.0f, 0.0f), true, "RightAnchor", engine);

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

    createBeam("LeftAnchor", "Support1", "Support_Beam9", engine);
    createBeam("RightAnchor", "Support4", "Support_Beam10", engine);

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

    // Physics component
    nodeEntity.addComponent<NodeComponent>(position, fixed);

    // Visual component
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
}

void BridgeScene::update(float dt) {
    updateCameraMovement(dt);
    auto& input = Input::getInstance();

    if (input.wasKeyJustReleased(Key::Z)) {
        m_isSimulationRunning = !m_isSimulationRunning;
    }

    if (input.isKeyDown(Key::R)) {
        PhysicsSystem::resetPhysics(*m_entityManager);
        //m_isSimulationRunning = false;
    }

    if (m_isSimulationRunning) {
        PhysicsSystem::updateNodes(*m_entityManager, dt);
        PhysicsSystem::updateBeams(*m_entityManager, dt);
    }
    auto* uiEntity = m_entityManager->findEntity("UI_StatusText");
    if (uiEntity) {
        if (auto* text = uiEntity->getComponent<TextComponent>()) {
            std::wstring status = m_isSimulationRunning ? L"Simulation Running: TRUE" : L"Simulation Running: FALSE";
            text->setText(status);
        }
    }
    // Handle node dragging
    if (input.isMouseDown(MouseClick::LeftMouse)) {
        if (!m_draggedNode) {
            Vec2 mousePos = input.getMousePosition();
            Vec2 mousePosWorld = screenToWorld(mousePos.x, mousePos.y);
            auto nodes = m_entityManager->getEntitiesWithComponent<NodeComponent>();
            for (auto* entity : nodes) {
                if (entity->getComponent<NodeComponent>()->mouseInside(mousePosWorld)) {
                    m_draggedNode = entity;
                    break;
                }
            }
        }
        else {
            bool textureSet = m_draggedNode->getComponent<NodeComponent>()->isTextureSet;
            if (!textureSet) {
                auto redTexture = Texture2D::LoadTexture2D(
                    m_graphicsDevice->getD3DDevice(),
                    L"DX3D/Assets/Textures/nodeRed.png"
                );
                m_draggedNode->getComponent<SpriteComponent>()->setTexture(redTexture);
                m_draggedNode->getComponent<NodeComponent>()->isTextureSet = true;
            }
            Vec2 mousePos = input.getMousePosition();
            Vec2 mousePosWorld = screenToWorld(mousePos.x, mousePos.y);
            if (!m_draggedNode->getComponent<NodeComponent>()->mouseInside(mousePosWorld)) {
                auto normalTexture = Texture2D::LoadTexture2D(
                    m_graphicsDevice->getD3DDevice(),
                    L"DX3D/Assets/Textures/node.png"
                );
                m_draggedNode->getComponent<SpriteComponent>()->setTexture(normalTexture);
                m_draggedNode->getComponent<NodeComponent>()->isTextureSet = false;
                m_draggedNode = nullptr;
            }
        }
    }
    else if (m_draggedNode) {
        bool textureSet = m_draggedNode->getComponent<NodeComponent>()->isTextureSet;
        if (textureSet) {
            auto normalTexture = Texture2D::LoadTexture2D(
                m_graphicsDevice->getD3DDevice(),
                L"DX3D/Assets/Textures/node.png"
            );
            m_draggedNode->getComponent<SpriteComponent>()->setTexture(normalTexture);
            m_draggedNode->getComponent<NodeComponent>()->isTextureSet = false;
            m_draggedNode = nullptr;
        }
    }




    if (auto* textEntity = m_entityManager->findEntity("UI_Text")) {
        if (auto* debugQuad = m_entityManager->findEntity("DebugQuad"))
        {
            float speed = 0.005f; // normalized space movement per frame
            auto theText = textEntity->getComponent<TextComponent>();
            auto debugQuadSprite = debugQuad->getComponent<SpriteComponent>();
            Vec2 newPos = debugQuadSprite->getScreenPosition();

            if (input.isKeyDown(Key::I)) {
                newPos.y += speed;
            }
            if (input.isKeyDown(Key::K)) {
                newPos.y -= speed;
            }
            if (input.isKeyDown(Key::J)) {
                newPos.x -= speed;
            }
            if (input.isKeyDown(Key::L)) {
                newPos.x += speed;
            }
            theText->setScreenPosition(newPos.x, newPos.y);
            debugQuadSprite->setScreenPosition(newPos.x, newPos.y);
        }
    }




    if (auto* debugQuad = m_entityManager->findEntity("DebugQuad"))
    {
        auto debugQuadSprite = debugQuad->getComponent<SpriteComponent>();
        auto& input = Input::getInstance();

        // Mouse in pixel space
        Vec2 mousePx = input.getMousePositionNDC();

        // Quad center (pixel coords)
        Vec2 center = debugQuadSprite->getScreenPosition();

        float winW = (float)GraphicsEngine::getWindowWidth();
        float winH = (float)GraphicsEngine::getWindowHeight();

        Vec2 centerNDC = debugQuadSprite->getScreenPosition(); // already in NDC
        float halfW_NDC = (debugQuadSprite->getWidth() * 0.5f) / winW;
        float halfH_NDC = (debugQuadSprite->getHeight() * 0.5f) / winH;

        float left = centerNDC.x - halfW_NDC;
        float right = centerNDC.x + halfW_NDC;
        float top = centerNDC.y - halfH_NDC;
        float bottom = centerNDC.y + halfH_NDC;

        Vec2 mouseNDC = input.getMousePositionNDC();

        bool inside = (mouseNDC.x >= left && mouseNDC.x <= right &&
            mouseNDC.y >= top && mouseNDC.y <= bottom);

        if (inside) {
            debugQuadSprite->setTint(Vec4(1, 0, 0, 0.5f)); // red hover
            if (input.isMouseDown(MouseClick::LeftMouse))
            {
                debugQuadSprite->setTint(Vec4(1, 0, 0, 0.8f)); // red hover

            }
            if (input.wasMouseJustReleased(MouseClick::LeftMouse))
            {
                m_isSimulationRunning = !m_isSimulationRunning;
            }
        }
        else {
            debugQuadSprite->setTint(Vec4(0, 0, 1, 0.5f)); // blue default
        }
    }
    auto mouse = Input::getInstance().getMousePositionNDC();
    auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
    for (auto* entity : buttonEntities) {
        if (auto* movement = entity->getComponent<ButtonComponent>()) {
            movement->update(dt);
        }
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
        if (auto* camera = cameraEntity->getComponent<Camera>()) {
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
    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }
    engine.endFrame(swapChain);
}

void BridgeScene::onMouseClick(int button, int x, int y) {
    if (button == 0) {
        Vec2 mouseWorld = screenToWorld(x, y);
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
    if (button == 0) {
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
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera>()) {
            return camera->screenToWorld(Vec2((float)screenX, (float)screenY));
        }
    }
    return Vec2((float)screenX, (float)screenY);
}
