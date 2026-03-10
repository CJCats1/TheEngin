#include "StudyScene.h"

#include <TheEngine/Core/Input.h>
#include <TheEngine/Core/Entity.h>
#include <TheEngine/Graphics/SpriteComponent.h>
#include <TheEngine/Graphics/Texture2D.h>
#include <TheEngine/Graphics/Camera.h>
#include <imgui.h>

namespace TheEngine
{

void StudyScene::load(GraphicsEngine& engine)
{
    // Basic scene setup: device + entity manager
    SceneHelper::initializeScene(engine, m_device, m_entityManager);

    // Create a simple 2D camera centered at origin
    SceneHelper::createDefaultCamera2D(*m_entityManager, 1.0f, Vec2(0.0f, 0.0f));

    createBackgroundSprite(engine);
    createBoxes();
    syncBoxSprites();
}

void StudyScene::reset(GraphicsEngine& engine)
{
    // Restore original box visibility/layout
    m_boxes = m_initialBoxes;
    syncBoxSprites();
}

void StudyScene::update(float /*dt*/)
{
    handleMouseClicks();
}

void StudyScene::render(GraphicsEngine& engine, IRenderSwapChain& /*swapChain*/)
{
    auto& ctx = engine.getContext();

    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    ctx.setGraphicsPipelineState(engine.getDefaultPipeline());

    // Set camera matrices
    if (auto* cameraEntity = m_entityManager->findEntity("MainCamera"))
    {
        if (auto* camera = cameraEntity->getComponent<Camera2D>())
        {
            camera->setScreenSize(screenWidth, screenHeight);
            ctx.setViewMatrix(camera->getViewMatrix());
            ctx.setProjectionMatrix(camera->getProjectionMatrix());
        }
    }

    // Draw background first
    if (m_backgroundEntity)
    {
        if (auto* sprite = m_backgroundEntity->getComponent<SpriteComponent>())
        {
            sprite->draw(ctx);
        }
    }

    // Draw visible boxes on top
    for (size_t i = 0; i < m_boxes.size() && i < m_boxEntities.size(); ++i)
    {
        if (!m_boxes[i].visible) continue;
        if (auto* sprite = m_boxEntities[i]->getComponent<SpriteComponent>())
        {
            sprite->draw(ctx);
        }
    }
}

void StudyScene::renderImGui(GraphicsEngine& engine)
{
    ImGui::Begin("Study Helper");

    ImGui::Text("Background image:");
    ImGui::TextWrapped("%ls", m_imagePath.c_str());

    ImGui::Separator();
    ImGui::Text("Boxes remaining: %d / %d",
        static_cast<int>(std::count_if(m_boxes.begin(), m_boxes.end(),
                                       [](const Box& b) { return b.visible; })),
        static_cast<int>(m_boxes.size()));

    if (ImGui::Button("Reset Boxes"))
    {
        reset(engine);
    }

    ImGui::End();
}

void StudyScene::createBackgroundSprite(GraphicsEngine& engine)
{
    if (!m_device || !m_entityManager) return;

    auto& device = *m_device;

    m_backgroundTexture = Texture2D::LoadTexture2D(device, m_imagePath.c_str());
    if (!m_backgroundTexture)
    {
        // Fallback to a debug texture if the image is missing
        m_backgroundTexture = Texture2D::CreateDebugTexture(device);
    }

    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    float backgroundWidth = screenWidth * 0.9f;
    float backgroundHeight = screenHeight * 0.9f;

    m_backgroundEntity = &m_entityManager->createEntity("BackgroundSprite");
    auto& backgroundSprite = m_backgroundEntity->addComponent<SpriteComponent>(
        device,
        m_backgroundTexture,
        backgroundWidth,
        backgroundHeight);

    backgroundSprite.setPosition(0.0f, 0.0f, 0.0f);
    backgroundSprite.setTint(Vec4(1.0f, 1.0f, 1.0f, 0.0f)); // use texture color
}

void StudyScene::createBoxes()
{
    if (!m_device || !m_entityManager) return;

    auto& device = *m_device;

    if (!m_boxTexture)
    {
        // 1x1 white texture we can tint for colored boxes
        m_boxTexture = Texture2D::CreateDebugTexture(device);
    }

    m_boxes.clear();
    m_initialBoxes.clear();
    m_boxEntities.clear();

    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();

    float backgroundWidth = screenWidth * 0.9f;
    float backgroundHeight = screenHeight * 0.9f;

    // Define a simple 3x3 grid of boxes over the background
    const int rows = 3;
    const int cols = 3;

    float startX = -backgroundWidth * 0.4f;
    float startY = -backgroundHeight * 0.4f;
    float stepX = backgroundWidth * 0.4f;
    float stepY = backgroundHeight * 0.4f;

    float boxWidth = backgroundWidth * 0.15f;
    float boxHeight = backgroundHeight * 0.15f;
    Vec2 halfSize(boxWidth * 0.5f, boxHeight * 0.5f);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            Box box;
            box.center = Vec2(startX + c * stepX, startY + r * stepY);
            box.halfSize = halfSize;
            box.visible = true;
            m_boxes.push_back(box);
        }
    }

    m_initialBoxes = m_boxes;

    // Create sprite entities for each box
    for (size_t i = 0; i < m_boxes.size(); ++i)
    {
        const auto& box = m_boxes[i];
        auto& entity = m_entityManager->createEntity("StudyBox_" + std::to_string(i));
        auto& sprite = entity.addComponent<SpriteComponent>(
            device,
            m_boxTexture,
            box.halfSize.x * 2.0f,
            box.halfSize.y * 2.0f);

        sprite.setPosition(box.center.x, box.center.y, 0.0f);
        sprite.setTint(Vec4(0.1f, 0.6f, 1.0f, 0.8f)); // semi-transparent blue

        m_boxEntities.push_back(&entity);
    }
}

