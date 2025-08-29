#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>
#include <chrono>
namespace dx3d {
    class BridgeScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;

        // Input handling
        void onMouseClick(int button, int x, int y) override;
        void onMouseRelease(int button, int x, int y) override;
        void onMouseMove(int x, int y) override;
        void createUI(GraphicsEngine& engine);
    private:
        std::unique_ptr<EntityManager> m_entityManager;
        Entity* m_draggedNode = nullptr;
        Vec2 m_dragOffset{ 0.0f, 0.0f };
        bool m_isSimulationRunning = false;
        GraphicsDevice* m_graphicsDevice = nullptr;
        void updateCameraMovement(float dt);
        void createBridge(GraphicsEngine& engine);
        void createNode(Vec2 position, bool fixed, const std::string& name, GraphicsEngine& engine);
        void createBeam(const std::string& node1Name, const std::string& node2Name, const std::string& beamName, GraphicsEngine& engine);
        Vec2 screenToWorld(int screenX, int screenY);

        std::chrono::high_resolution_clock::time_point m_startTime;
        float m_simulationTime;
        bool m_showDebugInfo;
        Entity* m_selectedNode;
    };
}