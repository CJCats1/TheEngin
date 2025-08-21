#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <vector>

namespace dx3d {
    class TestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;

    private:
        std::vector<std::unique_ptr<SpriteComponent>> m_sprites;
        std::unique_ptr<Camera> m_camera;

        void updateCameraMovement(float dt);
    };
}