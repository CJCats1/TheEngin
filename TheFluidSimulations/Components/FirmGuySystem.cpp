#include <TheEngine/Components/FirmGuySystem.h>
#include <TheEngine/Components/FirmGuyComponent.h>
#include <TheEngine/Core/EntityManager.h>
#include <TheEngine/Core/Entity.h>

namespace TheEngine {
    void FirmGuySystem::update(EntityManager& manager, float dt) {
        const float gravity = -500.0f;
        for (const auto& up : manager.getEntities()) {
            auto* rb = up->getComponent<FirmGuyComponent>();
            if (!rb || rb->isStatic()) continue;
            Vec2 pos = rb->getPosition();
            Vec2 vel = rb->getVelocity();
            vel.y += gravity * dt;
            pos += vel * dt;
            rb->setPosition(pos);
            rb->setVelocity(vel);
        }
    }
}
