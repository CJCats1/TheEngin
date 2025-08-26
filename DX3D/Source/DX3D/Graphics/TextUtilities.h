#pragma once
#include "DirectWriteText.h"
#include <vector>
#include <functional>

namespace dx3d {

    // Text alignment options
    enum class TextAlignment {
        Left,
        Center,
        Right,
        Justify
    };

    enum class VerticalAlignment {
        Top,
        Middle,
        Bottom
    };

    // Advanced text component with alignment, word wrapping, etc.
    class AdvancedTextComponent : public TextComponent {
    public:
        AdvancedTextComponent(GraphicsDevice& device, DirectWriteRenderer& textRenderer,
            const std::wstring& text = L"", float fontSize = 24.0f);

        // Alignment
        void setTextAlignment(TextAlignment alignment);
        TextAlignment getTextAlignment() const { return m_textAlignment; }

        void setVerticalAlignment(VerticalAlignment alignment);
        VerticalAlignment getVerticalAlignment() const { return m_verticalAlignment; }

        // Word wrapping
        void setWordWrapping(bool enable);
        bool getWordWrapping() const { return m_wordWrapping; }

        // Padding
        void setPadding(float left, float top, float right, float bottom);
        void setPadding(float uniform) { setPadding(uniform, uniform, uniform, uniform); }

        // Background
        void setBackgroundColor(const Vec4& color);
        void setBackgroundVisible(bool visible);
        const Vec4& getBackgroundColor() const { return m_backgroundColor; }
        bool isBackgroundVisible() const { return m_backgroundVisible; }

        // Border
        void setBorderColor(const Vec4& color);
        void setBorderWidth(float width);
        void setBorderVisible(bool visible);

        // Override draw to include background and alignment
        void draw(DeviceContext& ctx) const override;

    private:
        TextAlignment m_textAlignment;
        VerticalAlignment m_verticalAlignment;
        bool m_wordWrapping;

        // Padding: left, top, right, bottom
        float m_padding[4];

        // Background
        Vec4 m_backgroundColor;
        bool m_backgroundVisible;
        mutable std::shared_ptr<class Mesh> m_backgroundMesh;
        mutable std::shared_ptr<Texture2D> m_backgroundTexture;

        // Border
        Vec4 m_borderColor;
        float m_borderWidth;
        bool m_borderVisible;
        mutable std::shared_ptr<Mesh> m_borderMesh;
        mutable std::shared_ptr<Texture2D> m_borderTexture;

        void rebuildBackground() const;
        Vec2 calculateAlignmentOffset() const;
    };

    // Text animation system
    class AnimatedTextComponent : public TextComponent {
    public:
        AnimatedTextComponent(GraphicsDevice& device, DirectWriteRenderer& textRenderer,
            const std::wstring& text = L"", float fontSize = 24.0f);

        // Typewriter effect
        void startTypewriter(float charactersPerSecond = 20.0f);
        void stopTypewriter();
        bool isTypewriterActive() const { return m_typewriterActive; }

        // Fade in/out
        void fadeIn(float duration = 1.0f);
        void fadeOut(float duration = 1.0f);

        // Pulse effect
        void startPulse(float minAlpha = 0.3f, float maxAlpha = 1.0f, float speed = 2.0f);
        void stopPulse();

        // Color transition
        void transitionColor(const Vec4& targetColor, float duration = 1.0f);

        // Scale animation
        void animateScale(float targetScale, float duration = 1.0f);

        // Update animations
        void updateAnimations(float dt);

        // Override draw to apply animation effects
        void draw(DeviceContext& ctx) const override;

    private:
        // Typewriter effect
        bool m_typewriterActive;
        float m_typewriterSpeed;
        float m_typewriterProgress;
        std::wstring m_fullText;
        std::wstring m_displayedText;

        // Fade animation
        bool m_fadeActive;
        float m_fadeDuration;
        float m_fadeProgress;
        float m_fadeStartAlpha;
        float m_fadeTargetAlpha;

        // Pulse animation
        bool m_pulseActive;
        float m_pulseMinAlpha;
        float m_pulseMaxAlpha;
        float m_pulseSpeed;
        float m_pulseTime;

        // Color transition
        bool m_colorTransitionActive;
        float m_colorTransitionDuration;
        float m_colorTransitionProgress;
        Vec4 m_colorTransitionStart;
        Vec4 m_colorTransitionTarget;

        // Scale animation
        bool m_scaleAnimationActive;
        float m_scaleAnimationDuration;
        float m_scaleAnimationProgress;
        float m_scaleAnimationStart;
        float m_scaleAnimationTarget;

        void updateTypewriter(float dt);
        void updateFade(float dt);
        void updatePulse(float dt);
        void updateColorTransition(float dt);
        void updateScaleAnimation(float dt);
    };

    // Text input component for simple text input
    class TextInputComponent : public AdvancedTextComponent {
    public:
        TextInputComponent(GraphicsDevice& device, DirectWriteRenderer& textRenderer,
            const std::wstring& placeholder = L"Enter text...", float fontSize = 24.0f);

        // Input handling
        void handleCharacterInput(wchar_t character);
        void handleKeyInput(int keyCode); // For backspace, delete, etc.

        // Cursor
        void setCursorVisible(bool visible);
        bool isCursorVisible() const { return m_cursorVisible; }

        void setCursorColor(const Vec4& color);
        const Vec4& getCursorColor() const { return m_cursorColor; }

        // Focus
        void setFocused(bool focused);
        bool isFocused() const { return m_focused; }

        // Placeholder
        void setPlaceholderText(const std::wstring& placeholder);
        const std::wstring& getPlaceholderText() const { return m_placeholderText; }

        // Callbacks
        void setOnTextChanged(std::function<void(const std::wstring&)> callback);
        void setOnEnterPressed(std::function<void(const std::wstring&)> callback);

        // Update cursor blinking
        void updateInput(float dt);

        // Override draw to include cursor
        void draw(DeviceContext& ctx) const override;

    private:
        std::wstring m_placeholderText;
        bool m_focused;
        bool m_cursorVisible;
        Vec4 m_cursorColor;
        float m_cursorBlinkTime;
        bool m_cursorBlink;
        int m_cursorPosition;

        std::function<void(const std::wstring&)> m_onTextChanged;
        std::function<void(const std::wstring&)> m_onEnterPressed;

        mutable std::shared_ptr<Mesh> m_cursorMesh;
        mutable std::shared_ptr<Texture2D> m_cursorTexture;

        void rebuildCursor() const;
        void insertCharacter(wchar_t character);
        void deleteCharacter();
        void moveCursor(int delta);
    };

    // Utility functions
    namespace TextUtils {
        // Convert between string types
        std::wstring stringToWString(const std::string& str);
        std::string wstringToString(const std::wstring& wstr);

        // Text formatting
        std::wstring formatText(const wchar_t* format, ...);
        std::wstring formatNumber(float value, int decimalPlaces = 2);
        std::wstring formatTime(float seconds); // Format as MM:SS or HH:MM:SS

        // Text measurement utilities
        Vec2 measureWrappedText(DirectWriteRenderer& renderer, const std::wstring& text,
            float maxWidth, const std::wstring& fontFamily = L"Arial",
            float fontSize = 24.0f);

        // Text effects
        std::wstring addTextShadow(const std::wstring& text); // For multiple text components
        std::wstring addTextOutline(const std::wstring& text);
    }
}