#include <TheEngine/Game/Scenes/TestScenes/TestScene.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <TheEngine/Graphics/Camera.h>
#include <TheEngine/Graphics/LineRenderer.h>
#include <TheEngine/Graphics/SpriteComponent.h>
#include <TheEngine/Graphics/Texture2D.h>
#if defined(_WIN32) || defined(THEENGINE_PLATFORM_ANDROID)
#include <TheEngine/Graphics/DirectWriteText.h>
#endif
#if defined(THEENGINE_PLATFORM_ANDROID)
#include <android/log.h>
#endif
#include <TheEngine/Physics/PhysicsSystem.h>
#include <TheEngine/Physics/PhysicsSystem.inl>
#include <TheEngine/Core/Input.h>
#include <TheEngine/Core/Entity.h>
#include <TheEngine/Core/EntityManager.h>
#include <imgui.h>
#include <cmath>
#include <sstream>

using namespace TheEngine;
using namespace TheEngine::physics;

void TestScene::load(GraphicsEngine& engine) {
    // Initialize Entity manager
    m_entityManager = std::make_unique<EntityManager>();
    m_graphicsDevice = &engine.getGraphicsDevice();

    // Create camera entity
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
#if defined(THEENGINE_PLATFORM_ANDROID)
    camera.setZoom(3.0f); // Higher zoom for Android to make objects more visible
#else
    camera.setZoom(1.0f);
#endif

    auto& device = engine.getGraphicsDevice();

    m_nodeTexture = Texture2D::LoadTexture2D(device, L"TheEngine/Assets/Textures/node.png");
    if (!m_nodeTexture) {
        m_nodeTexture = Texture2D::CreateDebugTexture(device);
    }

    auto& lineEntity = m_entityManager->createEntity("DebugLines");
    m_lineRenderer = &lineEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->enableScreenSpace(false);
    m_lineRenderer->enableLocalPositioning(false);
    m_lineRenderer->setPosition(0.0f, 0.0f);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->setCamera(&camera);
    if (auto* linePipeline = engine.getLinePipeline()) {
        m_lineRenderer->setLinePipeline(linePipeline);
    }

    m_backgroundTime = 0.0f;
    m_backgroundTexture = Texture2D::LoadTexture2D(device, L"TheEngine/Assets/Textures/cat.jpg");
    #if defined(THEENGINE_PLATFORM_ANDROID)
    if (m_backgroundTexture) {
        const auto textureId = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(m_backgroundTexture->getNativeView()));
        __android_log_print(ANDROID_LOG_INFO, "TestScene", "Background texture loaded successfully, ID: %u", textureId);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "TestScene", "Failed to load cat.jpg!");
    }
    #endif
    if (!m_backgroundTexture) {
        m_backgroundTexture = Texture2D::CreateDebugTexture(device);
        #if defined(THEENGINE_PLATFORM_ANDROID)
        const auto debugTextureId = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(m_backgroundTexture->getNativeView()));
        __android_log_print(ANDROID_LOG_WARN, "TestScene", "Using debug texture instead, ID: %u", debugTextureId);
        #endif
    }
    float backgroundWidth = screenWidth * 0.7f;
    float backgroundHeight = screenHeight * 0.7f;
    m_backgroundEntity = &m_entityManager->createEntity("BackgroundSprite");
    #if defined(THEENGINE_PLATFORM_ANDROID)
    const auto textureIdBeforeSprite = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(m_backgroundTexture->getNativeView()));
    __android_log_print(ANDROID_LOG_INFO, "TestScene", "Creating background sprite with texture ID: %u", textureIdBeforeSprite);
    #endif
    auto& backgroundSprite = m_backgroundEntity->addComponent<SpriteComponent>(device, m_backgroundTexture,
        backgroundWidth, backgroundHeight);
    backgroundSprite.setPosition(0.0f, 0.0f, 0.0f);
    // Tint alpha of 0.0 means use texture color, 1.0 means use tint color (white)
    // RGB doesn't matter when alpha is 0, but we set it to white as a neutral value
    backgroundSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // Alpha 0 = use texture, not tint
    createOriginNodeSprite();

#if defined(_WIN32)
    if (!TextSystem::isInitialized()) {
        TextSystem::initialize(device);
    }
    if (TextSystem::isInitialized()) {
        m_screenText = std::make_unique<TextComponent>(device, TextSystem::getRenderer(),
            L"Text Demo", 18.0f);
        m_screenText->setScreenPosition(0.09f, 0.93f);
        m_screenText->setColor(Vec4(0.9f, 0.95f, 1.0f, 1.0f));

        m_worldText = std::make_unique<TextComponent>(device, TextSystem::getRenderer(),
            L"Player", 16.0f);
        m_worldText->setColor(Vec4(1.0f, 0.85f, 0.2f, 1.0f));
        m_worldText->setPosition(m_playerBox.pos.x, m_playerBox.pos.y + 14.0f, 0.0f);
    }
