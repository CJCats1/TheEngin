#pragma once
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/TransformComponent.h>
#include <DX3D/Math/Geometry.h>

#include <wrl.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

namespace dx3d {

    class DirectWriteRenderer {
    public:
        DirectWriteRenderer(GraphicsDevice& device);
        ~DirectWriteRenderer();

        bool initialize();
        void shutdown();

        // Create a text texture from the given parameters
        std::shared_ptr<Texture2D> renderTextToTexture(
            const std::wstring& text,
            const std::wstring& fontFamily = L"Arial",
            float fontSize = 24.0f,
            DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
            const Vec4& color = Vec4(1.0f, 1.0f, 1.0f, 1.0f),
            UINT32 maxWidth = 1024,
            UINT32 maxHeight = 512
        );

        // Calculate text metrics without rendering
        Vec2 measureText(
            const std::wstring& text,
            const std::wstring& fontFamily = L"Arial",
            float fontSize = 24.0f,
            DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
            UINT32 maxWidth = UINT32_MAX
        );

    private:
        GraphicsDevice& m_device;

        // DirectWrite/Direct2D interfaces
        Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
        Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
        Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;

        bool initializeDirectWrite();
        bool initializeDirect2D();
        bool initializeWIC();
    };

    class TextComponent {
    public:
        TextComponent(GraphicsDevice& device, DirectWriteRenderer& textRenderer,
            const std::wstring& text = L"", float fontSize = 24.0f);
        virtual ~TextComponent() = default;

        // Text properties
        void setText(const std::wstring& text);
        const std::wstring& getText() const { return m_text; }

        void setFontSize(float size);
        float getFontSize() const { return m_fontSize; }

        void setFontFamily(const std::wstring& fontFamily);
        const std::wstring& getFontFamily() const { return m_fontFamily; }

        void setFontWeight(DWRITE_FONT_WEIGHT weight);
        DWRITE_FONT_WEIGHT getFontWeight() const { return m_fontWeight; }

        void setFontStyle(DWRITE_FONT_STYLE style);
        DWRITE_FONT_STYLE getFontStyle() const { return m_fontStyle; }

        void setColor(const Vec4& color);
        const Vec4& getColor() const { return m_color; }

        void setMaxWidth(UINT32 maxWidth);
        UINT32 getMaxWidth() const { return m_maxWidth; }

        void setMaxHeight(UINT32 maxHeight);
        UINT32 getMaxHeight() const { return m_maxHeight; }

        // Transform access
        TransformComponent& transform() { return m_transform; }
        const TransformComponent& transform() const { return m_transform; }

        // Convenience transform methods
        void setPosition(float x, float y, float z = 0.0f) {
            m_transform.setPosition(x, y, z);
            m_useScreenSpace = false;
        }
        void setPosition(const Vec3& pos) {
            m_transform.setPosition(pos);
            m_useScreenSpace = false;
        }
        void setPosition(const Vec2& pos) {
            m_transform.setPosition(pos);
            m_useScreenSpace = false;
        }
        void setScreenPosition(float x, float y);
        void setScreenPosition(const Vec2& pos);
        Vec2 getScreenPosition() const { return m_screenPosition; }
        Vec3 getPosition() const {
            return m_useScreenSpace ? Vec3(m_screenPosition.x, m_screenPosition.y, 0.0f)
                : m_transform.getPosition();
        }
        Vec2 getPosition2D() const {
            return m_useScreenSpace ? m_screenPosition
                : m_transform.getPosition2D();
        }
        bool isScreenSpace() const { return m_useScreenSpace; }
        void setScale(float scale) { m_transform.setScale(scale); }
        void setScale(const Vec3& scale) { m_transform.setScale(scale); }

        // Utility
        Vec2 getTextSize() const;
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        // Force texture rebuild on next draw
        void markDirty() { m_needsRebuild = true; }

        // Rendering
        virtual void draw(class DeviceContext& ctx) const;
        TransformComponent m_transform;
        bool m_useScreenSpace;
        Vec2 m_screenPosition;
        // Rendered texture and sprite mesh
        mutable std::shared_ptr<Texture2D> m_textTexture;
        mutable std::shared_ptr<class Mesh> m_textMesh;
    protected:
        GraphicsDevice& m_device;
        DirectWriteRenderer& m_textRenderer;

        // Text properties
        std::wstring m_text;
        std::wstring m_fontFamily;
        float m_fontSize;
        DWRITE_FONT_WEIGHT m_fontWeight;
        DWRITE_FONT_STYLE m_fontStyle;
        Vec4 m_color;
        UINT32 m_maxWidth;
        UINT32 m_maxHeight;

        bool m_visible;
        mutable bool m_needsRebuild;

        

        

        virtual void rebuildTexture() const;
    };

    // Global text renderer instance manager
    class TextSystem {
    public:
        static void initialize(GraphicsDevice& device);
        static void shutdown();
        static DirectWriteRenderer& getRenderer();
        static bool isInitialized();

    private:
        static std::unique_ptr<DirectWriteRenderer> s_renderer;
        static bool s_initialized;
    };

    // Utility functions for string conversion
    namespace TextUtils {
        std::wstring stringToWString(const std::string& str);
        std::string wstringToString(const std::wstring& wstr);

        // Simple formatting helpers
        std::wstring formatFloat(float value, int decimalPlaces = 2);
        std::wstring formatInt(int value);
    }
}