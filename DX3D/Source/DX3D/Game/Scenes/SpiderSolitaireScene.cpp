#include "SpiderSolitaireScene.h"
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/DraggableComponent.h>
#include <DX3D/Components/ColliderComponent.h>
#include <DX3D/Components/CardComponent.h>
#include <DX3D/Components/CardFrameComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>

using namespace dx3d;

// CardStack implementation
void CardStack::addCard(Entity* card, float zSpacing) {
    cards.push_back(card);
    updateCardPositions(zSpacing);
}

Entity* CardStack::removeTopCard(float zSpacing) {
    if (cards.empty()) return nullptr;
    Entity* card = cards.back();
    cards.pop_back();
    updateCardPositions(zSpacing);
    return card;
}

Entity* CardStack::getTopCard() const {
    return cards.empty() ? nullptr : cards.back();
}

void CardStack::updateCardPositions(float zSpacing) {
    for (size_t i = 0; i < cards.size(); ++i) {
        if (auto* sprite = cards[i]->getComponent<SpriteComponent>()) {
            float yOffset = faceDown ? cardOffset * 0.3f : cardOffset;
            Vec2 targetPos;
            float zDepth;
            
            if (cardOffset < 5.0f) { // Stock pile
                zDepth = -50 + (static_cast<float>(i) * 0.1f);
                targetPos = Vec2(position.x, position.y - (i * 0.3f));
                
            }
            else {
                zDepth = -50 + (static_cast<float>(i) * 0.5f);
                targetPos = Vec2(position.x, position.y - (i * yOffset));
            }

            // Move the frame to the target position
            if (auto* frame = cards[i]->getComponent<CardFrameComponent>()) {
                frame->setPosition(targetPos);
            }
            
            // Update physics target if card has physics component
            if (auto* physics = cards[i]->getComponent<CardPhysicsComponent>()) {
                physics->setTargetPosition(targetPos);
                physics->setRestPosition(targetPos);
                
            }

            // Ensure sprite Z matches stack layering while preserving current X/Y
            Vec3 currentPos = sprite->getPosition();
            sprite->setPosition(currentPos.x, currentPos.y, zDepth);
        }
    }
}

bool CardStack::canDropCard(Entity* card) const {
    if (cards.empty()) {
        return true; // Can always drop on empty stack
    }

    auto* cardComp = card->getComponent<CardComponent>();
    auto* topCardComp = getTopCard()->getComponent<CardComponent>();

    if (!cardComp || !topCardComp) return false;

    // In Spider Solitaire, cards must be placed in descending rank order
    // Suit doesn't matter for initial placement, but matters for moving sequences
    return static_cast<int>(cardComp->getRank()) == static_cast<int>(topCardComp->getRank()) - 1;
}

// SpiderSolitaireScene implementation
SpiderSolitaireScene::SpiderSolitaireScene(SpiderDifficulty difficulty)
    : m_difficulty(difficulty), m_completedSuits(0), m_draggedCard(nullptr),
    m_isDragging(false), m_dragOffset(0, 0), m_celebrationActive(false),
    m_celebrationTimer(0.0f), m_gravity(200.0f), m_lastMousePosition(0, 0),
    m_dragVelocity(0, 0), m_dragVelocitySmoothing(0.8f) {
}

void SpiderSolitaireScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();

    // Create line renderer for debug visualization
    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->setPosition(0.0f, 0.0f);
    

    // Create camera
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(0.8f); // Zoom out to see more cards

    // Setup tableau (10 columns)
    m_tableau.resize(10);
    for (int i = 0; i < 10; ++i) {
        m_tableau[i].position = Vec2(TABLEAU_START_X + i * COLUMN_SPACING, TABLEAU_Y);
        m_tableau[i].cardOffset = 25.0f;
    }

    // Setup foundations (8 completed suit stacks)
    m_foundations.resize(8);
    for (int i = 0; i < 8; ++i) {
        // Use full column spacing for better visibility
        m_foundations[i].position = Vec2(TABLEAU_START_X + i * COLUMN_SPACING*0.8, FOUNDATION_Y);
        m_foundations[i].cardOffset = 2.0f; // Tightly stacked
    }
    // Setup stock pile
    m_stock.position = Vec2(STOCK_X, STOCK_Y);
    m_stock.cardOffset = 1.0f; // Very tight stack

    // Create empty spot indicators
    createEmptySpots(device);

    // Create all cards and setup game
    createCards(device);
    setupTableau();
    dealInitialCards();
    createUI(device);

    int numDeals = m_stock.size() / 10; // each deal = 10 cards
    m_stockIndicators.clear();

    for (int i = 0; i < numDeals; ++i) {
        auto& fakeStock = m_entityManager->createEntity("StockIndicator_" + std::to_string(i));
        auto& sprite = fakeStock.addComponent<SpriteComponent>(
            device, L"DX3D/Assets/Textures/CardSpriteSheet.png", CARD_WIDTH, CARD_HEIGHT
        );

        sprite.setupSpritesheet(13, 6);
        sprite.setSpriteFrame(3, 4); // card back

        // Position each indicator with proper Z-depth
        float x = STOCK_X;
        float y = STOCK_Y + i * 5;
        float z = -(i+20) * 0.1f; // Each indicator slightly in front

        sprite.setPosition(Vec3(x, y, z));
        sprite.setVisible(true); // Ensure visibility

        m_stockIndicators.push_back(&fakeStock);
    }
    // Initialize stock click area
    m_stockClickArea.position = Vec2(STOCK_X, STOCK_Y);
    m_stockClickArea.width = CARD_WIDTH * 1.5f;  // Make it a bit larger than card
    m_stockClickArea.height = CARD_HEIGHT * 1.5f;

	auto& undoButtonEntity = m_entityManager->createEntity("UndoButton");
    auto& undoButton = undoButtonEntity.addComponent<ButtonComponent>(
        device,
        L"Undo",
        22.0f
    );
    undoButton.setOnClickCallback([this]() {
        if ( !m_celebrationActive) {
            restoreGameState();
        }
		});
    undoButton.enableScreenSpace(true);
	undoButton.setScreenPosition(0.9f,0.8f);
    undoButton.setNormalTint(Vec4(0.2f, 0.6f, 0.8f, 0.5f));
    undoButton.setHoveredTint(Vec4(0.4f, 0.8f, 1.0f, 0.5f));
    undoButton.setPressedTint(Vec4(0.1f, 0.4f, 0.6f, 0.5f));
    
    // Create debug toggle button
    createDebugToggleButton();
    
    // Create transparent sprite at world origin LAST to ensure LineRenderer works
    auto& transparentSpriteEntity = m_entityManager->createEntity("TransparentSprite");
    auto& transparentSprite = transparentSpriteEntity.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/CardSpriteSheet.png", 1.0f, 1.0f
    );
    transparentSprite.setPosition(Vec3(0.0f, 0.0f, 0.0f)); 
    transparentSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Completely transparent
    transparentSprite.setVisible(true);
}

