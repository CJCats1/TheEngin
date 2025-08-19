// SpriteComponent.h
#pragma once
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Geometry.h>
#include <memory>
#include <string>

namespace dx3d {
    class SpriteComponent {
    public:
        SpriteComponent(GraphicsDevice& device, const std::wstring& texturePath,
            float width = 1.0f, float height = 1.0f);

        // Alternative constructor for existing texture
        SpriteComponent(GraphicsDevice& device, std::shared_ptr<Texture2D> texture,
            float width = 1.0f, float height = 1.0f);

        ~SpriteComponent() = default;

        // Getters
        std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
        std::shared_ptr<Texture2D> getTexture() const { return m_texture; }
        Vec3 getPosition() const { return m_position; }
        Vec3 getRotation() const { return m_rotation; }
        Vec3 getScale() const { return m_scale; }
        Mat4 getWorldMatrix() const;  

        // Setters
        void setTexture(std::shared_ptr<Texture2D> texture);
        void setPosition(float x, float y, float z) { m_position = Vec3(x, y, z); }
        void setPosition(const Vec3& pos) { m_position = pos; }
        void setRotation(float x, float y, float z) { m_rotation = Vec3(x, y, z); }
        void setRotation(const Vec3& rot) { m_rotation = rot; }
        void setScale(float x, float y, float z) { m_scale = Vec3(x, y, z); }
        void setScale(const Vec3& scale) { m_scale = scale; }
        void setScale(float uniformScale) { m_scale = Vec3(uniformScale, uniformScale, uniformScale); }

        // Transform operations
        void translate(float x, float y, float z);
        void translate(const Vec3& delta);  
        void rotate(float x, float y, float z);
        void rotate(const Vec3& delta);    
        void scaleBy(float factor);        
        void scaleBy(const Vec3& factor);

        // Utility
        bool isValid() const { return m_mesh && m_texture; }
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        // Rendering
        void draw(DeviceContext& ctx) const;

    private:
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Texture2D> m_texture;

        // Transform data
        Vec3 m_position = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 m_rotation = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 m_scale = Vec3(1.0f, 1.0f, 1.0f);

        bool m_visible = true;

        void initialize(GraphicsDevice& device, float width, float height);
    };
}