#endif
#if defined(THEENGINE_PLATFORM_ANDROID)
    // On Android, initialize TextSystem for ImGui fallback
    if (!TextSystem::isInitialized()) {
        TextSystem::initialize(device);
    }
    if (TextSystem::isInitialized()) {
        m_screenText = std::make_unique<TextComponent>(device, TextSystem::getRenderer(),
            L"Text Demo", 18.0f * 2.5f); // Scaled font size for Android
        m_screenText->setScreenPosition(0.02f, 0.98f); // Top-left corner
        m_screenText->setColor(Vec4(0.9f, 0.95f, 1.0f, 1.0f));

        m_worldText = std::make_unique<TextComponent>(device, TextSystem::getRenderer(),
            L"Player", 16.0f * 2.5f); // Scaled font size for Android
        m_worldText->setColor(Vec4(1.0f, 0.85f, 0.2f, 1.0f));
        m_worldText->setPosition(m_playerBox.pos.x, m_playerBox.pos.y + 14.0f, 0.0f);
    }
#endif

    // Initialize player AABB (movable)
    m_playerBox = AABB(Vec2(0.0f, 0.0f), Vec2(5.0f, 5.0f)); // center at origin, 10x10 box
    m_playerVelocity = Vec2(0.0f, 0.0f);

    m_beamTexture = Texture2D::LoadTexture2D(device, L"TheEngine/Assets/Textures/beam.png");
    if (!m_beamTexture) {
        m_beamTexture = Texture2D::CreateDebugTexture(device);
    }

    // Create player sprite - exact size of AABB, colored green
    Vec2 playerSize = m_playerBox.half * 2.0f;
    m_playerEntity = &m_entityManager->createEntity("PlayerAABB");
    auto& playerSprite = m_playerEntity->addComponent<SpriteComponent>(device, m_beamTexture,
        playerSize.x, playerSize.y);
    playerSprite.setPosition(m_playerBox.pos.x, m_playerBox.pos.y, 0.0f);
    playerSprite.setTint(Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    createOriginNodeSprite();

    // Create static obstacle AABBs
    m_staticBoxes.clear();
    m_staticBoxes.push_back(AABB(Vec2(150.0f, 0.0f), Vec2(8.0f, 8.0f)));      // Right obstacle
    m_staticBoxes.push_back(AABB(Vec2(-150.0f, 0.0f), Vec2(8.0f, 8.0f)));     // Left obstacle
    m_staticBoxes.push_back(AABB(Vec2(0.0f, 150.0f), Vec2(8.0f, 8.0f)));      // Top obstacle
    m_staticBoxes.push_back(AABB(Vec2(0.0f, -150.0f), Vec2(8.0f, 8.0f)));    // Bottom obstacle
    m_staticBoxes.push_back(AABB(Vec2(100.0f, 100.0f), Vec2(6.0f, 6.0f)));   // Diagonal obstacle
    m_staticBoxes.push_back(AABB(Vec2(-100.0f, -100.0f), Vec2(6.0f, 6.0f))); // Diagonal obstacle
    
    // Add long static platform at the bottom
    m_staticBoxes.push_back(AABB(Vec2(0.0f, -200.0f), Vec2(100.0f, 5.0f))); // Long platform at bottom

    // Create sprites for static boxes - exact size of each AABB, colored red
    m_staticBoxEntities.clear();
    for (size_t i = 0; i < m_staticBoxes.size(); ++i) {
        const auto& box = m_staticBoxes[i];
        Vec2 boxSize = box.half * 2.0f;
        auto& entity = m_entityManager->createEntity("StaticAABB_" + std::to_string(i));
        auto& sprite = entity.addComponent<SpriteComponent>(device, m_beamTexture, boxSize.x, boxSize.y);
        sprite.setPosition(box.pos.x, box.pos.y, 0.0f);
        sprite.setTint(Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        m_staticBoxEntities.push_back(&entity);
        createOriginNodeSprite();
    }

    // Create dynamic AABBs with gravity (spawned at top, will fall)
    m_dynamicBoxes.clear();
    m_dynamicVelocities.clear();
    m_dynamicMasses.clear();
    m_dynamicFrictions.clear();
    m_dynamicRestitutions.clear();
    m_dynamicBoxes.push_back(AABB(Vec2(-50.0f, 100.0f), Vec2(5.0f, 5.0f)));   // Dynamic box 1
    m_dynamicBoxes.push_back(AABB(Vec2(0.0f, 120.0f), Vec2(6.0f, 6.0f)));    // Dynamic box 2
    m_dynamicBoxes.push_back(AABB(Vec2(50.0f, 110.0f), Vec2(4.0f, 4.0f)));   // Dynamic box 3
    m_dynamicBoxes.push_back(AABB(Vec2(-30.0f, 130.0f), Vec2(7.0f, 7.0f)));  // Dynamic box 4
    m_dynamicBoxes.push_back(AABB(Vec2(30.0f, 125.0f), Vec2(5.5f, 5.5f)));   // Dynamic box 5

    // Initialize velocities to zero (gravity will accelerate them)
    for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
        m_dynamicVelocities.push_back(Vec2(0.0f, 0.0f));
    }

    // Initialize masses (heavier boxes are harder to push)
    m_dynamicMasses = { 6.0f, 8.0f, 5.0f, 10.0f, 7.0f };
    if (m_dynamicMasses.size() != m_dynamicBoxes.size()) {
        m_dynamicMasses.resize(m_dynamicBoxes.size(), 6.0f);
    }

    // Initialize frictions for dynamics (lower = more slippery)
    m_dynamicFrictions = { 0.15f, 0.2f, 0.12f, 0.25f, 0.18f };
    if (m_dynamicFrictions.size() != m_dynamicBoxes.size()) {
        m_dynamicFrictions.resize(m_dynamicBoxes.size(), 0.18f);
    }

    // Initialize restitutions for dynamics (lower = less bouncy)
    m_dynamicRestitutions = { 0.05f, 0.1f, 0.08f, 0.12f, 0.06f };
    if (m_dynamicRestitutions.size() != m_dynamicBoxes.size()) {
        m_dynamicRestitutions.resize(m_dynamicBoxes.size(), 0.08f);
    }

    // Create sprites for dynamic boxes - exact size of each AABB, colored blue
    m_dynamicBoxEntities.clear();
    for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
        const auto& box = m_dynamicBoxes[i];
        Vec2 boxSize = box.half * 2.0f;
        auto& entity = m_entityManager->createEntity("DynamicAABB_" + std::to_string(i));
        auto& sprite = entity.addComponent<SpriteComponent>(device, m_beamTexture, boxSize.x, boxSize.y);
        sprite.setPosition(box.pos.x, box.pos.y, 0.0f);
        sprite.setTint(Vec4(0.0f, 0.5f, 1.0f, 1.0f));
        m_dynamicBoxEntities.push_back(&entity);
        createOriginNodeSprite();
    }

    // Initialize a couple of circles
    m_circles.clear();
    m_circles.push_back({ Vec2(-80.0f, 80.0f), 8.0f, Vec2(0.0f, 0.0f), 6.0f, 0.18f, 0.1f });
    m_circles.push_back({ Vec2(80.0f, 90.0f), 10.0f, Vec2(0.0f, 0.0f), 8.0f, 0.2f, 0.12f });
    m_circleTexture = Texture2D::LoadTexture2D(device, L"TheEngine/Assets/Textures/node.png");
    if (!m_circleTexture) {
        m_circleTexture = Texture2D::CreateDebugTexture(device);
    }
    m_circleEntities.clear();
    for (size_t i = 0; i < m_circles.size(); ++i) {
        const auto& circle = m_circles[i];
        auto& entity = m_entityManager->createEntity("Circle_" + std::to_string(i));
        auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_circleTexture,
            circle.radius * 2.0f, circle.radius * 2.0f);
        sprite.setPosition(circle.pos.x, circle.pos.y, 0.0f);
        sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        m_circleEntities.push_back(&entity);
        createOriginNodeSprite();
    }
}

