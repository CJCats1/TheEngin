#pragma once
#include <TheEngine/Core/Scene.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <TheEngine/Graphics/Mesh.h>
#include <TheEngine/Graphics/Camera.h>
#include <memory>

namespace TheEngine {
    class ThreeDTestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, IRenderSwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
    private:
        std::shared_ptr<Mesh> m_cube;
        std::shared_ptr<Mesh> m_model;
        std::shared_ptr<Mesh> m_groundPlane;
        std::shared_ptr<Mesh> m_skybox;
        std::vector<std::shared_ptr<Mesh>> m_modelMeshes; // For multi-material models
        Camera3D m_camera{ 1.04719755f, 16.0f/9.0f, 0.1f, 1000.0f }; // ~60 deg
        float m_angleY{ 0.0f };
        float m_angleX{ 0.0f };
        float m_modelAngle{ 0.0f };
        float m_yaw{ 0.0f };
        float m_pitch{ 0.0f };
        Vec2 m_lastMouse{ 0.0f, 0.0f };
        bool m_mouseCaptured{ false };
        // FPS camera controls
        float m_cameraMoveSpeed{ 10.0f };
        float m_cameraRunMultiplier{ 3.0f };
        float m_cameraMouseSensitivity{ 1.8f };
        // Skybox debug state
        bool m_showSkybox{ true };
        float m_skyboxSize{ 1000.0f };
        IRenderDevice* m_devicePtr{ nullptr };
    };
}


