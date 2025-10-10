#include <DX3D/Game/Scenes/ThreeDTestScene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Core/Input.h>
#include <windows.h>

using namespace dx3d;

void ThreeDTestScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_cube = Mesh::CreateCube(device, 1.0f);
    
    
    // Use single mesh approach for now to ensure basic texture loading works
    m_model = Mesh::CreateFromOBJ(device, "D:/TheEngine/TheEngine/DX3D/Assets/models/headcrab/headcrab.obj");

    float w = GraphicsEngine::getWindowWidth();
    float h = GraphicsEngine::getWindowHeight();
    m_camera.setPerspective(1.04719755f, w / h, 0.1f, 1000.0f);
    m_camera.setPosition({ 0.0f, 0.0f, -4.0f });
    m_camera.setTarget({ 0.0f, 0.0f, 0.0f });
}

void ThreeDTestScene::update(float dt)
{
    m_angleY += dt * 0.8f;
    m_angleX += dt * 0.4f;
    m_modelAngle += dt * 0.6f;

    auto& input = Input::getInstance();
    const float move = 2.0f * dt;

    // Mouse look when RMB held
    if (input.isMouseDown(MouseClick::RightMouse))
    {
        Vec2 mouse = input.getMousePositionClient();
        if (!m_mouseCaptured) { m_lastMouse = mouse; m_mouseCaptured = true; }
        Vec2 delta = mouse - m_lastMouse; m_lastMouse = mouse;
        const float sensitivity = 0.0035f;
        m_yaw += delta.x * sensitivity;
        m_pitch += -delta.y * sensitivity;
        m_pitch = std::max(-1.55f, std::min(1.55f, m_pitch));

        // Build forward from yaw/pitch
        Vec3 forward(
            std::cos(m_pitch) * std::sin(m_yaw),
            std::sin(m_pitch),
            std::cos(m_pitch) * std::cos(m_yaw)
        );
        Vec3 pos = m_camera.getPosition();
        m_camera.setTarget(pos + forward);
    }
    else { m_mouseCaptured = false; }

    // WASD in camera-local space
    Vec3 camPos = m_camera.getPosition();
    Mat4 view = m_camera.getViewMatrix();
    // Extract forward/right from yaw/pitch to avoid roll
    Vec3 forward(
        std::cos(m_pitch) * std::sin(m_yaw),
        0.0f,
        std::cos(m_pitch) * std::cos(m_yaw)
    );
    forward.normalize();
    Vec3 right(forward.z, 0.0f, -forward.x);
    right.normalize();

    Vec3 moveDelta(0,0,0);
    if (input.isKeyDown(Key::W)) moveDelta = moveDelta + forward * move;
    if (input.isKeyDown(Key::S)) moveDelta = moveDelta - forward * move;
    if (input.isKeyDown(Key::A)) moveDelta = moveDelta - right * move;
    if (input.isKeyDown(Key::D)) moveDelta = moveDelta + right * move;
    if (moveDelta.lengthSquared() > 0) m_camera.move(moveDelta);
}

void ThreeDTestScene::render(GraphicsEngine& engine, SwapChain& swapChain)
{
    auto& ctx = engine.getContext();

    ctx.enableDepthTest();
    ctx.setGraphicsPipelineState(engine.get3DPipeline());
    // Two lights
    std::vector<Vec3> dirs{ Vec3(-0.4f,-1.0f,-0.3f), Vec3(0.6f,-0.2f,0.5f) };
    std::vector<Vec3> cols{ Vec3(1.0f,0.95f,0.9f), Vec3(0.3f,0.4f,1.0f) };
    std::vector<float> intens{ 1.0f, 0.6f };
    ctx.setLights(dirs, cols, intens);
    ctx.setMaterial(Vec3(1.0f,1.0f,1.0f), 64.0f, 0.2f);
    ctx.setCameraPosition(m_camera.getPosition());
    ctx.setViewMatrix(m_camera.getViewMatrix());
    ctx.setProjectionMatrix(m_camera.getProjectionMatrix());

    // Draw cube at left
    Mat4 cubeWorld = Mat4::translation(Vec3(-1.5f, 0.0f, 0.0f)) * (Mat4::rotationY(m_angleY) * Mat4::rotationX(m_angleX));
    ctx.setWorldMatrix(cubeWorld);
    if (m_cube) m_cube->draw(ctx);

    // Draw model at right
    if (m_model) {
        float s = 0.02f; // scale down
        Mat4 modelWorld = Mat4::translation(Vec3(1.5f, 0.0f, 0.0f)) * Mat4::rotationY(m_modelAngle) * Mat4::scale(Vec3(s, s, s));
        ctx.setWorldMatrix(modelWorld);
        m_model->draw(ctx);
    }

    // frame begin/end handled centrally
}