void StudyScene::syncBoxSprites()
{
    // Ensure each sprite matches its box visibility and position
    for (size_t i = 0; i < m_boxes.size() && i < m_boxEntities.size(); ++i)
    {
        auto* sprite = m_boxEntities[i]->getComponent<SpriteComponent>();
        if (!sprite) continue;

        const auto& box = m_boxes[i];
        sprite->setPosition(box.center.x, box.center.y, 0.0f);

        if (box.visible)
        {
            sprite->setTint(Vec4(0.1f, 0.6f, 1.0f, 0.8f));
        }
        else
        {
            // Hide by making it fully transparent
            sprite->setTint(Vec4(0.1f, 0.6f, 1.0f, 0.0f));
        }
    }
}

void StudyScene::handleMouseClicks()
{
    auto& input = Input::getInstance();

    if (!input.wasMouseJustPressed(MouseClick::LeftMouse))
        return;

    if (!m_entityManager) return;

    auto* cameraEntity = m_entityManager->findEntity("MainCamera");
    auto* camera = cameraEntity ? cameraEntity->getComponent<Camera2D>() : nullptr;
    if (!camera) return;

    Vec2 mouseScreen = input.getMousePositionClient();
    Vec2 worldPos = camera->screenToWorld(mouseScreen);

    for (size_t i = 0; i < m_boxes.size(); ++i)
    {
        auto& box = m_boxes[i];
        if (!box.visible) continue;

        Vec2 d = worldPos - box.center;
        if (std::abs(d.x) <= box.halfSize.x &&
            std::abs(d.y) <= box.halfSize.y)
        {
            box.visible = false;
            // Immediately update corresponding sprite tint
            if (i < m_boxEntities.size())
            {
                if (auto* sprite = m_boxEntities[i]->getComponent<SpriteComponent>())
                {
                    sprite->setTint(Vec4(0.1f, 0.6f, 1.0f, 0.0f));
                }
            }
            break; // hide only one box per click
        }
    }
}

} // namespace TheEngine
