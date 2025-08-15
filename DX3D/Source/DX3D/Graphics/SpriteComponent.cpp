#include "SpriteComponent.h"

namespace dx3d {

    SpriteComponent::SpriteComponent(GraphicsDevice& device, const std::wstring& texturePath,
        float width, float height) {
        // Load texture
        m_texture = Texture2D::LoadTexture2D(device.getD3DDevice(), texturePath.c_str());
        if (!m_texture) {
            // Fallback to white texture if loading fails
            m_texture = Texture2D::CreateWhiteTexture(device.getD3DDevice());
        }

        initialize(device, width, height);
    }

    SpriteComponent::SpriteComponent(GraphicsDevice& device, std::shared_ptr<Texture2D> texture,
        float width, float height)
        : m_texture(texture) {
        if (!m_texture) {
            m_texture = Texture2D::CreateWhiteTexture(device.getD3DDevice());
        }

        initialize(device, width, height);
    }

    void SpriteComponent::initialize(GraphicsDevice& device, float width, float height) {
        // Create textured quad mesh
        m_mesh = Mesh::CreateQuadTextured(device, width, height);
        if (m_mesh && m_texture) {
            m_mesh->setTexture(m_texture);
        }
    }

    Mat4 SpriteComponent::getWorldMatrix() const {
        // Create transformation matrices
        Mat4 scaleMatrix = Mat4::scale(m_scale);
        Mat4 rotationMatrixX = Mat4::rotationX(m_rotation.x);
        Mat4 rotationMatrixY = Mat4::rotationY(m_rotation.y);
        Mat4 rotationMatrixZ = Mat4::rotationZ(m_rotation.z);
        Mat4 translationMatrix = Mat4::translation(m_position);

        // Combine them: Scale * Rotation * Translation
        Mat4 rotationMatrix = rotationMatrixX * rotationMatrixY * rotationMatrixZ;
        return scaleMatrix * rotationMatrix * translationMatrix;
    }

    void SpriteComponent::setTexture(std::shared_ptr<Texture2D> texture) {
        m_texture = texture;
        if (m_mesh && m_texture) {
            m_mesh->setTexture(m_texture);
        }
    }

    void SpriteComponent::translate(float x, float y, float z) {
        m_position.x += x;
        m_position.y += y;
        m_position.z += z;
    }

    void SpriteComponent::translate(const Vec3& delta) {
        m_position = m_position + delta;
    }

    void SpriteComponent::rotate(float x, float y, float z) {
        m_rotation.x += x;
        m_rotation.y += y;
        m_rotation.z += z;
    }

    void SpriteComponent::rotate(const Vec3& delta) {
        m_rotation = m_rotation + delta;
    }

    void SpriteComponent::scaleBy(float factor) {
        m_scale = m_scale * factor;
    }

    void SpriteComponent::scaleBy(const Vec3& factor) {
        m_scale.x *= factor.x;
        m_scale.y *= factor.y;
        m_scale.z *= factor.z;
    }

    void SpriteComponent::draw(DeviceContext& ctx) const {
        if (isVisible() && isValid()) {
            m_mesh->draw(ctx);
        }
    }
}
