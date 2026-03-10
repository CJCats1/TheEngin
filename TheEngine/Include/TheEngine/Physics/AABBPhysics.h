#pragma once
#include <TheEngine/Math/Geometry.h>
#include <TheEngine/Core/Export.h>
#include <memory>
#include <vector>

namespace TheEngine {
    namespace physics {
        // Constants
        constexpr float EPSILON = 1e-8f;

        // Helper functions
        inline float abs(float value) {
            return value < 0 ? -value : value;
        }

        inline float sign(float value) {
            return value < 0 ? -1.0f : 1.0f;
        }

        // Forward declaration
        struct AABB;

        // Hit result from intersection tests
        struct Hit {
            const AABB* collider;
            Vec2 pos;
            Vec2 delta;
            Vec2 normal;
            float time;

            Hit(const AABB* collider) 
                : collider(collider), pos(0.0f, 0.0f), delta(0.0f, 0.0f), 
                  normal(0.0f, 0.0f), time(0.0f) {}
        };

        // Sweep result from sweep tests
        struct Sweep {
            std::unique_ptr<Hit> hit;
            Vec2 pos;
            float time;

            Sweep() : hit(nullptr), pos(0.0f, 0.0f), time(1.0f) {}
        };

        // AABB with physics collision methods
        // Based on: https://noonat.github.io/intersect/#aabb-vs-aabb
        struct THEENGINE_API AABB {
            Vec2 pos;   // Center position (alias for center)
            Vec2 half;  // Half-extents (alias for halfSize)

            AABB() : pos(0.0f, 0.0f), half(0.0f, 0.0f) {}
            AABB(const Vec2& center, const Vec2& halfSize) : pos(center), half(halfSize) {}
            AABB(const Vec2& center, float width, float height) 
                : pos(center), half(width * 0.5f, height * 0.5f) {}

            // Construct from existing AABB (from Geometry.h)
            AABB(const TheEngine::AABB& aabb) : pos(aabb.center), half(aabb.halfSize) {}

            // Convert to TheEngine::AABB
            TheEngine::AABB toAABB() const {
                return TheEngine::AABB(pos, half);
            }

            // AABB vs Point intersection test
            // Returns Hit if point is inside AABB, null otherwise
            std::unique_ptr<Hit> intersectPoint(const Vec2& point) const;

            // AABB vs Segment (raycast) intersection test
            // Tests if a line segment intersects with this AABB
            std::unique_ptr<Hit> intersectSegment(const Vec2& pos, const Vec2& delta, 
                                                  float paddingX = 0.0f, float paddingY = 0.0f) const;

            // AABB vs AABB intersection test
            // Returns Hit if two AABBs overlap, null otherwise
            std::unique_ptr<Hit> intersectAABB(const AABB& box) const;

            // AABB vs Circle intersection test
            // Returns Hit if circle overlaps, null otherwise
            std::unique_ptr<Hit> intersectCircle(const Vec2& center, float radius) const;

            // AABB vs Swept AABB test
            // Tests if a moving AABB collides with this static AABB
            Sweep sweepAABB(const AABB& box, const Vec2& delta) const;

            // Sweep this AABB through multiple static colliders
            // Returns the nearest collision along the movement path
            Sweep sweepInto(const std::vector<AABB>& staticColliders, const Vec2& delta) const;
        };

    }
}