void SpiderSolitaireScene::createEmptySpots(GraphicsDevice& device) {
    // Create empty spots for tableau columns
    m_tableauEmptySpots.resize(10);
    for (int i = 0; i < 10; ++i) {
        auto& emptySpot = m_entityManager->createEntity("TableauEmpty_" + std::to_string(i));
        auto& sprite = emptySpot.addComponent<SpriteComponent>(
            device, L"DX3D/Assets/Textures/CardSpriteSheet.png", CARD_WIDTH, CARD_HEIGHT
        );

        sprite.setupSpritesheet(13, 6);
        sprite.setSpriteFrame(12, 4); 

        // Position at tableau base with appropriate Z-depth
        float x = TABLEAU_START_X + i * COLUMN_SPACING;
        float y = TABLEAU_Y;
        float z = -100;

        sprite.setPosition(Vec3(x, y, z));
        sprite.setVisible(true); 

        m_tableauEmptySpots[i] = &emptySpot;
    }

    // Create empty spots for foundations
    m_foundationEmptySpots.resize(8);
    for (int i = 0; i < 8; ++i) {
        auto& emptySpot = m_entityManager->createEntity("FoundationEmpty_" + std::to_string(i));
        auto& sprite = emptySpot.addComponent<SpriteComponent>(
            device, L"DX3D/Assets/Textures/CardSpriteSheet.png", CARD_WIDTH, CARD_HEIGHT
        );

        sprite.setupSpritesheet(13, 6);
        sprite.setSpriteFrame(12, 4); // Empty spot frame

        // Position at foundation base - USE FULL COLUMN SPACING
        float x = TABLEAU_START_X + i * COLUMN_SPACING*0.8;  // Changed from COLUMN_SPACING * 0.6f
        float y = FOUNDATION_Y;
        float z = -100; // Empty spots behind cards

        sprite.setPosition(Vec3(x, y, z));
        sprite.setVisible(true); // Foundations start empty

        m_foundationEmptySpots[i] = &emptySpot;
    }

    // Create empty spot for stock pile (shows when stock is completely empty)
    auto& stockEmptySpot = m_entityManager->createEntity("StockEmpty");
    auto& stockSprite = stockEmptySpot.addComponent<SpriteComponent>(
        device, L"DX3D/Assets/Textures/CardSpriteSheet.png", CARD_WIDTH, CARD_HEIGHT
    );

    stockSprite.setupSpritesheet(13, 6);
    stockSprite.setSpriteFrame(11, 4); // Empty spot frame

    // Position at stock location
    stockSprite.setPosition(Vec3(STOCK_X, STOCK_Y, -100));
    stockSprite.setVisible(true); // Hidden initially

    m_stockEmptySpot = &stockEmptySpot;
}

void SpiderSolitaireScene::updateEmptySpotVisibility() {
    // Update tableau empty spots
    for (int i = 0; i < 10; ++i) {
        if (auto* sprite = m_tableauEmptySpots[i]->getComponent<SpriteComponent>()) {
            sprite->setVisible(m_tableau[i].isEmpty());
        }
    }

    // Update foundation empty spots
    for (int i = 0; i < 8; ++i) {
        if (auto* sprite = m_foundationEmptySpots[i]->getComponent<SpriteComponent>()) {
            sprite->setVisible(m_foundations[i].isEmpty());
        }
    }

    // Update stock empty spot
    if (auto* sprite = m_stockEmptySpot->getComponent<SpriteComponent>()) {
        sprite->setVisible(m_stock.isEmpty() && m_stockIndicators.empty());
    }
}

void SpiderSolitaireScene::createCards(GraphicsDevice& device) {
    // Create 2 decks (104 cards total)
    std::vector<Entity*> allCards;

    for (int deck = 0; deck < 2; ++deck) {
        for (int suit = 0; suit < 4; ++suit) {
            for (int rank = 0; rank < 13; ++rank) {
                std::string entityName = "Card_" + std::to_string(deck) + "_" +
                    std::to_string(suit) + "_" + std::to_string(rank);
                auto& cardEntity = m_entityManager->createEntity(entityName);

                // Add sprite component
                auto& cardSprite = cardEntity.addComponent<SpriteComponent>(
                    device, L"DX3D/Assets/Textures/CardSpriteSheet.png", CARD_WIDTH, CARD_HEIGHT
                );

                cardSprite.setupSpritesheet(13, 6);

                // Set appropriate suit based on difficulty
                int actualSuit = suit;
                if (m_difficulty == SpiderDifficulty::OneSuit) {
                    actualSuit = 0; // All spades
                }
                else if (m_difficulty == SpiderDifficulty::TwoSuit) {
                    actualSuit = (suit < 2) ? 0 : 1; // Spades and hearts only
                }

                cardSprite.setSpriteFrame(rank, actualSuit);

                // Add card component
                auto& cardComp = cardEntity.addComponent<CardComponent>(
                    static_cast<Suit>(actualSuit),
                    static_cast<Rank>(rank)
                );
                cardComp.setFaceUp(false);
                // Show card back initially
                cardSprite.setSpriteFrame(3, 4); // Card back frame
                
                // Add frame component for rigid frame reference
                auto& frame = cardEntity.addComponent<CardFrameComponent>();
                
                // Add physics component for springy behavior
                auto& physics = cardEntity.addComponent<CardPhysicsComponent>();
                
                // Connect physics to frame
                physics.setFrame(&frame);
                
                // Add some initial random jitter to make physics more noticeable
                physics.addRandomJitter(10.0f);
                
                // Add collider and draggable
                auto& collider = cardEntity.addComponent<ColliderComponent>(CARD_WIDTH, CARD_HEIGHT);
                auto& draggable = cardEntity.addComponent<DraggableComponent>();

                allCards.push_back(&cardEntity);
            }
        }
    }

    // Shuffle the deck
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(allCards.begin(), allCards.end(), gen);

    // Add all cards to stock initially
    for (auto* card : allCards) {
        m_stock.addCard(card, -1);
    }
}

void SpiderSolitaireScene::setupTableau() {
    // Set base Z depths for each stack
    for (int col = 0; col < 10; ++col) {
        m_tableau[col].baseZDepth = 0;
    }

    for (int i = 0; i < 8; ++i) {
        m_foundations[i].baseZDepth = 0;
    }

   
    m_stock.baseZDepth = 0; 

    // Spider Solitaire initial deal:
    for (int col = 0; col < 10; ++col) {
        int cardCount = (col < 4) ? 6 : 5;

        for (int i = 0; i < cardCount; ++i) {
            if (!m_stock.isEmpty()) {
                Entity* card = m_stock.removeTopCard(0.1f); // Smaller spacing for stock operations
                m_tableau[col].addCard(card, 0);

                // Add some initial physics to make cards settle into position
                if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
                    // Add a gentle downward velocity as if cards were just dealt
                    physics->addVelocity(Vec2(0.0f, -30.0f));
                    // Add tiny random jitter for natural settling
                    physics->addRandomJitter(5.0f);
                    
                    // Make sure target position is set correctly
                    if (auto* sprite = card->getComponent<SpriteComponent>()) {
                        Vec3 spritePos = sprite->getPosition();
                        physics->setTargetPosition(Vec2(spritePos.x, spritePos.y));
                        physics->setRestPosition(Vec2(spritePos.x, spritePos.y));
                    }
                }

                // Set face up/down status
                if (auto* sprite = card->getComponent<SpriteComponent>()) {
                    bool isFaceUp = (i == cardCount - 1);
                    if (!isFaceUp) {
                        sprite->setSpriteFrame(3, 4); // Card back
                    }
                    else {
                        // Show actual card
                        if (auto* cardComp = card->getComponent<CardComponent>()) {
                            sprite->setSpriteFrame(
                                static_cast<int>(cardComp->getRank()),
                                static_cast<int>(cardComp->getSuit())
                            );
                        }
                    }
                    if (auto* cardComp = card->getComponent<CardComponent>()) {
                        cardComp->setFaceUp(isFaceUp);
                    }
                }
            }
        }
    }
}

void SpiderSolitaireScene::dealInitialCards() {
    // Cards are already dealt in setupTableau
    // This could be used for dealing new rows from stock
}

