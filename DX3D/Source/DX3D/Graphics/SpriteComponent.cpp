#include "SpriteComponent.h"

namespace dx3d {

    SpriteComponent::SpriteComponent(GraphicsDevice& device, const std::wstring& texturePath,
        float width, float height) : m_device(device) {
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
        : m_device(device),m_texture(texture)  {
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

    void SpriteComponent::setTexture(std::shared_ptr<Texture2D> texture) {
        m_texture = texture;
        if (m_mesh && m_texture) {
            m_mesh->setTexture(m_texture);
        }
    }

    void SpriteComponent::draw(DeviceContext& ctx) const {
        if (isVisible() && isValid()) {
            // Enable alpha blending for this sprite
            ctx.enableAlphaBlending();
            ctx.enableTransparentDepth(); // optional if you want depth test but no depth writes

            // Set world matrix using the transform component
            ctx.setWorldMatrix(getWorldMatrix());
            ctx.setTint(m_tint);
            // Draw the mesh (handles texture binding internally)
            m_mesh->draw(ctx);

            // Disable alpha blending afterward (so other objects are not affected)
            ctx.disableAlphaBlending();
            ctx.enableDefaultDepth(); // reset depth state if you changed it
        }
    }
}