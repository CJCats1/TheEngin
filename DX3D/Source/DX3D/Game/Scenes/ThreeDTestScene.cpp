#include <DX3D/Game/Scenes/ThreeDTestScene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/Input.h>
#include <imgui.h>
#include <windows.h>

using namespace dx3d;

void ThreeDTestScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_cube = Mesh::CreateCube(device, 1.0f);
    
    
    // Use single mesh approach for now to ensure basic texture loading works
    m_model = Mesh::CreateFromOBJ(device, "D:/TheEngine/TheEngine/DX3D/Assets/models/headcrab/headcrab.obj");

    // (Ground plane removed)
    // Create a smaller ground plane (significantly smaller than PartitionScene's 100x100)
    m_groundPlane = Mesh::CreatePlane(device, 20.0f, 20.0f);
    if (m_groundPlane)
    {
        std::wstring groundTexturePath = L"DX3D/Assets/Textures/beam.png";
        auto groundTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), groundTexturePath.c_str());
        if (groundTexture)
        {
            m_groundPlane->setTexture(groundTexture);
        }
    }

    // Create skybox (large cube that surrounds the scene)
    m_skybox = Mesh::CreateCube(device, 10.0f); // Make it smaller for debugging
    if (m_skybox) {
        // Try to load skybox from file first, fallback to solid colors
        std::wstring skyboxPath = L"DX3D/Assets/Textures/Skybox.png";
        auto skyboxTexture = Texture2D::LoadSkyboxCubemap(device.getD3DDevice(), skyboxPath.c_str());
        if (!skyboxTexture) {
            // Fallback to solid color cubemap
            skyboxTexture = Texture2D::CreateSkyboxCubemap(device.getD3DDevice());
        }
        
        if (skyboxTexture) {
            m_skybox->setTexture(skyboxTexture);
        }
    }

    float w = GraphicsEngine::getWindowWidth();
    float h = GraphicsEngine::getWindowHeight();
    // Initialize like PartitionScene's FPS preset
    m_camera.setPerspective(1.22173048f, w / h, 0.1f, 5000.0f);
    m_yaw = 0.0f;
    m_pitch = 0.0f;
    m_camera.setPosition({ 0.0f, 5.0f, 15.0f });
    m_camera.setTarget({ 0.0f, 5.0f, 0.0f });

    // (light debug spheres removed)
}