void TestScene::reset(GraphicsEngine& engine) {
    m_spawnMode = SpawnMode::Aabb;
    m_springDynamicActive = false;
    m_springDynamicIndex = -1;
    m_springCircleActive = false;
    m_springCircleIndex = -1;
    m_springTarget = Vec2(0.0f, 0.0f);
    load(engine);
}

void TestScene::update(float dt) {
    // Update camera movement
    updateCameraMovement(dt);
    
    // Update player movement
    updatePlayerMovement(dt);

    if (m_backgroundEntity) {
        if (auto* sprite = m_backgroundEntity->getComponent<SpriteComponent>()) {
            m_backgroundTime += dt;
            float scale = 1.0f + 0.05f * std::sin(m_backgroundTime * 1.3f);
            float rotation = 0.15f * std::sin(m_backgroundTime * 0.7f);
            sprite->setScale2D(scale);
            sprite->setRotationZ(rotation);
        }
    }

#if defined(_WIN32) || defined(THEENGINE_PLATFORM_ANDROID)
    if (m_showTextDemo && (m_screenText || m_worldText)) {
        m_textUpdateTimer += dt;
        if (m_textUpdateTimer >= m_textUpdateInterval) {
            m_textUpdateTimer = 0.0f;
            if (m_screenText) {
                std::wostringstream ss;
                ss.setf(std::ios::fixed);
                ss.precision(1);
                ss << L"Text Demo\n";
                ss << L"Player (" << m_playerBox.pos.x << L", " << m_playerBox.pos.y << L")\n";
                ss << L"Vel (" << m_playerVelocity.x << L", " << m_playerVelocity.y << L")";
                m_screenText->setText(ss.str());
            }
            if (m_worldText) {
                m_worldText->setText(L"Player");
#if defined(THEENGINE_PLATFORM_ANDROID)
                // On Android, update world text position accounting for camera
                auto* cameraEntity = m_entityManager->findEntity("MainCamera");
                auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
                if (camera) {
                    // Transform world position to screen space for ImGui fallback
                    Vec2 worldPos(m_playerBox.pos.x, m_playerBox.pos.y + 14.0f);
                    Vec2 screenPos = camera->worldToScreen(worldPos);
                    // Convert to normalized coordinates for setScreenPosition
                    float screenWidth = GraphicsEngine::getWindowWidth();
                    float screenHeight = GraphicsEngine::getWindowHeight();
                    float normX = screenPos.x / screenWidth;
                    float normY = 1.0f - (screenPos.y / screenHeight); // Invert Y for normalized coords
                    m_worldText->setScreenPosition(normX, normY);
                } else {
                    m_worldText->setPosition(m_playerBox.pos.x, m_playerBox.pos.y + 14.0f, 0.0f);
                }
#else
                m_worldText->setPosition(m_playerBox.pos.x, m_playerBox.pos.y + 14.0f, 0.0f);
#endif
            }
        }
    }
#endif
}

