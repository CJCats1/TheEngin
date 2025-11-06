/*MIT License

C++ 3D Game Tutorial Series (https://github.com/PardCode/CPP-3D-Game-Tutorial-Series)

Copyright (c) 2019-2025, PardCode

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/Entity.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Components/SpringGuyComponent.h>
#include <vector>
#include <memory>

namespace dx3d {
    class Entity;
    class SoftGuySystem;
    class FrameComponent; // Forward declaration

    // Soft body configuration
    struct SoftGuyConfig {
        float stiffness = 1000.0f;
        float damping = 80.0f;
        float maxForce = 1000.0f;
        float nodeMass = 1.0f;
        float nodeRadius = 14.0f;
        Vec4 nodeColor = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
        Vec4 beamColor = Vec4(0.0f, 0.8f, 0.0f, 0.8f);
        bool showBeams = true;
        bool showNodes = true;
    };

    // Soft body component that combines FrameComponent with SpringGuy components
    class SoftGuyComponent {
    public:
        SoftGuyComponent() = default;
        
        // Factory methods for easy soft body creation
        static SoftGuyComponent* createCircle(EntityManager& em, const std::string& name, Vec2 position, float radius, int segments = 8, const SoftGuyConfig& config = SoftGuyConfig());
        static SoftGuyComponent* createRectangle(EntityManager& em, const std::string& name, Vec2 position, Vec2 size, int segmentsX = 4, int segmentsY = 4, const SoftGuyConfig& config = SoftGuyConfig());
        static SoftGuyComponent* createTriangle(EntityManager& em, const std::string& name, Vec2 position, float size, const SoftGuyConfig& config = SoftGuyConfig());
        static SoftGuyComponent* createLine(EntityManager& em, const std::string& name, Vec2 start, Vec2 end, int segments = 3, const SoftGuyConfig& config = SoftGuyConfig());
        
        // Manual creation for custom shapes
        static SoftGuyComponent* createCustom(EntityManager& em, const std::string& name, Vec2 position, const std::vector<Vec2>& nodePositions, const std::vector<std::pair<int, int>>& connections, const SoftGuyConfig& config = SoftGuyConfig());
        
        // SoftGuy management
        void setPosition(const Vec2& position);
        Vec2 getPosition() const;
        void setVelocity(const Vec2& velocity);
        Vec2 getVelocity() const;
        void setAngularVelocity(float angularVelocity);
        float getAngularVelocity() const;
        void setRotation(float rotation);
        float getRotation() const;
        
        // Physics control
        void setStatic(bool isStatic);
        bool isStatic() const;
        void addForce(const Vec2& force);
        void addTorque(float torque);
        
        // Configuration
        void setConfig(const SoftGuyConfig& config);
        SoftGuyConfig getConfig() const;
        
        // Node and beam access
        const std::vector<Entity*>& getNodes() const { return m_nodes; }
        const std::vector<Entity*>& getBeams() const { return m_beams; }
        Entity* getFrame() const { return m_frameEntity; }
        
        // Soft body state
        bool isBroken() const;
        void reset();
        void destroy();
        
        // Visual control
        void setVisible(bool visible);
        bool isVisible() const;
        
    private:
        SoftGuyComponent(EntityManager& em, const std::string& name, Vec2 position, const SoftGuyConfig& config);
        
        void createFrame(EntityManager& em, const std::string& name, Vec2 position);
        void createNodes(EntityManager& em, const std::string& baseName, const std::vector<Vec2>& positions);
        void createBeams(EntityManager& em, const std::string& baseName, const std::vector<std::pair<int, int>>& connections);
        void updateVisuals();
        
        EntityManager* m_entityManager;
        std::string m_name;
        SoftGuyConfig m_config;
        
        Entity* m_frameEntity;
        std::vector<Entity*> m_nodes;
        std::vector<Entity*> m_beams;
        
        bool m_isStatic;
        bool m_isVisible;
        bool m_isBroken;
    };

    // System to manage SoftGuy physics
    class SoftGuySystem {
    public:
        static void update(EntityManager& em, float dt);
        static void resetAll(EntityManager& em);
        static void setGravity(float gravity);
        static float getGravity();
        
    private:
        static float s_gravity;
    };
}
