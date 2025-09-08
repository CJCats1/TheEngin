#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <iostream>
#include <DX3D/Graphics/GraphicsEngine.h>

using namespace dx3d;

ButtonComponent::ButtonComponent(GraphicsDevice& device,
    const std::wstring& text,
    float fontSize,
    float paddingX,
    float paddingY)
    : m_device(device), m_fontSize(fontSize), m_paddingX(paddingX), m_paddingY(paddingY)
{
    if (!TextSystem::isInitialized()) {
        TextSystem::initialize(device);
    }

    m_text = std::make_unique<TextComponent>(
        device,
        TextSystem::getRenderer(),
        text,
        fontSize
    );

    m_text->setFontFamily(L"Arial");
    m_text->setColor(Vec4(0, 0, 0, 1));

    Vec2 textSize = m_text->getTextSize();
    m_width = textSize.x + 2.0f * m_paddingX;
    m_height = textSize.y + 2.0f * m_paddingY;

    // Create sprite component
    m_sprite = std::make_unique<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/beam.png",
        m_width,
        m_height
    );

    initialize();
}


void ButtonComponent::setPadding(float paddingX, float paddingY) {
    m_paddingX = paddingX;
    m_paddingY = paddingY;

    // Recompute size
    Vec2 textSize = m_text->getTextSize();
    m_width = textSize.x + 2.0f * m_paddingX;
    m_height = textSize.y + 2.0f * m_paddingY;

    m_sprite->setScale(m_width, m_height);

    // Re-center text
    setPosition(getPosition());
}

ButtonComponent::ButtonComponent(GraphicsDevice& device,
    const std::wstring& normalTexture,
    const std::wstring& hoveredTexture,
    const std::wstring& pressedTexture,
    const std::wstring& text,
    float width,
    float height)
    : m_device(device), m_width(width), m_height(height) {

    // Create sprite component with normal texture
    m_sprite = std::make_unique<SpriteComponent>(device, normalTexture, width, height);

    // Load custom textures
    m_normalTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), normalTexture.c_str());
    m_hoveredTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), hoveredTexture.c_str());
    m_pressedTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), pressedTexture.c_str());

    // Ensure text system is initialized
    if (!TextSystem::isInitialized()) {
        TextSystem::initialize(device);
    }

    // Create text component
    m_text = std::make_unique<TextComponent>(
        device,
        TextSystem::getRenderer(),
        text,
        16.0f
    );
    initialize();
}

void ButtonComponent::initialize() {
    // Set default text properties
    m_text->setFontFamily(L"Arial");
    m_text->setColor(Vec4(0.0f, 0.0f, 0.0f, 1.0f)); // Black text

    // Center text on button
    Vec2 textSize = m_text->getTextSize();
    float textOffsetX = (m_width - textSize.x) * 0.5f;
    float textOffsetY = (m_height - textSize.y) * 0.5f;

    // Apply initial visual state
    updateVisualState();
}

void ButtonComponent::setPosition(float x, float y, float z) {
    setPosition(Vec3(x, y, z));
}

void ButtonComponent::setPosition(const Vec3& pos) {
    m_sprite->setPosition(pos);

    // Update text position to stay centered on button
    Vec2 textSize = m_text->getTextSize();
    float textOffsetX = (m_width - textSize.x) * 0.5f;
    float textOffsetY = (m_height - textSize.y) * 0.5f;

    if (m_useScreenSpace) {
        m_text->setScreenPosition(
            m_screenPosition.x + textOffsetX / GraphicsEngine::getWindowWidth(),
            m_screenPosition.y + textOffsetY / GraphicsEngine::getWindowHeight()
        );
    }
    else {
        m_text->setPosition(pos.x + textOffsetX, pos.y + textOffsetY, pos.z + 0.1f);
    }
}

void ButtonComponent::setPosition(const Vec2& pos) {
    setPosition(Vec3(pos.x, pos.y, 0.0f));
}

void ButtonComponent::setScreenPosition(float x, float y) {
    m_useScreenSpace = true;
    m_screenPosition = Vec2(x, y);

    // Convert normalized coordinates to screen space
    float screenX = x;
    float screenY = y;

    m_sprite->setScreenPosition(screenX, screenY);
    m_sprite->enableScreenSpace(true);

    // Center text on button in screen space
    Vec2 textSize = m_text->getTextSize();
    float textOffsetX = (m_width - textSize.x) * 0.5f;
    float textOffsetY = (m_height - textSize.y) * 0.5f;

    float winW = (float)GraphicsEngine::getWindowWidth();
    float winH = (float)GraphicsEngine::getWindowHeight();

    m_text->setScreenPosition(
        screenX + (textOffsetX / winW),
        screenY + (textOffsetY / winH)
    );
}

void ButtonComponent::enableScreenSpace(bool enable) {
    m_useScreenSpace = enable;
    m_sprite->enableScreenSpace(enable);
}

Vec3 ButtonComponent::getPosition() const {
    return m_sprite->getPosition();
}

Vec2 ButtonComponent::getScreenPosition() const {
    return m_sprite->getScreenPosition();
}

bool ButtonComponent::isScreenSpace() const {
    return m_useScreenSpace;
}

void ButtonComponent::setSize(float width, float height) {
    m_width = width;
    m_height = height;

    // Update sprite size - you'll need to recreate the mesh or add a resize method
    // For now, we'll assume the SpriteComponent handles this internally
}

void ButtonComponent::setText(const std::wstring& text) {
    m_text->setText(text);

    // Recalculate text positioning
    Vec3 currentPos = getPosition();
    setPosition(currentPos);
}

void ButtonComponent::setFontFamily(const std::wstring& fontFamily) {
    m_text->setFontFamily(fontFamily);
}

