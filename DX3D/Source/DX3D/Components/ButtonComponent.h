#pragma once
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/Input.h>
#include <functional>
#include <memory>
#include <string>

namespace dx3d {

    enum class ButtonState {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };

    class ButtonComponent {
    public:
        // Constructor with default button styling
        ButtonComponent(GraphicsDevice& device,
            const std::wstring& text,
            float fontSize,
            float paddingX = 10.0f,
            float paddingY = 6.0f);

        // Constructor with custom textures
        ButtonComponent(GraphicsDevice& device,
            const std::wstring& normalTexture,
            const std::wstring& hoveredTexture,
            const std::wstring& pressedTexture,
            const std::wstring& text = L"Button",
            float width = 200.0f,
            float height = 50.0f);

        ~ButtonComponent() = default;
        void setPadding(float paddingX, float paddingY);
        // Position and transform methods
        void setPosition(float x, float y, float z = 0.0f);
        void setPosition(const Vec3& pos);
        void setPosition(const Vec2& pos);
        void setScreenPosition(float x, float y);
        void enableScreenSpace(bool enable = true);

        Vec3 getPosition() const;
        Vec2 getScreenPosition() const;
        bool isScreenSpace() const;

        // Size methods
        void setSize(float width, float height);
        float getWidth() const { return m_width; }
        float getHeight() const { return m_height; }

        // Text methods
        void setText(const std::wstring& text);
        void setFontFamily(const std::wstring& fontFamily);
        void setFontSize(float fontSize);
        void setTextColor(const Vec4& color);
        std::wstring getText() const;

        // Button state and styling
        void setState(ButtonState state);
        ButtonState getState() const { return m_currentState; }

        void setNormalTint(const Vec4& tint) { m_normalTint = tint; }
        void setHoveredTint(const Vec4& tint) { m_hoveredTint = tint; }
        void setPressedTint(const Vec4& tint) { m_pressedTint = tint; }
        void setDisabledTint(const Vec4& tint) { m_disabledTint = tint; }

        // Custom textures
        void setNormalTexture(const std::wstring& texturePath);
        void setHoveredTexture(const std::wstring& texturePath);
        void setPressedTexture(const std::wstring& texturePath);
        void setDisabledTexture(const std::wstring& texturePath);

        // Button functionality
        void setOnClickCallback(std::function<void()> callback) { m_onClickCallback = callback; }
        void setEnabled(bool enabled);
        bool isEnabled() const { return m_enabled; }

        // Update and rendering
        void update(float dt);
        void draw(DeviceContext& ctx);

        // Utility
        bool isPointInside(const Vec2& point) const;
        bool isVisible() const { return m_visible; }
        void setVisible(bool visible) { m_visible = visible; }

    private:
        GraphicsDevice& m_device;
        std::unique_ptr<SpriteComponent> m_sprite;
        std::unique_ptr<TextComponent> m_text;

        // Button properties
        float m_width = 0.0f;
        float m_height = 0.0f;
        float m_fontSize = 16.0f;  // default font size
        float m_paddingX = 10.0f;  // default horizontal padding
        float m_paddingY = 6.0f;   // default vertical padding
        bool m_enabled = true;
        bool m_visible = true;
        bool m_useScreenSpace = false;
        Vec2 m_screenPosition = { 0.0f, 0.0f };

        // Button states and styling
        ButtonState m_currentState = ButtonState::Normal;
        Vec4 m_normalTint = { 0.8f, 0.8f, 0.8f, 1.0f };      // Light gray
        Vec4 m_hoveredTint = { 1.0f, 1.0f, 1.0f, 1.0f };     // White
        Vec4 m_pressedTint = { 0.6f, 0.6f, 0.6f, 1.0f };     // Dark gray
        Vec4 m_disabledTint = { 0.5f, 0.5f, 0.5f, 0.5f };    // Translucent gray

        // Custom textures (optional)
        std::shared_ptr<Texture2D> m_normalTexture;
        std::shared_ptr<Texture2D> m_hoveredTexture;
        std::shared_ptr<Texture2D> m_pressedTexture;
        std::shared_ptr<Texture2D> m_disabledTexture;

        // Input tracking
        bool m_wasPressed = false;

        // Callback
        std::function<void()> m_onClickCallback;

        // Private helper methods
        void initialize();
        void updateVisualState();
        void handleInput();
        Vec2 getButtonBounds() const;
    };
}