void SpiderSolitaireScene::createUI(GraphicsDevice& device) {
    // Game title
    auto& titleEntity = m_entityManager->createEntity("GameTitle");
    auto& titleText = titleEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Spider Solitaire", 32.0f
    );
    titleText.setFontFamily(L"Arial");
    titleText.setColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    titleText.setScreenPosition(0.5f, 0.02f);

    // Score display
    auto& scoreEntity = m_entityManager->createEntity("ScoreText");
    auto& scoreText = scoreEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Completed Suits: 0/8", 24.0f
    );
    scoreText.setFontFamily(L"Consolas");
    scoreText.setColor(Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    scoreText.setScreenPosition(0.15f, 0.1f);

    // Instructions - updated to include undo and physics debug
    auto& instructEntity = m_entityManager->createEntity("Instructions");
    auto& instructText = instructEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(),
        L"Build sequences K-A in same suit | Space: Deal new row | Z: Undo | P: Physics test | WASD: Move camera", 18.0f
    );
    instructText.setFontFamily(L"Arial");
    instructText.setColor(Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    instructText.setScreenPosition(0.3f, 0.95f);

    // FPS counter
    auto& fpsEntity = m_entityManager->createEntity("UI_FPS");
    auto& fpsText = fpsEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"FPS: 0", 20.0f
    );
    fpsText.setScreenPosition(0.95f, 0.02f);
    fpsText.setColor(Vec4(1, 1, 0, 1));

    // Stock pile indicator
    auto& stockEntity = m_entityManager->createEntity("StockInfo");
    auto& stockText = stockEntity.addComponent<TextComponent>(
        device, TextSystem::getRenderer(), L"Stock: 50 cards", 20.0f
    );
    stockText.setFontFamily(L"Consolas");
    stockText.setColor(Vec4(1.0f, 0.8f, 0.2f, 1.0f));
    stockText.setScreenPosition(0.90f, 0.95f);




}

void SpiderSolitaireScene::update(float dt) {
	updateCameraMovement(dt);

    // Update celebration before normal card dragging
    updateCelebration(dt);
    applySubtleCelebrationEffects(dt);

    // Only allow normal gameplay if celebration isn't active
    if (!m_celebrationActive) {
        updateCardDragging();
        updateCardHoverEffects();
    }

    // Update physics for all cards
    updateCardPhysics(dt);

    // Apply magnetic attraction for cards near valid drop zones
    applyMagneticAttraction(dt);

    updateGameLogic();
    updateFPSCounter(dt);

    auto& input = Input::getInstance();

    // Handle space key for dealing new row (only if not celebrating)
    if (input.wasKeyJustPressed(Key::Space) && !m_celebrationActive) {
        dealNewRow();
    }

    // Handle undo - Ctrl+Z or just Z (only if not celebrating)
    if (input.wasKeyJustPressed(Key::Z) && !m_celebrationActive) {
        restoreGameState();
    }

    // Allow restarting celebration with R key
    if (input.wasKeyJustPressed(Key::T) && isGameWon()) {
        startCelebration();
    }

    // Debug: Press P to add random physics to all cards (for testing)
    if (input.wasKeyJustPressed(Key::P)) {
        auto cardEntities = m_entityManager->getEntitiesWithComponent<CardPhysicsComponent>();
        std::cout << "Physics Debug: P key pressed! Found " << cardEntities.size() << " cards to jitter" << std::endl;
        
        int jitteredCount = 0;
        for (auto* cardEntity : cardEntities) {
            if (auto* physics = cardEntity->getComponent<CardPhysicsComponent>()) {
                // Make sure the target position is set correctly before jittering
                auto* sprite = cardEntity->getComponent<SpriteComponent>();
                if (sprite) {
                    Vec3 currentPos = sprite->getPosition();
                    physics->setTargetPosition(Vec2(currentPos.x, currentPos.y));
                    physics->setRestPosition(Vec2(currentPos.x, currentPos.y));
                }
                
                physics->addRandomJitter(200.0f); // Extremely strong jitter for dramatic effect
                
                // DEBUG: Check velocity immediately after jitter
                Vec2 immediateVel = physics->getVelocity();
                
                jitteredCount++;
            }
        }
    }

    auto buttonEntities = m_entityManager->getEntitiesWithComponent<ButtonComponent>();
    for (auto* entity : buttonEntities) {
        if (auto* button = entity->getComponent<ButtonComponent>()) {
            button->update(dt);
        }
    }
    
    // Update frame debug visualization
    updateFrameDebugVisualization();
}

