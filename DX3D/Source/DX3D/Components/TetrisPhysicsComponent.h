#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    class TetrisPhysicsComponent  {
    public:
        TetrisPhysicsComponent(Vec2 position, bool isStatic = false)
            : m_position(position)
            , m_velocity(0.0f, 0.0f)
            , m_acceleration(0.0f, 0.0f)
            , m_angularVelocity(0.0f)
            , m_torque(0.0f)
            , m_mass(1.0f)
            , m_friction(0.8f)
            , m_restitution(0.2f)
            , m_isStatic(isStatic)
            , m_grounded(false)
        {
        }

        void update(float dt) {
            if (m_isStatic) return;

            // Apply gravity
            m_acceleration.y += GRAVITY * dt;

            // Apply accumulated forces
            m_acceleration += m_forces / m_mass;

            // Update velocity with damping
            m_velocity += m_acceleration * dt;
            m_velocity *= (1.0f - m_friction * dt);

            // Update angular velocity
            m_angularVelocity += m_torque / m_mass * dt;
            m_angularVelocity *= (1.0f - m_friction * dt);

            // Update position
            m_position += m_velocity * dt;

            // Simple boundary collision (you can expand this)
            checkBoundaryCollisions();

            // Clear forces for next frame
            m_forces = Vec2(0.0f, 0.0f);
            m_torque = 0.0f;
            m_acceleration.y -= AIR_FRICTION;
            m_acceleration.y = clamp(m_acceleration.y, 0,100);
            m_acceleration.x = 0;

        }

        // Force and movement methods
        void addForce(Vec2 force) { m_forces += force; }
        void addTorque(float torque) { m_torque += torque; }
        void setVelocity(Vec2 velocity) { m_velocity = velocity; }
        void setPosition(Vec2 position) { m_position = position; }

        // Getters
        Vec2 getPosition() const { return m_position; }
        Vec2 getVelocity() const { return m_velocity; }
        float getAngularVelocity() const { return m_angularVelocity; }
        bool isGrounded() const { return m_grounded; }
        bool isStatic() const { return m_isStatic; }

        // Setters
        void setMass(float mass) { m_mass = mass; }
        void setFriction(float friction) { m_friction = friction; }
        void setRestitution(float restitution) { m_restitution = restitution; }
        void setGrounded(bool grounded) { m_grounded = grounded; }

    private:
        static constexpr float GRAVITY = 980.0f; // pixels per second squared
        static constexpr float GRID_WIDTH_PIXELS = 320.0f; // 10 * 32
        static constexpr float GRID_HEIGHT_PIXELS = 640.0f; // 20 * 32
        static constexpr float AIR_FRICTION = -50.0F;
        Vec2 m_position;
        Vec2 m_velocity;
        Vec2 m_acceleration;
        Vec2 m_forces;

        float m_angularVelocity;
        float m_torque;

        float m_mass;
        float m_friction;
        float m_restitution;

        bool m_isStatic;
        bool m_grounded;

        void checkBoundaryCollisions() {
            // Left wall
            if (m_position.x < -GRID_WIDTH_PIXELS / 2.0f + 16.0f) {
                m_position.x = -GRID_WIDTH_PIXELS / 2.0f + 16.0f;
                m_velocity.x = -m_velocity.x * m_restitution;
            }

            // Right wall
            if (m_position.x > GRID_WIDTH_PIXELS / 2.0f - 16.0f) {
                m_position.x = GRID_WIDTH_PIXELS / 2.0f - 16.0f;
                m_velocity.x = -m_velocity.x * m_restitution;
            }

            // Floor
            if (m_position.y > GRID_HEIGHT_PIXELS / 2.0f - 16.0f) {
                m_position.y = GRID_HEIGHT_PIXELS / 2.0f - 16.0f;
                m_velocity.y = -m_velocity.y * m_restitution;
                m_grounded = true;

                // Reduce velocity when hitting ground
                if (abs(m_velocity.y) < 50.0f) {
                    m_velocity.y = 0.0f;
                }
            }
            else {
                m_grounded = false;
            }
        }
    };
    struct SpringConstraint {
        TetrisPhysicsComponent* blockA;
        TetrisPhysicsComponent* blockB;
        float restLength;
        float springConstant;
        float dampingFactor;
        bool isActive;

        SpringConstraint(TetrisPhysicsComponent* a, TetrisPhysicsComponent* b, 
                        float length, float k = 1000.0f, float damping = 0.9f)
            : blockA(a)
            , blockB(b)
            , restLength(length)
            , springConstant(k)
            , dampingFactor(damping)
            , isActive(true)
        {
        }

        void updateSpring(float dt) {
            if (!isActive || !blockA || !blockB) return;
            if (blockA->isStatic() && blockB->isStatic()) return;

            Vec2 posA = blockA->getPosition();
            Vec2 posB = blockB->getPosition();
            
            // Calculate spring vector
            Vec2 springVector = posB - posA;
            float currentLength = springVector.length();
            
            // Avoid division by zero
            if (currentLength < 0.001f) return;
            
            Vec2 springDirection = springVector / currentLength;
            
            // Calculate spring force (Hooke's law)
            float extension = currentLength - restLength;
            float springForceMagnitude = -springConstant * extension;
            
            // Calculate damping force
            Vec2 velA = blockA->getVelocity();
            Vec2 velB = blockB->getVelocity();
            Vec2 relativeVelocity = velB - velA;
            float dampingForceMagnitude = -dampingFactor * Vec2::dot(relativeVelocity, springDirection);
            
            // Total force
            float totalForceMagnitude = springForceMagnitude + dampingForceMagnitude;
            Vec2 force = springDirection * totalForceMagnitude;
            
            // Apply forces (Newton's 3rd law)
            if (!blockA->isStatic()) {
                blockA->addForce(force);
            }
            if (!blockB->isStatic()) {
                blockB->addForce(-force);
            }
            
            // Optional: Add angular stabilization to prevent excessive rotation
            float angularDamping = 0.1f;
            if (!blockA->isStatic()) {
                blockA->addTorque(-blockA->getAngularVelocity() * angularDamping);
            }
            if (!blockB->isStatic()) {
                blockB->addTorque(-blockB->getAngularVelocity() * angularDamping);
            }
        }
        
        void setActive(bool active) { isActive = active; }
        bool getActive() const { return isActive; }
        
        // Break spring if stretched too far
        bool shouldBreak(float breakThreshold = 3.0f) const {
            if (!blockA || !blockB) return true;
            
            Vec2 posA = blockA->getPosition();
            Vec2 posB = blockB->getPosition();
            float currentLength = (posB - posA).length();
            
            return currentLength > restLength * breakThreshold;
        }
    };
}