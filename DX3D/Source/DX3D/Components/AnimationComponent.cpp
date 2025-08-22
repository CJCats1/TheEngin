#include <DX3D/Components/AnimationComponent.h>
#include <DX3D/Core/Entity.h>
#include <DX3D/Graphics/SpriteComponent.h>

namespace dx3d {
    void MovementComponent::update(Entity& entity, float dt) {
        if (auto* sprite = entity.getComponent<SpriteComponent>()) {
            Vec2 movement = m_velocity * dt;
            sprite->translate(movement);
        }
    }
}