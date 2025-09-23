#pragma once
#include <DX3D/Math/Geometry.h>
#include <iostream>
#include "CardFrameComponent.h"

namespace dx3d {
    enum class PhysicsMode {
        NORMAL,      // Normal springy behavior
        CELEBRATION, // Enhanced physics for celebrations
        MAGNETIC,    // Magnetic attraction effects
        DRAG,        // Strong spring toward cursor while dragging
        SETTLING     // Gentle settling motion
    };

    class CardPhysicsComponent {
    public:
                CardPhysicsComponent()
            : m_velocity(0.0f, 0.0f)
            , m_targetPosition(0.0f, 0.0f)
            , m_restPosition(0.0f, 0.0f)
            , m_frame(nullptr)
            , m_isDragging(false)
            , m_isSettling(false)
            , m_physicsMode(PhysicsMode::NORMAL)
            , m_springStrength(2000.0f)   // Much stronger springs for dramatic effect
            , m_dampingFactor(0.6f)      // Much less damping for bouncy feel
            , m_dragMomentum(0.9f)
            , m_maxVelocity(1200.0f)     // Higher max velocity for more dramatic movement
            , m_settleThreshold(0.5f)    // Very small threshold
            , m_bounceStrength(0.4f)
            , m_mass(1.2f)               // Slightly increased mass for subtle inertia
        {}

        // Physics properties
        Vec2 getVelocity() const { return m_velocity; }
        void setVelocity(const Vec2& velocity) { m_velocity = velocity; }
        void addVelocity(const Vec2& deltaVelocity) { m_velocity += deltaVelocity; }

        Vec2 getTargetPosition() const { return m_targetPosition; }
        void setTargetPosition(const Vec2& target) { 
            m_targetPosition = target; 
            if (!m_isDragging) {
                m_restPosition = target;
            }
        }

        Vec2 getRestPosition() const { return m_restPosition; }
        void setRestPosition(const Vec2& rest) { m_restPosition = rest; }

        // Dragging state
        bool isDragging() const { return m_isDragging; }
        void setDragging(bool dragging) {
            m_isDragging = dragging;
            if (!dragging) {
                m_isSettling = true;
            }
        }

        bool isSettling() const { return m_isSettling; }
        void setSettling(bool settling) { m_isSettling = settling; }

        // Physics mode control
        PhysicsMode getPhysicsMode() const { return m_physicsMode; }
        void setPhysicsMode(PhysicsMode mode) { m_physicsMode = mode; }

        void setCelebrationMode() {
            m_physicsMode = PhysicsMode::CELEBRATION;
            m_springStrength = 1200.0f;  // Stronger springs for celebration
            m_dampingFactor = 0.8f;      // Less damping for more bounce
            m_settleThreshold = 5.0f;    // Larger threshold for more movement
        }

        void setMagneticMode() {
            m_physicsMode = PhysicsMode::MAGNETIC;
            m_springStrength = 400.0f;   // Weaker springs for gentle attraction
            m_dampingFactor = 0.95f;     // More damping for smooth attraction
            m_settleThreshold = 2.0f;    // Smaller threshold for precision
        }

        void setNormalMode() {
            m_physicsMode = PhysicsMode::NORMAL;
            m_springStrength = 600.0f;
            m_dampingFactor = 0.88f;
            m_settleThreshold = 3.0f;
        }

        void setDragMode() {
            m_physicsMode = PhysicsMode::DRAG;
            m_springStrength = 1200.0f;   // Moderate spring strength
            m_dampingFactor = 0.85f;      // More damping for smoother feel
            m_settleThreshold = 5.0f;     // Reasonable settle distance
        }
        // Physics parameters (can be tweaked for different effects)
        float getSpringStrength() const { return m_springStrength; }
        void setSpringStrength(float strength) { m_springStrength = strength; }

        float getDampingFactor() const { return m_dampingFactor; }
        void setDampingFactor(float damping) { m_dampingFactor = damping; }

        float getDragMomentum() const { return m_dragMomentum; }
        void setDragMomentum(float momentum) { m_dragMomentum = momentum; }

        float getBounceStrength() const { return m_bounceStrength; }
        void setBounceStrength(float bounce) { m_bounceStrength = bounce; }

        float getMass() const { return m_mass; }
        void setMass(float mass) { m_mass = mass; }

        // Set the frame component this physics component should follow
        void setFrame(CardFrameComponent* frame) { m_frame = frame; }
        CardFrameComponent* getFrame() const { return m_frame; }