void TestScene::fixedUpdate(float dt) {
    // Spring update - affects velocities, physics system handles collisions
    if (m_springDynamicActive && m_springDynamicIndex >= 0
        && static_cast<size_t>(m_springDynamicIndex) < m_dynamicBoxes.size()) {
        size_t idx = static_cast<size_t>(m_springDynamicIndex);
        Vec2 displacement = m_springTarget - m_dynamicBoxes[idx].pos;
        Vec2 acceleration = displacement * m_springK - m_dynamicVelocities[idx] * m_springDamping;
        m_dynamicVelocities[idx] += acceleration * dt;
    }
    if (m_springCircleActive && m_springCircleIndex >= 0
        && static_cast<size_t>(m_springCircleIndex) < m_circles.size()) {
        size_t idx = static_cast<size_t>(m_springCircleIndex);
        Vec2 displacement = m_springTarget - m_circles[idx].pos;
        Vec2 acceleration = displacement * m_springK - m_circles[idx].velocity * m_springDamping;
        m_circles[idx].velocity += acceleration * dt;
    }

    TheEngine::physics::WorldStepConfig config{
        m_playerBox,
        m_playerVelocity,
        m_staticBoxes,
        m_dynamicBoxes,
        m_dynamicVelocities,
        m_dynamicMasses,
        m_dynamicFrictions,
        m_dynamicRestitutions,
        m_circles,
        m_gravity,
        m_staticFriction,
        m_staticRestitution,
        m_playerFrictionStatic,
        m_playerFrictionDynamic,
        m_playerRestitution,
        m_playerPushMass
    };
    TheEngine::physics::PhysicsSystem::stepWorld(config, dt);

#if defined(THEENGINE_PLATFORM_ANDROID)
    // On Android, update spring target position while dragging (spring effect handles the rest)
    if (m_springDynamicActive || m_springCircleActive) {
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            auto& input = Input::getInstance();
            auto* cameraEntity = m_entityManager->findEntity("MainCamera");
            auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
            if (camera && input.isMouseDown(MouseClick::LeftMouse)) {
                Vec2 touchScreen = input.getMousePositionClient();
                m_springTarget = camera->screenToWorld(touchScreen);
            }
        }
    }
