#include <DX3D/Game/Scenes/CloudScene.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <random>
#include <iostream>
#include <imgui.h>
#include <cmath>
#include <float.h>

using namespace dx3d;

void CloudScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;

    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();
    
    // Initialize sun rotation variables
    m_sunRotationX = 0.0f;
    m_sunRotationY = 0.0f;
    m_sunRotationZ = 0.0f;
    m_sunManualRotation = true;

    // Create 3D camera
    float aspect = GraphicsEngine::getWindowWidth() / std::max(1.0f, GraphicsEngine::getWindowHeight());
    m_camera3D.setPerspective(1.22173048f, aspect, 0.1f, 5000.0f);
    setCameraPreset(CameraPreset::FirstPerson);

    // Create line renderer for potential debug visualization
    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false);
    
    // Clear the camera from LineRenderer so it uses the 3D camera matrices set by the scene
    m_lineRenderer->clearCamera();
    
    // Set dedicated line pipeline for optimal performance
    if (auto* linePipeline = engine.getLinePipeline()) {
        m_lineRenderer->setLinePipeline(linePipeline);
    }

    // Initialize shadow mapping
    GraphicsResourceDesc gDesc = device.getGraphicsResourceDesc();
    m_shadowMap = std::make_unique<ShadowMap>(gDesc, m_shadowMapSize, m_shadowMapSize);
    m_shadowMap2 = std::make_unique<ShadowMap>(gDesc, m_shadowMapSize, m_shadowMapSize);
    createShadowSampler(device.getD3DDevice());

    // Lighting is now integrated into the Sun structs

    // Create ground plane
    createGroundPlane(device);


    // Create sun entity
    createSunEntity(device);

    // Create a simple test cube for clouds
    createSimpleCloudCube(device);

    // Always show dotted background in 3D mode
    m_showDottedBackground = true;
    
    // Generate initial Worley noise texture
    generateWorleyNoiseTexture(device);
}

void CloudScene::createGroundPlane(GraphicsDevice& device) {
    // Create a large ground plane
    auto& groundEntity = m_entityManager->createEntity("GroundPlane");
    auto groundMesh = Mesh::CreatePlane(device, 200.0f, 200.0f); // 200x200 unit plane
    
    if (groundMesh) {
        // Apply a simple texture to the ground
        std::wstring groundTexturePath;
        groundTexturePath.assign(L"DX3D/Assets/Textures/beam.png");
        auto groundTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), groundTexturePath.c_str());
        if (groundTexture) {
            groundMesh->setTexture(groundTexture);
        }
        
        auto& groundMeshComp = groundEntity.addComponent<Mesh3DComponent>(groundMesh);
        groundMeshComp.setPosition(Vec3(0.0f, -5.0f, 0.0f)); // Position below the scene
        groundMeshComp.setScale(Vec3(1.0f, 1.0f, 1.0f));
        groundMeshComp.setVisible(true);
        
        // Set material for ground plane - darker to contrast with sky
        groundMeshComp.setMaterial(Vec3(0.2f, 0.3f, 0.2f), 32.0f, 0.3f); // Dark green material
    }
}


void CloudScene::createSunEntity(GraphicsDevice& device) {
    // Initialize both sun components
    initializeSuns();
    
    // Load existing textures
    std::wstring nodeTexturePath = L"DX3D/Assets/Textures/node.png";
    std::wstring bloomTexturePath = L"DX3D/Assets/Textures/MetaballFalloff.png";

    // Create sprites for both suns using the component methods
    m_sun1.createSprites(device, *m_entityManager, nodeTexturePath, bloomTexturePath);
    m_sun2.createSprites(device, *m_entityManager, nodeTexturePath, bloomTexturePath);

    // Back-compat pointer for main sun core
    m_sunSprite = m_sun1.getCoreSprite();
}

void CloudScene::createSimpleCloudCube(GraphicsDevice& device) {
    // Create a simple cube mesh for testing
    auto cloudMesh = Mesh::CreateCube(device, 1.0f);
    
    if (cloudMesh) {
        // Create cloud cube entity
        auto& cloudEntity = m_entityManager->createEntity("SimpleCloudCube");
        
        // Add mesh component with basic settings
        auto& cloudMeshComp = cloudEntity.addComponent<Mesh3DComponent>(cloudMesh);
        cloudMeshComp.setPosition(Vec3(0.0f, 5.0f, 0.0f));  // Much closer to ground
        cloudMeshComp.setScale(Vec3(20.0f, 8.0f, 20.0f));   // Smaller and more manageable
        cloudMeshComp.setVisible(true);
        
        // Set dark cloud material
        cloudMeshComp.setMaterial(Vec3(0.2f, 0.2f, 0.2f), 32.0f, 0.8f);
        
        std::cout << "Simple cloud cube created successfully" << std::endl;
    }
}


void CloudScene::initializeSuns() {
    // Configure sun1
    m_sun1.setBaseName("Sun1");
    m_sun1.setPosition(m_sunPosition);
    m_sun1.setRadius(m_sunRadius);
    m_sun1.setColor(m_sunColor);
    m_sun1.setVisible(m_sunVisible);
    
    // Initialize sun1 light properties
    m_sun1.setLightEnabled(true);
    m_sun1.setLightTarget(Vec3(0.0f, 0.0f, 0.0f));
    m_sun1.setLightColor(Vec3(1.0f, 0.95f, 0.8f));
    m_sun1.setLightIntensity(1.2f);
    m_sun1.setLightOrthoSize(200.0f);
    m_sun1.setLightNearPlane(0.1f);
    m_sun1.setLightFarPlane(400.0f);
    m_sun1.setLightShadows(true);
    
    // Position sun2 on the opposite side of the plane (negative Z, same height)
    Vec3 sun2Position = Vec3(-m_sunPosition.x, m_sunPosition.y, -m_sunPosition.z);
    m_sun2.setBaseName("Sun2");
    m_sun2.setPosition(sun2Position);
    m_sun2.setRadius(m_sunRadius * 0.7f);
    m_sun2.setColor(Vec3(0.6f, 0.7f, 1.0f)); // Cool sky light color
    m_sun2.setVisible(true);
    
    // Initialize sun2 light properties
    m_sun2.setLightEnabled(true);
    m_sun2.setLightTarget(Vec3(0.0f, 0.0f, 0.0f));
    m_sun2.setLightColor(Vec3(0.6f, 0.7f, 1.0f));
    m_sun2.setLightIntensity(0.4f);
    m_sun2.setLightOrthoSize(200.0f);
    m_sun2.setLightNearPlane(0.1f);
    m_sun2.setLightFarPlane(400.0f);
    m_sun2.setLightShadows(true);
}

