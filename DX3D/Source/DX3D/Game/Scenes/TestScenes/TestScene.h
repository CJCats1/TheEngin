#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Physics/PhysicsSystem.h>
#include <memory>
#include <vector>

namespace dx3d {
    class LineRenderer;
    class IRenderDevice;
    class Texture2D;
    class TextComponent;
    class TestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void reset(GraphicsEngine& engine) override;
        void update(float dt) override;
        void fixedUpdate(float dt) override;
        void render(GraphicsEngine& engine, IRenderSwapChain& swapChain) override;
        EntityManager* getEntityManager() override { return m_entityManager.get(); }
        void renderImGui(GraphicsEngine& engine);
    private:
        enum class SpawnMode
        {
            Aabb,
            Circle
        };
        std::unique_ptr<EntityManager> m_entityManager;
        IRenderDevice* m_graphicsDevice = nullptr;

        // Physics AABBs
        dx3d::physics::AABB m_playerBox;
        std::vector<dx3d::physics::AABB> m_staticBoxes;
        Vec2 m_playerVelocity;

        // Dynamic AABBs with gravity
        std::vector<dx3d::physics::AABB> m_dynamicBoxes;
        std::vector<Vec2> m_dynamicVelocities;
        std::vector<float> m_dynamicMasses;
        std::vector<float> m_dynamicRestitutions;
        float m_gravity = 500.0f; // gravity acceleration
        float m_playerPushMass = 30.0f;
        float m_staticFriction = 0.2f;
        float m_staticRestitution = 0.1f;
        float m_playerFrictionStatic = 0.1f;
        float m_playerFrictionDynamic = 0.15f;
        float m_playerRestitution = 0.05f;
        std::vector<float> m_dynamicFrictions;
        bool m_springDynamicActive = false;
        int m_springDynamicIndex = -1;
        bool m_springCircleActive = false;
        int m_springCircleIndex = -1;
        Vec2 m_springTarget = Vec2(0.0f, 0.0f);
        float m_springK = 25.0f;
        float m_springDamping = 6.0f;

        // Sprite entities for visualization
        Entity* m_playerEntity = nullptr;
        std::vector<Entity*> m_staticBoxEntities;
        std::vector<Entity*> m_dynamicBoxEntities;
        Entity* m_backgroundEntity = nullptr;
        std::shared_ptr<Texture2D> m_backgroundTexture;
        std::shared_ptr<Texture2D> m_nodeTexture;
        float m_backgroundTime = 0.0f;
        int m_debugNodeCounter = 0;
        std::vector<dx3d::physics::CircleBody> m_circles;
        std::vector<Entity*> m_circleEntities;
        std::shared_ptr<Texture2D> m_circleTexture;
        std::shared_ptr<Texture2D> m_beamTexture;
        LineRenderer* m_lineRenderer = nullptr;
        bool m_showDebugLines = false;
        bool m_showTextDemo = true;
        float m_textUpdateTimer = 0.0f;
        float m_textUpdateInterval = 0.15f;
        std::unique_ptr<TextComponent> m_screenText;
        std::unique_ptr<TextComponent> m_worldText;
        SpawnMode m_spawnMode = SpawnMode::Aabb;

        void updateCameraMovement(float dt);
        void updatePlayerMovement(float dt);
        void createOriginNodeSprite();
    };
}
