#pragma once
#include <TheEngine/Core/Scene.h>
#include <TheEngine/Core/EntityManager.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <memory>
#include <chrono>

namespace TheEngine {
    
    class ThreeDBridgeGameScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;
    };
}
