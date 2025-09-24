#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/Entity.h>
#include <vector>

namespace dx3d {
    class EntityManager;
}

namespace dx3d {
    class Entity;
    class NodeComponent;
    class BeamComponent;

    // Physics component for the car that drives across the bridge
    class CarPhysicsComponent {
    public:
        CarPhysicsComponent(float mass = 50.0f, float maxSpeed = 200.0f);
        
        // Physics update
        void update(float dt, EntityManager& entityManager);
        
        // Car control
        void setDriving(bool driving) { m_isDriving = driving; }
        bool isDriving() const { return m_isDriving; }
        void setTargetSpeed(float speed) { m_targetSpeed = speed; }
        
        // Position and movement
        Vec2 getPosition() const { return m_position; }
        void setPosition(const Vec2& pos) { m_position = pos; }
        Vec2 getVelocity() const { return m_velocity; }
        void setVelocity(const Vec2& vel) { m_velocity = vel; }
        
        // Physics properties
        float getMass() const { return m_mass; }
        void setMass(float mass) { m_mass = mass; }
        float getMaxSpeed() const { return m_maxSpeed; }
        void setMaxSpeed(float speed) { m_maxSpeed = speed; }
        
        // Collision detection
        bool isCollidingWithNode(const NodeComponent& node) const;
        bool isCollidingWithBeam(const BeamComponent& beam) const;
        
        // Car dimensions
        Vec2 getSize() const { return m_size; }
        void setSize(const Vec2& size) { m_size = size; }
        
        // Reset functionality
        void reset();
        void setStartingPosition(const Vec2& pos) { m_startingPosition = pos; }
        
    private:
        // Physics properties
        Vec2 m_position;
        Vec2 m_velocity;
        Vec2 m_acceleration;
        float m_mass;
        float m_maxSpeed;
        float m_targetSpeed;
        bool m_isDriving;
        
        // Car dimensions
        Vec2 m_size;
        
        // Starting position for reset
        Vec2 m_startingPosition;
        
        // Collision detection helpers
        bool checkNodeCollision(const NodeComponent& node) const;
        bool checkBeamCollision(const BeamComponent& beam) const;
        Vec2 getCollisionForce(const NodeComponent& node) const;
        Vec2 getCollisionForce(const BeamComponent& beam) const;
        
        // Driving mechanics
        void applyDrivingForce(float dt);
        void applyCollisionForces(EntityManager& entityManager);
        void applyGravity();
        
        // Constants
        static constexpr float DRIVING_FORCE = 1000.0f;
        static constexpr float FRICTION = 0.95f;
        static constexpr float GRAVITY = -9.81f * 10.0f;
        static constexpr float COLLISION_DAMPING = 0.8f;
    };
}
