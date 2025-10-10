#include <DX3D/Components/Physics3DComponent.h>
#include <algorithm>

namespace dx3d {

void Physics3DComponent::update(float dt) {
    // Apply input forces
    m_acceleration += m_inputForce / m_mass;
    
    // Apply gravity
    m_acceleration.y += m_gravity;
    
    // Update velocity and position
    m_velocity += m_acceleration * dt;
    m_position += m_velocity * dt;
    
    // Apply friction
    m_velocity *= m_friction;
    
    // Reset acceleration for next frame
    m_acceleration = Vec3(0.0f, 0.0f, 0.0f);
    m_inputForce = Vec3(0.0f, 0.0f, 0.0f);
}

bool Physics3DComponent::checkCollision(const Vec3& otherPos, float otherRadius) const {
    float distance = (m_position - otherPos).length();
    return distance < (m_radius + otherRadius);
}

void Physics3DComponent::handleCollision(const Vec3& collisionNormal, float penetration) {
    // Move out of collision
    m_position += collisionNormal * penetration;
    
    // Reflect velocity
    float velocityDotNormal = m_velocity.dot(collisionNormal);
    if (velocityDotNormal < 0) { // Only bounce if moving towards the surface
        m_velocity -= collisionNormal * (2.0f * velocityDotNormal * m_bounce);
    }
}

}