void SpiderSolitaireScene::updateCardDragging() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePositionNDC();
    Vec2 worldMousePos = screenToWorldPosition(mousePos);
    
    // Calculate drag velocity for physics
    Vec2 mouseDelta = worldMousePos - m_lastMousePosition;
    m_dragVelocity = m_dragVelocity * m_dragVelocitySmoothing + mouseDelta * (1.0f - m_dragVelocitySmoothing);
    m_lastMousePosition = worldMousePos;

    // Start dragging
    if (input.wasMouseJustPressed(MouseClick::LeftMouse)) {
        // First check if clicking on stock area

        Entity* clickedCard = findCardUnderMouse(worldMousePos);
        if (clickedCard) {
            if (auto* cardComp = clickedCard->getComponent<CardComponent>()) {
                if (cardComp->isFaceUp()) {
                    m_draggedSequence = getCardSequence(clickedCard);
                    if (!m_draggedSequence.empty()) {
                        // SAVE STATE before starting drag (potential move)
                        saveGameState();

                        m_draggedCard = clickedCard;
                        m_isDragging = true;

                        if (auto* sprite = clickedCard->getComponent<SpriteComponent>()) {
                            Vec3 spritePos = sprite->getPosition();
                            m_dragOffset = Vec2(spritePos.x, spritePos.y) - worldMousePos;
                        }

                        for (size_t i = 0; i < m_draggedSequence.size(); ++i) {
                            if (auto* sprite = m_draggedSequence[i]->getComponent<SpriteComponent>()) {
                                Vec3 pos = sprite->getPosition();
                                // Bring dragged cards to the front, keep sequence order on top
                                float frontZ = -1.0f - static_cast<float>(i) * 0.01f;
                                sprite->setPosition(pos.x, pos.y, frontZ);
                            }
                            
                            if (auto* physics = m_draggedSequence[i]->getComponent<CardPhysicsComponent>()) {
                                physics->reset();
                                physics->setDragMode();
                                physics->setDragging(true); // Set dragging state
                            }
                            
                            // Move the frame to the current position for dragging
                            if (auto* frame = m_draggedSequence[i]->getComponent<CardFrameComponent>()) {
                                if (auto* sprite = m_draggedSequence[i]->getComponent<SpriteComponent>()) {
                                    Vec3 pos = sprite->getPosition();
                                    frame->setPosition(Vec2(pos.x, pos.y));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

       // Update dragging position
       if (m_isDragging && !m_draggedSequence.empty()) {
        Vec2 newPos = worldMousePos + m_dragOffset;
        for (size_t i = 0; i < m_draggedSequence.size(); ++i) {
            if (auto* sprite = m_draggedSequence[i]->getComponent<SpriteComponent>()) {
                Vec2 cardPos = Vec2(newPos.x, newPos.y - i * 25.0f);

                // Move the frame to the new position
                if (auto* frame = m_draggedSequence[i]->getComponent<CardFrameComponent>()) {
                    frame->setPosition(cardPos);
                }

                if (auto* physics = m_draggedSequence[i]->getComponent<CardPhysicsComponent>()) {
                    physics->setDragMode();
                    physics->setTargetPosition(cardPos);
                }
            }
        }
    }

    // Stop dragging and handle drop
    if (input.wasMouseJustReleased(MouseClick::LeftMouse)) {
        if (m_stockClickArea.containsPoint(worldMousePos)) {
            dealNewRow();
            return; // Don't process card dragging if we clicked stock
        }
        if (m_isDragging)
        {
            bool validDrop = false;

            // Find target stack
            for (auto& stack : m_tableau) {
                float distanceToStack = abs(worldMousePos.x - stack.position.x);
                if (distanceToStack < COLUMN_SPACING * 0.4f) {
                    if (stack.canDropCard(m_draggedCard) || stack.isEmpty()) {
                        CardStack* sourceStack = findStackContaining(m_draggedCard);
                        if (sourceStack) {
                            // Remove cards from source
                            for (int i = 0; i < static_cast<int>(m_draggedSequence.size()); ++i) {
                                sourceStack->removeTopCard(0);
                            }
                            // Add to target and set physics target positions
                            for (size_t cardIdx = 0; cardIdx < m_draggedSequence.size(); ++cardIdx) {
                                auto* card = m_draggedSequence[cardIdx];
                                stack.addCard(card, 0);

                                if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
                                    // Ensure physics is properly reset and activated
                                    physics->setDragging(false);
                                    physics->setSettling(true);
                                    physics->setNormalMode();
                                    
                                    // Use frame position which was set by addCard/updateCardPositions
                                    if (auto* frame = card->getComponent<CardFrameComponent>()) {
                                        Vec2 framePos = frame->getPosition();
                                        physics->setTargetPosition(framePos);
                                        physics->setRestPosition(framePos);

                                        // Add a small upward velocity for gentle settling bounce
                                        physics->addVelocity(Vec2(0.0f, 50.0f)); // Gentle upward bounce
                                        
                                        // Debug output to verify physics is being set up
                                        std::cout << "Valid drop: Setting physics target to (" << framePos.x << "," << framePos.y << ")" << std::endl;
                                    }
                                }
                            }

                            // Flip top card in source stack if needed
                            if (!sourceStack->isEmpty()) {
                                Entity* topCard = sourceStack->getTopCard();
                                if (auto* cardComp = topCard->getComponent<CardComponent>()) {
                                    if (!cardComp->isFaceUp()) {
                                        cardComp->setFaceUp(true);
                                        if (auto* sprite = topCard->getComponent<SpriteComponent>()) {
                                            sprite->setSpriteFrame(
                                                static_cast<int>(cardComp->getRank()),
                                                static_cast<int>(cardComp->getSuit())
                                            );
                                        }
                                    }
                                }
                            }

                            // CRITICAL FIX: Check if this move created any complete sequences
                            // If so, save another state before they get auto-removed
                            bool hasCompletedSequence = false;
                            for (auto& checkStack : m_tableau) {
                                if (isSequenceComplete(checkStack)) {
                                    hasCompletedSequence = true;
                                    break;
                                }
                            }

                            if (hasCompletedSequence) {
                                // Save state BEFORE sequences get automatically removed
                                saveGameState();
                            }

                            validDrop = true;
                        }
                    }
                    break;
                }
            }

            if (!validDrop) {
                // Invalid drop - remove the saved state since no move occurred
                if (!m_undoStack.empty()) {
                    m_undoStack.pop_back();
                }

                // Return cards to original position with bouncy physics
                CardStack* originalStack = findStackContaining(m_draggedCard);
                if (originalStack) {
                    originalStack->updateCardPositions(0);
                    
                    // Add bounce effect to all dragged cards
                    for (auto* card : m_draggedSequence) {
                        if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
                            // Ensure physics is properly reset and activated
                            physics->setDragging(false);
                            physics->setSettling(true);
                            physics->setNormalMode();
                            
                            // Set target to original position - use the frame position which was set by updateCardPositions
                            if (auto* frame = card->getComponent<CardFrameComponent>()) {
                                Vec2 framePos = frame->getPosition();
                                physics->setTargetPosition(framePos);
                                physics->setRestPosition(framePos);
                                
                                // Add bounce away from mouse
                                Vec2 bounceDirection = (framePos - worldMousePos).normalized();
                                physics->applyBounce(bounceDirection);
                                
                                // Add subtle random jitter for more dynamic feel
                                physics->addRandomJitter(15.0f);
                                
                                // Debug output to verify physics is being set up
                                std::cout << "Invalid drop: Setting physics target to (" << framePos.x << "," << framePos.y << ")" << std::endl;
                            }
                        }
                    }
                }
            }

            m_isDragging = false;
            m_draggedCard = nullptr;
            m_draggedSequence.clear();
        }
    }
}

void SpiderSolitaireScene::updateCardHoverEffects() {
    auto& input = Input::getInstance();
    Vec2 mousePos = input.getMousePositionNDC();
    Vec2 worldMousePos = screenToWorldPosition(mousePos);

    Entity* hoveredCard = findCardUnderMouse(worldMousePos);

    // Reset all card tints
    for (auto& stack : m_tableau) {
        for (auto* card : stack.cards) {
            if (auto* sprite = card->getComponent<SpriteComponent>()) {
                if (!m_isDragging) {
                    sprite->setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f));
                }
            }
        }
    }

    // Highlight valid sequences
    if (hoveredCard && !m_isDragging) {
        auto sequence = getCardSequence(hoveredCard);
        for (auto* card : sequence) {
            if (auto* sprite = card->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(0.8f, 1.0f, 0.8f, 0.3f));
            }
        }
    }
}

void SpiderSolitaireScene::updateGameLogic() {
    // Skip sequence checking this frame if we just did an undo
    if (m_skipSequenceCheckThisFrame) {
        m_skipSequenceCheckThisFrame = false;

        // Still update UI elements
        if (auto* scoreEntity = m_entityManager->findEntity("ScoreText")) {
            if (auto* scoreText = scoreEntity->getComponent<TextComponent>()) {
                scoreText->setText(L"Completed Suits: " + std::to_wstring(m_completedSuits) + L"/8");
            }
        }

        if (auto* stockEntity = m_entityManager->findEntity("StockInfo")) {
            if (auto* stockText = stockEntity->getComponent<TextComponent>()) {
                stockText->setText(L"Stock: " + std::to_wstring(m_stock.size()) + L" cards");
            }
        }

        if (isGameWon() && !m_celebrationActive) {
            if (auto* titleEntity = m_entityManager->findEntity("GameTitle")) {
                if (auto* titleText = titleEntity->getComponent<TextComponent>()) {
                    titleText->setText(L"Spider Solitaire - YOU WON!");
                    titleText->setColor(Vec4(0.0f, 1.0f, 0.0f, 1.0f));
                }
            }
            startCelebration();
        }

        return;
    }

    // Check for completed sequences and remove them
    for (auto& stack : m_tableau) {
        if (isSequenceComplete(stack)) {
            removeCompletedSequence(stack);
            m_completedSuits++;
        }
    }

    // Update score display
    if (auto* scoreEntity = m_entityManager->findEntity("ScoreText")) {
        if (auto* scoreText = scoreEntity->getComponent<TextComponent>()) {
            scoreText->setText(L"Completed Suits: " + std::to_wstring(m_completedSuits) + L"/8");
        }
    }

    // Update stock count
    if (auto* stockEntity = m_entityManager->findEntity("StockInfo")) {
        if (auto* stockText = stockEntity->getComponent<TextComponent>()) {
            stockText->setText(L"Stock: " + std::to_wstring(m_stock.size()) + L" cards");
        }
    }

    // Check win condition and start celebration
    if (isGameWon() && !m_celebrationActive) {
        if (auto* titleEntity = m_entityManager->findEntity("GameTitle")) {
            if (auto* titleText = titleEntity->getComponent<TextComponent>()) {
                titleText->setText(L"Spider Solitaire - YOU WON!");
                titleText->setColor(Vec4(0.0f, 1.0f, 0.0f, 1.0f));
            }
        }

        // Start the celebration!
        startCelebration();
    }
}


bool SpiderSolitaireScene::isSequenceComplete(const CardStack& stack) const {
    if (stack.size() < 13) return false;

    // Check if top 13 cards form a complete suit K-A
    for (int i = 0; i < 13; ++i) {
        Entity* card = stack.cards[stack.size() - 13 + i];
        auto* cardComp = card->getComponent<CardComponent>();
        if (!cardComp || !cardComp->isFaceUp()) return false;

        if (static_cast<int>(cardComp->getRank()) != 12 - i) return false; // K=12, Q=11, ..., A=0

        // Check suit consistency
        if (i > 0) {
            Entity* prevCard = stack.cards[stack.size() - 13 + i - 1];
            auto* prevCardComp = prevCard->getComponent<CardComponent>();
            if (cardComp->getSuit() != prevCardComp->getSuit()) return false;
        }
    }

    return true;
}

void SpiderSolitaireScene::removeCompletedSequence(CardStack& stack) {
    // DON'T save state here anymore - it's now handled in updateCardDragging
    // when the move that creates the complete sequence is made

    // Remove top 13 cards and move to foundation
    std::vector<Entity*> completedSequence;
    for (int i = 0; i < 13; ++i) {
        auto* card = stack.removeTopCard(0);
        completedSequence.push_back(card);
        
        // Reset physics for completed cards to avoid conflicts with celebration
        if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
            physics->reset(); // Clear any existing physics state
        }
    }

    // Reverse the sequence so King is at bottom, Ace at top
    std::reverse(completedSequence.begin(), completedSequence.end());

    // Add to foundation - FIND FIRST EMPTY FOUNDATION
    for (auto& foundation : m_foundations) {
        if (foundation.isEmpty()) {

            for (size_t cardIdx = 0; cardIdx < completedSequence.size(); ++cardIdx) {
                auto* card = completedSequence[cardIdx];

                foundation.addCard(card, 0); 

                // Ensure card is face up and properly displayed
                if (auto* cardComp = card->getComponent<CardComponent>()) {
                    cardComp->setFaceUp(true);
                    if (auto* sprite = card->getComponent<SpriteComponent>()) {
                        sprite->setSpriteFrame(
                            static_cast<int>(cardComp->getRank()),
                            static_cast<int>(cardComp->getSuit())
                        );
                        sprite->setVisible(true);

                        // Debug output
                        Vec3 pos = sprite->getPosition();
                        sprite->setPosition(pos.x, pos.y, -5);
                    }
                }
            }

            break;
        }
    }

    // Flip top card in tableau stack if needed
    if (!stack.isEmpty()) {
        Entity* topCard = stack.getTopCard();
        if (auto* cardComp = topCard->getComponent<CardComponent>()) {
            if (!cardComp->isFaceUp()) {
                cardComp->setFaceUp(true);
                if (auto* sprite = topCard->getComponent<SpriteComponent>()) {
                    sprite->setSpriteFrame(
                        static_cast<int>(cardComp->getRank()),
                        static_cast<int>(cardComp->getSuit())
                    );
                }
            }
        }
    }
}

void SpiderSolitaireScene::dealNewRow() {
    if (m_stock.size() < 10) return; // Need at least 10 cards

    // Save state before dealing
    saveGameState();

    for (int i = 0; i < 10; ++i) {
        if (!m_stock.isEmpty()) {
            Entity* card = m_stock.removeTopCard(0);
            if (auto* cardComp = card->getComponent<CardComponent>()) {
                cardComp->setFaceUp(true);
                if (auto* sprite = card->getComponent<SpriteComponent>()) {
                    sprite->setSpriteFrame(
                        static_cast<int>(cardComp->getRank()),
                        static_cast<int>(cardComp->getSuit())
                    );
                }
            }
            m_tableau[i].addCard(card, 0);
            
            // Add subtle dealing physics effect
            if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
                // Add gentle velocity as if the card was "dealt" from the stock
                Vec2 dealDirection = m_tableau[i].position - m_stock.position;
                dealDirection = dealDirection.normalized();
                physics->addVelocity(dealDirection * 60.0f + Vec2(0, -20.0f)); // Gentler arc
                physics->addRandomJitter(3.0f); // Minimal randomness

                // Set target position for smooth settling
                if (auto* sprite = card->getComponent<SpriteComponent>()) {
                    Vec3 finalPos = sprite->getPosition();
                    physics->setTargetPosition(Vec2(finalPos.x, finalPos.y));
                    physics->setRestPosition(Vec2(finalPos.x, finalPos.y));
                }
            }
        }
    }

    // CRITICAL FIX: Check if dealing created any complete sequences
    // If so, save another state before they get auto-removed
    bool hasCompletedSequence = false;
    for (auto& checkStack : m_tableau) {
        if (isSequenceComplete(checkStack)) {
            hasCompletedSequence = true;
            break;
        }
    }

    if (hasCompletedSequence) {
        // Save state BEFORE sequences get automatically removed
        saveGameState();
    }

    updateStockIndicators();
}

