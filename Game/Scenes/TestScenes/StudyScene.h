#pragma once

#include <TheEngine/Core/Scene.h>
#include <TheEngine/Core/EntityManager.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <TheEngine/Game/SceneHelper.h>
#include <TheEngine/Math/Types.h>
#include <memory>
#include <vector>

namespace TheEngine
{
    class Texture2D;
    class IRenderDevice;
    class Entity;

    // Simple study-helper scene:
    // - shows a background image
    // - overlays clickable boxes
    // - clicking a box hides it
    // - reset button restores all boxes
    class StudyScene : public Scene
    {
    public:
        void load(GraphicsEngine& engine) override;
        void reset(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, IRenderSwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
        EntityManager* getEntityManager() override { return m_entityManager.get(); }

    private:
        struct Box
        {
            Vec2 center;
            Vec2 halfSize;
            bool visible;
        };

        std::unique_ptr<EntityManager> m_entityManager;
        IRenderDevice* m_device = nullptr;

        Entity* m_backgroundEntity = nullptr;
        std::shared_ptr<Texture2D> m_backgroundTexture;

        std::vector<Box> m_initialBoxes;
        std::vector<Box> m_boxes;
        std::vector<Entity*> m_boxEntities;
        std::shared_ptr<Texture2D> m_boxTexture;

        // Background image path (relative to project root)
        std::wstring m_imagePath = L"TheEngine/Assets/Textures/cat.jpg";

        void createBackgroundSprite(GraphicsEngine& engine);
        void createBoxes();
        void syncBoxSprites();
        void handleMouseClicks();
    };
}