// setupLighting() removed - lighting is now integrated into Sun structs

void CloudScene::setCameraPreset(CameraPreset preset) {
    m_cameraPreset = preset;
    float aspect = GraphicsEngine::getWindowWidth() / std::max(1.0f, GraphicsEngine::getWindowHeight());
    float fovY = 1.22173048f; // ~70 degrees
    float nearZ = 0.1f;
    float farZ = 5000.0f;
    
    if (preset == CameraPreset::TopDown) {
        m_camera3D.setPosition(Vec3(0.0f, 80.0f, 0.0f));
        m_cameraYaw = 0.0f;
        m_cameraPitch = -3.14159f * 0.5f;
        m_camera3D.setTarget(Vec3(0.0f, 0.0f, 0.0f));
        m_camera3D.setUp(Vec3(0.0f, 0.0f, -1.0f));
        fovY = 1.22173048f;
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    } else if (preset == CameraPreset::FirstPerson) {
        // FPS-style camera position - start at ground level looking up
        m_camera3D.setPosition(Vec3(0.0f, 2.0f, 10.0f));
        m_cameraYaw = 0.0f;
        m_cameraPitch = 0.0f; // Level horizon
        m_camera3D.setTarget(Vec3(0.0f, 20.0f, 0.0f)); // Look up
        m_camera3D.setUp(Vec3(0.0f, 1.0f, 0.0f));
        fovY = 1.22173048f; // ~70 degrees FOV for FPS feel
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    } else if (preset == CameraPreset::Isometric) {
        Vec3 pos = Vec3(60.0f, 40.0f, 60.0f);
        m_camera3D.setPosition(pos);
        m_cameraYaw = 0.785398f; // 45 degrees
        m_cameraPitch = -0.523599f; // -30 degrees
        m_camera3D.setTarget(Vec3(0.0f, 20.0f, 0.0f));
        m_camera3D.setUp(Vec3(0.0f, 1.0f, 0.0f));
        fovY = 0.785398f; // 45 degrees
        m_camera3D.setPerspective(fovY, aspect, nearZ, farZ);
    }
}

void CloudScene::update3DCamera(float dt) {
    auto& input = Input::getInstance();
    
    // Only update camera if we're in FPS mode
    if (m_cameraPreset != CameraPreset::FirstPerson) return;
    
    // Mouse look controls - right click and drag
    if (input.isMouseDown(MouseClick::RightMouse)) {
        Vec2 currentMouse = input.getMousePositionNDC();
        
        // Only apply mouse delta if we have a previous mouse position
        if (m_mouseCaptured) {
            Vec2 mouseDelta = currentMouse - m_lastMouse;
            
            // Apply mouse sensitivity
            m_cameraYaw += mouseDelta.x * m_cameraMouseSensitivity;
            m_cameraPitch += mouseDelta.y * m_cameraMouseSensitivity;
            
            // Clamp pitch to prevent over-rotation
            const float maxPitch = 1.57f; // 90 degrees
            m_cameraPitch = std::max(-maxPitch, std::min(maxPitch, m_cameraPitch));
        }
        
        m_lastMouse = currentMouse;
        m_mouseCaptured = true;
    } else {
        // Reset mouse capture when not holding right mouse
        m_mouseCaptured = false;
    }
    
    // WASD movement controls
    float moveSpeed = m_cameraMoveSpeed;
    if (input.isKeyDown(Key::Shift)) {
        moveSpeed *= m_cameraRunMultiplier; // Run when holding shift
    }
    
    Vec3 moveDirection(0.0f, 0.0f, 0.0f);
    
    // Calculate forward and right vectors based on yaw
    Vec3 forward = Vec3(
        std::sin(m_cameraYaw),
        0.0f,
        std::cos(m_cameraYaw)
    );
    Vec3 right = Vec3(
        std::cos(m_cameraYaw),
        0.0f,
        -std::sin(m_cameraYaw)
    );
    
    // WASD movement
    if (input.isKeyDown(Key::W)) {
        moveDirection += forward;
    }
    if (input.isKeyDown(Key::S)) {
        moveDirection -= forward;
    }
    if (input.isKeyDown(Key::A)) {
        moveDirection -= right;
    }
    if (input.isKeyDown(Key::D)) {
        moveDirection += right;
    }
    
    // Vertical movement (Space/C for up/down)
    if (input.isKeyDown(Key::Space)) {
        moveDirection.y += 1.0f;
    }
    if (input.isKeyDown(Key::Control)) {
        moveDirection.y -= 1.0f;
    }
    
    // Normalize movement direction and apply speed
    if (moveDirection.length() > 0.0f) {
        moveDirection = moveDirection.normalized();
        Vec3 currentPos = m_camera3D.getPosition();
        Vec3 newPos = currentPos + moveDirection * moveSpeed * dt;
        m_camera3D.setPosition(newPos);
    }
    
    // Update camera target based on yaw and pitch
    Vec3 target = m_camera3D.getPosition() + Vec3(
        std::sin(m_cameraYaw) * std::cos(m_cameraPitch),
        std::sin(m_cameraPitch),
        std::cos(m_cameraYaw) * std::cos(m_cameraPitch)
    );
    
    m_camera3D.setTarget(target);
}


