#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>

namespace dx3d {
    class TestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void createPlayingCards(GraphicsDevice& device);
        void createUIElements(GraphicsDevice& device);
        void updateCardDragging();
        void updateCardHoverEffects();
        void updateFPSCounter(float dt);
        Entity* findCardUnderMouse(const Vec2& worldMousePos);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
    private:
        std::unique_ptr<EntityManager> m_entityManager;

        void updateCameraMovement(float dt);
    };
}