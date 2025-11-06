#include <DX3D/Components/SpringGuyComponent.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <cmath>

namespace dx3d {

    // SpringGuyNodeComponent Implementation
    SpringGuyNodeComponent::SpringGuyNodeComponent(Vec2 position, bool positionFixed)
        : m_position(position)
        , startingPos(position)
        , m_positionFixed(positionFixed) {
    }

    void SpringGuyNodeComponent::update(float dt) {
        if (!m_positionFixed && m_totalMass > 0.0f) {
            // F = ma, so a = F/m
            // Include external forces (like from car interaction)
            Vec2 totalForce = m_totalForce + m_externalForce;

            Vec2 acceleration = Vec2(totalForce.x / m_totalMass, totalForce.y / m_totalMass);
            m_velocity += acceleration * dt;
            m_position += m_velocity * dt;
        }
    }

    void SpringGuyNodeComponent::calculateForces(const std::vector<Entity*>& beamEntities) {
        m_totalForce = Vec2(0.0f, 0.0f);
        m_totalMass = 0.0f;

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>()) {
                if (beam->isConnectedToNode(*this)) {
                    beam->addForceAndMassDiv2AtNode(*this, m_totalForce, m_totalMass);
                }
            }
        }
    }

    void SpringGuyNodeComponent::resetTotalMass() {
        m_totalMass = 0.0f;
        m_totalForce = Vec2(0.0f, 0.0f);
        m_externalForce = Vec2(0.0f, 0.0f);
        m_velocity = Vec2(0.0f, 0.0f);
    }

    void SpringGuyNodeComponent::addExternalForce(const Vec2& force) {
        m_externalForce += force;
    }

    void SpringGuyNodeComponent::clearExternalForces() {
        m_externalForce = Vec2(0.0f, 0.0f);
    }

    bool SpringGuyNodeComponent::mouseInside(const Vec2& mouseWorldPos, float nodeSize) const {
        float halfSize = nodeSize * 0.5f;
        return (mouseWorldPos.x > m_position.x - halfSize &&
            mouseWorldPos.x < m_position.x + halfSize &&
            mouseWorldPos.y > m_position.y - halfSize &&
            mouseWorldPos.y < m_position.y + halfSize);
    }

    // SpringGuyBeamComponent Implementation
    SpringGuyBeamComponent::SpringGuyBeamComponent(Entity* node1Entity, Entity* node2Entity)
        : m_node1Entity(node1Entity)
        , m_node2Entity(node2Entity)
        , m_node1StartEntity(node1Entity)
        , m_node2StartEntity(node2Entity) {

        if (node1Entity && node2Entity) {
            auto* node1 = node1Entity->getComponent<SpringGuyNodeComponent>();
            auto* node2 = node2Entity->getComponent<SpringGuyNodeComponent>();
            if (node1 && node2) {
                Vec2 diff = node1->getPosition() - node2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    void SpringGuyBeamComponent::update(float dt) {
        // Update visual stress state based on force
        if (m_colorForceFactor >= 1.0f) {
            m_isBroken = true;
        }
    }

    void SpringGuyBeamComponent::resetBeam() {
        m_colorForceFactor = 0.0f;
        m_isBroken = false;

        if (!m_node1Entity) m_node1Entity = m_node1StartEntity;
        if (!m_node2Entity) m_node2Entity = m_node2StartEntity;

        if (m_node1StartEntity && m_node2StartEntity) {
            auto* node1 = m_node1StartEntity->getComponent<SpringGuyNodeComponent>();
            auto* node2 = m_node2StartEntity->getComponent<SpringGuyNodeComponent>();
            if (node1 && node2) {
                Vec2 diff = node1->getPosition() - node2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    Vec2 SpringGuyBeamComponent::getForceAtNode(const SpringGuyNodeComponent& node) const {
        if (!m_node1Entity || !m_node2Entity || m_length0 <= 0.0f || !m_enabled) {
            return Vec2(0.0f, 0.0f);
        }

        auto* node1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
        auto* node2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
        if (!node1 || !node2) {
            return Vec2(0.0f, 0.0f);
        }

        // Calculate current beam vector and displacement
        Vec2 currentLength = node1->getPosition() - node2->getPosition();
        float restLength = m_length0 * m_restLengthMultiplier;
        Vec2 displacement = currentLength.normalized() * (currentLength.length() - restLength);

        // Calculate forces using configurable parameters
        Vec2 forceBeam = displacement * m_stiffness;
        Vec2 forceGravity = Vec2(0.0f, m_mass * GRAVITY);

        // Apply damping force (opposite to velocity)
        Vec2 relativeVelocity = node1->getVelocity() - node2->getVelocity();
        Vec2 dampingForce = relativeVelocity * -m_damping;

        // Clamp force to maximum
        float forceMagnitude = forceBeam.length();
        if (forceMagnitude > m_maxForce) {
            forceBeam = forceBeam.normalized() * m_maxForce;
        }

        // Update stress visualization (cast away const for this calculation)
        const_cast<SpringGuyBeamComponent*>(this)->m_colorForceFactor = forceMagnitude / m_maxForce;
        if (m_colorForceFactor >= 1.0f) {
            const_cast<SpringGuyBeamComponent*>(this)->m_colorForceFactor = 1.0f;
            const_cast<SpringGuyBeamComponent*>(this)->m_isBroken = true;
        }

        // Add damping to the spring force
        Vec2 totalForce = forceBeam + dampingForce;

        // Return force based on which node is being queried
        if (node1 == &node) {
            return Vec2(-totalForce.x, -totalForce.y) + Vec2(forceGravity.x * 0.5f, forceGravity.y * 0.5f);
        }
        else if (node2 == &node) {
            return totalForce + Vec2(forceGravity.x * 0.5f, forceGravity.y * 0.5f);
        }

        return Vec2(0.0f, 0.0f);
    }

    void SpringGuyBeamComponent::addForceAndMassDiv2AtNode(const SpringGuyNodeComponent& node, Vec2& forceSum, float& massSum) {
        forceSum += getForceAtNode(node);
        massSum += m_mass * 0.5f;
    }

    bool SpringGuyBeamComponent::isConnectedToNode(const SpringGuyNodeComponent& node) const {
        auto* node1 = m_node1Entity ? m_node1Entity->getComponent<SpringGuyNodeComponent>() : nullptr;
        auto* node2 = m_node2Entity ? m_node2Entity->getComponent<SpringGuyNodeComponent>() : nullptr;
        return (node1 == &node) || (node2 == &node);
    }

    Vec2 SpringGuyBeamComponent::getCenterPosition() const {
        if (!m_node1Entity || !m_node2Entity) return Vec2(0.0f, 0.0f);

        auto* node1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
        auto* node2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
        if (!node1 || !node2) return Vec2(0.0f, 0.0f);

        return Vec2(
            (node1->getPosition().x + node2->getPosition().x) * 0.5f,
            (node1->getPosition().y + node2->getPosition().y) * 0.5f
        );
    }

    float SpringGuyBeamComponent::getLength() const {
        if (!m_node1Entity || !m_node2Entity) return 0.0f;

        auto* node1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
        auto* node2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
        if (!node1 || !node2) return 0.0f;

        Vec2 diff = node1->getPosition() - node2->getPosition();
        return diff.length();
    }

    float SpringGuyBeamComponent::getAngle() const {
        if (!m_node1Entity || !m_node2Entity) return 0.0f;

        auto* node1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
        auto* node2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
        if (!node1 || !node2) return 0.0f;

        Vec2 diff = node1->getPosition() - node2->getPosition();
        return std::atan2(diff.y, diff.x);
    }

    // SpringGuySystem Implementation
    void SpringGuySystem::updateNodes(EntityManager& entityManager, float dt) {
        auto nodeEntities = entityManager.getEntitiesWithComponent<SpringGuyNodeComponent>();
        auto beamEntities = entityManager.getEntitiesWithComponent<SpringGuyBeamComponent>();

        // First, calculate forces for all nodes (don't clear external forces yet)
        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>()) {
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
            if (auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>()) {
                node->update(dt);

                // Update sprite position after physics update
                if (auto* sprite = nodeEntity->getComponent<SpriteComponent>()) {
                    Vec2 pos = node->getPosition();
                    sprite->setPosition(pos.x, pos.y, 0.0f);
                }
            }
        }

        // Finally, clear external forces after physics update
        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>()) {
                node->clearExternalForces(); // Clear external forces after they've been used
            }
        }
    }

    void SpringGuySystem::updateBeams(EntityManager& entityManager, float dt) {
        auto beamEntities = entityManager.getEntitiesWithComponent<SpringGuyBeamComponent>();

        for (auto* beamEntity : beamEntities) {
            auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>();
            if (!beam) continue;

            // Physics tick for the beam
            beam->update(dt);

            // Get live node positions
            auto* node1 = beam->getNode1Entity()->getComponent<SpringGuyNodeComponent>();
            auto* node2 = beam->getNode2Entity()->getComponent<SpringGuyNodeComponent>();
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
            float thickness = beam->getThickness();

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
                    sprite->setScale(length, clamp(thickness, 10, 500), 1.0f);

                }
                else {
                    // Fallback if mesh dimensions aren't available
                    sprite->setPosition(center.x, center.y, 0.0f);
                    sprite->setRotationZ(angleRad);
                    sprite->setScale(length, clamp(thickness, 10, 500), 1.0f);
                }
            }
        }
    }

    void SpringGuySystem::resetPhysics(EntityManager& entityManager) {
        auto nodeEntities = entityManager.getEntitiesWithComponent<SpringGuyNodeComponent>();
        auto beamEntities = entityManager.getEntitiesWithComponent<SpringGuyBeamComponent>();

        for (auto* nodeEntity : nodeEntities) {
            if (auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>()) {
                node->setPosition(node->startingPos);
                node->setVelocity(Vec2(0.0f, 0.0f));
                node->resetTotalMass();
            }
        }

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>()) {
                beam->resetBeam();
            }
        }
    }

    // Additional SpringGuyBeamComponent methods
    bool SpringGuyBeamComponent::isConnectedToNode(const Entity* nodeEntity) const {
        return (m_node1Entity == nodeEntity) || (m_node2Entity == nodeEntity);
    }

    void SpringGuyBeamComponent::updateNodeConnection(Entity* oldNode, Entity* newNode) {
        if (m_node1Entity == oldNode) {
            m_node1Entity = newNode;
        }
        else if (m_node2Entity == oldNode) {
            m_node2Entity = newNode;
        }

        // Recalculate beam properties with new connection
        if (m_node1Entity && m_node2Entity) {
            auto* node1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
            auto* node2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
            if (node1 && node2) {
                Vec2 diff = node1->getPosition() - node2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    void SpringGuyBeamComponent::setNodeConnection1(Entity* node1) {
        m_node1Entity = node1;

        // Recalculate beam properties
        if (m_node1Entity && m_node2Entity) {
            auto* nodeComp1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
            auto* nodeComp2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
            if (nodeComp1 && nodeComp2) {
                Vec2 diff = nodeComp1->getPosition() - nodeComp2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    void SpringGuyBeamComponent::setNodeConnection2(Entity* node2) {
        m_node2Entity = node2;

        // Recalculate beam properties
        if (m_node1Entity && m_node2Entity) {
            auto* nodeComp1 = m_node1Entity->getComponent<SpringGuyNodeComponent>();
            auto* nodeComp2 = m_node2Entity->getComponent<SpringGuyNodeComponent>();
            if (nodeComp1 && nodeComp2) {
                Vec2 diff = nodeComp1->getPosition() - nodeComp2->getPosition();
                m_length0 = diff.length();
                m_mass = MASS_PER_LENGTH * m_length0;
            }
        }
    }

    // Additional SpringGuySystem methods
    void SpringGuySystem::removeBeamsConnectedToNode(EntityManager& entityManager, Entity* nodeEntity) {
        auto beamEntities = entityManager.getEntitiesWithComponent<SpringGuyBeamComponent>();
        std::vector<std::string> beamsToRemove;

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>()) {
                if (beam->isConnectedToNode(nodeEntity)) {
                    beamsToRemove.push_back(beamEntity->getName());
                }
            }
        }

        // Remove the beams
        for (const auto& beamName : beamsToRemove) {
            entityManager.removeEntity(beamName);
        }
    }

    std::vector<Entity*> SpringGuySystem::getBeamsConnectedToNode(EntityManager& entityManager, Entity* nodeEntity) {
        std::vector<Entity*> connectedBeams;
        auto beamEntities = entityManager.getEntitiesWithComponent<SpringGuyBeamComponent>();

        for (auto* beamEntity : beamEntities) {
            if (auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>()) {
                if (beam->isConnectedToNode(nodeEntity)) {
                    connectedBeams.push_back(beamEntity);
                }
            }
        }

        return connectedBeams;
    }
}