void CloudScene::updateSunAnimation(float dt) {
    if (!m_sunVisible || !m_sunSprite) return;
    
    // Gentle pulsing animation
    static float sunTime = 0.0f;
    sunTime += dt * m_sunPulseSpeed;
    float pulse = 1.0f + std::sin(sunTime) * m_sunPulseAmplitude;
    
    // Update tint color with slight variation
    Vec3 colorVariation = m_sunColor * (1.0f + std::sin(sunTime * 0.5f) * 0.1f);
    
    // Update both suns consistently using component methods
    m_sun1.updateVisuals(pulse, colorVariation);
    m_sun2.updateVisuals(pulse, m_sun2.getColor());
    
    // Update light directions to match sun positions
    m_sun1.updateLightDirection();
    m_sun2.updateLightDirection();
    
    // Billboard rotation is now handled in the render loop
    // No need to set rotation here since we calculate it dynamically
    // based on camera position during rendering
}

void CloudScene::update(float dt) {
    auto& input = Input::getInstance();

    // Update 3D camera
    update3DCamera(dt);
    
    
    // Update sun animation
    updateSunAnimation(dt);
    
    // Generate Worley noise texture if needed
    if (m_worleyTextureNeedsUpdate && m_graphicsDevice) {
        generateWorleyNoiseTexture(*m_graphicsDevice);
    }
    
    // Handle camera preset switching
    if (input.wasKeyJustPressed(Key::F1)) {
        setCameraPreset(CameraPreset::FirstPerson);
    }
    if (input.wasKeyJustPressed(Key::F2)) {
        setCameraPreset(CameraPreset::TopDown);
    }
    if (input.wasKeyJustPressed(Key::F3)) {
        setCameraPreset(CameraPreset::Isometric);
    }
    
    // Toggle shadow map debug
    if (input.wasKeyJustPressed(Key::F4)) {
        m_showShadowMapDebug = !m_showShadowMapDebug;
    }
}

void CloudScene::render(GraphicsEngine& engine, SwapChain& swapChain) {
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // Render shadow map first
    if (m_shadowMap) {
        renderShadowMap(engine);
    }
    
    // Begin main rendering
    ctx.clearAndSetBackBuffer(swapChain, m_backgroundColor);
    
    // Set viewport to full window size for 3D rendering
    ctx.setViewportSize(Rect{0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight)});
    
    // Render dotted background using the same shader as 2D mode
    if (m_showDottedBackground && engine.getBackgroundDotsPipeline()) {
        GraphicsEngine::renderBackgroundDots(ctx, engine.getBackgroundDotsPipeline(), 
            screenWidth, screenHeight, m_dotSpacing, m_dotRadius, m_backgroundColor);
    }
    
    // Enable depth testing for 3D rendering
    ctx.enableDepthTest();
    
    // Ensure 3D pipeline is active for meshes with shadow mapping
    ctx.setGraphicsPipelineState(engine.get3DPipeline());
    
    // Set 3D camera matrices
    ctx.setViewMatrix(m_camera3D.getViewMatrix());
    ctx.setProjectionMatrix(m_camera3D.getProjectionMatrix());
    
    // Lighting setup with integrated sun lights
    std::vector<Vec3> lightDirs;
    std::vector<Vec3> lightColors;
    std::vector<float> lightIntensities;
    
    if (m_sun1.isLightEnabled()) {
        lightDirs.push_back(m_sun1.getLightDirection());
        lightColors.push_back(m_sun1.getLightColor());
        lightIntensities.push_back(m_sun1.getLightIntensity());
    }
    
    if (m_sun2.isLightEnabled()) {
        lightDirs.push_back(m_sun2.getLightDirection());
        lightColors.push_back(m_sun2.getLightColor());
        lightIntensities.push_back(m_sun2.getLightIntensity());
    }
    
    // If no lights are enabled, add a minimal ambient light
    if (lightDirs.empty()) {
        lightDirs.push_back(Vec3(0.0f, -1.0f, 0.0f));
        lightColors.push_back(Vec3(0.1f, 0.1f, 0.1f));
        lightIntensities.push_back(0.1f);
    }
    
    ctx.setLights(lightDirs, lightColors, lightIntensities);
    ctx.setCameraPosition(m_camera3D.getPosition());
    
    // Set shadow maps for enabled lights only
    if (m_shadowSampler && m_shadowMap && m_shadowMap2 && m_enableShadowMapping) {
        std::vector<ID3D11ShaderResourceView*> shadowSRVs;
        std::vector<Mat4> shadowMatrices;
        
        if (m_sun1.isLightEnabled() && m_sun1.hasLightShadows()) {
            shadowSRVs.push_back(m_shadowMap->getDepthSRV());
            shadowMatrices.push_back(m_lightViewProj);
        }
        
        if (m_sun2.isLightEnabled() && m_sun2.hasLightShadows()) {
            shadowSRVs.push_back(m_shadowMap2->getDepthSRV());
            shadowMatrices.push_back(m_lightViewProj2);
        }
        
        if (!shadowSRVs.empty()) {
            ctx.setShadowMaps(shadowSRVs.data(), static_cast<int>(shadowSRVs.size()), m_shadowSampler.Get());
            ctx.setShadowMatrices(shadowMatrices);
        }
    }
    
    // Render 3D meshes
    auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
    
    for (auto* entity : mesh3DEntities) {
        if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
            if (meshComp->isVisible()) {
                // Set world matrix for proper 3D positioning
                Mat4 worldMatrix = Mat4::identity();
                worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                ctx.setWorldMatrix(worldMatrix);
                
                meshComp->draw(ctx);
            }
        }
    }
    
    // Render sprites (like the sun) - switch to default pipeline for sprites
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    
    auto spriteEntities = m_entityManager->getEntitiesWithComponent<SpriteComponent>();
    
    //
    
    for (auto* entity : spriteEntities) {
        if (auto* spriteComp = entity->getComponent<SpriteComponent>()) {
            if (spriteComp->isVisible()) {
                //
                
                // For billboarded sprites in 3D space, we need to ensure they use 3D matrices
                // Set the 3D camera matrices for proper 3D positioning
                ctx.setViewMatrix(m_camera3D.getViewMatrix());
                ctx.setProjectionMatrix(m_camera3D.getProjectionMatrix());
                
                // Create billboarded world matrix
                // Keep sun positions synced to their logical structures
                if (entity->getName().find("Sun1") != std::string::npos) {
                    spriteComp->setPosition(m_sun1.getPosition());
                    ctx.setTint(spriteComp->getTint());
                } else if (entity->getName().find("Sun2") != std::string::npos) {
                    spriteComp->setPosition(m_sun2.getPosition());
                    ctx.setTint(spriteComp->getTint());
                }
                Vec3 spritePos = spriteComp->getPosition();
                Vec3 cameraPos = m_camera3D.getPosition();
                
                // Calculate horizontal direction only for stable billboarding
                Vec3 horizontalDirection = Vec3(spritePos.x - cameraPos.x, 0.0f, spritePos.z - cameraPos.z);
                
                // Handle edge case when camera is directly under the sprite
                float yaw = 0.0f;
                if (horizontalDirection.length() > 0.001f) {
                    horizontalDirection = horizontalDirection.normalized();
                    yaw = std::atan2(horizontalDirection.x, horizontalDirection.z);
                }
                
                //
                
                // Create stable billboard rotation matrix (Y rotation only, no pitch)
                Mat4 billboardRotation = Mat4::rotationY(yaw);
                
                // Create translation matrix
                Mat4 translationMatrix = Mat4::translation(spritePos);
                
                // Combine: R * T (rotate then translate for proper billboarding)
                Mat4 worldMatrix = billboardRotation * translationMatrix;
                
                ctx.setWorldMatrix(worldMatrix);
                
                // Draw the sprite mesh directly instead of using spriteComp->draw()
                auto mesh = spriteComp->getMesh();
                if (mesh) {
                    // Enable alpha blending for the sun sprite
                    ctx.enableAlphaBlending();
                    ctx.enableTransparentDepth();
                    
                    ctx.setTint(spriteComp->getTint());
                    mesh->draw(ctx);
                    
                    // Restore depth state
                    ctx.disableAlphaBlending();
                    ctx.enableDefaultDepth();
                }
            }
        }
    }
    
    // Render shadow map debug overlay if enabled
    if (m_showShadowMapDebug) {
        renderShadowMapDebug(engine);
    }

    // Switch to default pipeline for UI in screen space
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    
    // Ensure proper viewport for UI rendering
    ctx.setViewportSize(Rect{0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight)});
    ctx.setScreenSpaceMatrices(screenWidth, screenHeight);
}

