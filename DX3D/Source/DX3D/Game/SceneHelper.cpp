#include <DX3D/Game/SceneHelper.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <algorithm>

namespace dx3d
{
    Camera2D* SceneHelper::createDefaultCamera2D(
        EntityManager& entityManager,
        float zoom,
        const Vec2& position,
        const std::string& entityName)
    {
        auto& cameraEntity = entityManager.createEntity(entityName);
        float screenWidth = GraphicsEngine::getWindowWidth();
        float screenHeight = GraphicsEngine::getWindowHeight();
        auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
        camera.setPosition(position.x, position.y);
        camera.setZoom(zoom);
        return &camera;
    }

    std::unique_ptr<Camera3D> SceneHelper::createDefaultCamera3D(
        float fov,
        float nearPlane,
        float farPlane)
    {
        float aspect = GraphicsEngine::getWindowWidth() / std::max(1.0f, GraphicsEngine::getWindowHeight());
        auto camera = std::make_unique<Camera3D>(fov, aspect, nearPlane, farPlane);
        return camera;
    }

    LineRenderer* SceneHelper::createLineRenderer(
        EntityManager& entityManager,
        GraphicsDevice& device,
        GraphicsEngine& engine,
        bool screenSpace,
        bool visible,
        const std::string& entityName)
    {
        auto& lineEntity = entityManager.createEntity(entityName);
        auto& lineRenderer = lineEntity.addComponent<LineRenderer>(device);
        lineRenderer.setVisible(visible);
        lineRenderer.enableScreenSpace(screenSpace);

        if (auto* linePipeline = engine.getLinePipeline())
        {
            lineRenderer.setLinePipeline(linePipeline);
        }

        return &lineRenderer;
    }

    GraphicsDevice& SceneHelper::initializeScene(
        GraphicsEngine& engine,
        GraphicsDevice*& outDevice,
        std::unique_ptr<EntityManager>& outEntityManager)
    {
        auto& device = engine.getGraphicsDevice();
        outDevice = &device;
        outEntityManager = std::make_unique<EntityManager>();
        return device;
    }
}