#endif

    if (m_lineRenderer) {
        m_lineRenderer->clear();
        if (m_showDebugLines) {
            const float velocityScale = 0.08f;
            const float normalScale = 12.0f;
            const float contactThreshold = 1.5f;
            const Vec4 velocityColor(0.0f, 0.0f, 0.0f, 1.0f);
            const Vec4 normalColor(0.0f, 0.0f, 0.0f, 1.0f);

            auto addVelocity = [&](const Vec2& pos, const Vec2& vel) {
                Vec2 end = pos + vel * velocityScale;
                m_lineRenderer->addLine(pos, end, velocityColor, 1.5f);
            };

            auto computeContactNormal = [&](const TheEngine::physics::AABB& a,
                                            const TheEngine::physics::AABB& b,
                                            Vec2& outNormal,
                                            Vec2& outPoint) -> bool {
                float dx = b.pos.x - a.pos.x;
                float px = (b.half.x + a.half.x) - std::abs(dx);
                float dy = b.pos.y - a.pos.y;
                float py = (b.half.y + a.half.y) - std::abs(dy);
                if (px < -contactThreshold || py < -contactThreshold) {
                    return false;
                }
                if (px < py) {
                    float sx = (dx < 0.0f) ? -1.0f : 1.0f;
                    outNormal = Vec2(sx, 0.0f);
                    outPoint = Vec2(a.pos.x + a.half.x * sx, b.pos.y);
                } else {
                    float sy = (dy < 0.0f) ? -1.0f : 1.0f;
                    outNormal = Vec2(0.0f, sy);
                    outPoint = Vec2(b.pos.x, a.pos.y + a.half.y * sy);
                }
                return true;
            };

            auto addNormal = [&](const Vec2& point, const Vec2& normal) {
                m_lineRenderer->addLine(point, point + normal * normalScale, normalColor, 2.0f);
            };

            addVelocity(m_playerBox.pos, m_playerVelocity);
            for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
                addVelocity(m_dynamicBoxes[i].pos, m_dynamicVelocities[i]);
            }
            for (const auto& circle : m_circles) {
                addVelocity(circle.pos, circle.velocity);
            }

            for (const auto& staticBox : m_staticBoxes) {
                Vec2 normal;
                Vec2 point;
                if (computeContactNormal(m_playerBox, staticBox, normal, point)) {
                    addNormal(point, normal);
                }
            }
            for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
                for (const auto& staticBox : m_staticBoxes) {
                    Vec2 normal;
                    Vec2 point;
                    if (computeContactNormal(m_dynamicBoxes[i], staticBox, normal, point)) {
                        addNormal(point, normal);
                    }
                }
            }
            for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
                for (size_t j = i + 1; j < m_dynamicBoxes.size(); ++j) {
                    Vec2 normal;
                    Vec2 point;
                    if (computeContactNormal(m_dynamicBoxes[i], m_dynamicBoxes[j], normal, point)) {
                        addNormal(point, normal);
                    }
                }
            }
            for (const auto& staticBox : m_staticBoxes) {
                for (const auto& circle : m_circles) {
                    auto hit = staticBox.intersectCircle(circle.pos, circle.radius);
                    if (hit) {
                        addNormal(hit->pos, hit->normal);
                    }
                }
            }
        }
    }

    if (m_playerEntity) {
        if (auto* sprite = m_playerEntity->getComponent<SpriteComponent>()) {
            sprite->setPosition(m_playerBox.pos.x, m_playerBox.pos.y, 0.0f);
        }
    }
    
    for (size_t i = 0; i < m_dynamicBoxes.size() && i < m_dynamicBoxEntities.size(); ++i) {
        if (auto* sprite = m_dynamicBoxEntities[i]->getComponent<SpriteComponent>()) {
            sprite->setPosition(m_dynamicBoxes[i].pos.x, m_dynamicBoxes[i].pos.y, 0.0f);
        }
    }

    for (size_t i = 0; i < m_circles.size() && i < m_circleEntities.size(); ++i) {
        if (auto* sprite = m_circleEntities[i]->getComponent<SpriteComponent>()) {
            sprite->setPosition(m_circles[i].pos.x, m_circles[i].pos.y, 0.0f);
        }
    }
}

void TestScene::updateCameraMovement(float dt) {
    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    if (!cameraEntity) return;

    auto* camera = cameraEntity->getComponent<Camera2D>();
    if (!camera) return;

    auto& input = Input::getInstance();

    float baseSpeed = 300.0f;
    float fastSpeed = 600.0f;
    float zoomSpeed = 2.0f;

    float currentSpeed = input.isKeyDown(Key::Shift) ? fastSpeed : baseSpeed;

    Vec2 moveDelta(0.0f, 0.0f);
    if (input.isKeyDown(Key::W)) moveDelta.y += currentSpeed * dt;
    if (input.isKeyDown(Key::S)) moveDelta.y -= currentSpeed * dt;
    if (input.isKeyDown(Key::A)) moveDelta.x -= currentSpeed * dt;
    if (input.isKeyDown(Key::D)) moveDelta.x += currentSpeed * dt;

    if (moveDelta.x != 0.0f || moveDelta.y != 0.0f) {
        camera->move(moveDelta);
    }

    float zoomDelta = 0.0f;
    if (input.isKeyDown(Key::Q)) zoomDelta -= zoomSpeed * dt;
    if (input.isKeyDown(Key::E)) zoomDelta += zoomSpeed * dt;

    if (zoomDelta != 0.0f) {
        camera->zoom(zoomDelta);
    }

    if (input.wasKeyJustPressed(Key::Space)) {
        camera->setPosition(0.0f, 0.0f);
#if defined(THEENGINE_PLATFORM_ANDROID)
        camera->setZoom(3.0f); // Reset to Android zoom level
#else
        camera->setZoom(1.0f);
#endif
    }
}

