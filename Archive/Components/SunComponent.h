#pragma once
#include <DX3D/Core/Entity.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Math/Geometry.h>
#include <string>

namespace dx3d
{
    class GraphicsDevice;
    class Entity;

    class SunComponent {
    public:
        // Constructor
        SunComponent();
        ~SunComponent();

        // Core properties
        void setBaseName(const std::string& name);
        void setPosition(const Vec3& position);
        void setRadius(float radius);
        void setColor(const Vec3& color);
        void setVisible(bool visible);

        // Light properties
        void setLightEnabled(bool enabled);
        void setLightIntensity(float intensity);
        void setLightColor(const Vec3& color);
        void setLightTarget(const Vec3& target);
        void setLightShadows(bool shadows);
        void setLightOrthoSize(float size);
        void setLightNearPlane(float nearPlane);
        void setLightFarPlane(float farPlane);

        // Visual scaling
        void setCoreScale(float scale);
        void setBloomScale(float scale);

        // Getters
        const std::string& getBaseName() const { return m_baseName; }
        const Vec3& getPosition() const { return m_position; }
        float getRadius() const { return m_radius; }
        const Vec3& getColor() const { return m_color; }
        bool isVisible() const { return m_visible; }

        bool isLightEnabled() const { return m_lightEnabled; }
        float getLightIntensity() const { return m_lightIntensity; }
        const Vec3& getLightColor() const { return m_lightColor; }
        const Vec3& getLightDirection() const { return m_lightDirection; }
        const Vec3& getLightTarget() const { return m_lightTarget; }
        bool hasLightShadows() const { return m_lightShadows; }
        float getLightOrthoSize() const { return m_lightOrthoSize; }
        float getLightNearPlane() const { return m_lightNearPlane; }
        float getLightFarPlane() const { return m_lightFarPlane; }

        float getCoreScale() const { return m_coreScale; }
        float getBloomScale() const { return m_bloomScale; }

        // Entity and sprite access
        Entity* getCoreEntity() const { return m_coreEntity; }
        Entity* getBloomEntity() const { return m_bloomEntity; }
        SpriteComponent* getCoreSprite() const { return m_coreSprite; }
        SpriteComponent* getBloomSprite() const { return m_bloomSprite; }

        // Initialization and management
        void createSprites(GraphicsDevice& device, EntityManager& entityManager, 
                          const std::wstring& nodePath, const std::wstring& bloomPath);
        void updateVisuals(float pulse, const Vec3& colorVariation);
        void updateLightDirection();

    private:
        // Core properties
        std::string m_baseName;
        Vec3 m_position = Vec3(0.0f, 0.0f, 0.0f);
        float m_radius = 10.0f;
        Vec3 m_color = Vec3(1.0f, 0.8f, 0.4f);
        bool m_visible = true;

        // Light properties
        bool m_lightEnabled = true;
        Vec3 m_lightDirection = Vec3(0.0f, -1.0f, 0.0f);
        Vec3 m_lightColor = Vec3(1.0f, 1.0f, 1.0f);
        float m_lightIntensity = 1.0f;
        Vec3 m_lightTarget = Vec3(0.0f, 0.0f, 0.0f);
        float m_lightOrthoSize = 100.0f;
        float m_lightNearPlane = 0.1f;
        float m_lightFarPlane = 200.0f;
        bool m_lightShadows = true;

        // Visual scaling
        float m_coreScale = 1.5f;
        float m_bloomScale = 8.0f;

        // Entities and sprites
        Entity* m_coreEntity = nullptr;
        Entity* m_bloomEntity = nullptr;
        SpriteComponent* m_coreSprite = nullptr;
        SpriteComponent* m_bloomSprite = nullptr;
    };
}
