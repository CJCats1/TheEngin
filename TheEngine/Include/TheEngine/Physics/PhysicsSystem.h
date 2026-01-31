#pragma once
#include <TheEngine/Physics/AABBPhysics.h>
#include <TheEngine/Math/Geometry.h>
#include <vector>
#include <memory>

namespace TheEngine {
    namespace physics {
        struct CircleBody {
            Vec2 pos;
            float radius = 5.0f;
            Vec2 velocity;
            float mass = 6.0f;
            float friction = 0.18f;
            float restitution = 0.1f;
        };

        struct WorldStepConfig {
            AABB& playerBox;
            Vec2& playerVelocity;
            std::vector<AABB>& staticBoxes;
            std::vector<AABB>& dynamicBoxes;
            std::vector<Vec2>& dynamicVelocities;
            std::vector<float>& dynamicMasses;
            std::vector<float>& dynamicFrictions;
            std::vector<float>& dynamicRestitutions;
            std::vector<CircleBody>& circles;
            float gravity = 0.0f;
            float staticFriction = 0.0f;
            float staticRestitution = 0.0f;
            float playerFrictionStatic = 0.0f;
            float playerFrictionDynamic = 0.0f;
            float playerRestitution = 0.0f;
            float playerPushMass = 0.0f;
        };

        // Physics system for managing AABB collisions
        // Based on: https://noonat.github.io/intersect/#aabb-vs-aabb
        class PhysicsSystem {
        public:
            // Move an AABB through a world of static colliders
            // Returns the final position after resolving collisions
            static Vec2 moveAABB(const AABB& movingBox, const Vec2& delta, 
                                 const std::vector<AABB>& staticColliders);

            // Check if an AABB collides with any static colliders
            static bool checkCollision(const AABB& box, const std::vector<AABB>& staticColliders);

            // Get the first collision hit for an AABB
            static std::unique_ptr<Hit> getFirstCollision(const AABB& box, 
                                                          const std::vector<AABB>& staticColliders);

            // Convert TheEngine::AABB to physics::AABB
            static AABB toPhysicsAABB(const TheEngine::AABB& aabb) {
                return AABB(aabb);
            }

            // Convert physics::AABB to TheEngine::AABB
            static TheEngine::AABB toEngineAABB(const AABB& aabb) {
                return aabb.toAABB();
            }

            // Step a 2D physics world containing AABBs and circles
            static void stepWorld(WorldStepConfig& config, float dt);
        };
    }
}

#include <TheEngine/Physics/PhysicsSystem.inl>
