#pragma once
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <memory>
#include <string>
#include <DX3D/Graphics/GraphicsDevice.h>

namespace dx3d {

    class PanelComponent {
    public:
        PanelComponent(GraphicsDevice& device,
            float width,
            float height,
            const std::wstring& titleText = L"",
            float fontSize = 18.0f,
            float paddingX = 8.0f,
            float paddingY = 8.0f)
            : m_device(device),
            m_width(width),
            m_height(height),
            m_paddingX(paddingX),
            m_paddingY(paddingY)
        {
            // Background sprite
            m_sprite = std::make_unique<SpriteComponent>(
                device,
                L"DX3D/Assets/Textures/beam.png", // semi-transparent panel texture
                width,
                height
            );
            m_sprite->setTint(Vec4(0.1f, 0.1f, 0.1f, 0.7f)); // dark background

            // Optional title text
            if (!titleText.empty()) {
                if (!TextSystem::isInitialized())
                    TextSystem::initialize(device);

                m_text = std::make_unique<TextComponent>(
                    device,
                    TextSystem::getRenderer(),
                    titleText,
                    fontSize
                );
                m_text->setFontFamily(L"Consolas");
                m_text->setColor(Vec4(1, 1, 1, 1)); // white text
            }
        }

        // --- Positioning ---
        void setPosition(const Vec3& pos) {
            m_sprite->setPosition(pos);
            if (m_text) {
                Vec2 textSize = m_text->getTextSize();
                float offsetX = m_paddingX;
                float offsetY = m_height - textSize.y - m_paddingY;
                m_text->setPosition(pos.x + offsetX, pos.y + offsetY, pos.z + 0.1f);
            }
        }

        void setScreenPosition(float x, float y) {
            m_useScreenSpace = true;
            m_screenPosition = Vec2(x, y);

            m_sprite->setScreenPosition(x, y);
            m_sprite->enableScreenSpace(true);

            if (m_text) {
                Vec2 textSize = m_text->getTextSize();
                float offsetX = m_paddingX / GraphicsEngine::getWindowWidth();
                float offsetY = (m_height - textSize.y - m_paddingY) / GraphicsEngine::getWindowHeight();
                m_text->setScreenPosition(x + offsetX, y + offsetY);
            }
        }
        // --- Drawing ---
        void draw(DeviceContext& ctx) {
            if (m_sprite && m_sprite->isVisible())
                m_sprite->draw(ctx);
            if (m_text && m_text->isVisible())
                m_text->draw(ctx);
        }

        // --- Styling ---
        void setTint(const Vec4& color) { if (m_sprite) m_sprite->setTint(color); }
        void setTitle(const std::wstring& text) { if (m_text) m_text->setText(text); }
        void setVisible(bool visible) {
            if (m_sprite) m_sprite->setVisible(visible);
            if (m_text) m_text->setVisible(visible);
        }

    private:
        GraphicsDevice& m_device;
        std::unique_ptr<SpriteComponent> m_sprite;
        std::unique_ptr<TextComponent> m_text;

        float m_width, m_height;
        float m_paddingX, m_paddingY;
        bool m_useScreenSpace = false;
        Vec2 m_screenPosition;
    };

} // namespace dx3d
