#include <DX3D/Components/SoftGuyComponent.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Game/Scenes/PhysicsTetrisScene.h> // For FrameComponent
#include <DX3D/Components/SpringGuyComponent.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dx3d {

    // Static gravity for SoftGuy system
    float SoftGuySystem::s_gravity = -2000.0f;

    // SoftGuyComponent Implementation
    SoftGuyComponent::SoftGuyComponent(EntityManager& em, const std::string& name, Vec2 position, const SoftGuyConfig& config)
        : m_entityManager(&em)
        , m_name(name)
        , m_config(config)
        , m_frameEntity(nullptr)
        , m_isStatic(false)
        , m_isVisible(true)
        , m_isBroken(false) {
        createFrame(em, name, position);
    }

    SoftGuyComponent* SoftGuyComponent::createCircle(EntityManager& em, const std::string& name, Vec2 position, float radius, int segments, const SoftGuyConfig& config) {
        auto* softGuy = new SoftGuyComponent(em, name, position, config);
        
        // Create nodes in a circle
        std::vector<Vec2> nodePositions;
        for (int i = 0; i < segments; ++i) {
            float angle = (2.0f * M_PI * i) / segments;
            Vec2 nodePos = position + Vec2(
                radius * std::cos(angle),
                radius * std::sin(angle)
            );
            nodePositions.push_back(nodePos);
        }
        
        // Create connections (each node connects to its neighbors and optionally to center)
        std::vector<std::pair<int, int>> connections;
        for (int i = 0; i < segments; ++i) {
            int next = (i + 1) % segments;
            connections.push_back({i, next});
            
            // Add cross-connections for stability (every other node)
            if (i % 2 == 0) {
                int opposite = (i + segments/2) % segments;
                connections.push_back({i, opposite});
            }
        }
        
        softGuy->createNodes(em, name, nodePositions);
        softGuy->createBeams(em, name, connections);
        
        return softGuy;
    }

    SoftGuyComponent* SoftGuyComponent::createRectangle(EntityManager& em, const std::string& name, Vec2 position, Vec2 size, int segmentsX, int segmentsY, const SoftGuyConfig& config) {
        auto* softGuy = new SoftGuyComponent(em, name, position, config);
        
        // Create nodes in a grid
        std::vector<Vec2> nodePositions;
        Vec2 halfSize = size * 0.5f;
        
        for (int y = 0; y < segmentsY; ++y) {
            for (int x = 0; x < segmentsX; ++x) {
                float fx = (float)x / (segmentsX - 1);
                float fy = (float)y / (segmentsY - 1);
                Vec2 nodePos = position + Vec2(
                    -halfSize.x + fx * size.x,
                    -halfSize.y + fy * size.y
                );
                nodePositions.push_back(nodePos);
            }
        }
        
        // Create connections (grid pattern)
        std::vector<std::pair<int, int>> connections;
        for (int y = 0; y < segmentsY; ++y) {
            for (int x = 0; x < segmentsX; ++x) {
                int current = y * segmentsX + x;
                
                // Connect to right neighbor
                if (x < segmentsX - 1) {
                    connections.push_back({current, current + 1});
                }
                
                // Connect to bottom neighbor
                if (y < segmentsY - 1) {
                    connections.push_back({current, current + segmentsX});
                }
                
                // Add diagonal connections for stability (every other cell)
                if (x < segmentsX - 1 && y < segmentsY - 1 && (x + y) % 2 == 0) {
                    connections.push_back({current, current + segmentsX + 1});
                }
            }
        }
        
        softGuy->createNodes(em, name, nodePositions);
        softGuy->createBeams(em, name, connections);
        
        return softGuy;
    }

    SoftGuyComponent* SoftGuyComponent::createTriangle(EntityManager& em, const std::string& name, Vec2 position, float size, const SoftGuyConfig& config) {
        auto* softGuy = new SoftGuyComponent(em, name, position, config);
        
        // Create nodes for triangle
        std::vector<Vec2> nodePositions;
        float halfSize = size * 0.5f;
        
        // Triangle vertices
        nodePositions.push_back(position + Vec2(0.0f, -halfSize));           // Top
        nodePositions.push_back(position + Vec2(-halfSize, halfSize));       // Bottom left
        nodePositions.push_back(position + Vec2(halfSize, halfSize));        // Bottom right
        
        // Add center node for stability
        nodePositions.push_back(position);
        
        // Create connections
        std::vector<std::pair<int, int>> connections;
        connections.push_back({0, 1}); // Top to bottom left
        connections.push_back({1, 2}); // Bottom left to bottom right
        connections.push_back({2, 0}); // Bottom right to top
        
        // Connect center to all vertices
        connections.push_back({3, 0});
        connections.push_back({3, 1});
        connections.push_back({3, 2});
        
        softGuy->createNodes(em, name, nodePositions);
        softGuy->createBeams(em, name, connections);
        
        return softGuy;
    }

    SoftGuyComponent* SoftGuyComponent::createLine(EntityManager& em, const std::string& name, Vec2 start, Vec2 end, int segments, const SoftGuyConfig& config) {
        auto* softGuy = new SoftGuyComponent(em, name, (start + end) * 0.5f, config);
        
        // Create nodes along the line
        std::vector<Vec2> nodePositions;
        Vec2 direction = end - start;
        float length = direction.length();
        direction = direction.normalized();
        
        for (int i = 0; i < segments; ++i) {
            float t = (float)i / (segments - 1);
            Vec2 nodePos = start + direction * (length * t);
            nodePositions.push_back(nodePos);
        }
        
        // Create connections (chain)
        std::vector<std::pair<int, int>> connections;
        for (int i = 0; i < segments - 1; ++i) {
            connections.push_back({i, i + 1});
        }
        
        softGuy->createNodes(em, name, nodePositions);
        softGuy->createBeams(em, name, connections);
        
        return softGuy;
    }

    SoftGuyComponent* SoftGuyComponent::createCustom(EntityManager& em, const std::string& name, Vec2 position, const std::vector<Vec2>& nodePositions, const std::vector<std::pair<int, int>>& connections, const SoftGuyConfig& config) {
        auto* softGuy = new SoftGuyComponent(em, name, position, config);
        
        softGuy->createNodes(em, name, nodePositions);
        softGuy->createBeams(em, name, connections);
        
        return softGuy;
    }

    void SoftGuyComponent::createFrame(EntityManager& em, const std::string& name, Vec2 position) {
        std::string frameName = name + "_Frame";
        m_frameEntity = &em.createEntity(frameName);
        m_frameEntity->addComponent<FrameComponent>(position, 0.0f);
    }

    void SoftGuyComponent::createNodes(EntityManager& em, const std::string& baseName, const std::vector<Vec2>& positions) {
        m_nodes.clear();
        
        for (size_t i = 0; i < positions.size(); ++i) {
            std::string nodeName = baseName + "_Node_" + std::to_string(i);
            auto& nodeEntity = em.createEntity(nodeName);
            auto& node = nodeEntity.addComponent<SpringGuyNodeComponent>(positions[i], false);
            
            // Set node mass
            node.setPosition(positions[i]);
            
            m_nodes.push_back(&nodeEntity);
        }
    }

    void SoftGuyComponent::createBeams(EntityManager& em, const std::string& baseName, const std::vector<std::pair<int, int>>& connections) {
        m_beams.clear();
        
        for (size_t i = 0; i < connections.size(); ++i) {
            std::string beamName = baseName + "_Beam_" + std::to_string(i);
            auto& beamEntity = em.createEntity(beamName);
            auto& beam = beamEntity.addComponent<SpringGuyBeamComponent>(
                m_nodes[connections[i].first],
                m_nodes[connections[i].second]
            );
            
            // Apply configuration
            beam.setStiffness(m_config.stiffness);
            beam.setDamping(m_config.damping);
            beam.setMaxForce(m_config.maxForce);
            
            m_beams.push_back(&beamEntity);
        }
    }

    void SoftGuyComponent::setPosition(const Vec2& position) {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->setPosition(position);
            }
        }
    }

    Vec2 SoftGuyComponent::getPosition() const {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                return frame->getPosition();
            }
        }
        return Vec2(0.0f, 0.0f);
    }

    void SoftGuyComponent::setVelocity(const Vec2& velocity) {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->setVelocity(velocity);
            }
        }
    }

    Vec2 SoftGuyComponent::getVelocity() const {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                return frame->getVelocity();
            }
        }
        return Vec2(0.0f, 0.0f);
    }

    void SoftGuyComponent::setAngularVelocity(float angularVelocity) {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->setAngularVelocity(angularVelocity);
            }
        }
    }

    float SoftGuyComponent::getAngularVelocity() const {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                return frame->getAngularVelocity();
            }
        }
        return 0.0f;
    }

    void SoftGuyComponent::setRotation(float rotation) {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->setRotation(rotation);
            }
        }
    }

    float SoftGuyComponent::getRotation() const {
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                return frame->getRotation();
            }
        }
        return 0.0f;
    }

    void SoftGuyComponent::setStatic(bool isStatic) {
        m_isStatic = isStatic;
        
        // Set all nodes to static
        for (auto* nodeEntity : m_nodes) {
            auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>();
            if (node) {
                node->setPositionFixed(isStatic);
            }
        }
    }

    bool SoftGuyComponent::isStatic() const {
        return m_isStatic;
    }

    void SoftGuyComponent::addForce(const Vec2& force) {
        // Distribute force among all nodes
        Vec2 forcePerNode = force * (1.0f / m_nodes.size());
        
        for (auto* nodeEntity : m_nodes) {
            auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>();
            if (node && !node->isPositionFixed()) {
                node->addExternalForce(forcePerNode);
            }
        }
    }

    void SoftGuyComponent::addTorque(float torque) {
        // Apply torque by adding forces to nodes based on their distance from center
        Vec2 center = getPosition();
        
        for (auto* nodeEntity : m_nodes) {
            auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>();
            if (node && !node->isPositionFixed()) {
                Vec2 nodePos = node->getPosition();
                Vec2 offset = nodePos - center;
                Vec2 tangent = Vec2(-offset.y, offset.x).normalized();
                Vec2 force = tangent * (torque / offset.length());
                node->addExternalForce(force);
            }
        }
    }

    void SoftGuyComponent::setConfig(const SoftGuyConfig& config) {
        m_config = config;
        
        // Update all beams with new configuration
        for (auto* beamEntity : m_beams) {
            auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>();
            if (beam) {
                beam->setStiffness(config.stiffness);
                beam->setDamping(config.damping);
                beam->setMaxForce(config.maxForce);
            }
        }
    }

    SoftGuyConfig SoftGuyComponent::getConfig() const {
        return m_config;
    }

    bool SoftGuyComponent::isBroken() const {
        // Check if any beam is broken
        for (auto* beamEntity : m_beams) {
            auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>();
            if (beam && beam->isBroken()) {
                return true;
            }
        }
        return false;
    }

    void SoftGuyComponent::reset() {
        // Reset frame
        if (m_frameEntity) {
            auto* frame = m_frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->reset();
            }
        }
        
        // Reset all nodes
        for (auto* nodeEntity : m_nodes) {
            auto* node = nodeEntity->getComponent<SpringGuyNodeComponent>();
            if (node) {
                node->setPosition(node->getStartingPosition());
                node->setVelocity(Vec2(0.0f, 0.0f));
                node->clearExternalForces();
            }
        }
        
        // Reset all beams
        for (auto* beamEntity : m_beams) {
            auto* beam = beamEntity->getComponent<SpringGuyBeamComponent>();
            if (beam) {
                beam->resetBeam();
            }
        }
        
        m_isBroken = false;
    }

    void SoftGuyComponent::destroy() {
        // Remove all entities
        if (m_frameEntity) {
            m_entityManager->removeEntity(m_frameEntity->getName());
        }
        
        for (auto* nodeEntity : m_nodes) {
            m_entityManager->removeEntity(nodeEntity->getName());
        }
        
        for (auto* beamEntity : m_beams) {
            m_entityManager->removeEntity(beamEntity->getName());
        }
        
        m_frameEntity = nullptr;
        m_nodes.clear();
        m_beams.clear();
    }

    void SoftGuyComponent::setVisible(bool visible) {
        m_isVisible = visible;
        updateVisuals();
    }

    bool SoftGuyComponent::isVisible() const {
        return m_isVisible;
    }

    void SoftGuyComponent::updateVisuals() {
        // This would update sprite visibility and colors based on m_config
        // Implementation depends on how you want to handle visual updates
    }

    // SoftGuySystem Implementation
    void SoftGuySystem::update(EntityManager& em, float dt) {
        // Update all SpringGuy components (nodes and beams)
        SpringGuySystem::updateNodes(em, dt);
        SpringGuySystem::updateBeams(em, dt);
        
        // Update frames with gravity
        auto frameEntities = em.getEntitiesWithComponent<FrameComponent>();
        for (auto* frameEntity : frameEntities) {
            auto* frame = frameEntity->getComponent<FrameComponent>();
            if (frame) {
                // Apply gravity to frame by updating velocity
                Vec2 currentVel = frame->getVelocity();
                currentVel.y += s_gravity * dt;
                frame->setVelocity(currentVel);
                
                // Update position
                Vec2 currentPos = frame->getPosition();
                frame->setPosition(currentPos + currentVel * dt);
            }
        }
    }

    void SoftGuySystem::resetAll(EntityManager& em) {
        SpringGuySystem::resetPhysics(em);
        
        // Reset all frames
        auto frameEntities = em.getEntitiesWithComponent<FrameComponent>();
        for (auto* frameEntity : frameEntities) {
            auto* frame = frameEntity->getComponent<FrameComponent>();
            if (frame) {
                frame->reset();
            }
        }
    }

    void SoftGuySystem::setGravity(float gravity) {
        s_gravity = gravity;
    }

    float SoftGuySystem::getGravity() {
        return s_gravity;
    }
}
