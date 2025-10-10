#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <cmath>

namespace dx3d {

Mesh3DComponent::Mesh3DComponent(std::shared_ptr<Mesh> mesh) 
    : m_mesh(mesh) {
}

Mat4 Mesh3DComponent::getWorldMatrix() const {
    // Create transformation matrix: Scale * Rotation * Translation
    Mat4 scaleMatrix = Mat4::scale(m_scale);
    Mat4 rotationMatrix = Mat4::rotationX(m_rotation.x) * 
                         Mat4::rotationY(m_rotation.y) * 
                         Mat4::rotationZ(m_rotation.z);
    Mat4 translationMatrix = Mat4::translation(m_position);
    
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void Mesh3DComponent::draw(DeviceContext& ctx) const {
    if (!m_visible || !m_mesh) return;
    
    // Set world matrix
    ctx.setWorldMatrix(getWorldMatrix());
    
    // Set material properties
    ctx.setMaterial(m_materialColor, m_shininess, m_roughness);
    
    // Draw the mesh
    m_mesh->draw(ctx);
}

}
