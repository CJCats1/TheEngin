#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>
#include <chrono>

namespace dx3d {
    enum class SceneMode {
        Build,
        Simulating
    };

    class BridgeScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;
        // UI and mode management
        void createUI(GraphicsEngine& engine);
        void setMode(SceneMode mode);
        void toggleDeleteMode();

    private:
        // Core systems
        std::unique_ptr<EntityManager> m_entityManager;
        GraphicsDevice* m_graphicsDevice = nullptr;
        SceneMode m_currentMode = SceneMode::Build;
        bool m_isSimulationRunning = false;

        // Node building/editing state
        bool m_nodeAttachedToMouse = false;
        bool m_inDeleteMode = false;
        bool m_deleteBeamsAndNodes = false;
        Entity* m_savedNode = nullptr;           // Source node for building
        Entity* m_tempNode = nullptr;            // Temporary node being dragged
        Entity* m_tempBeam = nullptr;            // Temporary beam being created

        // Legacy dragging system (kept for compatibility)
        Entity* m_draggedNode = nullptr;         // The node being dragged
        Entity* m_draggedFromNode = nullptr;     // The original node we clicked on
        Vec2 m_dragOffset{ 0.0f, 0.0f };

        // Counters and metrics
        float m_numberOfBeams = 0.0f;
        float m_numberOfNodes = 0.0f;
        std::chrono::high_resolution_clock::time_point m_startTime;
        float m_simulationTime = 0.0f;
        bool m_showDebugInfo = false;
        Entity* m_selectedNode = nullptr;

        // Core update methods
        void updateCameraMovement(float dt);
        void updateUIStatus();
        void updateButtonInteractions(float dt);

        // Building system methods
        void handleBuildMode();
        void startBuildingFromNode(Entity* sourceNode, Vec2 mousePos);
        void completeBuildOperation(Vec2 mousePos);

        // Delete system methods
        void handleDeleteMode();
        void deleteNodeAndConnectedBeams(Entity* nodeToDelete);
        void resetAllNodeAndBeamTextures();

        // Bridge creation and management
        void createBridge(GraphicsEngine& engine);
        void createNode(Vec2 position, bool fixed, const std::string& name);
        void createBeam(const std::string& node1Name, const std::string& node2Name, const std::string& beamName);
        void resetBridge();

        // Utility methods
        Vec2 screenToWorld(int screenX, int screenY);
    };
}