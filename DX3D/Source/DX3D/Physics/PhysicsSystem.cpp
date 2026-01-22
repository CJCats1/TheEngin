#include <DX3D/Physics/PhysicsSystem.h>

namespace dx3d {
    namespace physics {
        Vec2 PhysicsSystem::moveAABB(const AABB& movingBox, const Vec2& delta,
                                     const std::vector<AABB>& staticColliders) {
            if (staticColliders.empty()) {
                return Vec2(movingBox.pos.x + delta.x, movingBox.pos.y + delta.y);
            }

            Sweep sweep = movingBox.sweepInto(staticColliders, delta);
            return sweep.pos;
        }

        bool PhysicsSystem::checkCollision(const AABB& box, const std::vector<AABB>& staticColliders) {
            for (const auto& collider : staticColliders) {
                auto hit = box.intersectAABB(collider);
                if (hit) {
                    return true;
                }
            }
            return false;
        }

        std::unique_ptr<Hit> PhysicsSystem::getFirstCollision(const AABB& box,
                                                              const std::vector<AABB>& staticColliders) {
            for (const auto& collider : staticColliders) {
                auto hit = box.intersectAABB(collider);
                if (hit) {
                    return hit;
                }
            }
            return nullptr;
        }
    }
}
