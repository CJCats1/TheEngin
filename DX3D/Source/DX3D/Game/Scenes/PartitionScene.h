#pragma once
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Quadtree.h>
#include <DX3D/Core/Input.h>
#include <memory>
#include <set>
 
namespace dx3d
{
    class PartitionScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;

    private:
        std::unique_ptr<EntityManager> m_entityManager;
        std::unique_ptr<Quadtree> m_quadtree;
        LineRenderer* m_lineRenderer = nullptr;
        bool m_showQuadtree = true;
        int m_entityCounter = 0;
        Vec2 m_quadtreeVisualOffset = Vec2(500, 175);
        void createTestEntities(GraphicsDevice& device);
        void createUIElements(GraphicsDevice& device);
        void addRandomEntities();
        void clearAllEntities();
        void addEntityAtPosition(const Vec2& worldPos);
        void updateQuadtreeVisualization();
        void updateCameraMovement(float dt);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
        void updateMovingEntities(float dt);
        void updateQuadtreePartitioning();
        struct MovingEntity {
            std::string name;
            Vec2 velocity;
            Vec2 bounds;  // For bouncing off edges
            QuadtreeEntity qtEntity;
            bool active = true;
        };

        std::vector<MovingEntity> m_movingEntities;
        float m_updateTimer = 0.0f;
        const float m_updateInterval = 0.016f; // ~60 FPS for quadtree updates
        bool m_entitiesMoving = false;
    };
}