void TestScene::render(GraphicsEngine& engine, IRenderSwapChain& swapChain) {
    auto& ctx = engine.getContext();
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    // ---------- PASS 1: world-space sprites using default pipeline ----------
    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());
    // set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera")) {
        if (auto* camera = cameraEntity->getComponent<Camera2D>()) {
            // Update camera screen size if window was resized
            camera->setScreenSize(screenWidth, screenHeight);
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
            if (m_lineRenderer) {
                m_lineRenderer->setCamera(camera);
            }
        }
    }

    // Render background sprite first
    if (m_backgroundEntity) {
        if (auto* sprite = m_backgroundEntity->getComponent<SpriteComponent>()) {
            sprite->draw(ctx);
        }
    }

    // Render AABB sprites
    if (m_playerEntity) {
        if (auto* sprite = m_playerEntity->getComponent<SpriteComponent>()) {
            sprite->draw(ctx);
        }
    }
    for (size_t i = 0; i < m_staticBoxEntities.size(); ++i) {
        if (auto* sprite = m_staticBoxEntities[i]->getComponent<SpriteComponent>()) {
            sprite->draw(ctx);
        }
    }
    for (size_t i = 0; i < m_dynamicBoxEntities.size(); ++i) {
        if (auto* sprite = m_dynamicBoxEntities[i]->getComponent<SpriteComponent>()) {
            sprite->draw(ctx);
        }
    }

    // Render circles using sprites (alpha-friendly)
    for (size_t i = 0; i < m_circleEntities.size(); ++i) {
        if (auto* sprite = m_circleEntities[i]->getComponent<SpriteComponent>()) {
            sprite->draw(ctx);
        }
    }

#if defined(_WIN32) || defined(THEENGINE_PLATFORM_ANDROID)
    if (m_showTextDemo) {
        if (m_worldText) {
            m_worldText->draw(ctx);
        }
        if (m_screenText) {
            m_screenText->draw(ctx);
        }
    }
#endif

    if (m_lineRenderer && m_showDebugLines) {
        m_lineRenderer->draw(ctx);
    }
    
    // frame begin/end handled centrally
    // Note: renderImGui is called by Game::onInternalUpdate(), not here
}

