#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Camera.h>
#include <memory>

namespace dx3d {
    class EntityManager;
    class GraphicsEngine;
    class SwapChain;
    class GraphicsDevice;
    class Texture2D;
    class Mesh3DComponent;
    class Physics3DComponent;
}

namespace dx3d {
    class MarbleMazeScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;
        
    private:
        // Marble physics
        Vec3 m_marblePosition{ 0.0f, 0.1f, 0.0f }; // Start on the ground (y = radius)
        Vec3 m_marbleVelocity{ 0.0f, 0.0f, 0.0f };
        Vec3 m_marbleAcceleration{ 0.0f, 0.0f, 0.0f };
        float m_marbleRadius{ 0.1f };
        float m_marbleMass{ 1.0f };
        
        // Physics constants
        float m_friction{ 0.95f };
        float m_gravity{ -9.8f };
        float m_bounce{ 0.6f };
        float m_forceStrength{ 15.0f };
        
        // ECS
        std::unique_ptr<EntityManager> m_entityManager;
        
        // Textures
        std::shared_ptr<Texture2D> m_beamTexture;
        
        // Camera (movable for debugging)
        Camera3D m_camera;
        
        // Camera controls
        float m_cameraYaw{ 0.0f };
        float m_cameraPitch{ -1.57f }; // Start looking down
        Vec2 m_lastMouse{ 0.0f, 0.0f };
        bool m_mouseCaptured{ false };
        
        // Input handling
        Vec2 m_inputDirection{ 0.0f, 0.0f };
        Vec2 m_cameraInput{ 0.0f, 0.0f };
        
        // Helper methods
        void handleCollisions();
        void updateCamera(float dt);
        void createWallEntity(const std::string& name, const Vec3& position, const Vec3& scale, GraphicsDevice& device);
    };
}
