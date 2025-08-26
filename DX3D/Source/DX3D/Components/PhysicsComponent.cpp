#include <DX3D/Components/PhysicsComponent.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <cmath>

namespace dx3d {

    // NodeComponent Implementation
    NodeComponent::NodeComponent(Vec2 position, bool positionFixed)
        : m_position(position)
        , startingPos(position)
        , m_positionFixed(positionFixed) {
    }

    void NodeComponent::update(float dt) {
        if (!m_positionFixed && m_totalMass > 0.0f) {
            // F = ma, so a = F/m
            
            Vec2 acceleration = Vec2(m_totalForce.x / m_totalMass, m_totalForce.y / m_totalMass);
            m_velocity += acceleration * dt;
            m_position += m_velocity * dt;
        }
    }

    void NodeComponent::calculateForces(const std::vector<Entity*>& beamEntities) {
        m_totalForce = Vec2(0.0f, 0.0f);
        m_totalMass = 0.0f;

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<BeamComponent>()) {
                if (beam->isConnectedToNode(*this)) {
                    beam->addForceAndMassDiv2AtNode(*this, m_totalForce, m_totalMass);
                }
            }
        }
    }

    void NodeComponent::resetTotalMass() {
        m_totalMass = 0.0f;
        m_totalForce = Vec2(0.0f, 0.0f);
        m_velocity = Vec2(0.0f, 0.0f);
    }

    bool NodeComponent::mouseInside(const Vec2& mouseWorldPos, float nodeSize) const {
        float halfSize = nodeSize * 0.5f;
        return (mouseWorldPos.x > m_position.x - halfSize &&
            mouseWorldPos.x < m_position.x + halfSize &&
            mouseWorldPos.y > m_position.y - halfSize &&
            mouseWorldPos.y < m_position.y + halfSize);
    }

    // BeamComponent Implementation
    BeamComponent::BeamComponent(Entity* node1Entity, Entity* node2Entity)
        : m_node1Entity(node1Entity)
        , m_node2Entity(node2Entity)
        , m_node1StartEntity(node1Entity)
        , m_node2StartEntity(node2Entity) {

        if (node1Entity && node2Entity) {
            auto* node1 = node1Entity->getComponent<NodeComponent>();
            auto* node2 = node2Entity->getComponent<NodeComponent>();
            if (node1 && node2) {
                Vec2 diff = node1->getPosition() - node2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    void BeamComponent::update(float dt) {
        // Update visual stress state based on force
        if (m_colorForceFactor >= 1.0f) {
            m_isBroken = true;
        }
    }

    void BeamComponent::resetBeam() {
        m_colorForceFactor = 0.0f;
        m_isBroken = false;

        if (!m_node1Entity) m_node1Entity = m_node1StartEntity;
        if (!m_node2Entity) m_node2Entity = m_node2StartEntity;

        if (m_node1StartEntity && m_node2StartEntity) {
            auto* node1 = m_node1StartEntity->getComponent<NodeComponent>();
            auto* node2 = m_node2StartEntity->getComponent<NodeComponent>();
            if (node1 && node2) {
                Vec2 diff = node1->getPosition() - node2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    Vec2 BeamComponent::getForceAtNode(const NodeComponent& node) const {
        if (!m_node1Entity || !m_node2Entity || m_length0 <= 0.0f) {
            return Vec2(0.0f, 0.0f);
        }

        auto* node1 = m_node1Entity->getComponent<NodeComponent>();
        auto* node2 = m_node2Entity->getComponent<NodeComponent>();
        if (!node1 || !node2) {
            return Vec2(0.0f, 0.0f);
        }

        // Calculate current beam vector and displacement
        Vec2 currentLength = node1->getPosition() - node2->getPosition();
        Vec2 displacement = currentLength.normalized() * (currentLength.length() - m_length0);

        // Calculate forces
        Vec2 forceBeam = displacement * STIFFNESS;
        Vec2 forceGravity = Vec2(0.0f, m_mass * GRAVITY);

        // Update stress visualization (cast away const for this calculation)
        const_cast<BeamComponent*>(this)->m_colorForceFactor = forceBeam.length() / FORCE_BEAM_MAX;
        if (m_colorForceFactor >= 1.0f) {
            const_cast<BeamComponent*>(this)->m_colorForceFactor = 1.0f;
            const_cast<BeamComponent*>(this)->m_isBroken = true;
        }

        // Return force based on which node is being queried
        if (node1 == &node) {
            return Vec2(-forceBeam.x, -forceBeam.y) + Vec2(forceGravity.x * 0.5f, forceGravity.y * 0.5f);
        }
        else if (node2 == &node) {
            return forceBeam + Vec2(forceGravity.x * 0.5f, forceGravity.y * 0.5f);
        }

        return Vec2(0.0f, 0.0f);
    }

    void BeamComponent::addForceAndMassDiv2AtNode(const NodeComponent& node, Vec2& forceSum, float& massSum) {
        forceSum += getForceAtNode(node);
        massSum += m_mass * 0.5f;
    }

    bool BeamComponent::isConnectedToNode(const NodeComponent& node) const {
        auto* node1 = m_node1Entity ? m_node1Entity->getComponent<NodeComponent>() : nullptr;
        auto* node2 = m_node2Entity ? m_node2Entity->getComponent<NodeComponent>() : nullptr;
        return (node1 == &node) || (node2 == &node);
    }

    Vec2 BeamComponent::getCenterPosition() const {
        if (!m_node1Entity || !m_node2Entity) return Vec2(0.0f, 0.0f);

        auto* node1 = m_node1Entity->getComponent<NodeComponent>();
        auto* node2 = m_node2Entity->getComponent<NodeComponent>();
        if (!node1 || !node2) return Vec2(0.0f, 0.0f);

        return Vec2(
            (node1->getPosition().x + node2->getPosition().x) * 0.5f,
            (node1->getPosition().y + node2->getPosition().y) * 0.5f
        );
    }

    float BeamComponent::getLength() const {
        if (!m_node1Entity || !m_node2Entity) return 0.0f;

        auto* node1 = m_node1Entity->getComponent<NodeComponent>();
        auto* node2 = m_node2Entity->getComponent<NodeComponent>();
        if (!node1 || !node2) return 0.0f;

        Vec2 diff = node1->getPosition() - node2->getPosition();
        return diff.length();
    }

    float BeamComponent::getAngle() const {
        if (!m_node1Entity || !m_node2Entity) return 0.0f;

        auto* node1 = m_node1Entity->getComponent<NodeComponent>();
        auto* node2 = m_node2Entity->getComponent<NodeComponent>();
        if (!node1 || !node2) return 0.0f;

        Vec2 diff = node1->getPosition() - node2->getPosition();
        return std::atan2(diff.y, diff.x);
    }

    // PhysicsSystem Implementation
    void PhysicsSystem::updateNodes(EntityManager& entityManager, float dt) {
        auto nodeEntities = entityManager.getEntitiesWithComponent<NodeComponent>();
        auto beamEntities = entityManager.getEntitiesWithComponent<BeamComponent>();

        // First, calculate forces for all nodes
        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<NodeComponent>()) {
                node->calculateForces(beamEntities);

                // Update sprite position if it exists
                if (auto* sprite = nodeEntity->getComponent<SpriteComponent>()) {
                    Vec2 pos = node->getPosition();
                    sprite->setPosition(pos.x, pos.y, 0.0f);
                }
            }
        }

        // Then, update node physics
        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<NodeComponent>()) {
                node->update(dt);

                // Update sprite position after physics update
                if (auto* sprite = nodeEntity->getComponent<SpriteComponent>()) {
                    Vec2 pos = node->getPosition();
                    sprite->setPosition(pos.x, pos.y, 0.0f);
                }
            }
        }
    }

    void PhysicsSystem::updateBeams(EntityManager& entityManager, float dt) {
        auto beamEntities = entityManager.getEntitiesWithComponent<BeamComponent>();

        for (auto* beamEntity : beamEntities) {
            auto* beam = beamEntity->getComponent<BeamComponent>();
            if (!beam) continue;

            // Physics tick for the beam
            beam->update(dt);

            // Get live node positions
            auto* node1 = beam->getNode1Entity()->getComponent<NodeComponent>();
            auto* node2 = beam->getNode2Entity()->getComponent<NodeComponent>();
            if (!node1 || !node2) continue;

            Vec2 p1 = node1->getPosition();
            Vec2 p2 = node2->getPosition();

            // Calculate beam vector (node1 to node2)
            Vec2 beamVector = p1 - p2;

            // Center position - exact midpoint between the two nodes
            Vec2 center = Vec2((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f);

            // Length and angle
            float length = beamVector.length();
            float angleRad = std::atan2(beamVector.y, beamVector.x);
            float thickness = beam->getThickness() ; 

            if (auto* sprite = beamEntity->getComponent<SpriteComponent>()) {
                // Get the mesh dimensions to understand how to scale properly
                auto mesh = sprite->getMesh();
                if (mesh) {
                    float meshWidth = mesh->getWidth();  
                    float meshHeight = mesh->getHeight();

                    // Scale the mesh to match the beam dimensions
                    float scaleX = (meshWidth > 0.0f) ? length / meshWidth : length;
                    float scaleY = (meshHeight > 0.0f) ? thickness / meshHeight : thickness;

                    sprite->setPosition(center.x, center.y, 0.0f);
                    sprite->setRotationZ(angleRad);
                    sprite->setScale(length, clamp(thickness,10,500), 1.0f);

                }
                else {
                    // Fallback if mesh dimensions aren't available
                    sprite->setPosition(center.x, center.y, 0.0f);
                    sprite->setRotationZ(angleRad);
                    sprite->setScale(length, thickness, 1.0f);
                }
            }
        }
    }


    void PhysicsSystem::resetPhysics(EntityManager& entityManager) {
        auto nodeEntities = entityManager.getEntitiesWithComponent<NodeComponent>();
        auto beamEntities = entityManager.getEntitiesWithComponent<BeamComponent>();

        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<NodeComponent>()) {
                node->setPosition(node->startingPos);
                node->setVelocity(Vec2(0.0f, 0.0f));
                node->resetTotalMass();
            }
        }

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<BeamComponent>()) {
                beam->resetBeam();
            }
        }
    }
}