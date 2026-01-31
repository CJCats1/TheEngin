#pragma once

namespace TheEngine {
    class GraphicsEngine;
    class IRenderSwapChain;
    class Game;
    class EntityManager;

    class Scene {
    public:
        virtual ~Scene() = default;

        // Core scene methods
        virtual void load(GraphicsEngine& engine) = 0;
        virtual void reset(GraphicsEngine& engine) { load(engine); }
        virtual void update(float dt) = 0;
		virtual void fixedUpdate(float dt) {}
        virtual void render(GraphicsEngine& engine, IRenderSwapChain& swapChain) = 0;
        // ImGui hook per scene; default draws fallback content
        virtual void renderImGui(GraphicsEngine& engine);
        
        // Get EntityManager for physics updates (returns nullptr if scene doesn't use ECS)
        virtual EntityManager* getEntityManager() { return nullptr; }

        // Input handling methods - override these in derived classes if needed
        virtual void onKeyDown(int keyCode) {}
        virtual void onKeyUp(int keyCode) {}
        virtual void onMouseMove(int x, int y) {}
        virtual void onMouseClick(int button, int x, int y) {}
        virtual void onMouseRelease(int button, int x, int y) {}
    };
}