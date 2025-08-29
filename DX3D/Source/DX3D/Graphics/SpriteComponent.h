// SpriteComponent.h
#pragma once
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/TransformComponent.h>
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

        // Transform access (direct access to transform component)
        TransformComponent& transform() { return m_transform; }
        const TransformComponent& transform() const { return m_transform; }

        // Convenience methods that delegate to transform
        void setPosition(float x, float y, float z = 0.0f) { m_transform.setPosition(x, y, z); }
        void setPosition(const Vec3& pos) { m_transform.setPosition(pos); }
        void setPosition(const Vec2& pos) { m_transform.setPosition(pos); }

        void translate(float x, float y, float z = 0.0f) { m_transform.translate(x, y, z); }
        void translate(const Vec3& delta) { m_transform.translate(delta); }
        void translate(const Vec2& delta) { m_transform.translate(delta); }

        void setRotation(float x, float y = 0.0f, float z = 0.0f) { m_transform.setRotation(x, y, z); }
        void setRotation(const Vec3& rot) { m_transform.setRotation(rot); }
        void setRotationZ(float z) { m_transform.setRotationZ(z); }

        void rotate(float x, float y = 0.0f, float z = 0.0f) { m_transform.rotate(x, y, z); }
        void rotate(const Vec3& delta) { m_transform.rotate(delta); }
        void rotateZ(float deltaZ) { m_transform.rotateZ(deltaZ); }

        void setScale(float x, float y, float z = 1.0f) { m_transform.setScale(x, y, z); }
        void setScale(const Vec3& scale) { m_transform.setScale(scale); }
        void setScale(float uniformScale) { m_transform.setScale(uniformScale); }
        void setScale2D(float uniformScale) { m_transform.setScale2D(uniformScale); }

        void scaleBy(float factor) { m_transform.scaleBy(factor); }
        void scaleBy(const Vec3& factor) { m_transform.scaleBy(factor); }

        // Getters that delegate to transform
        Vec3 getPosition() const { return m_transform.getPosition(); }
        Vec3 getRotation() const { return m_transform.getRotation(); }
        Vec3 getScale() const { return m_transform.getScale(); }
        Vec2 getPosition2D() const { return m_transform.getPosition2D(); }
        float getRotationZ() const { return m_transform.getRotationZ(); }
        Mat4 getWorldMatrix() const { return m_transform.getWorldMatrix2D(); } // Use 2D optimized version

        // Mesh and texture accessors
        std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
        std::shared_ptr<Texture2D> getTexture() const { return m_texture; }
        void setTexture(std::shared_ptr<Texture2D> texture);

        // Utility
        bool isValid() const { return m_mesh && m_texture; }
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        // Rendering
        void draw(DeviceContext& ctx) const;
        GraphicsDevice& getGraphicsDevice() { return m_device; }
        void setTint(const Vec4& tint) { m_tint = tint; }
        Vec4 getTint() const { return m_tint; }

        bool m_useScreenSpace = false;   // default: world space
        Vec2 m_screenPosition = { 0, 0 };  // pixel coordinates when in screen space

        void setScreenPosition(float x, float y) { m_screenPosition = { x, y }; }
        Vec2 getScreenPosition() const { return m_screenPosition; }
        void enableScreenSpace(bool enable = true) {
            m_useScreenSpace = enable;
        }

        bool isScreenSpace() const {
            return m_useScreenSpace;
        }
		float getWidth() const { return m_width; }
		float getHeight() const { return m_height; }
    private:
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Texture2D> m_texture;
        TransformComponent m_transform;
        bool m_visible = true;
        GraphicsDevice& m_device;
        Vec4 m_tint = { 1,1,1,0 };
        // inside SpriteComponent class (private:)
        float m_width = 1.0f;
        float m_height = 1.0f;

        void initialize(GraphicsDevice& device, float width, float height);
    };
}