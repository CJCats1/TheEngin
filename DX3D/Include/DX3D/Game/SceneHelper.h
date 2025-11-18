#pragma once
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Math/Geometry.h>
#include <memory>
#include <string>

namespace dx3d
{
    // Helper class to reduce code duplication in scene initialization
    // Provides common setup patterns used across multiple scenes
    class SceneHelper
    {
    public:
        // Creates a default 2D camera with common settings
        // Returns pointer to the camera component for further customization
        static Camera2D* createDefaultCamera2D(
            EntityManager& entityManager,
            float zoom = 1.0f,
            const Vec2& position = Vec2(0.0f, 0.0f),
            const std::string& entityName = "MainCamera"
        );

        // Creates a default 3D camera with perspective projection
        // Note: Camera3D is standalone (not a component), so this just creates and configures it
        // Returns a unique_ptr to the camera
        static std::unique_ptr<Camera3D> createDefaultCamera3D(
            float fov = 1.22173048f, // ~70 degrees
            float nearPlane = 0.1f,
            float farPlane = 5000.0f
        );

        // Creates a line renderer with common settings
        // Returns pointer to the LineRenderer component
        static LineRenderer* createLineRenderer(
            EntityManager& entityManager,
            GraphicsDevice& device,
            GraphicsEngine& engine,
            bool screenSpace = false,
            bool visible = true,
            const std::string& entityName = "LineRenderer"
        );

        // Common initialization pattern: Get device and create entity manager
        // Returns the graphics device reference
        static GraphicsDevice& initializeScene(
            GraphicsEngine& engine,
            GraphicsDevice*& outDevice,
            std::unique_ptr<EntityManager>& outEntityManager
        );
    };
}

