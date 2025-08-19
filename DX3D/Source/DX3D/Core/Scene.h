// Scene.h
#pragma once
#include <memory>

namespace dx3d {
    class GraphicsEngine;
    class SwapChain;

    class Scene {
    public:
        virtual ~Scene() = default;
        virtual void load(GraphicsEngine& engine) = 0;   // load meshes, sprites, etc.
        virtual void update(float deltaTime) = 0;        // game logic
        virtual void render(GraphicsEngine& engine, SwapChain& swapChain) = 0;
    };
}