void CloudScene::renderImGui(GraphicsEngine& engine) {
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("3D Scene Controls")) {
        ImGui::Text("3D Scene - 3D Mode");
        ImGui::Separator();
        
        // Camera controls
        ImGui::Text("Camera");
        ImGui::Separator();
        int presetIdx = (m_cameraPreset == CameraPreset::FirstPerson) ? 0 : 
                       (m_cameraPreset == CameraPreset::TopDown) ? 1 : 2;
        const char* presets[] = { "First Person", "Top Down", "Isometric" };
        if (ImGui::Combo("Camera Mode", &presetIdx, presets, 3)) {
            m_cameraPreset = (presetIdx == 0) ? CameraPreset::FirstPerson : 
                            (presetIdx == 1) ? CameraPreset::TopDown : CameraPreset::Isometric;
            setCameraPreset(m_cameraPreset);
        }
        
        ImGui::SliderFloat("Move Speed", &m_cameraMoveSpeed, 5.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Mouse Sensitivity", &m_cameraMouseSensitivity, 0.5f, 5.0f, "%.1f");
        
        ImGui::Spacing();
        ImGui::Text("Sun");
        ImGui::Separator();
        
        // Sun 1 Controls (integrated visual + lighting)
        ImGui::Text("Sun 1");
        ImGui::Separator();
        bool sun1Visible = m_sun1.isVisible();
        if (ImGui::Checkbox("Show Sun 1", &sun1Visible)) {
            m_sun1.setVisible(sun1Visible);
        }
        if (sun1Visible) {
            Vec3 sun1Color = m_sun1.getColor();
            float sun1ColorArray[3] = { sun1Color.x, sun1Color.y, sun1Color.z };
            if (ImGui::ColorEdit3("Sun 1 Color", sun1ColorArray)) {
                Vec3 newColor(sun1ColorArray[0], sun1ColorArray[1], sun1ColorArray[2]);
                m_sun1.setColor(newColor);
                m_sun1.setLightColor(newColor);
            }
            float sun1Radius = m_sun1.getRadius();
            if (ImGui::SliderFloat("Sun 1 Radius", &sun1Radius, 2.0f, 20.0f, "%.1f")) {
                m_sun1.setRadius(sun1Radius);
            }
            ImGui::SliderFloat("Pulse Speed", &m_sunPulseSpeed, 0.1f, 3.0f, "%.1f");
            ImGui::SliderFloat("Pulse Amplitude", &m_sunPulseAmplitude, 0.0f, 0.5f, "%.2f");
        }
        
        ImGui::Spacing();
        ImGui::Text("Sun 1 Lighting");
        ImGui::Separator();
        bool sun1LightEnabled = m_sun1.isLightEnabled();
        if (ImGui::Checkbox("Enable Sun 1 Light", &sun1LightEnabled)) {
            m_sun1.setLightEnabled(sun1LightEnabled);
        }
        if (sun1LightEnabled) {
            float sun1Intensity = m_sun1.getLightIntensity();
            if (ImGui::SliderFloat("Sun 1 Intensity", &sun1Intensity, 0.0f, 3.0f, "%.2f")) {
                m_sun1.setLightIntensity(sun1Intensity);
            }
            bool sun1Shadows = m_sun1.hasLightShadows();
            if (ImGui::Checkbox("Sun 1 Shadows", &sun1Shadows)) {
                m_sun1.setLightShadows(sun1Shadows);
            }
        }
        
        ImGui::Spacing();
        
        // Sun 2 Controls (integrated visual + lighting)
        ImGui::Text("Sun 2");
        ImGui::Separator();
        bool sun2Visible = m_sun2.isVisible();
        if (ImGui::Checkbox("Show Sun 2", &sun2Visible)) {
            m_sun2.setVisible(sun2Visible);
        }
        if (sun2Visible) {
            Vec3 sun2Color = m_sun2.getColor();
            float sun2ColorArray[3] = { sun2Color.x, sun2Color.y, sun2Color.z };
            if (ImGui::ColorEdit3("Sun 2 Color", sun2ColorArray)) {
                Vec3 newColor(sun2ColorArray[0], sun2ColorArray[1], sun2ColorArray[2]);
                m_sun2.setColor(newColor);
                m_sun2.setLightColor(newColor);
            }
            float sun2Radius = m_sun2.getRadius();
            if (ImGui::SliderFloat("Sun 2 Radius", &sun2Radius, 2.0f, 20.0f, "%.1f")) {
                m_sun2.setRadius(sun2Radius);
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Sun 2 Lighting");
        ImGui::Separator();
        bool sun2LightEnabled = m_sun2.isLightEnabled();
        if (ImGui::Checkbox("Enable Sun 2 Light", &sun2LightEnabled)) {
            m_sun2.setLightEnabled(sun2LightEnabled);
        }
        if (sun2LightEnabled) {
            float sun2Intensity = m_sun2.getLightIntensity();
            if (ImGui::SliderFloat("Sun 2 Intensity", &sun2Intensity, 0.0f, 2.0f, "%.2f")) {
                m_sun2.setLightIntensity(sun2Intensity);
            }
            bool sun2Shadows = m_sun2.hasLightShadows();
            if (ImGui::Checkbox("Sun 2 Shadows", &sun2Shadows)) {
                m_sun2.setLightShadows(sun2Shadows);
            }
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Debug");
        ImGui::Separator();
        ImGui::Checkbox("Show Shadow Debug", &m_showShadowMapDebug);
        
        if (m_showShadowMapDebug) {
            const char* lightItems[] = { "Sun Light", "Sky Light" };
            ImGui::Combo("Shadow Map", &m_selectedShadowMap, lightItems, 2);
            ImGui::SliderFloat("Preview Size", &m_shadowPreviewSize, 100.0f, 400.0f, "%.0f");
        }
        
        ImGui::Spacing();
        ImGui::Text("Worley Noise Generator");
        ImGui::Separator();
        
        // Worley noise controls
        bool settingsChanged = false;
        
        // Basic settings
        if (ImGui::CollapsingHeader("Basic Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderInt("Seed", &m_worleySettings.seed, 0, 1000)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderInt("Divisions A", &m_worleySettings.numDivisionsA, 1, 50)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderInt("Divisions B", &m_worleySettings.numDivisionsB, 1, 50)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderInt("Divisions C", &m_worleySettings.numDivisionsC, 1, 50)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Persistence", &m_worleySettings.persistence, 0.1f, 1.0f, "%.2f")) {
                settingsChanged = true;
            }
            
            if (ImGui::Checkbox("Invert", &m_worleySettings.invert)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderInt("Texture Size", &m_worleySettings.textureSize, 64, 512)) {
                settingsChanged = true;
            }
            
            ImGui::Checkbox("Auto Update", &m_worleySettings.autoUpdate);
        }
        
        // Advanced settings
        if (ImGui::CollapsingHeader("Advanced Settings")) {
            if (ImGui::SliderFloat("Shape Resolution", &m_worleySettings.shapeResolution, 32.0f, 256.0f, "%.0f")) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Detail Resolution", &m_worleySettings.detailResolution, 16.0f, 128.0f, "%.0f")) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Noise Scale", &m_worleySettings.noiseScale, 0.1f, 3.0f, "%.2f")) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Noise Offset", &m_worleySettings.noiseOffset, -2.0f, 2.0f, "%.2f")) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Noise Rotation", &m_worleySettings.noiseRotation, 0.0f, 6.28f, "%.2f")) {
                settingsChanged = true;
            }
            
            if (ImGui::Checkbox("Use Distance", &m_worleySettings.useDistance)) {
                settingsChanged = true;
            }
            
            if (ImGui::Checkbox("Use F1-F2", &m_worleySettings.useF1F2)) {
                settingsChanged = true;
            }
            
            if (m_worleySettings.useF1F2) {
                if (ImGui::SliderFloat("F1 Weight", &m_worleySettings.f1Weight, 0.0f, 2.0f, "%.2f")) {
                    settingsChanged = true;
                }
                
                if (ImGui::SliderFloat("F2 Weight", &m_worleySettings.f2Weight, 0.0f, 2.0f, "%.2f")) {
                    settingsChanged = true;
                }
            }
        }
        
        // Viewer settings
        if (ImGui::CollapsingHeader("Viewer Settings")) {
            if (ImGui::Checkbox("Viewer Enabled", &m_worleySettings.viewerEnabled)) {
                settingsChanged = true;
            }
            
            if (ImGui::Checkbox("Viewer Greyscale", &m_worleySettings.viewerGreyscale)) {
                settingsChanged = true;
            }
            
            if (ImGui::Checkbox("Show All Channels", &m_worleySettings.viewerShowAllChannels)) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Slice Depth", &m_worleySettings.viewerSliceDepth, 0.0f, 1.0f, "%.3f")) {
                settingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Tile Amount", &m_worleySettings.viewerTileAmount, 0.1f, 4.0f, "%.1f")) {
                settingsChanged = true;
            }
        }
        
        if (ImGui::Button("Generate Texture") || (settingsChanged && m_worleySettings.autoUpdate)) {
            m_worleyTextureNeedsUpdate = true;
        }
        
        // Display texture preview
        if (m_worleyTexture) {
            ImGui::Spacing();
            ImGui::Text("Preview:");
            ImGui::Separator();
            
            // Create a preview quad
            float previewSize = 200.0f;
            ImVec2 uv0(0.0f, 0.0f);
            ImVec2 uv1(1.0f, 1.0f);
            ImVec4 tint_col(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 border_col(0.5f, 0.5f, 0.5f, 1.0f);
            
            ImGui::Image((void*)m_worleyTexture->getSRV(), ImVec2(previewSize, previewSize), uv0, uv1, tint_col, border_col);
        }
        
        /*
        ImGui::Spacing();
        ImGui::Text("Cloud Cube");
        ImGui::Separator();
        
        // Cloud cube controls
        bool cloudSettingsChanged = false;
        
        if (ImGui::Checkbox("Show Cloud Cube", &m_cloudCubeSettings.visible)) {
            cloudSettingsChanged = true;
        }
        
        if (m_cloudCubeSettings.visible) {
            float pos[3] = { m_cloudCubeSettings.position.x, m_cloudCubeSettings.position.y, m_cloudCubeSettings.position.z };
            if (ImGui::SliderFloat3("Position", pos, -100.0f, 100.0f, "%.1f")) {
                m_cloudCubeSettings.position = Vec3(pos[0], pos[1], pos[2]);
                cloudSettingsChanged = true;
            }
            
            float scale[3] = { m_cloudCubeSettings.scale.x, m_cloudCubeSettings.scale.y, m_cloudCubeSettings.scale.z };
            if (ImGui::SliderFloat3("Scale", scale, 1.0f, 100.0f, "%.1f")) {
                m_cloudCubeSettings.scale = Vec3(scale[0], scale[1], scale[2]);
                cloudSettingsChanged = true;
            }
            
            float color[3] = { m_cloudCubeSettings.color.x, m_cloudCubeSettings.color.y, m_cloudCubeSettings.color.z };
            if (ImGui::ColorEdit3("Cloud Color", color)) {
                m_cloudCubeSettings.color = Vec3(color[0], color[1], color[2]);
                cloudSettingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Density", &m_cloudCubeSettings.density, 0.1f, 5.0f, "%.2f")) {
                cloudSettingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Coverage", &m_cloudCubeSettings.coverage, 0.0f, 1.0f, "%.2f")) {
                cloudSettingsChanged = true;
            }
            
            if (ImGui::SliderFloat("Speed", &m_cloudCubeSettings.speed, 0.0f, 5.0f, "%.2f")) {
                cloudSettingsChanged = true;
            }
            
            if (ImGui::SliderInt("Ray Steps", &m_cloudCubeSettings.numSteps, 16, 128)) {
                cloudSettingsChanged = true;
            }
        }
        
        // Update cloud cube if settings changed
        if (cloudSettingsChanged && m_cloudCubeComponent) {
            m_cloudCubeComponent->setPosition(m_cloudCubeSettings.position);
            m_cloudCubeComponent->setScale(m_cloudCubeSettings.scale);
            m_cloudCubeComponent->setVisible(m_cloudCubeSettings.visible);
            m_cloudCubeComponent->setMaterial(m_cloudCubeSettings.color, 32.0f, 0.8f);
        }
        */
        
        ImGui::Spacing();
        ImGui::Text("Controls:");
        ImGui::Text("WASD - Move");
        ImGui::Text("Space/Ctrl - Up/Down");
        ImGui::Text("Right Mouse - Look around");
        ImGui::Text("F1/F2/F3 - Camera presets");
        ImGui::Text("F4 - Toggle shadow debug");
    }
    ImGui::End();
}

