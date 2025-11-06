#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>
#include <string>
#include <vector>

namespace dx3d {
    class TestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void createPlayingCards(GraphicsDevice& device);
        void createUIElements(GraphicsDevice& device);
        void createSoftGuyExamples(GraphicsDevice& device);
        void updateCardDragging();
        void updateCardHoverEffects();
        void updateFPSCounter(float dt);
        Entity* findCardUnderMouse(const Vec2& worldMousePos);
        void renderImGui(GraphicsEngine& engine);
        void spawnSoftGuyCircle(Vec2 position);
        void spawnSoftGuyRectangle(Vec2 position);
        void spawnSoftGuyTriangle(Vec2 position);
        void spawnSoftGuyLine(Vec2 position);
        void spawnFirmGuyCircle(Vec2 position);
        void spawnFirmGuyRectangle(Vec2 position);
    private:
        std::unique_ptr<EntityManager> m_entityManager;

        void updateCameraMovement(float dt);
    };
}