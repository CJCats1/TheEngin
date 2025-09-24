#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Camera.h>
#include <memory>

namespace dx3d {
    class ThreeDTestScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
    private:
        std::shared_ptr<Mesh> m_cube;
        std::shared_ptr<Mesh> m_model;
        Camera3D m_camera{ 1.04719755f, 16.0f/9.0f, 0.1f, 1000.0f }; // ~60 deg
        float m_angleY{ 0.0f };
        float m_angleX{ 0.0f };
        float m_modelAngle{ 0.0f };
        float m_yaw{ 0.0f };
        float m_pitch{ 0.0f };
        Vec2 m_lastMouse{ 0.0f, 0.0f };
        bool m_mouseCaptured{ false };
    };
}