void CloudScene::calculateLightViewProj() {
    // Light 1: Sun (top-down)
    if (m_sun1.isLightEnabled()) {
        Vec3 lightUp(0.0f, 0.0f, 1.0f);
        Mat4 lightView = Mat4::lookAt(m_sun1.getPosition(), m_sun1.getLightTarget(), lightUp);
        Mat4 lightProj = Mat4::orthographic(m_sun1.getLightOrthoSize(), m_sun1.getLightOrthoSize(), 
                                           m_sun1.getLightNearPlane(), m_sun1.getLightFarPlane());
        m_lightViewProj = lightView * lightProj;
    }

    // Light 2: Sky light (angled)
    if (m_sun2.isLightEnabled()) {
        Vec3 lightUp2(0.0f, 1.0f, 0.0f);
        Mat4 lightView2 = Mat4::lookAt(m_sun2.getPosition(), m_sun2.getLightTarget(), lightUp2);
        Mat4 lightProj2 = Mat4::orthographic(m_sun2.getLightOrthoSize(), m_sun2.getLightOrthoSize(), 
                                            m_sun2.getLightNearPlane(), m_sun2.getLightFarPlane());
        m_lightViewProj2 = lightView2 * lightProj2;
    }
}

void CloudScene::renderShadowMap(GraphicsEngine& engine) {
    if (!m_shadowMap || !m_enableShadowMapping) return;
    
    auto& ctx = engine.getContext();
    auto* d3dContext = ctx.getD3DDeviceContext();
    
    // Ensure the shadow map SRVs are unbound before binding as DSVs
    ID3D11ShaderResourceView* nullSRVs[10] = {};
    d3dContext->PSSetShaderResources(1, 10, nullSRVs);
    
    // Calculate light matrices
    calculateLightViewProj();
    
    // Render Light 1 shadow map (only if enabled and shadows enabled for this light)
    if (m_sun1.isLightEnabled() && m_sun1.hasLightShadows()) {
        m_shadowMap->clear(d3dContext);
        m_shadowMap->setAsRenderTarget(d3dContext);
        m_shadowMap->setViewport(d3dContext);
        
        // Set light 1 matrices using integrated sun settings
        Vec3 lightUp(0.0f, 0.0f, 1.0f);
        ctx.setViewMatrix(Mat4::lookAt(m_sun1.getPosition(), m_sun1.getLightTarget(), lightUp));
        ctx.setProjectionMatrix(Mat4::orthographic(m_sun1.getLightOrthoSize(), m_sun1.getLightOrthoSize(), 
                                                  m_sun1.getLightNearPlane(), m_sun1.getLightFarPlane()));
        
        // Use shadow map pipeline
        ctx.setGraphicsPipelineState(engine.getShadowMapPipeline());
        
        // Render 3D meshes to shadow map (exclude sun)
        auto mesh3DEntities = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
        for (auto* entity : mesh3DEntities) {
            // Skip sun entity for shadow mapping
            if (entity->getName() == "Sun") continue;
            
            if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                if (meshComp->isVisible()) {
                    // Set world matrix
                    Mat4 worldMatrix = Mat4::identity();
                    worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                    worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                    ctx.setWorldMatrix(worldMatrix);
                    
                    meshComp->draw(ctx);
                }
            }
        }
        
        // Unbind
        d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    // Render Light 2 shadow map (only if enabled and shadows enabled for this light)
    if (m_sun2.isLightEnabled() && m_sun2.hasLightShadows()) {
        m_shadowMap2->clear(d3dContext);
        m_shadowMap2->setAsRenderTarget(d3dContext);
        m_shadowMap2->setViewport(d3dContext);

        // Set light 2 matrices using integrated sun settings
        Vec3 lightUp2(0.0f, 1.0f, 0.0f);
        ctx.setViewMatrix(Mat4::lookAt(m_sun2.getPosition(), m_sun2.getLightTarget(), lightUp2));
        ctx.setProjectionMatrix(Mat4::orthographic(m_sun2.getLightOrthoSize(), m_sun2.getLightOrthoSize(), 
                                                 m_sun2.getLightNearPlane(), m_sun2.getLightFarPlane()));

        ctx.setGraphicsPipelineState(engine.getShadowMapPipeline());

        auto mesh3DEntities2 = m_entityManager->getEntitiesWithComponent<Mesh3DComponent>();
        for (auto* entity : mesh3DEntities2) {
            // Skip sun entity for shadow mapping
            if (entity->getName() == "Sun") continue;
            
            if (auto* meshComp = entity->getComponent<Mesh3DComponent>()) {
                if (meshComp->isVisible()) {
                    Mat4 worldMatrix = Mat4::identity();
                    worldMatrix = worldMatrix * Mat4::translation(meshComp->getPosition());
                    worldMatrix = worldMatrix * Mat4::scale(meshComp->getScale());
                    ctx.setWorldMatrix(worldMatrix);
                    meshComp->draw(ctx);
                }
            }
        }

        // Unbind
        d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    }
}