bool SpiderSolitaireScene::isValidMove(Entity* card, CardStack& targetStack) const {
    if (!card) return false;

    auto* cardComp = card->getComponent<CardComponent>();
    if (!cardComp) return false;

    // Can always place on empty stack
    if (targetStack.isEmpty()) {
        return true;
    }

    // Get the top card of the target stack
    Entity* topCard = targetStack.getTopCard();
    if (!topCard) return false;

    auto* topCardComp = topCard->getComponent<CardComponent>();
    if (!topCardComp) return false;

    // In Spider Solitaire, cards must be placed in descending rank order
    // Suit doesn't matter for initial placement, but matters for moving sequences
    int cardRank = static_cast<int>(cardComp->getRank());
    int topRank = static_cast<int>(topCardComp->getRank());

    return cardRank == topRank - 1;
}

// Undo/Redo system implementation
void SpiderSolitaireScene::saveGameState() {
    // Capture current state before any major move
    GameState currentState = captureCurrentState();

    // Add to undo stack
    m_undoStack.push_back(currentState);

    // Limit undo history to prevent memory issues
    if (m_undoStack.size() > MAX_UNDO_STATES) {
        m_undoStack.erase(m_undoStack.begin());
    }
}

void SpiderSolitaireScene::restoreGameState() {
    if (m_undoStack.empty()) return;

    // Get the last saved state
    GameState lastState = m_undoStack.back();
    m_undoStack.pop_back();

    // Apply the state
    applyGameState(lastState);

    // CRITICAL: Skip sequence checking for one frame after undo
    // This prevents the game from immediately re-detecting and removing
    // sequences that were just restored
    m_skipSequenceCheckThisFrame = true;
}

GameState SpiderSolitaireScene::captureCurrentState() {
    GameState state;

    // Capture tableau stacks
    state.tableauStacks.resize(10);
    for (int i = 0; i < 10; ++i) {
        state.tableauStacks[i] = m_tableau[i].cards;

        // Store face states for all cards in this stack
        for (auto* card : m_tableau[i].cards) {
            if (auto* cardComp = card->getComponent<CardComponent>()) {
                state.cardFaceStates[card] = cardComp->isFaceUp();
            }
        }
    }

    // Capture foundation stacks
    state.foundationStacks.resize(8);
    for (int i = 0; i < 8; ++i) {
        state.foundationStacks[i] = m_foundations[i].cards;

        // Store face states
        for (auto* card : m_foundations[i].cards) {
            if (auto* cardComp = card->getComponent<CardComponent>()) {
                state.cardFaceStates[card] = cardComp->isFaceUp();
            }
        }
    }

    // Capture stock
    state.stockCards = m_stock.cards;
    for (auto* card : m_stock.cards) {
        if (auto* cardComp = card->getComponent<CardComponent>()) {
            state.cardFaceStates[card] = cardComp->isFaceUp();
        }
    }

    // Capture completed suits count
    state.completedSuits = m_completedSuits;

    return state;
}

