#pragma once
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Math/Geometry.h>
#include <memory>

namespace dx3d {
    class DeviceContext; // Forward declaration
}

namespace dx3d {
    class Mesh3DComponent {
    public:
        Mesh3DComponent() = default;
        Mesh3DComponent(std::shared_ptr<Mesh> mesh);
        
        // Mesh management
        void setMesh(std::shared_ptr<Mesh> mesh) { m_mesh = mesh; }
        std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
        bool hasMesh() const { return m_mesh != nullptr; }
        
        // Transform
        void setPosition(const Vec3& position) { m_position = position; }
        Vec3 getPosition() const { return m_position; }
        
        void setRotation(const Vec3& rotation) { m_rotation = rotation; }
        Vec3 getRotation() const { return m_rotation; }
        
        void setScale(const Vec3& scale) { m_scale = scale; }
        Vec3 getScale() const { return m_scale; }
        
        // World matrix
        Mat4 getWorldMatrix() const;
        
        // Material properties
        void setMaterial(const Vec3& color, float shininess, float roughness) {
            m_materialColor = color;
            m_shininess = shininess;
            m_roughness = roughness;
        }
        
        Vec3 getMaterialColor() const { return m_materialColor; }
        float getShininess() const { return m_shininess; }
        float getRoughness() const { return m_roughness; }
        
        // Visibility
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }
        
        // Rendering
        void draw(DeviceContext& ctx) const;
        
    private:
        std::shared_ptr<Mesh> m_mesh;
        Vec3 m_position{ 0.0f, 0.0f, 0.0f };
        Vec3 m_rotation{ 0.0f, 0.0f, 0.0f };
        Vec3 m_scale{ 1.0f, 1.0f, 1.0f };
        Vec3 m_materialColor{ 1.0f, 1.0f, 1.0f };
        float m_shininess{ 64.0f };
        float m_roughness{ 0.2f };
        bool m_visible{ true };
    };
}
