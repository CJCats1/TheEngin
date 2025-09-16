#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Components/CardComponent.h>
#include <memory>
#include <vector>
#include <stack>

namespace dx3d {
    enum class SpiderDifficulty {
        OneSuit,    // Only Spades
        TwoSuit,    // Spades and Hearts
        FourSuit    // All four suits
    };
    struct GameState {
        std::vector<std::vector<Entity*>> tableauStacks;
        std::vector<std::vector<Entity*>> foundationStacks;
        std::vector<Entity*> stockCards;
        int completedSuits;

        // Store face-up/face-down state for each card
        std::unordered_map<Entity*, bool> cardFaceStates;

        GameState() : completedSuits(0) {}
    };
    struct StockClickArea {
        Vec2 position;
        float width;
        float height;

        bool containsPoint(const Vec2& point) const {
            return point.x >= position.x - width * 0.5f &&
                point.x <= position.x + width * 0.5f &&
                point.y >= position.y - height * 0.5f &&
                point.y <= position.y + height * 0.5f;
        }
    };
    struct CardPhysics {
        Entity* card;
        Vec2 velocity;
        float angularVelocity;
        float currentRotation;
        bool isActive;

        CardPhysics() : card(nullptr), velocity(0, 0), angularVelocity(0),
            currentRotation(0), isActive(false) {
        }
    };

    struct HintMove {
        enum Type {
            MOVE_SEQUENCE,      // Move card sequence between tableau columns
            FLIP_CARD,          // Flip face-down card (automatically happens)
            DEAL_CARDS,         // Deal new row from stock
            BUILD_SEQUENCE,     // Suggest building a specific sequence
            CLEAR_COLUMN        // Suggest clearing a column for better moves
        };

        Type type;
        Entity* sourceCard = nullptr;      // Card to move (or top card of sequence)
        int sourceColumn = -1;             // Source tableau column
        int targetColumn = -1;             // Target tableau column
        std::vector<Entity*> sequence;     // Cards involved in the move
        int priority = 0;                  // Higher = better move (0-100)
        std::wstring description;          // Human-readable hint text

        HintMove(Type t) : type(t) {}
    };
    struct CardStack {
        std::vector<Entity*> cards;
        Vec2 position;
        float cardOffset = 25.0f;   // Vertical offset between cards
        bool faceDown = false;      // Are cards face down by default
        float baseZDepth = -70.0f;  // Base Z depth for this stack

        void addCard(Entity* card, float zSpacing = 0.01f);  // Add parameter with default
        Entity* removeTopCard(float zSpacing = 0.01f);       // Add parameter with default
        Entity* getTopCard() const;
        bool isEmpty() const { return cards.empty(); }
        size_t size() const { return cards.size(); }
        void updateCardPositions(float zSpacing = 0.01f);    // Add parameter with default
        bool canDropCard(Entity* card) const;
    };

    class SpiderSolitaireScene : public Scene {
    public:
        SpiderSolitaireScene(SpiderDifficulty difficulty = SpiderDifficulty::OneSuit);
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;

    private:
        // Game setup
        void createCards(GraphicsDevice& device);
        void setupTableau();
        void dealInitialCards();
        void createUI(GraphicsDevice& device);
        void createEmptySpots(GraphicsDevice& device);      // NEW METHOD
        void updateEmptySpotVisibility();                   // NEW METHOD

        // Game logic
        void updateCardDragging();
        void updateCardHoverEffects();
        void updateGameLogic();
        bool isValidMove(Entity* card, CardStack& targetStack) const;
        bool isSequenceComplete(const CardStack& stack) const;
        void removeCompletedSequence(CardStack& stack);
        void dealNewRow();
        bool isGameWon() const;

        // Utility functions
        Entity* findCardUnderMouse(const Vec2& worldMousePos);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
        void updateCameraMovement(float dt);
        void updateFPSCounter(float dt);
        CardStack* findStackContaining(Entity* card);
        std::vector<Entity*> getCardSequence(Entity* startCard);
        bool isValidSequenceFromPosition(CardStack* stack, size_t startIndex);

        // Game state
        std::unique_ptr<EntityManager> m_entityManager;
        SpiderDifficulty m_difficulty;

        // Card containers
        std::vector<CardStack> m_tableau;      // 10 columns
        std::vector<CardStack> m_foundations;  // 8 foundation piles (completed sequences)
        CardStack m_stock;                     // Remaining cards to deal
        std::vector<Entity*> m_stockIndicators;

        // Empty spot indicators (NEW)
        std::vector<Entity*> m_tableauEmptySpots;    // Empty spot indicators for tableau
        std::vector<Entity*> m_foundationEmptySpots; // Empty spot indicators for foundations
        Entity* m_stockEmptySpot;                    // Empty spot indicator for stock

        // Game variables
        int m_completedSuits;
        Entity* m_draggedCard;
        std::vector<Entity*> m_draggedSequence;
        bool m_isDragging;
        Vec2 m_dragOffset;

        bool m_celebrationActive;
        std::vector<CardPhysics> m_celebrationCards;
        float m_celebrationTimer;
        float m_gravity;
        bool m_skipSequenceCheckThisFrame = false;


        StockClickArea m_stockClickArea;
        // Constants
        static constexpr float CARD_WIDTH = 80.0f;
        static constexpr float CARD_HEIGHT = 120.0f;
        static constexpr float COLUMN_SPACING = 100.0f;
        static constexpr float TABLEAU_START_X = -450.0f;
        static constexpr float TABLEAU_Y = 100.0f;
        static constexpr float FOUNDATION_Y = 300.0f;
        static constexpr float STOCK_X = 450.0f;
        static constexpr float STOCK_Y = 300.0f;

        // Z-Depth constants
        static constexpr float Z_DEPTH_STOCK = -90.0f;
        static constexpr float Z_DEPTH_FOUNDATION_BASE = -80.0f;
        static constexpr float Z_DEPTH_TABLEAU_BASE = -70.0f;
        static constexpr float Z_DEPTH_DRAGGING_BASE = -10.0f;
        static constexpr float Z_DEPTH_CARD_SPACING = 0.01f;


        // Add these member variables:
        std::vector<GameState> m_undoStack;
        static const size_t MAX_UNDO_STATES = 50; // Limit undo history
        // Add to private members of SpiderSolitaireScene class:
        std::vector<HintMove> m_currentHints;
        int m_currentHintIndex = -1;
        bool m_showingHint = false;
        Entity* m_hintTextEntity = nullptr;
        // Add these method declarations:
        void saveGameState();
        void restoreGameState();
        GameState captureCurrentState();
        void applyGameState(const GameState& state);
        void updateStockIndicators();
        GraphicsDevice* m_graphicsDevice = nullptr;
        void startCelebration();
        void updateCelebration(float dt);
        void resetCelebrationCards();
    };
}