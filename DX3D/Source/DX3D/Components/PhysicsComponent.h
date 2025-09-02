#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/Entity.h>
#include <memory>
#include <vector>
#include <DX3D/Core/EntityManager.h>

namespace dx3d {
    class Entity;
    class BeamComponent;

    // Physics node component - equivalent to your Node class
    class NodeComponent {
    public:
        NodeComponent(Vec2 position, bool positionFixed = false);
        void update(float dt);
        void calculateForces(const std::vector<Entity*>& beamEntities);
        void resetTotalMass();

        // Getters/Setters
        Vec2 getPosition() const { return m_position; }
        void setPosition(const Vec2& pos) { m_position = pos; }
        Vec2 getVelocity() const { return m_velocity; }
        void setVelocity(const Vec2& vel) { m_velocity = vel; }
        bool isPositionFixed() const { return m_positionFixed; }
        void setPositionFixed(bool fixed) { m_positionFixed = fixed; }

        // Starting position management (for reset functionality)
        Vec2 getStartingPosition() const { return startingPos; }
        void setStartingPosition(const Vec2& pos) { startingPos = pos; }

        // Mouse interaction
        bool mouseInside(const Vec2& mouseWorldPos, float nodeSize = 28.0f) const;

        // Visual state
        void setStressed(bool stressed) { m_isStressed = stressed; }
        bool isStressed() const { return m_isStressed; }

        // Fixed node check (needed for delete mode)
        bool isFixed() const { return m_positionFixed; }

        Vec2 startingPos;
        bool isTextureSet = false;

    private:
        Vec2 m_position;
        Vec2 m_velocity{ 0.0f, 0.0f };
        Vec2 m_totalForce{ 0.0f, 0.0f };
        float m_totalMass = 0.0f;
        bool m_positionFixed;
        bool m_isStressed = false;
    };

    // Physics beam component - equivalent to your Beam class
    class BeamComponent {
    public:
        BeamComponent(Entity* node1Entity, Entity* node2Entity);
        void update(float dt);
        void resetBeam();

        // Physics calculations
        Vec2 getForceAtNode(const NodeComponent& node) const;
        void addForceAndMassDiv2AtNode(const NodeComponent& node, Vec2& forceSum, float& massSum);

        // State queries
        bool isBroken() const { return m_isBroken; }
        bool isConnectedToNode(const NodeComponent& node) const;
        bool isConnectedToNode(const Entity* nodeEntity) const; // Overload for Entity*
        float getStressFactor() const { return m_colorForceFactor; }

        // Visual properties
        Vec2 getCenterPosition() const;
        float getLength() const;
        float getAngle() const;
        float getThickness() const { return m_thickness * (1.0f - m_colorForceFactor); }

        // Node connections
        Entity* getNode1Entity() const { return m_node1Entity; }
        Entity* getNode2Entity() const { return m_node2Entity; }
        void setNode2Entity(Entity* node2) { m_node2Entity = node2; }

        // Node connection updates (needed for building system)
        void updateNodeConnection(Entity* oldNode, Entity* newNode);
        void setNodeConnection1(Entity* node1) ;
        void setNodeConnection2(Entity* node2) ;

        // Breaking check
        bool getIsBroken() const { return m_isBroken; }
        void setBroken(bool broken) { m_isBroken = broken; }

        // Constants
        static constexpr float MASS_PER_LENGTH = 0.01f;
        static constexpr float STIFFNESS = 2500.0f;
        static constexpr float FORCE_BEAM_MAX = 2500.0f;
        static constexpr float GRAVITY = -9.81f * 10.0f; // Scale for your world units

    private:
        Entity* m_node1Entity;
        Entity* m_node2Entity;
        Entity* m_node1StartEntity;
        Entity* m_node2StartEntity;
        float m_length0 = 0.0f;
        float m_mass = 0.0f;
        float m_colorForceFactor = 0.0f;
        bool m_isBroken = false;
        static constexpr float m_thickness = 22.0f;
    };

    // System to update all physics nodes and beams
    class PhysicsSystem {
    public:
        static void updateNodes(EntityManager& entityManager, float dt);
        static void updateBeams(EntityManager& entityManager, float dt);
        static void resetPhysics(EntityManager& entityManager);

        // Additional utility methods for the building system
        static void removeBeamsConnectedToNode(EntityManager& entityManager, Entity* nodeEntity);
        static std::vector<Entity*> getBeamsConnectedToNode(EntityManager& entityManager, Entity* nodeEntity);
    };
}