void SpiderSolitaireScene::applyGameState(const GameState& state) {
    // Clear current stacks
    for (auto& stack : m_tableau) {
        stack.cards.clear();
    }
    for (auto& foundation : m_foundations) {
        foundation.cards.clear();
    }
    m_stock.cards.clear();

    // Restore tableau stacks
    for (int i = 0; i < 10; ++i) {
        m_tableau[i].cards = state.tableauStacks[i];
        m_tableau[i].updateCardPositions(0);
    }

    // Restore foundation stacks
    for (int i = 0; i < 8; ++i) {
        m_foundations[i].cards = state.foundationStacks[i];
        m_foundations[i].updateCardPositions(0);
    }

    // Restore stock
    m_stock.cards = state.stockCards;
    m_stock.updateCardPositions(0.1f);

    // Restore completed suits
    m_completedSuits = state.completedSuits;

    // Restore all card face states and update visuals
    for (const auto& cardState : state.cardFaceStates) {
        Entity* card = cardState.first;
        bool isFaceUp = cardState.second;

        if (auto* cardComp = card->getComponent<CardComponent>()) {
            cardComp->setFaceUp(isFaceUp);

            if (auto* sprite = card->getComponent<SpriteComponent>()) {
                if (isFaceUp) {
                    sprite->setSpriteFrame(
                        static_cast<int>(cardComp->getRank()),
                        static_cast<int>(cardComp->getSuit())
                    );
                }
                else {
                    sprite->setSpriteFrame(3, 4); // Card back
                }
            }
        }
    }

    // Update stock indicators
    updateStockIndicators();
}

void SpiderSolitaireScene::updateStockIndicators() {
    // Clear existing indicators
    for (auto* indicator : m_stockIndicators) {
        m_entityManager->removeEntity(indicator->getName());
    }
    m_stockIndicators.clear();

    // Recreate indicators based on current stock size
    int numDeals = m_stock.size() / 10;

    for (int i = 0; i < numDeals; ++i) {
        auto& fakeStock = m_entityManager->createEntity("StockIndicator_" + std::to_string(i));
        auto& sprite = fakeStock.addComponent<SpriteComponent>(
            *m_graphicsDevice,
            L"DX3D/Assets/Textures/CardSpriteSheet.png",
            CARD_WIDTH, CARD_HEIGHT
        );

        sprite.setupSpritesheet(13, 6);
        sprite.setSpriteFrame(3, 4); // card back

        float x = STOCK_X;
        float y = STOCK_Y + i * 5;
        float z = ((i-20 )* 0.1f);

        sprite.setPosition(Vec3(x, y, z));
        sprite.setVisible(true);

        m_stockIndicators.push_back(&fakeStock);
    }
}

bool SpiderSolitaireScene::isGameWon() const {
    return m_completedSuits >= 8;
}

std::vector<Entity*> SpiderSolitaireScene::getCardSequence(Entity* startCard) {
    std::vector<Entity*> sequence;
    CardStack* stack = findStackContaining(startCard);
    if (!stack) return sequence;

    // Find start card index
    auto it = std::find(stack->cards.begin(), stack->cards.end(), startCard);
    if (it == stack->cards.end()) return sequence;

    size_t startIndex = std::distance(stack->cards.begin(), it);

    // Get the starting card component
    auto* startCardComp = startCard->getComponent<CardComponent>();
    if (!startCardComp || !startCardComp->isFaceUp()) return sequence;

    // First, validate that we can pick up from this position
    // Check if the sequence from startCard to the end is valid
    if (!isValidSequenceFromPosition(stack, startIndex)) {
        return sequence; // Return empty sequence if invalid
    }

    // Build the valid sequence from start card to end
    sequence.push_back(startCard);
    Suit expectedSuit = startCardComp->getSuit();
    int expectedRank = static_cast<int>(startCardComp->getRank());

    for (size_t i = startIndex + 1; i < stack->cards.size(); ++i) {
        auto* cardComp = stack->cards[i]->getComponent<CardComponent>();
        if (!cardComp || !cardComp->isFaceUp()) break;

        expectedRank--;
        if (expectedRank < 0) break;

        // Must be same suit and descending rank
        if (static_cast<int>(cardComp->getRank()) != expectedRank ||
            cardComp->getSuit() != expectedSuit) {
            break;
        }

        sequence.push_back(stack->cards[i]);
    }

    return sequence;
}

// Add this new helper function to validate sequences
bool SpiderSolitaireScene::isValidSequenceFromPosition(CardStack* stack, size_t startIndex) {
    if (!stack || startIndex >= stack->cards.size()) return false;

    // Get the starting card
    auto* startCardComp = stack->cards[startIndex]->getComponent<CardComponent>();
    if (!startCardComp || !startCardComp->isFaceUp()) return false;

    Suit expectedSuit = startCardComp->getSuit();
    int expectedRank = static_cast<int>(startCardComp->getRank());

    // Check each subsequent card
    for (size_t i = startIndex + 1; i < stack->cards.size(); ++i) {
        auto* cardComp = stack->cards[i]->getComponent<CardComponent>();

        // All cards in the sequence must be face up
        if (!cardComp || !cardComp->isFaceUp()) return false;

        expectedRank--;
        if (expectedRank < 0) return false;

        // Must be same suit and exactly one rank lower
        if (static_cast<int>(cardComp->getRank()) != expectedRank ||
            cardComp->getSuit() != expectedSuit) {
            return false;
        }
    }

    return true;
}

CardStack* SpiderSolitaireScene::findStackContaining(Entity* card) {
    for (auto& stack : m_tableau) {
        auto it = std::find(stack.cards.begin(), stack.cards.end(), card);
        if (it != stack.cards.end()) {
            return &stack;
        }
    }
    return nullptr;
}

Entity* SpiderSolitaireScene::findCardUnderMouse(const Vec2& worldMousePos) {
    Entity* topCard = nullptr;

    // Check tableau stacks from last to first (rightmost to leftmost)
    // Within each stack, check from top to bottom
    for (int stackIdx = m_tableau.size() - 1; stackIdx >= 0; --stackIdx) {
        auto& stack = m_tableau[stackIdx];
        for (int cardIdx = stack.cards.size() - 1; cardIdx >= 0; --cardIdx) {
            auto* card = stack.cards[cardIdx];
            if (auto* collider = card->getComponent<ColliderComponent>()) {
                if (auto* sprite = card->getComponent<SpriteComponent>()) {
                    Vec3 spritePos = sprite->getPosition();
                    if (collider->containsPoint(worldMousePos, spritePos)) {
                        return card; // Return first (topmost) card found
                    }
                }
            }
        }
    }

    return nullptr;
}

Vec2 SpiderSolitaireScene::screenToWorldPosition(const Vec2& screenPos) {
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            float screenWidth = GraphicsEngine::getWindowWidth();
            float screenHeight = GraphicsEngine::getWindowHeight();

            float pixelX = screenPos.x * screenWidth;
            float pixelY = screenPos.y * screenHeight;

            Vec2 cameraPos = camera->getPosition();
            float zoom = camera->getZoom();

            float worldX = (pixelX - screenWidth * 0.5f) / zoom + cameraPos.x;
            float worldY = (pixelY - screenHeight * 0.5f) / zoom + cameraPos.y;

            return Vec2(worldX, worldY);
        }
    }
    return Vec2(0, 0);
}

