#pragma once

namespace dx3d {
    class GraphicsEngine;
    class SwapChain;

    class Scene {
    public:
        virtual ~Scene() = default;

        // Core scene methods
        virtual void load(GraphicsEngine& engine) = 0;
        virtual void update(float dt) = 0;
		virtual void fixedUpdate(float dt) {}
        virtual void render(GraphicsEngine& engine, SwapChain& swapChain) = 0;

        // Input handling methods - override these in derived classes if needed
        virtual void onKeyDown(int keyCode) {}
        virtual void onKeyUp(int keyCode) {}
        virtual void onMouseMove(int x, int y) {}
        virtual void onMouseClick(int button, int x, int y) {}
        virtual void onMouseRelease(int button, int x, int y) {}
    };
}