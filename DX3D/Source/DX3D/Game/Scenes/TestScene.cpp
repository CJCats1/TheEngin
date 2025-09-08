// Updated TestScene.cpp with card functionality

#include <DX3D/Game/Scenes/TestScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <cmath>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include "DX3D/Components/DraggableComponent.h"
#include "DX3D/Components/ColliderComponent.h" 
#include "DX3D/Components/CardComponent.h"
#include <iostream>
#include <algorithm>
using namespace dx3d;

void TestScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();

    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();

    // Create camera entity
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);

    // CREATE ALL 52 PLAYING CARDS
    createPlayingCards(device);

    // Keep your existing UI elements
    createUIElements(device);
}

void TestScene::createPlayingCards(GraphicsDevice& device) {
    const float cardWidth = 80.0f;
    const float cardHeight = 120.0f;
    const float cardSpacing = 15.0f;
    const int cardsPerRow = 13; // 13 cards per suit

    // Create cards in a grid layout
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 0; rank < 13; rank++) {
            // Calculate position
            float x = (rank - 6) * (cardWidth + cardSpacing); // Center the row
            float y = (suit - 1.5f) * (cardHeight + cardSpacing * 2); // Center vertically

            // Create entity name
            std::string entityName = "Card_" + std::to_string(suit) + "_" + std::to_string(rank);
            auto& cardEntity = m_entityManager->createEntity(entityName);

            // Add sprite component
            auto& cardSprite = cardEntity.addComponent<SpriteComponent>(
                device, L"DX3D/Assets/Textures/CardSpriteSheet.png", cardWidth, cardHeight
            );

            // Setup spritesheet (13 ranks x 4 suits)
            cardSprite.setupSpritesheet(13, 6);
            cardSprite.setSpriteFrame(rank, suit);
            cardSprite.setPosition(x, y, 0.0f);


            // Add card component with suit and rank data
            auto& cardComp = cardEntity.addComponent<CardComponent>(
                static_cast<Suit>(suit),
                static_cast<Rank>(rank)
            );

            // Add collider for mouse detection
            auto& collider = cardEntity.addComponent<ColliderComponent>(cardWidth, cardHeight);

            // Add draggable component
            auto& draggable = cardEntity.addComponent<DraggableComponent>();
        }
    }
}

void TestScene::createUIElements(GraphicsDevice& device) {
    // Your existing UI code...
    auto& debugQuadEntity = m_entityManager->createEntity("DebugQuad");
    auto& debugQuad = debugQuadEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/node.png", 25.0f, 25.0f
    );
    debugQuad.setPosition(0.0f, 0.0f, 0.0f);
    debugQuad.enableScreenSpace(true);
    debugQuad.setScreenPosition(0.5f, 0.5f);

    // Add card info display
    auto& cardInfoEntity = m_entityManager->createEntity("CardInfo");
    auto& cardInfoText = cardInfoEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Hover over a card", 24.0f
    );
    cardInfoText.setFontFamily(L"Consolas");
    cardInfoText.setColor(Vec4(1.0f, 1.0f, 0.0f, 1.0f));
    cardInfoText.setScreenPosition(0.02f, 0.1f);

    // Add FPS counter
    auto& fpsTextE = m_entityManager->createEntity("UI_FPS");
    auto& fpsText = fpsTextE.addComponent<TextComponent>(device, TextSystem::getRenderer(), L"FPS: 0", 20.0f);
    fpsText.setScreenPosition(0.05f, 0.02f);
    fpsText.setColor(Vec4(1, 1, 0, 1));
}

void TestScene::update(float dt) {
    auto& input = Input::getInstance();

    // Update camera movement
    updateCameraMovement(dt);

    // Handle card dragging
    updateCardDragging();

    // Update card hover effects
    updateCardHoverEffects();

    // Update FPS counter
    updateFPSCounter(dt);

    // Your existing movement/animation updates...
    auto animatedEntities = m_entityManager->getEntitiesWithComponent<AnimationComponent>();
    for (auto* entity : animatedEntities) {
        if (auto* animation = entity->getComponent<AnimationComponent>()) {
            animation->update(*entity, dt);
        }
    }
}

void TestScene::updateCardDragging() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePositionNDC();
    // Convert mouse position to world coordinates
    Vec2 worldMousePos = screenToWorldPosition(mousePos);

    // Check for mouse click to start dragging
    if (input.wasMouseJustPressed(MouseClick::LeftMouse)) {
        // Find the topmost card under the mouse
        Entity* topCard = findCardUnderMouse(worldMousePos);
        if (topCard) {
            if (auto* draggable = topCard->getComponent<DraggableComponent>()) {
                if (auto* sprite = topCard->getComponent<SpriteComponent>()) {
                    draggable->setDragging(true);
                    draggable->setOriginalPosition(sprite->getPosition());

                    // Calculate offset from mouse to sprite center
                    Vec3 spritePos = sprite->getPosition();
                    Vec2 offset = Vec2(spritePos.x, spritePos.y) - worldMousePos;
                    draggable->setDragOffset(offset);

                    // Bring card to front by adjusting Z position
                    sprite->setPosition(spritePos.x, spritePos.y, 0.0f);
                }
            }
        }
    }

    // Update dragging cards
    if (input.isMouseDown(MouseClick::LeftMouse)) {
        auto draggableEntities = m_entityManager->getEntitiesWithComponent<DraggableComponent>();
        for (auto* entity : draggableEntities) {
            if (auto* draggable = entity->getComponent<DraggableComponent>()) {
                if (draggable->isDragging()) {
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        Vec2 offset = draggable->getDragOffset();
                        Vec2 newPos = worldMousePos + offset;
                        sprite->setPosition(newPos.x, newPos.y, 0.0f); // Keep on top
                    }
                }
            }
        }
    }

    // Stop dragging on mouse release
    if (input.wasMouseJustReleased(MouseClick::LeftMouse)) {
        auto draggableEntities = m_entityManager->getEntitiesWithComponent<DraggableComponent>();
        for (auto* entity : draggableEntities) {
            if (auto* draggable = entity->getComponent<DraggableComponent>()) {
                if (draggable->isDragging()) {
                    draggable->setDragging(false);

                    // Reset Z position
                    if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                        Vec3 pos = sprite->getPosition();
                        sprite->setPosition(pos.x, pos.y, 0.0f);
                    }
                }
            }
        }
    }
}