void TestScene::updatePlayerMovement(float dt) {
    auto& input = Input::getInstance();
    
#if defined(THEENGINE_PLATFORM_ANDROID)
    // Android-specific touch controls
    // Skip if ImGui wants to capture mouse input (e.g., clicking on ImGui window)
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        auto* cameraEntity = m_entityManager->findEntity("MainCamera");
        auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
        
        if (camera) {
            Vec2 touchScreen = input.getMousePositionClient();
            Vec2 worldPos = camera->screenToWorld(touchScreen);
            
            if (m_androidMode == AndroidInteractionMode::Grab) {
                // Grab mode: drag objects with finger using spring effect (like desktop)
                if (input.isMouseDown(MouseClick::LeftMouse)) {
                    m_springTarget = worldPos;
                    
                    if (!m_springDynamicActive && !m_springCircleActive) {
                        // Try to grab a circle first
                        m_springCircleIndex = -1;
                        for (size_t i = 0; i < m_circles.size(); ++i) {
                            Vec2 toTouch = worldPos - m_circles[i].pos;
                            float distSq = toTouch.x * toTouch.x + toTouch.y * toTouch.y;
                            float radius = m_circles[i].radius;
                            if (distSq <= radius * radius) {
                                m_springCircleIndex = static_cast<int>(i);
                                m_springCircleActive = true;
                                #if defined(THEENGINE_PLATFORM_ANDROID)
                                __android_log_print(ANDROID_LOG_INFO, "TestScene", "Grabbed circle %d", m_springCircleIndex);
                                #endif
                                break;
                            }
                        }
                        
                        // If no circle, try to grab a dynamic box
                        if (!m_springCircleActive) {
                            m_springDynamicIndex = -1;
                            for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
                                if (m_dynamicBoxes[i].intersectPoint(worldPos)) {
                                    m_springDynamicIndex = static_cast<int>(i);
                                    m_springDynamicActive = true;
                                    #if defined(THEENGINE_PLATFORM_ANDROID)
                                    __android_log_print(ANDROID_LOG_INFO, "TestScene", "Grabbed box %d", m_springDynamicIndex);
                                    #endif
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    // Touch released - disable spring
                    if (m_springDynamicActive || m_springCircleActive) {
                        #if defined(THEENGINE_PLATFORM_ANDROID)
                        __android_log_print(ANDROID_LOG_INFO, "TestScene", "Released grab");
                        #endif
                    }
                    m_springDynamicActive = false;
                    m_springDynamicIndex = -1;
                    m_springCircleActive = false;
                    m_springCircleIndex = -1;
                }
        } else {
            // Spawn mode: tap to spawn
            if (input.wasMouseJustPressed(MouseClick::LeftMouse) && m_graphicsDevice) {
                if (m_spawnMode == SpawnMode::Circle) {
                    CircleBody circle;
                    circle.pos = worldPos;
                    circle.radius = 8.0f;
                    circle.velocity = Vec2(0.0f, 0.0f);
                    circle.mass = 6.0f;
                    circle.friction = 0.18f;
                    circle.restitution = 0.1f;
                    m_circles.push_back(circle);
                    auto& entity = m_entityManager->createEntity("Circle_spawn_" + std::to_string(m_circleEntities.size()));
                    auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_circleTexture,
                        circle.radius * 2.0f, circle.radius * 2.0f);
                    sprite.setPosition(circle.pos.x, circle.pos.y, 0.0f);
                    sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    m_circleEntities.push_back(&entity);
                    createOriginNodeSprite();
                } else {
                    Vec2 halfSize(5.0f, 5.0f);
                    m_dynamicBoxes.push_back(TheEngine::physics::AABB(worldPos, halfSize));
                    m_dynamicVelocities.push_back(Vec2(0.0f, 0.0f));
                    m_dynamicMasses.push_back(6.0f);
                    m_dynamicFrictions.push_back(0.18f);
                    m_dynamicRestitutions.push_back(0.08f);
                    
                    Vec2 boxSize = halfSize * 2.0f;
                    auto& entity = m_entityManager->createEntity("DynamicAABB_spawn_" + std::to_string(m_dynamicBoxEntities.size()));
                    auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_beamTexture,
                        boxSize.x, boxSize.y);
                    sprite.setPosition(worldPos.x, worldPos.y, 0.0f);
                    sprite.setTint(Vec4(0.0f, 0.5f, 1.0f, 1.0f));
                    m_dynamicBoxEntities.push_back(&entity);
                    createOriginNodeSprite();
                }
            }
        }
    }
    }
    
    // Don't update player movement on Android
    m_playerVelocity = Vec2(0.0f, 0.0f);
    
#else
    // Desktop controls (original code)
    float moveSpeed = 200.0f;
    Vec2 moveInput(0.0f, 0.0f);
    
    // Arrow keys for player movement (WASD is for camera)
    if (input.isKeyDown(Key::Up)) {
        moveInput.y += moveSpeed;
    }
    if (input.isKeyDown(Key::Down)) {
        moveInput.y -= moveSpeed;
    }
    if (input.isKeyDown(Key::Left)) {
        moveInput.x -= moveSpeed;
    }
    if (input.isKeyDown(Key::Right)) {
        moveInput.x += moveSpeed;
    }
    
    // Spring move dynamic AABB or circle toward mouse when LMB is held
    if (input.isMouseDown(MouseClick::LeftMouse)) {
        auto* cameraEntity = m_entityManager->findEntity("MainCamera");
        auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
        if (camera) {
            Vec2 mouseScreen = input.getMousePositionClient();
            m_springTarget = camera->screenToWorld(mouseScreen);

            if (!m_springDynamicActive && !m_springCircleActive) {
                m_springCircleIndex = -1;
                for (size_t i = 0; i < m_circles.size(); ++i) {
                    Vec2 toMouse = m_springTarget - m_circles[i].pos;
                    float distSq = toMouse.x * toMouse.x + toMouse.y * toMouse.y;
                    float radius = m_circles[i].radius;
                    if (distSq <= radius * radius) {
                        m_springCircleIndex = static_cast<int>(i);
                        break;
                    }
                }
                m_springCircleActive = (m_springCircleIndex >= 0);
            }

            if (!m_springCircleActive && !m_springDynamicActive) {
                m_springDynamicIndex = -1;
                for (size_t i = 0; i < m_dynamicBoxes.size(); ++i) {
                    if (m_dynamicBoxes[i].intersectPoint(m_springTarget)) {
                        m_springDynamicIndex = static_cast<int>(i);
                        break;
                    }
                }
                m_springDynamicActive = (m_springDynamicIndex >= 0);
            }
        }
    } else {
        m_springDynamicActive = false;
        m_springDynamicIndex = -1;
        m_springCircleActive = false;
        m_springCircleIndex = -1;
    }

    // Update player velocity from keys
    m_playerVelocity = moveInput;

    if (input.wasMouseJustPressed(MouseClick::RightMouse)) {
        auto* cameraEntity = m_entityManager->findEntity("MainCamera");
        auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
        if (camera && m_graphicsDevice) {
            Vec2 mouseScreen = input.getMousePositionClient();
            Vec2 worldPos = camera->screenToWorld(mouseScreen);
            if (m_spawnMode == SpawnMode::Circle) {
                CircleBody circle;
                circle.pos = worldPos;
                circle.radius = 8.0f;
                circle.velocity = Vec2(0.0f, 0.0f);
                circle.mass = 6.0f;
                circle.friction = 0.18f;
                circle.restitution = 0.1f;
                m_circles.push_back(circle);
                auto& entity = m_entityManager->createEntity("Circle_spawn_" + std::to_string(m_circleEntities.size()));
                auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_circleTexture,
                    circle.radius * 2.0f, circle.radius * 2.0f);
                sprite.setPosition(circle.pos.x, circle.pos.y, 0.0f);
                sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 1.0f));
                m_circleEntities.push_back(&entity);
                createOriginNodeSprite();
            } else {
                Vec2 halfSize(5.0f, 5.0f);

                m_dynamicBoxes.push_back(TheEngine::physics::AABB(worldPos, halfSize));
                m_dynamicVelocities.push_back(Vec2(0.0f, 0.0f));
                m_dynamicMasses.push_back(6.0f);
                m_dynamicFrictions.push_back(0.18f);
                m_dynamicRestitutions.push_back(0.08f);

                Vec2 boxSize = halfSize * 2.0f;
                auto& entity = m_entityManager->createEntity("DynamicAABB_spawn_" + std::to_string(m_dynamicBoxEntities.size()));
                auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_beamTexture,
                    boxSize.x, boxSize.y);
                sprite.setPosition(worldPos.x, worldPos.y, 0.0f);
                sprite.setTint(Vec4(0.0f, 0.5f, 1.0f, 1.0f));
                m_dynamicBoxEntities.push_back(&entity);
                createOriginNodeSprite();
            }
        }
    }
    
    // Reset player position with R key
    if (input.wasKeyJustPressed(Key::R)) {
        m_playerBox.pos = Vec2(0.0f, 0.0f);
        m_playerVelocity = Vec2(0.0f, 0.0f);
    }