void ButtonComponent::setFontSize(float fontSize) {
    m_text->setFontSize(fontSize);

    // Recalculate text positioning
    Vec3 currentPos = getPosition();
    setPosition(currentPos);
}

void ButtonComponent::setTextColor(const Vec4& color) {
    m_text->setColor(color);
}

std::wstring ButtonComponent::getText() const {
    return m_text->getText();
}

void ButtonComponent::setState(ButtonState state) {
    if (m_currentState != state) {
        m_currentState = state;
        updateVisualState();
    }
}

void ButtonComponent::setNormalTexture(const std::wstring& texturePath) {
    m_normalTexture = Texture2D::LoadTexture2D(m_device.getD3DDevice(), texturePath.c_str());
    if (m_currentState == ButtonState::Normal) {
        updateVisualState();
    }
}

void ButtonComponent::setHoveredTexture(const std::wstring& texturePath) {
    m_hoveredTexture = Texture2D::LoadTexture2D(m_device.getD3DDevice(), texturePath.c_str());
    if (m_currentState == ButtonState::Hovered) {
        updateVisualState();
    }
}

void ButtonComponent::setPressedTexture(const std::wstring& texturePath) {
    m_pressedTexture = Texture2D::LoadTexture2D(m_device.getD3DDevice(), texturePath.c_str());
    if (m_currentState == ButtonState::Pressed) {
        updateVisualState();
    }
}

void ButtonComponent::setDisabledTexture(const std::wstring& texturePath) {
    m_disabledTexture = Texture2D::LoadTexture2D(m_device.getD3DDevice(), texturePath.c_str());
    if (m_currentState == ButtonState::Disabled) {
        updateVisualState();
    }
}

void ButtonComponent::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled && m_currentState != ButtonState::Disabled) {
        setState(ButtonState::Disabled);
    }
    else if (enabled && m_currentState == ButtonState::Disabled) {
        setState(ButtonState::Normal);
    }
}

void ButtonComponent::update(float dt) {
    if (!m_visible || !m_enabled) {
        return;
    }

    handleInput();
}

void ButtonComponent::draw(DeviceContext& ctx) {
    if (!m_visible) {
        return;
    }

    m_sprite->draw(ctx);
    m_text->draw(ctx);
}

bool ButtonComponent::isPointInside(const Vec2& point) const {
    if (m_useScreenSpace) {
        // Convert mouse position to screen space coordinates
        float winW = (float)GraphicsEngine::getWindowWidth();
        float winH = (float)GraphicsEngine::getWindowHeight();

        Vec2 centerNDC = m_screenPosition;
        float halfW_NDC = (m_width * 0.5f) / winW;
        float halfH_NDC = (m_height * 0.5f) / winH;

        float left = centerNDC.x - halfW_NDC;
        float right = centerNDC.x + halfW_NDC;
        float top = centerNDC.y - halfH_NDC;
        float bottom = centerNDC.y + halfH_NDC;

        return (point.x >= left && point.x <= right &&
            point.y >= top && point.y <= bottom);
    }
    else {
        // World space collision
        Vec3 pos = getPosition();
        float halfW = m_width * 0.5f;
        float halfH = m_height * 0.5f;

        return (point.x >= pos.x - halfW && point.x <= pos.x + halfW &&
            point.y >= pos.y - halfH && point.y <= pos.y + halfH);
    }
}



void ButtonComponent::updateVisualState() {
    Vec4 currentTint;
    std::shared_ptr<Texture2D> currentTexture = nullptr;

    switch (m_currentState) {
    case ButtonState::Normal:
        currentTint = m_normalTint;
        currentTexture = m_normalTexture;
        break;
    case ButtonState::Hovered:
        currentTint = m_hoveredTint;
        currentTexture = m_hoveredTexture ? m_hoveredTexture : m_normalTexture;
        break;
    case ButtonState::Pressed:
        currentTint = m_pressedTint;
        currentTexture = m_pressedTexture ? m_pressedTexture : m_normalTexture;
        break;
    case ButtonState::Disabled:
        currentTint = m_disabledTint;
        currentTexture = m_disabledTexture ? m_disabledTexture : m_normalTexture;
        break;
    }

    m_sprite->setTint(currentTint);

    if (currentTexture) {
        m_sprite->setTexture(currentTexture);
    }
}

void ButtonComponent::handleInput() {
    auto& input = Input::getInstance();

    Vec2 mousePos;
    if (m_useScreenSpace) {
        mousePos = input.getMousePositionNDC();
    }
    else {
        mousePos = input.getMousePosition();
        // You'll need to convert to world coordinates if needed
        // mousePos = screenToWorld(mousePos.x, mousePos.y);
    }

    bool mouseInside = isPointInside(mousePos);
    bool mousePressed = input.isMouseDown(MouseClick::LeftMouse);
    bool mouseJustReleased = input.wasMouseJustReleased(MouseClick::LeftMouse);

    if (!m_enabled) {
        setState(ButtonState::Disabled);
        return;
    }

    if (mouseInside) {
        if (mousePressed) {
            setState(ButtonState::Pressed);

            m_wasPressed = true;
        }
        else if (m_wasPressed && mouseJustReleased) {
            // Button was clicked!
            if (m_onClickCallback) {
                m_onClickCallback();
            }
            setState(ButtonState::Hovered);
            m_wasPressed = false;
        }
        else {
            setState(ButtonState::Hovered);
        }
    }
    else {
        setState(ButtonState::Normal);
        if (mouseJustReleased) {
            m_wasPressed = false;
        }
    }
}

Vec2 ButtonComponent::getButtonBounds() const {
    return Vec2(m_width, m_height);
}