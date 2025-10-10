#pragma once
#include <DX3D/Math/Geometry.h>
#include <memory>

namespace dx3d {
    class Physics3DComponent {
    public:
        Physics3DComponent() = default;
        
        // Physics properties
        void setPosition(const Vec3& position) { m_position = position; }
        Vec3 getPosition() const { return m_position; }
        
        void setVelocity(const Vec3& velocity) { m_velocity = velocity; }
        Vec3 getVelocity() const { return m_velocity; }
        
        void setAcceleration(const Vec3& acceleration) { m_acceleration = acceleration; }
        Vec3 getAcceleration() const { return m_acceleration; }
        
        void setMass(float mass) { m_mass = mass; }
        float getMass() const { return m_mass; }
        
        void setRadius(float radius) { m_radius = radius; }
        float getRadius() const { return m_radius; }
        
        // Physics constants
        void setFriction(float friction) { m_friction = friction; }
        float getFriction() const { return m_friction; }
        
        void setGravity(float gravity) { m_gravity = gravity; }
        float getGravity() const { return m_gravity; }
        
        void setBounce(float bounce) { m_bounce = bounce; }
        float getBounce() const { return m_bounce; }
        
        // Input forces
        void setInputForce(const Vec3& force) { m_inputForce = force; }
        Vec3 getInputForce() const { return m_inputForce; }
        
        // Physics update
        void update(float dt);
        
        // Collision detection
        bool checkCollision(const Vec3& otherPos, float otherRadius) const;
        void handleCollision(const Vec3& collisionNormal, float penetration);
        
    private:
        Vec3 m_position{ 0.0f, 0.1f, 0.0f };
        Vec3 m_velocity{ 0.0f, 0.0f, 0.0f };
        Vec3 m_acceleration{ 0.0f, 0.0f, 0.0f };
        Vec3 m_inputForce{ 0.0f, 0.0f, 0.0f };
        
        float m_mass{ 1.0f };
        float m_radius{ 0.1f };
        float m_friction{ 0.95f };
        float m_gravity{ -9.8f };
        float m_bounce{ 0.6f };
    };
}
