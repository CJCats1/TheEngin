#include <DX3D/Components/SunComponent.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <iostream>

namespace dx3d
{
    SunComponent::SunComponent() {
        // Initialize with default values
    }

    SunComponent::~SunComponent() {
        // Cleanup handled by EntityManager
    }

    void SunComponent::setBaseName(const std::string& name) {
        m_baseName = name;
    }

    void SunComponent::setPosition(const Vec3& position) {
        m_position = position;
        updateLightDirection();
    }

    void SunComponent::setRadius(float radius) {
        m_radius = radius;
    }

    void SunComponent::setColor(const Vec3& color) {
        m_color = color;
    }

    void SunComponent::setVisible(bool visible) {
        m_visible = visible;
        if (m_coreSprite) m_coreSprite->setVisible(visible);
        if (m_bloomSprite) m_bloomSprite->setVisible(visible);
    }

    void SunComponent::setLightEnabled(bool enabled) {
        m_lightEnabled = enabled;
    }

    void SunComponent::setLightIntensity(float intensity) {
        m_lightIntensity = intensity;
    }

    void SunComponent::setLightColor(const Vec3& color) {
        m_lightColor = color;
    }

    void SunComponent::setLightTarget(const Vec3& target) {
        m_lightTarget = target;
        updateLightDirection();
    }

    void SunComponent::setLightShadows(bool shadows) {
        m_lightShadows = shadows;
    }

    void SunComponent::setLightOrthoSize(float size) {
        m_lightOrthoSize = size;
    }

    void SunComponent::setLightNearPlane(float nearPlane) {
        m_lightNearPlane = nearPlane;
    }

    void SunComponent::setLightFarPlane(float farPlane) {
        m_lightFarPlane = farPlane;
    }

    void SunComponent::setCoreScale(float scale) {
        m_coreScale = scale;
    }

    void SunComponent::setBloomScale(float scale) {
        m_bloomScale = scale;
    }

    void SunComponent::createSprites(GraphicsDevice& device, EntityManager& entityManager, 
                                     const std::wstring& nodePath, const std::wstring& bloomPath) {
        auto nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), nodePath.c_str());
        auto bloomTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), bloomPath.c_str());

        if (!nodeTexture || !bloomTexture) {
            std::cout << "Warning: Could not load sun textures for " << m_baseName << std::endl;
            return;
        }

        // Bloom entity (renders first)
        auto& bloomEntity = entityManager.createEntity(m_baseName + "Bloom");
        m_bloomEntity = &bloomEntity;
        float bloomSize = static_cast<float>(m_radius * m_bloomScale);
        auto& bloomSprite = bloomEntity.addComponent<SpriteComponent>(device, bloomTexture, bloomSize, bloomSize);
        bloomSprite.setPosition(m_position);
        bloomSprite.setVisible(m_visible);
        bloomSprite.setTint(Vec4(m_color.x, m_color.y, m_color.z, 0.8f));
        bloomSprite.enableScreenSpace(false);
        m_bloomSprite = &bloomSprite;

        // Core entity (renders second)
        auto& coreEntity = entityManager.createEntity(m_baseName + "Core");
        m_coreEntity = &coreEntity;
        float coreSize = static_cast<float>(m_radius * m_coreScale);
        auto& coreSprite = coreEntity.addComponent<SpriteComponent>(device, nodeTexture, coreSize, coreSize);
        coreSprite.setPosition(m_position);
        coreSprite.setVisible(m_visible);
        coreSprite.setTint(Vec4(m_color.x, m_color.y, m_color.z, 1.0f));
        coreSprite.enableScreenSpace(false);
        m_coreSprite = &coreSprite;
    }

    void SunComponent::updateVisuals(float pulse, const Vec3& colorVariation) {
        // Use the integrated light intensity from the sun
        float lightIntensity = m_lightIntensity;
        
        // Scale the base radius based on light intensity
        // Higher intensity = bigger sun, but with some reasonable limits
        float intensityRadiusMultiplier = 0.5f + (lightIntensity * 0.5f); // Range: 0.5x to 1.5x
        float adjustedRadius = m_radius * intensityRadiusMultiplier;
        
        // Scale core and bloom according to adjusted radius, pulse, and additional intensity scaling
        float intensityScale = std::max(0.1f, lightIntensity); // Minimum scale to prevent invisible suns
        float core = adjustedRadius * m_coreScale * pulse * intensityScale;
        float bloom = adjustedRadius * m_bloomScale * pulse * intensityScale;
        
        if (m_coreSprite) m_coreSprite->setScale(Vec3(core, core, 1.0f));
        if (m_bloomSprite) m_bloomSprite->setScale(Vec3(bloom, bloom, 1.0f));

        // Apply color variation
        Vec3 color = colorVariation;
        if (m_coreSprite) m_coreSprite->setTint(Vec4(color.x, color.y, color.z, 1.0f));
        if (m_bloomSprite) m_bloomSprite->setTint(Vec4(color.x, color.y, color.z, 0.8f));
    }

    void SunComponent::updateLightDirection() {
        m_lightDirection = (m_lightTarget - m_position).normalized();
    }
}