#endif
}


void TestScene::renderImGui(GraphicsEngine& engine) {
    ImGui::Begin("Test Scene");
    ImGui::Text("Test Scene - AABB Physics Demo");
    if (ImGui::Button("Reset Scene")) {
        reset(engine);
    }
    ImGui::Separator();
    
#if defined(THEENGINE_PLATFORM_ANDROID)
    // Android-specific controls
    const char* modeLabel = (m_androidMode == AndroidInteractionMode::Grab) ? "Mode: Grab" : "Mode: Spawn";
    if (ImGui::Button(modeLabel)) {
        m_androidMode = (m_androidMode == AndroidInteractionMode::Grab) ? AndroidInteractionMode::Spawn : AndroidInteractionMode::Grab;
    }
    ImGui::Separator();
    const char* spawnLabel = (m_spawnMode == SpawnMode::Circle) ? "Type: Circle" : "Type: AABB";
    if (ImGui::Button(spawnLabel)) {
        m_spawnMode = (m_spawnMode == SpawnMode::Aabb) ? SpawnMode::Circle : SpawnMode::Aabb;
    }
    ImGui::Text("Grab: Drag objects");
    ImGui::Text("Spawn: Tap to create");
#else
    // Desktop controls
    const char* spawnLabel = (m_spawnMode == SpawnMode::Circle) ? "Spawn: Circle" : "Spawn: AABB";
    if (ImGui::Button(spawnLabel)) {
        m_spawnMode = (m_spawnMode == SpawnMode::Aabb) ? SpawnMode::Circle : SpawnMode::Aabb;
    }
    ImGui::SameLine();
    ImGui::Text("RMB to spawn");
#endif
    ImGui::Separator();
    const char* debugLabel = m_showDebugLines ? "Debug Lines: On" : "Debug Lines: Off";
    if (ImGui::Button(debugLabel)) {
        m_showDebugLines = !m_showDebugLines;
    }
    ImGui::Separator();
    const char* textLabel = m_showTextDemo ? "Text Demo: On" : "Text Demo: Off";
    if (ImGui::Button(textLabel)) {
        m_showTextDemo = !m_showTextDemo;
    }
    ImGui::Separator();
    const char* backendLabel = (engine.getBackendType() == RenderBackendType::OpenGL) ? "OpenGL" : "DirectX11";
    ImGui::Text("Backend: %s", backendLabel);
    ImGui::Separator();
#if !defined(THEENGINE_PLATFORM_ANDROID)
    ImGui::Text("Camera Controls:");
    ImGui::Text("WASD - Move camera");
    ImGui::Text("Q/E - Zoom");
    ImGui::Text("Space - Reset camera");
    ImGui::Separator();
    ImGui::Text("Player Controls:");
    ImGui::Text("Arrow Keys - Move player box");
    ImGui::Text("R - Reset player position");
    ImGui::Separator();
#endif
    ImGui::Text("Player Position: (%.1f, %.1f)", m_playerBox.pos.x, m_playerBox.pos.y);
    ImGui::Text("Player Velocity: (%.1f, %.1f)", m_playerVelocity.x, m_playerVelocity.y);
    ImGui::Text("Static Obstacles: %d", (int)m_staticBoxes.size());
    ImGui::Text("Dynamic Objects: %d", (int)m_dynamicBoxes.size());
    ImGui::Text("Gravity: %.1f", m_gravity);
    ImGui::End();
}

void TestScene::createOriginNodeSprite() {
    if (!m_entityManager || !m_graphicsDevice) return;
    if (!m_nodeTexture) {
        m_nodeTexture = Texture2D::CreateDebugTexture(*m_graphicsDevice);
    }
    auto& entity = m_entityManager->createEntity("DebugNode_" + std::to_string(m_debugNodeCounter++));
    auto& sprite = entity.addComponent<SpriteComponent>(*m_graphicsDevice, m_nodeTexture, 1.0f, 1.0f);
    sprite.setPosition(0.0f, 0.0f, 0.0f);
    sprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f));
}