void SpiderSolitaireScene::updateFPSCounter(float dt) {
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

void SpiderSolitaireScene::updateCameraMovement(float dt) {
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

    if (input.isKeyDown(Key::R)) {
        camera->setPosition(0.0f, 0.0f);
        camera->setZoom(0.8f);
    }
}

void SpiderSolitaireScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
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

    // Collect all entities for Z-sorting
    std::vector<Entity*> allSprites;

    // Add empty spots first (they should render behind everything)
    for (auto* emptySpot : m_tableauEmptySpots) {
        allSprites.push_back(emptySpot);
    }
    for (auto* emptySpot : m_foundationEmptySpots) {
        allSprites.push_back(emptySpot);
    }
    allSprites.push_back(m_stockEmptySpot);

    // Add cards
    for (auto& stack : m_tableau) {
        for (auto* card : stack.cards) {
            allSprites.push_back(card);
        }
    }

    for (auto& foundation : m_foundations) {
        for (auto* card : foundation.cards) {
            allSprites.push_back(card);
        }
    }

    if (!m_stock.isEmpty()) {
        allSprites.push_back(m_stock.getTopCard());
    }
    for (auto* indicator : m_stockIndicators) {
        allSprites.push_back(indicator);
    }
    
    // Add transparent sprite last to ensure LineRenderer works
    if (auto* transparentEntity = m_entityManager->findEntity("TransparentSprite")) {
        allSprites.push_back(transparentEntity);
    }


    // Sort by Z so painter's order matches depth (back to front)
    std::sort(allSprites.begin(), allSprites.end(), [](Entity* a, Entity* b) {
        auto* sa = a ? a->getComponent<SpriteComponent>() : nullptr;
        auto* sb = b ? b->getComponent<SpriteComponent>() : nullptr;
        float za = sa ? sa->getPosition().z : -100.0f;
        float zb = sb ? sb->getPosition().z : -100.0f;
        return za < zb; // lower Z (further back) first
    });

    // Render sorted sprites
    for (auto* entity : allSprites) {
        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
            if (sprite->isVisible() && sprite->isValid()) {
                sprite->draw(ctx);
            }
        }
    }
    
    // Render frame debug visualization in world space
    if (m_showFrameDebug) {
        renderFrameDebug(ctx);
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
    for (auto* entity : m_entityManager->getEntitiesWithComponent<ButtonComponent>()) {
        if (auto* text = entity->getComponent<ButtonComponent>()) {
            if (text->isVisible()) {
                text->draw(ctx); // Will respect m_useScreenSpace
            }
        }
    }
    
    engine.endFrame(swapChain);
}

// Physics functions
void SpiderSolitaireScene::applySubtleCelebrationEffects(float dt) {
    if (!m_celebrationActive) return;

    // Get all entities with physics components
    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardPhysicsComponent>();

    for (auto* cardEntity : cardEntities) {
        auto* physics = cardEntity->getComponent<CardPhysicsComponent>();
        auto* sprite = cardEntity->getComponent<SpriteComponent>();

        if (!physics || !sprite) continue;

        // Skip cards that are already in celebration
        bool isInCelebration = false;
        for (const auto& celebCard : m_celebrationCards) {
            if (celebCard.card == cardEntity) {
                isInCelebration = true;
                break;
            }
        }

        if (isInCelebration) continue;

        // Apply subtle celebration mode to other cards
        physics->setPhysicsMode(PhysicsMode::CELEBRATION);

        // Add very gentle jitter
        if (rand() % 100 < 5) { // 5% chance per frame
            physics->addRandomJitter(3.0f); // Very subtle
        }
    }
}

// Magnetic attraction for cards near valid drop zones
void SpiderSolitaireScene::applyMagneticAttraction(float dt) {
    if (m_draggedSequence.empty()) return; // Only apply when dragging cards

    // Get the dragged card (first card in sequence)
    Entity* draggedCard = m_draggedSequence[0];
    if (!draggedCard) return;

    auto* draggedSprite = draggedCard->getComponent<SpriteComponent>();
    if (!draggedSprite) return;

    Vec3 draggedPos = draggedSprite->getPosition();
    Vec2 draggedCardPos(draggedPos.x, draggedPos.y);

    // Check each tableau column for magnetic attraction
    for (int col = 0; col < 10; ++col) {
        auto& stack = m_tableau[col];

        // Skip if stack is empty - cards can always drop on empty stacks
        if (stack.isEmpty()) continue;

        auto* topCard = stack.getTopCard();
        if (!topCard) continue;

        auto* topCardSprite = topCard->getComponent<SpriteComponent>();
        if (!topCardSprite) continue;

        Vec3 topCardPos = topCardSprite->getPosition();
        Vec2 topCardPos2D(topCardPos.x, topCardPos.y);

        // Calculate distance between dragged card and top card
        float distance = (draggedCardPos - topCardPos2D).length();

        // Magnetic range - only attract if close enough
        const float MAGNETIC_RANGE = 150.0f;
        if (distance > MAGNETIC_RANGE) continue;

        // Check if this is a valid move (Spider Solitaire rules)
        bool isValid = isValidMove(draggedCard, stack);

        if (isValid) {
            // Apply gentle magnetic attraction
            Vec2 direction = (topCardPos2D - draggedCardPos).normalized();

            // Strength decreases with distance (inverse square)
            float strength = 1.0f / (distance * 0.01f + 1.0f);
            strength = std::min(strength, 2.0f); // Cap the strength

            // Apply magnetic force to the dragged card
            for (auto* card : m_draggedSequence) {
                if (auto* physics = card->getComponent<CardPhysicsComponent>()) {
                    physics->setPhysicsMode(PhysicsMode::MAGNETIC);
                    physics->setTargetPosition(draggedCardPos + direction * strength * 20.0f * dt);
                }
            }
        }
    }
}

void SpiderSolitaireScene::startCelebration() {
    if (m_celebrationActive) return;

    m_celebrationActive = true;
    m_celebrationTimer = 0.0f;
    m_celebrationCards.clear();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> velocityX(-400.0f, 400.0f);
    std::uniform_real_distribution<float> velocityY(-800.0f, -400.0f); // Upward velocity
    std::uniform_real_distribution<float> angularVel(-720.0f, 720.0f); // Degrees per second

    // Add all foundation cards to celebration
    for (const auto& foundation : m_foundations) {
        for (auto* card : foundation.cards) {
            CardPhysics physics;
            physics.card = card;
            physics.velocity = Vec2(velocityX(gen), velocityY(gen));
            physics.angularVelocity = angularVel(gen);
            physics.currentRotation = 0.0f;
            physics.isActive = true;

            m_celebrationCards.push_back(physics);
        }
    }

    // Optionally add some tableau cards for extra effect
    for (const auto& tableau : m_tableau) {
        if (!tableau.cards.empty()) {
            // Add top 2-3 cards from each non-empty tableau
            int cardsToAdd = std::min(3, static_cast<int>(tableau.cards.size()));
            for (int i = tableau.cards.size() - cardsToAdd; i < static_cast<int>(tableau.cards.size()); ++i) {
                CardPhysics physics;
                physics.card = tableau.cards[i];
                physics.velocity = Vec2(velocityX(gen), velocityY(gen));
                physics.angularVelocity = angularVel(gen);
                physics.currentRotation = 0.0f;
                physics.isActive = true;

                m_celebrationCards.push_back(physics);
            }
        }
    }
}

// Add this method to update celebration physics:
void SpiderSolitaireScene::updateCelebration(float dt) {
    if (!m_celebrationActive) return;

    m_celebrationTimer += dt;

    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // Convert screen bounds to world coordinates
    Vec2 worldTopLeft = screenToWorldPosition(Vec2(0.0f, 0.0f));
    Vec2 worldBottomRight = screenToWorldPosition(Vec2(1.0f, 1.0f));

    bool anyCardsActive = false;

    for (auto& physics : m_celebrationCards) {
        if (!physics.isActive) continue;

        auto* sprite = physics.card->getComponent<SpriteComponent>();
        if (!sprite) continue;

        // Apply gravity to Y velocity
        physics.velocity.y -= m_gravity * dt;

        // Update position
        Vec3 currentPos = sprite->getPosition();
        Vec3 newPos = currentPos;
        newPos.x += physics.velocity.x * dt;
        newPos.y += physics.velocity.y * dt;

        // Update rotation
        physics.currentRotation += physics.angularVelocity * dt;

        // Apply position and rotation
        sprite->setPosition(newPos);

        // Add some sparkle effect with tint cycling
        float sparkle = sin(m_celebrationTimer * 6.0f + physics.currentRotation * 0.01f) * 0.5f + 0.5f;
        sprite->setTint(Vec4(1.0f, 1.0f - sparkle * 0.3f, 1.0f - sparkle * 0.5f, 0.0f));

        // Check if card is still on screen
        if (newPos.x > worldTopLeft.x - 100 && newPos.x < worldBottomRight.x + 100 &&
            newPos.y > worldTopLeft.y - 100 && newPos.y < worldBottomRight.y + 200) {
            anyCardsActive = true;
        }
        else {
            physics.isActive = false;
        }
    }

    // End celebration after all cards fall off screen or after max time
    if (!anyCardsActive || m_celebrationTimer > 10.0f) {
        m_celebrationActive = false;
        resetCelebrationCards();
    }
}

