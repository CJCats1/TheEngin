#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>
#include <chrono>

namespace dx3d {
    
    class ThreeDBridgeGameScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;
    };
}
