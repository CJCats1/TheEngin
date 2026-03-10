#pragma once
#include <TheEngine/Core/Export.h>
#include <TheEngine/Math/Types.h>
#include <algorithm>

namespace TheEngine {
    // Minimal 2D rigid body component for fluid scene boundaries and interactive ball.
    // Used by FLIP/SPH scenes when the engine is built as DLL (component lives in app).
    class FirmGuyComponent {
    public:
        void setCircle(float radius) {
            m_isCircle = true;
            m_radius = radius;
            m_halfExtents = Vec2(radius, radius);
        }
        void setRectangle(const Vec2& halfExtents) {
            m_isCircle = false;
            m_halfExtents = halfExtents;
            m_radius = std::max(halfExtents.x, halfExtents.y);
        }
        void setPosition(const Vec2& pos) { m_position = pos; }
        void setVelocity(const Vec2& vel) { m_velocity = vel; }
        void setAngle(float angle) { m_angle = angle; }
        void setStatic(bool s) { m_static = s; }
        void setRestitution(float r) { m_restitution = r; }
        void setFriction(float f) { m_friction = f; }
        void setMass(float m) { m_mass = m; }

        Vec2 getPosition() const { return m_position; }
        Vec2 getVelocity() const { return m_velocity; }
        float getRadius() const { return m_radius; }
        float getMass() const { return m_mass; }
        bool isStatic() const { return m_static; }
        bool isCircle() const { return m_isCircle; }
        const Vec2& getHalfExtents() const { return m_halfExtents; }
        float getAngle() const { return m_angle; }

    private:
        Vec2 m_position{ 0.0f, 0.0f };
        Vec2 m_velocity{ 0.0f, 0.0f };
        float m_radius = 1.0f;
        Vec2 m_halfExtents{ 1.0f, 1.0f };
        float m_angle = 0.0f;
        float m_mass = 1.0f;
        float m_restitution = 0.5f;
        float m_friction = 0.9f;
        bool m_static = false;
        bool m_isCircle = false;
    };
}