void CloudScene::createShadowSampler(ID3D11Device* device) {
    D3D11_SAMPLER_DESC samplerDesc = {};
    
    if (m_softShadows) {
        // Use linear filtering for soft shadows
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    } else {
        // Use point filtering for hard shadows
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    }
    
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    device->CreateSamplerState(&samplerDesc, m_shadowSampler.GetAddressOf());
}

void CloudScene::renderShadowMapDebug(GraphicsEngine& engine) {
    if (!m_shadowMap || !m_showShadowMapDebug) return;
    
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    
    // Switch to screen space for debug overlay
    if (auto* debugPipeline = engine.getShadowMapDebugPipeline()) {
        ctx.setGraphicsPipelineState(*debugPipeline);
    } else {
        ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    }
    ctx.setScreenSpaceMatrices(screenWidth, screenHeight);
    
    // Create debug quad in top-right corner
    float quadSize = m_shadowPreviewSize;
    float margin = 20.0f;
    float x = screenWidth - quadSize - margin;
    float y = margin;
    
    // Convert to NDC coordinates (-1 to 1)
    float ndcX = (x / screenWidth) * 2.0f - 1.0f;
    float ndcY = 1.0f - (y / screenHeight) * 2.0f; // Flip Y for DirectX
    float ndcX2 = ((x + quadSize) / screenWidth) * 2.0f - 1.0f;
    float ndcY2 = 1.0f - ((y + quadSize) / screenHeight) * 2.0f;
    
    struct DebugVertex {
        float x, y, z;
        float u, v;
    };
    
    DebugVertex vertices[] = {
        { ndcX, ndcY, 0.0f, 0.0f, 0.0f },
        { ndcX2, ndcY, 0.0f, 1.0f, 0.0f },
        { ndcX, ndcY2, 0.0f, 0.0f, 1.0f },
        { ndcX2, ndcY2, 0.0f, 1.0f, 1.0f }
    };
    
    // Create vertex buffer
    static Microsoft::WRL::ComPtr<ID3D11Buffer> debugVertexBuffer;
    if (!debugVertexBuffer) {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(vertices);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertices;
        
        auto device = engine.getGraphicsDevice().getD3DDevice();
        device->CreateBuffer(&bufferDesc, &initData, debugVertexBuffer.GetAddressOf());
    }
    
    // Set up rendering
    auto* d3dContext = ctx.getD3DDeviceContext();
    UINT stride = sizeof(DebugVertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, debugVertexBuffer.GetAddressOf(), &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // Bind shadow map
    ID3D11ShaderResourceView* shadowSRV = nullptr;
    if (m_selectedShadowMap == 0) {
        if (m_shadowMap) shadowSRV = m_shadowMap->getDepthSRV();
    } else {
        if (m_shadowMap2) shadowSRV = m_shadowMap2->getDepthSRV();
    }
    
    if (shadowSRV) {
        d3dContext->PSSetShaderResources(0, 1, &shadowSRV);
    }
    
    // Create simple sampler
    static Microsoft::WRL::ComPtr<ID3D11SamplerState> debugSampler;
    if (!debugSampler) {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        
        auto device = engine.getGraphicsDevice().getD3DDevice();
        device->CreateSamplerState(&samplerDesc, debugSampler.GetAddressOf());
    }
    
    d3dContext->PSSetSamplers(0, 1, debugSampler.GetAddressOf());
    ctx.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    // Draw debug quad
    d3dContext->Draw(4, 0);
    
    // Unbind
    ID3D11ShaderResourceView* nullSRV = nullptr;
    d3dContext->PSSetShaderResources(0, 1, &nullSRV);
}

void CloudScene::fixedUpdate(float dt) {
    // Fixed update logic if needed
}

Vec3 CloudScene::screenToWorldPosition3D(const Vec2& screenPos) {
    // Convert screen position to 3D world position
    float ndcX = screenPos.x * 2.0f - 1.0f;
    float ndcY = screenPos.y * 2.0f - 1.0f;
    
    // For now, just project onto the ground plane at y=0
    Vec3 worldPos;
    worldPos.x = ndcX * 50.0f;
    worldPos.y = 0.0f;
    worldPos.z = ndcY * 50.0f;
    
    return worldPos;
}

// updateSunVisuals() moved to SunComponent class

// syncLightsToSuns() removed - lights are now integrated into Sun structs

void CloudScene::generateWorleyNoiseTexture(GraphicsDevice& device) {
    if (!m_worleyTextureNeedsUpdate) return;
    
    auto d3dDevice = device.getD3DDevice();
    if (!d3dDevice) return;
    
    int size = m_worleySettings.textureSize;
    
    // Create pixel data
    std::unique_ptr<BYTE[]> pixels(new BYTE[size * size * 4]);
    
    // Generate random points for each division level
    std::mt19937 rng(m_worleySettings.seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate feature points for each octave
    std::vector<std::vector<Vec2>> featurePoints(3);
    std::vector<int> divisions = {m_worleySettings.numDivisionsA, m_worleySettings.numDivisionsB, m_worleySettings.numDivisionsC};
    
    for (int octave = 0; octave < 3; octave++) {
        int div = divisions[octave];
        featurePoints[octave].resize(div * div);
        
        for (int i = 0; i < div * div; i++) {
            featurePoints[octave][i] = Vec2(dist(rng), dist(rng));
        }
    }
    
    // Generate noise for each pixel
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 4;
            
            float u = (float)x / size;
            float v = (float)y / size;
            
            // Apply noise scale and offset
            float scaledU = u * m_worleySettings.noiseScale + m_worleySettings.noiseOffset;
            float scaledV = v * m_worleySettings.noiseScale + m_worleySettings.noiseOffset;
            
            // Apply rotation if needed
            if (m_worleySettings.noiseRotation != 0.0f) {
                float cosR = std::cos(m_worleySettings.noiseRotation);
                float sinR = std::sin(m_worleySettings.noiseRotation);
                float centerU = scaledU - 0.5f;
                float centerV = scaledV - 0.5f;
                scaledU = centerU * cosR - centerV * sinR + 0.5f;
                scaledV = centerU * sinR + centerV * cosR + 0.5f;
            }
            
            float noise = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;
            
            // Combine multiple octaves
            for (int octave = 0; octave < 3; octave++) {
                int div = divisions[octave];
                
                // Find the grid cell this pixel belongs to
                float gridU = scaledU * div;
                float gridV = scaledV * div;
                
                int gridX = (int)gridU;
                int gridY = (int)gridV;
                
                // Get fractional part for interpolation
                float fracU = gridU - gridX;
                float fracV = gridV - gridY;
                
                // Wrap grid coordinates
                gridX = gridX % div;
                gridY = gridY % div;
                if (gridX < 0) gridX += div;
                if (gridY < 0) gridY += div;
                
                float minDist = FLT_MAX;
                float secondMinDist = FLT_MAX;
                
                // Check neighboring cells for closest feature points
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int checkX = (gridX + dx + div) % div;
                        int checkY = (gridY + dy + div) % div;
                        int pointIndex = checkY * div + checkX;
                        
                        Vec2 featurePoint = featurePoints[octave][pointIndex];
                        
                        // Calculate distance to feature point
                        float pointU = (gridX + dx + featurePoint.x) / div;
                        float pointV = (gridY + dy + featurePoint.y) / div;
                        
                        float dist = sqrt((scaledU - pointU) * (scaledU - pointU) + (scaledV - pointV) * (scaledV - pointV));
                        
                        if (dist < minDist) {
                            secondMinDist = minDist;
                            minDist = dist;
                        } else if (dist < secondMinDist) {
                            secondMinDist = dist;
                        }
                    }
                }
                
                // Use F1-F2 combination if enabled
                if (m_worleySettings.useF1F2) {
                    float f1 = minDist;
                    float f2 = secondMinDist;
                    float combined = f1 * m_worleySettings.f1Weight + f2 * m_worleySettings.f2Weight;
                    noise += combined * amplitude;
                } else {
                    noise += minDist * amplitude;
                }
                
                amplitude *= m_worleySettings.persistence;
                frequency *= 2.0f;
            }
            
            // Apply shape and detail resolution scaling
            noise *= (m_worleySettings.shapeResolution / 100.0f);
            if (m_worleySettings.detailResolution > 0.0f) {
                noise += (m_worleySettings.detailResolution / 100.0f) * 0.1f;
            }
            
            // Normalize noise value
            noise = std::max(0.0f, std::min(1.0f, noise));
            
            // Apply inversion if enabled
            if (m_worleySettings.invert) {
                noise = 1.0f - noise;
            }
            
            // Apply viewer settings
            if (m_worleySettings.viewerEnabled) {
                if (m_worleySettings.viewerGreyscale) {
                    // Convert to grayscale
                    BYTE noiseValue = (BYTE)(noise * 255);
                    pixels[index] = noiseValue;     // R
                    pixels[index + 1] = noiseValue; // G
                    pixels[index + 2] = noiseValue; // B
                    pixels[index + 3] = 255;        // A
                } else {
                    // Color-coded output
                    if (m_worleySettings.viewerShowAllChannels) {
                        pixels[index] = (BYTE)(noise * 255);           // R
                        pixels[index + 1] = (BYTE)(noise * 255);     // G
                        pixels[index + 2] = (BYTE)(noise * 255);     // B
                        pixels[index + 3] = 255;                     // A
                    } else {
                        // Single channel output
                        BYTE noiseValue = (BYTE)(noise * 255);
                        pixels[index] = noiseValue;     // R
                        pixels[index + 1] = 0;          // G
                        pixels[index + 2] = 0;         // B
                        pixels[index + 3] = 255;       // A
                    }
                }
            } else {
                // Default grayscale output
                BYTE noiseValue = (BYTE)(noise * 255);
                pixels[index] = noiseValue;     // R
                pixels[index + 1] = noiseValue; // G
                pixels[index + 2] = noiseValue; // B
                pixels[index + 3] = 255;        // A
            }
        }
    }
    
    // Create texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = size;
    texDesc.Height = size;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = pixels.get();
    data.SysMemPitch = size * 4;
    
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = d3dDevice->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());
    if (FAILED(hr)) return;
    
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    hr = d3dDevice->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
    if (FAILED(hr)) return;
    
    m_worleyTexture = std::make_shared<Texture2D>(srv);
    m_worleyTextureNeedsUpdate = false;
    
    std::cout << "Worley noise texture generated successfully (size: " << size << "x" << size << ")" << std::endl;
}