void TestScene::updateCardHoverEffects() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePositionNDC();
    Vec2 worldMousePos = screenToWorldPosition(mousePos);

    Entity* hoveredCard = findCardUnderMouse(worldMousePos);

    // Reset all card tints first
    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardComponent>();
    for (auto* entity : cardEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (auto* draggable = entity->getComponent<DraggableComponent>()) {
                if (!draggable->isDragging()) {
                    sprite->setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Reset to normal
                }
            }
        }
    }

    // Highlight hovered card
    if (hoveredCard) {
        if (auto* sprite = hoveredCard->getComponent<SpriteComponent>()) {
            if (auto* draggable = hoveredCard->getComponent<DraggableComponent>()) {
                if (!draggable->isDragging()) {
                    sprite->setTint(Vec4(1.0f, 1.0f, 0.5f, 0.5f)); // Slight yellow tint
                }
            }
        }

        // Update card info text
        if (auto* cardComp = hoveredCard->getComponent<CardComponent>()) {
            if (auto* infoEntity = m_entityManager->findEntity("CardInfo")) {
                if (auto* infoText = infoEntity->getComponent<TextComponent>()) {
                    std::wstring cardName = cardComp->getCardName();
                    infoText->setText(L"Card: " + cardName);
                }
            }
        }
    }
    else {
        // No card hovered, show default text
        if (auto* infoEntity = m_entityManager->findEntity("CardInfo")) {
            if (auto* infoText = infoEntity->getComponent<TextComponent>()) {
                infoText->setText(L"Hover over a card");
            }
        }
    }
}

Entity* TestScene::findCardUnderMouse(const Vec2& worldMousePos) {
    Entity* topCard = nullptr;
    float highestZ = -999.0f;

    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardComponent>();
    for (auto* entity : cardEntities) {
        if (auto* collider = entity->getComponent<ColliderComponent>()) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                Vec3 spritePos = sprite->getPosition();

                if (collider->containsPoint(worldMousePos, spritePos)) {
                    if (spritePos.z > highestZ) {
                        highestZ = spritePos.z;
                        topCard = entity;
                    }
                }
            }
        }
    }

    return topCard;
}

Vec2 TestScene::screenToWorldPosition(const Vec2& screenPos) {
    // Get camera for coordinate conversion
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            float screenWidth = GraphicsEngine::getWindowWidth();
            float screenHeight = GraphicsEngine::getWindowHeight();

            // screenPos is already in [0,1] range, not NDC [-1,1]
            // Convert directly to pixel coordinates
            float pixelX = screenPos.x * screenWidth;
            float pixelY = ( screenPos.y) * screenHeight; // Flip Y since screen coords have Y=0 at top

            // Convert to world coordinates accounting for camera
            Vec2 cameraPos = camera->getPosition();
            float zoom = camera->getZoom();

            float worldX = (pixelX - screenWidth * 0.5f) / zoom + cameraPos.x;
            float worldY = (pixelY - screenHeight * 0.5f) / zoom + cameraPos.y;

            return Vec2(worldX, worldY);
        }
    }

    return Vec2(0, 0);
}
void TestScene::updateFPSCounter(float dt) {
    static float timer = 0;
    static int frames = 0;
    timer += dt;
    frames++;

    if (timer >= 1.0f) {
        if (auto* fpsEntity = m_entityManager->findEntity("UI_FPS")) {
            if (auto* fpsText = fpsEntity->getComponent<TextComponent>()) {
                fpsText->setText(L"FPS: " + std::to_wstring(frames));
            }
        }
        frames = 0;
        timer = 0;
    }
}

void TestScene::updateCameraMovement(float dt) {
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

    if (zoomDelta != 0.0f) {
        camera->zoom(zoomDelta);
    }

    if (input.isKeyDown(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(1.0f);
    }
}

void TestScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    engine.beginFrame(swapChain);
    auto& ctx = engine.getContext();
    // Set up camera matrices
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // Render all cards (sorted by Z position for proper layering)
    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardComponent>();

    // Sort by Z position (back to front)
    std::sort(cardEntities.begin(), cardEntities.end(), [](Entity* a, Entity* b) {
        auto* spriteA = a->getComponent<SpriteComponent>();
        auto* spriteB = b->getComponent<SpriteComponent>();
        if (spriteA && spriteB) {
            return spriteA->getPosition().z < spriteB->getPosition().z;
        }
        return false;
        });

    // Render sorted cards
    for (auto* entity : cardEntities) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render screen-space UI elements
    for (auto* entity : m_entityManager->getEntitiesWithComponent<SpriteComponent>()) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isScreenSpace()) {
                sprite->draw(ctx);
            }
        }
    }

    // Render text
    for (auto* entity : m_entityManager->getEntitiesWithComponent<TextComponent>()) {
        if (auto* text = entity->getComponent<TextComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx);
            }
        }
    }

    engine.endFrame(swapChain);
}