void ThreeDTestScene::update(float dt)
{
    m_angleY += dt * 0.8f;
    m_angleX += dt * 0.4f;
    m_modelAngle += dt * 0.6f;

    auto& input = Input::getInstance();
    const float move = 2.0f * dt;

    // Mouse look when RMB held (FPS-style like PartitionScene)
    if (input.isMouseDown(MouseClick::RightMouse))
    {
        Vec2 currentMouse = input.getMousePositionNDC();
        if (m_mouseCaptured)
        {
            Vec2 mouseDelta = currentMouse - m_lastMouse;
            m_yaw += mouseDelta.x * m_cameraMouseSensitivity;
            m_pitch += mouseDelta.y * m_cameraMouseSensitivity;
            const float maxPitch = 1.57f;
            m_pitch = std::max(-maxPitch, std::min(maxPitch, m_pitch));
        }
        m_lastMouse = currentMouse;
        m_mouseCaptured = true;
    }
    else { m_mouseCaptured = false; }

    // FPS WASD + Shift run + Space/Ctrl vertical
    float moveSpeed = m_cameraMoveSpeed;
    if (input.isKeyDown(Key::Shift)) moveSpeed *= m_cameraRunMultiplier;

    Vec3 forward(std::sin(m_yaw), 0.0f, std::cos(m_yaw));
    Vec3 right(std::cos(m_yaw), 0.0f, -std::sin(m_yaw));
    Vec3 moveDir(0.0f, 0.0f, 0.0f);
    if (input.isKeyDown(Key::W)) moveDir += forward;
    if (input.isKeyDown(Key::S)) moveDir -= forward;
    if (input.isKeyDown(Key::A)) moveDir -= right;
    if (input.isKeyDown(Key::D)) moveDir += right;
    if (input.isKeyDown(Key::Space)) moveDir.y += 1.0f;
    if (input.isKeyDown(Key::Control)) moveDir.y -= 1.0f;
    if (moveDir.length() > 0.0f) {
        moveDir = moveDir.normalized();
        m_camera.setPosition(m_camera.getPosition() + moveDir * moveSpeed * dt);
    }

    // Update camera target from yaw/pitch
    Vec3 lookDir(
        std::sin(m_yaw) * std::cos(m_pitch),
        std::sin(m_pitch),
        std::cos(m_yaw) * std::cos(m_pitch)
    );
    m_camera.setTarget(m_camera.getPosition() + lookDir);
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

    // Draw skybox first (behind everything)
    if (m_skybox) {
        // Use skybox pipeline
        if (auto* skyboxPipeline = engine.getSkyboxPipeline()) {
            ctx.setGraphicsPipelineState(*skyboxPipeline);
        }

        // Configure depth: enable test with LESS_EQUAL and disable depth writes
        auto* d3dDeviceContext = ctx.getD3DDeviceContext();
        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = TRUE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Don't write to depth buffer
        depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Standard skybox depth test
        depthDesc.StencilEnable = FALSE;
        
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyboxDepthState;
        engine.getGraphicsDevice().getD3DDevice()->CreateDepthStencilState(&depthDesc, skyboxDepthState.GetAddressOf());
        d3dDeviceContext->OMSetDepthStencilState(skyboxDepthState.Get(), 0);
        
        // For skybox, we want to remove translation from view matrix
        // so the skybox always appears at infinity (like the OpenGL version)
        // Create a view matrix without translation
        Vec3 cameraPos = m_camera.getPosition();
        Vec3 target = m_camera.getTarget();
        Vec3 up = m_camera.getUp();
        
        // Create view matrix at origin (no translation)
        Mat4 viewMatrix = Mat4::lookAt(Vec3(0, 0, 0), target - cameraPos, up);
        ctx.setViewMatrix(viewMatrix);
        
        // Position skybox at origin (large enough to always be visible)
        Mat4 skyboxWorld = Mat4::identity(); // No translation, just at origin
        ctx.setWorldMatrix(skyboxWorld);
        
        // Draw skybox

        // Cull front faces so we render the inside of the cube
        D3D11_RASTERIZER_DESC rastDesc = {};
        rastDesc.FillMode = D3D11_FILL_SOLID;
        rastDesc.CullMode = D3D11_CULL_FRONT; // Cull front faces
        rastDesc.FrontCounterClockwise = FALSE;
        rastDesc.DepthBias = 0;
        rastDesc.SlopeScaledDepthBias = 0.0f;
        rastDesc.DepthBiasClamp = 0.0f;
        rastDesc.DepthClipEnable = TRUE;
        rastDesc.ScissorEnable = FALSE;
        rastDesc.MultisampleEnable = FALSE;
        rastDesc.AntialiasedLineEnable = FALSE;
        
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyboxRastState;
        engine.getGraphicsDevice().getD3DDevice()->CreateRasterizerState(&rastDesc, skyboxRastState.GetAddressOf());
        d3dDeviceContext->RSSetState(skyboxRastState.Get());

        // Bind cubemap SRV and a sampler to the skybox pixel shader slot 0
        if (m_skybox && m_skybox->getTexture())
        {
            ID3D11ShaderResourceView* srv = m_skybox->getTexture()->getSRV();
            d3dDeviceContext->PSSetShaderResources(0, 1, &srv);

            static Microsoft::WRL::ComPtr<ID3D11SamplerState> s_skyboxSampler;
            if (!s_skyboxSampler)
            {
                D3D11_SAMPLER_DESC samp{};
                samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                samp.ComparisonFunc = D3D11_COMPARISON_NEVER;
                samp.MinLOD = 0;
                samp.MaxLOD = D3D11_FLOAT32_MAX;
                engine.getGraphicsDevice().getD3DDevice()->CreateSamplerState(&samp, s_skyboxSampler.GetAddressOf());
            }
            ID3D11SamplerState* sampPtr = s_skyboxSampler.Get();
            d3dDeviceContext->PSSetSamplers(0, 1, &sampPtr);
        }

        if (m_showSkybox && m_skybox) m_skybox->draw(ctx);
        
        // Restore default depth state for other objects
        d3dDeviceContext->OMSetDepthStencilState(nullptr, 0);
        d3dDeviceContext->RSSetState(nullptr);
        
        // Switch back to 3D pipeline for other objects
        ctx.setGraphicsPipelineState(engine.get3DPipeline());
        
        // Restore original view matrix
        ctx.setViewMatrix(m_camera.getViewMatrix());
    }

    // (Ground plane removed)
    // Draw small ground plane
    if (m_groundPlane)
    {
        Mat4 groundWorld = Mat4::translation(Vec3(0.0f, -1.5f, 0.0f));
        ctx.setWorldMatrix(groundWorld);
        m_groundPlane->draw(ctx);
    }

    // (light debug spheres removed)

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

void ThreeDTestScene::renderImGui(GraphicsEngine& engine)
{
    if (!ImGui::Begin("Scene UI")) { ImGui::End(); return; }
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Pos: (%.2f, %.2f, %.2f)", m_camera.getPosition().x, m_camera.getPosition().y, m_camera.getPosition().z);
        ImGui::Text("Target: (%.2f, %.2f, %.2f)", m_camera.getTarget().x, m_camera.getTarget().y, m_camera.getTarget().z);
        ImGui::Text("Yaw: %.2f, Pitch: %.2f", m_yaw, m_pitch);
        if (ImGui::Button("Reset Camera")) {
            m_camera.setPosition({ 0.0f, 0.0f, -4.0f });
            m_camera.setTarget({ 0.0f, 0.0f, 0.0f });
            m_yaw = 0.0f; m_pitch = 0.0f;
        }
    }
    
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show skybox", &m_showSkybox);
        ImGui::Text("Mesh: %s", m_skybox ? "YES" : "NO");
        ImGui::Text("Texture: %s", (m_skybox && m_skybox->getTexture()) ? "YES" : "NO");
        if (ImGui::SliderFloat("Size", &m_skyboxSize, 10.0f, 5000.0f)) {
            auto& device = engine.getGraphicsDevice();
            m_skybox = Mesh::CreateCube(device, m_skyboxSize);
        }
        if (ImGui::Button("Reload Skybox Texture")) {
            auto& device = engine.getGraphicsDevice();
            std::wstring skyboxPath = L"DX3D/Assets/Textures/Skybox.png";
            auto skyboxTexture = Texture2D::LoadSkyboxCubemap(device.getD3DDevice(), skyboxPath.c_str());
            if (!skyboxTexture) skyboxTexture = Texture2D::CreateSkyboxCubemap(device.getD3DDevice());
            if (skyboxTexture && m_skybox) m_skybox->setTexture(skyboxTexture);
        }
        if (ImGui::Button("Use Debug Cubemap (colors)")) {
            auto& device = engine.getGraphicsDevice();
            auto skyboxTexture = Texture2D::CreateSkyboxCubemap(device.getD3DDevice());
            if (skyboxTexture && m_skybox) m_skybox->setTexture(skyboxTexture);
        }
        ImGui::Text("Drawing skybox - Pipeline: %s", engine.getSkyboxPipeline() ? "YES" : "NO");
        ImGui::Text("Camera: (%.2f, %.2f, %.2f)", m_camera.getPosition().x, m_camera.getPosition().y, m_camera.getPosition().z);
    }
    ImGui::End();
}