// Add this method to reset cards after celebration:
void SpiderSolitaireScene::resetCelebrationCards() {
    // Reset all cards to their original positions and states
    for (int i = 0; i < static_cast<int>(m_foundations.size()); ++i) {
        m_foundations[i].updateCardPositions(0);

        // Reset tints
        for (auto* card : m_foundations[i].cards) {
            if (auto* sprite = card->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f));
            }
        }
    }

    for (int i = 0; i < static_cast<int>(m_tableau.size()); ++i) {
        m_tableau[i].updateCardPositions(0);

        // Reset tints
        for (auto* card : m_tableau[i].cards) {
            if (auto* sprite = card->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f));
            }
        }
    }

    m_celebrationCards.clear();
}

void SpiderSolitaireScene::createDebugToggleButton() {
    auto& debugButtonEntity = m_entityManager->createEntity("DebugToggleButton");
    
    // Create button with screen space positioning
    auto& debugButton = debugButtonEntity.addComponent<ButtonComponent>(
        *m_graphicsDevice,
        L"Toggle Debug",
        18.0f,
        8.0f,
        4.0f
    );
    
    // Position in top-left corner of screen
    debugButton.enableScreenSpace(true);
    debugButton.setScreenPosition(0.05f, 0.05f);
    
    // Set button styling
    debugButton.setNormalTint(Vec4(0.2f, 0.2f, 0.2f, 0.8f));   // Dark background
    debugButton.setHoveredTint(Vec4(0.3f, 0.3f, 0.3f, 0.9f));  // Lighter on hover
    debugButton.setPressedTint(Vec4(0.1f, 0.1f, 0.1f, 0.9f));  // Darker when pressed
    debugButton.setTextColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f));    // White text
    
    // Set callback to toggle debug rendering
    debugButton.setOnClickCallback([this]() {
        m_showFrameDebug = !m_showFrameDebug;
        
        // Update button text to reflect current state
        auto* buttonEntity = m_entityManager->findEntity("DebugToggleButton");
        if (buttonEntity) {
            auto* button = buttonEntity->getComponent<ButtonComponent>();
            if (button) {
                button->setText(m_showFrameDebug ? L"Debug: ON" : L"Debug: OFF");
            }
        }
    });
    
    // Set initial text based on current debug state
    debugButton.setText(m_showFrameDebug ? L"Debug: ON" : L"Debug: OFF");
}

void SpiderSolitaireScene::updateFrameDebugVisualization() {
    if (!m_lineRenderer) return;

    m_lineRenderer->enableScreenSpace(false); // Use world space coordinates
    m_lineRenderer->clear();
    
    // Get all card entities with frame components
    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardFrameComponent>();
    
    for (auto* cardEntity : cardEntities) {
        auto* frame = cardEntity->getComponent<CardFrameComponent>();
        auto* sprite = cardEntity->getComponent<SpriteComponent>();
        auto* physics = cardEntity->getComponent<CardPhysicsComponent>();
        
        if (!frame || !sprite) continue;
        
        Vec2 framePos = frame->getPosition();
        Vec3 spritePos = sprite->getPosition();
        Vec2 cardPos(spritePos.x, spritePos.y);
        
        // Determine colors based on card state
        Vec4 frameColor = Vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for frame position
        Vec4 cardColor = Vec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red for actual card position
        
        // Check if card is being dragged
        bool isBeingDragged = false;
        for (auto* draggedCard : m_draggedSequence) {
            if (draggedCard == cardEntity) {
                isBeingDragged = true;
                break;
            }
        }
        
        if (isBeingDragged) {
            frameColor = Vec4(0.0f, 1.0f, 1.0f, 1.0f); // Cyan for dragged frame
            cardColor = Vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow for dragged card
        }
        
        // 1. Render frame position as cross
        float crossSize = 15.0f;
        m_lineRenderer->addLine(
            Vec2(framePos.x - crossSize, framePos.y),
            Vec2(framePos.x + crossSize, framePos.y),
            frameColor, 2.0f
        );
        m_lineRenderer->addLine(
            Vec2(framePos.x, framePos.y - crossSize),
            Vec2(framePos.x, framePos.y + crossSize),
            frameColor, 2.0f
        );
        
        // 2. Render actual card position as X
        float cardSize = 10.0f;
        m_lineRenderer->addLine(
            Vec2(cardPos.x - cardSize, cardPos.y - cardSize),
            Vec2(cardPos.x + cardSize, cardPos.y + cardSize),
            cardColor, 2.0f
        );
        m_lineRenderer->addLine(
            Vec2(cardPos.x - cardSize, cardPos.y + cardSize),
            Vec2(cardPos.x + cardSize, cardPos.y - cardSize),
            cardColor, 2.0f
        );
        
        // 3. Draw line connecting frame position to actual card position
        m_lineRenderer->addLine(framePos, cardPos, Vec4(0.5f, 0.5f, 0.5f, 0.6f), 1.0f);
        
        // 4. Show velocity if physics component exists
        if (physics) {
            Vec2 velocity = physics->getVelocity();
            if (velocity.length() > 0.1f) {
                Vec2 velocityEnd = cardPos + velocity * 0.1f; // Scale down for visibility
                m_lineRenderer->addLine(cardPos, velocityEnd, Vec4(1.0f, 1.0f, 1.0f, 0.8f), 1.5f);
            }
        }
    }
}

void SpiderSolitaireScene::renderFrameDebug(DeviceContext& ctx) {
    if (!m_lineRenderer) return;
    
    m_lineRenderer->updateBuffer();
    m_lineRenderer->draw(ctx);
}

void SpiderSolitaireScene::updateCardPhysics(float dt) {
    // Get all entities with physics components
    auto cardEntities = m_entityManager->getEntitiesWithComponent<CardPhysicsComponent>();

    int physicsActiveCount = 0;
    int positionUpdates = 0;

    for (auto* cardEntity : cardEntities) {
        auto* physics = cardEntity->getComponent<CardPhysicsComponent>();
        auto* sprite = cardEntity->getComponent<SpriteComponent>();

        if (!physics || !sprite) continue;

        // Skip physics for currently dragged cards UNLESS they're in DRAG mode
        bool isBeingDragged = false;
        for (auto* draggedCard : m_draggedSequence) {
            if (draggedCard == cardEntity) {
                isBeingDragged = true;
                break;
            }
        }

        // Allow DRAG mode to work even when being dragged
        if (isBeingDragged && physics->getPhysicsMode() != PhysicsMode::DRAG) {
            continue;
        }

        Vec3 currentSpritePos = sprite->getPosition();
        Vec2 currentPos(currentSpritePos.x, currentSpritePos.y);

        // Apply spring forces - allow DRAG mode to work even when dragging
        if (!physics->isDragging() || physics->getPhysicsMode() == PhysicsMode::DRAG) {
            physicsActiveCount++;
            
            
            // Add continuous jitter to non-dragged cards
            physics->addContinuousJitter(dt);
            
            Vec2 velocity = physics->getVelocity();
            
            // Apply physics if the card has any velocity or is far from target
            Vec2 targetPos = physics->getTargetPosition();
            float distanceToTarget = (targetPos - currentPos).length();
            
            
            if (velocity.length() > 0.1f || distanceToTarget > 2.0f) {
                physics->applySpringForce(currentPos, dt);

                // Update position based on velocity
                Vec2 newPos = physics->updatePosition(currentPos, dt);

                // Update sprite position
                sprite->setPosition(newPos.x, newPos.y, currentSpritePos.z);
                positionUpdates++;
                
            }
        }
    }
}