        // Apply spring force toward frame position
        void applySpringForce(const Vec2& currentPosition, float deltaTime) {
            if (m_isDragging && m_physicsMode != PhysicsMode::DRAG) return; // Allow DRAG mode to work while dragging

            // Use frame position if available, otherwise fall back to target position
            Vec2 targetPos = m_frame ? m_frame->getTargetPosition() : m_targetPosition;
            Vec2 displacement = targetPos - currentPosition;
            float distance = displacement.length();

            if (distance < m_settleThreshold) {
                // Close enough - settle the card based on mode
                float settleDamping = (static_cast<int>(m_physicsMode) == 1) ? 0.5f : 0.7f;
                float settleVelocityThreshold = (static_cast<int>(m_physicsMode) == 1) ? 2.0f : 1.0f; // Much lower!

                m_velocity *= settleDamping; // Mode-specific damping
                if (m_velocity.length() < settleVelocityThreshold) {
                    m_velocity = Vec2(0.0f, 0.0f);
                    m_isSettling = false;
                    setNormalMode(); // Reset to normal mode after settling
                }
                return;
            }

            // Mode-specific physics behavior
            Vec2 springForce;
            Vec2 dampingForce;
            float velocityDamping = 15.0f; // Reduced damping for more bounce
            Vec2 gravityForce = Vec2(0.0f, -800.0f); // Downward gravity
            float distanceMultiplier = std::min(distance / 100.0f, 3.0f); // Scale up to 3x for far distances

            switch (static_cast<int>(m_physicsMode)) {
                case 1: // CELEBRATION
                    // Strong springs, less damping for bouncy celebration
                    springForce = displacement * m_springStrength;
                    dampingForce = m_velocity * -velocityDamping * 0.5f; // Less damping
                    break;

                case 2: // MAGNETIC
                    // Gentle attraction with strong damping for smooth magnetic effect
                    springForce = displacement * m_springStrength * 0.5f;
                    dampingForce = m_velocity * -velocityDamping * 2.0f; // Strong damping
                    break;

                    case 3: // DRAG
                    // Distance-proportional spring toward cursor with moderate damping + gravity
                    springForce = displacement * m_springStrength * (1.0f + distanceMultiplier); // Stronger when further
                    dampingForce = m_velocity * -velocityDamping * 0.8f; // Moderate damping
                    
                    // Add gravity effect - pulls card down from cursor
                    springForce += gravityForce;
                    break;
                default: // NORMAL (case 0)
                    springForce = displacement * m_springStrength;
                    dampingForce = m_velocity * -velocityDamping;
                    break;
            }

            Vec2 totalForce = springForce + dampingForce;

            // Apply force to velocity (F = ma, so a = F/m)
            Vec2 acceleration = totalForce / m_mass;
            m_velocity += acceleration * deltaTime;

                        // Apply mode-specific frame damping
            float modeDamping = (static_cast<int>(m_physicsMode) == 1) ? 0.75f : 
                               (static_cast<int>(m_physicsMode) == 3) ? 0.90f : m_dampingFactor; // Moderate damping for DRAG mode
            m_velocity *= modeDamping;
            // Clamp velocity to prevent explosions
            float speed = m_velocity.length();
            if (speed > m_maxVelocity) {
                m_velocity = m_velocity.normalized() * m_maxVelocity;
            }
        }

        // Apply bounce effect when hitting invalid drop zones
        void applyBounce(const Vec2& bounceDirection) {
            Vec2 bounceVelocity = bounceDirection * m_bounceStrength * 200.0f; // Reduced bounce force
            m_velocity += bounceVelocity;
        }

        // Update position based on current velocity
        Vec2 updatePosition(const Vec2& currentPosition, float deltaTime) {
            if (m_isDragging && m_physicsMode != PhysicsMode::DRAG) return currentPosition; // Allow DRAG mode to work while dragging

            Vec2 newPosition = currentPosition + m_velocity * deltaTime;
            return newPosition;
        }

        // For dragging - maintain momentum
        void updateDragMomentum(const Vec2& dragVelocity) {
            if (m_isDragging) {
                // Blend in drag velocity with momentum
                m_velocity = m_velocity * m_dragMomentum + dragVelocity * (1.0f - m_dragMomentum);
            }
        }

        // Reset physics state
        void reset() {
            m_velocity = Vec2(0.0f, 0.0f);
            m_isDragging = false;
            m_isSettling = false;
        }

        // Add continuous jitter for non-dragged cards
        void addContinuousJitter(float dt) {
            if (m_isDragging) return; // Don't jitter while dragging
            
            // Add small random jitter every frame
            float jitterStrength = 2.0f; // Very subtle jitter
            float randomX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * jitterStrength;
            float randomY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * jitterStrength;
            Vec2 jitterVel(randomX, randomY);
            m_velocity += jitterVel;
        }

        // Special effects
        void addRandomJitter(float strength = 25.0f) {  // Reduced default strength
            float randomX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * strength;
            float randomY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * strength;
            Vec2 jitterVel(randomX, randomY);
            m_velocity += jitterVel;
            
        }

        void addExplosiveForce(const Vec2& explosionCenter, const Vec2& cardPosition, float explosionStrength = 500.0f) {
            Vec2 direction = cardPosition - explosionCenter;
            float distance = direction.length();
            if (distance > 0.1f) {
                direction = direction.normalized();
                float force = explosionStrength / (distance * 0.01f + 1.0f); // Inverse distance falloff
                m_velocity += direction * force;
            }
        }

    private:
        Vec2 m_velocity;
        Vec2 m_targetPosition;
        Vec2 m_restPosition;
        CardFrameComponent* m_frame; // Reference to the frame this card should follow

        bool m_isDragging;
        bool m_isSettling;
        PhysicsMode m_physicsMode; // Current physics mode

        // Physics parameters
        float m_springStrength;    // How strong the spring force is
        float m_dampingFactor;     // How much velocity is reduced each frame
        float m_dragMomentum;      // How much drag velocity affects physics velocity
        float m_maxVelocity;       // Maximum speed to prevent physics explosions
        float m_settleThreshold;   // Distance at which card "settles"
        float m_bounceStrength;    // How strong bounce effects are
        float m_mass;              // Mass for force calculations
    };
}
