#include <TheEngine/Physics/AABBPhysics.h>
#include <algorithm>
#include <cmath>

namespace TheEngine {
    namespace physics {
        // AABB vs Point intersection test
        std::unique_ptr<Hit> AABB::intersectPoint(const Vec2& point) const {
            float dx = point.x - pos.x;
            float px = half.x - abs(dx);
            if (px <= 0) {
                return nullptr;
            }

            float dy = point.y - pos.y;
            float py = half.y - abs(dy);
            if (py <= 0) {
                return nullptr;
            }

            auto hit = std::make_unique<Hit>(this);
            if (px < py) {
                float sx = sign(dx);
                hit->delta.x = px * sx;
                hit->normal.x = sx;
                hit->pos.x = pos.x + (half.x * sx);
                hit->pos.y = point.y;
            } else {
                float sy = sign(dy);
                hit->delta.y = py * sy;
                hit->normal.y = sy;
                hit->pos.x = point.x;
                hit->pos.y = pos.y + (half.y * sy);
            }
            return hit;
        }

        // AABB vs Segment (raycast) intersection test
        std::unique_ptr<Hit> AABB::intersectSegment(const Vec2& pos, const Vec2& delta, 
                                                     float paddingX, float paddingY) const {
            // Handle zero delta case - treat as point test
            if (delta.x == 0.0f && delta.y == 0.0f) {
                return intersectPoint(pos);
            }

            // Handle cases where one component is zero
            if (delta.x == 0.0f) {
                // Vertical line segment
                if (pos.x < this->pos.x - half.x - paddingX || pos.x > this->pos.x + half.x + paddingX) {
                    return nullptr;
                }
                float nearTimeY = (this->pos.y - sign(delta.y) * (half.y + paddingY) - pos.y) / delta.y;
                float farTimeY = (this->pos.y + sign(delta.y) * (half.y + paddingY) - pos.y) / delta.y;
                if (nearTimeY > farTimeY) std::swap(nearTimeY, farTimeY);
                if (nearTimeY >= 1.0f || farTimeY <= 0.0f) return nullptr;
                
                auto hit = std::make_unique<Hit>(this);
                hit->time = TheEngine::clamp(nearTimeY, 0.0f, 1.0f);
                hit->normal.x = 0.0f;
                hit->normal.y = -sign(delta.y);
                hit->delta.x = 0.0f;
                hit->delta.y = (1.0f - hit->time) * -delta.y;
                hit->pos.x = pos.x;
                hit->pos.y = pos.y + delta.y * hit->time;
                return hit;
            }

            if (delta.y == 0.0f) {
                // Horizontal line segment
                if (pos.y < this->pos.y - half.y - paddingY || pos.y > this->pos.y + half.y + paddingY) {
                    return nullptr;
                }
                float nearTimeX = (this->pos.x - sign(delta.x) * (half.x + paddingX) - pos.x) / delta.x;
                float farTimeX = (this->pos.x + sign(delta.x) * (half.x + paddingX) - pos.x) / delta.x;
                if (nearTimeX > farTimeX) std::swap(nearTimeX, farTimeX);
                if (nearTimeX >= 1.0f || farTimeX <= 0.0f) return nullptr;
                
                auto hit = std::make_unique<Hit>(this);
                hit->time = TheEngine::clamp(nearTimeX, 0.0f, 1.0f);
                hit->normal.x = -sign(delta.x);
                hit->normal.y = 0.0f;
                hit->delta.x = (1.0f - hit->time) * -delta.x;
                hit->delta.y = 0.0f;
                hit->pos.x = pos.x + delta.x * hit->time;
                hit->pos.y = pos.y;
                return hit;
            }

            // General case: both delta components are non-zero
            float scaleX = 1.0f / delta.x;
            float scaleY = 1.0f / delta.y;
            float signX = sign(scaleX);
            float signY = sign(scaleY);
            float nearTimeX = (this->pos.x - signX * (half.x + paddingX) - pos.x) * scaleX;
            float nearTimeY = (this->pos.y - signY * (half.y + paddingY) - pos.y) * scaleY;
            float farTimeX = (this->pos.x + signX * (half.x + paddingX) - pos.x) * scaleX;
            float farTimeY = (this->pos.y + signY * (half.y + paddingY) - pos.y) * scaleY;

            if (nearTimeX > farTimeY || nearTimeY > farTimeX) {
                return nullptr;
            }

            float nearTime = (nearTimeX > nearTimeY) ? nearTimeX : nearTimeY;
            float farTime = (farTimeX < farTimeY) ? farTimeX : farTimeY;

            if (nearTime >= 1.0f || farTime <= 0.0f) {
                return nullptr;
            }

            auto hit = std::make_unique<Hit>(this);
            hit->time = TheEngine::clamp(nearTime, 0.0f, 1.0f);
            if (nearTimeX > nearTimeY) {
                hit->normal.x = -signX;
                hit->normal.y = 0.0f;
            } else {
                hit->normal.x = 0.0f;
                hit->normal.y = -signY;
            }
            hit->delta.x = (1.0f - hit->time) * -delta.x;
            hit->delta.y = (1.0f - hit->time) * -delta.y;
            hit->pos.x = pos.x + delta.x * hit->time;
            hit->pos.y = pos.y + delta.y * hit->time;
            return hit;
        }

        // AABB vs AABB intersection test
        std::unique_ptr<Hit> AABB::intersectAABB(const AABB& box) const {
            float dx = box.pos.x - pos.x;
            float px = (box.half.x + half.x) - abs(dx);
            if (px <= 0) {
                return nullptr;
            }

            float dy = box.pos.y - pos.y;
            float py = (box.half.y + half.y) - abs(dy);
            if (py <= 0) {
                return nullptr;
            }

            auto hit = std::make_unique<Hit>(this);
            if (px < py) {
                float sx = sign(dx);
                hit->delta.x = px * sx;
                hit->normal.x = sx;
                hit->pos.x = pos.x + (half.x * sx);
                hit->pos.y = box.pos.y;
            } else {
                float sy = sign(dy);
                hit->delta.y = py * sy;
                hit->normal.y = sy;
                hit->pos.x = box.pos.x;
                hit->pos.y = pos.y + (half.y * sy);
            }
            return hit;
        }

        // AABB vs Circle intersection test
        std::unique_ptr<Hit> AABB::intersectCircle(const Vec2& center, float radius) const {
            float closestX = TheEngine::clamp(center.x, pos.x - half.x, pos.x + half.x);
            float closestY = TheEngine::clamp(center.y, pos.y - half.y, pos.y + half.y);

            Vec2 closest(closestX, closestY);
            Vec2 diff = center - closest;
            float distSq = diff.x * diff.x + diff.y * diff.y;
            float radiusSq = radius * radius;

            if (distSq > radiusSq) {
                return nullptr;
            }

            auto hit = std::make_unique<Hit>(this);

            if (distSq > EPSILON) {
                float dist = std::sqrt(distSq);
                Vec2 normal = diff / dist;
                hit->normal = normal;
                hit->pos = closest;
                hit->delta = normal * (radius - dist);
            } else {
                // Circle center is inside the box; push out along the smallest axis
                float dx = center.x - pos.x;
                float dy = center.y - pos.y;
                float px = half.x - abs(dx);
                float py = half.y - abs(dy);

                if (px < py) {
                    float sx = sign(dx);
                    hit->normal.x = sx;
                    hit->delta.x = (radius + px) * sx;
                    hit->pos.x = pos.x + half.x * sx;
                    hit->pos.y = center.y;
                } else {
                    float sy = sign(dy);
                    hit->normal.y = sy;
                    hit->delta.y = (radius + py) * sy;
                    hit->pos.x = center.x;
                    hit->pos.y = pos.y + half.y * sy;
                }
            }

            return hit;
        }

        // AABB vs Swept AABB test
        Sweep AABB::sweepAABB(const AABB& box, const Vec2& delta) const {
            Sweep sweep;

            // If the sweep isn't actually moving anywhere, just do a static test
            if (delta.x == 0.0f && delta.y == 0.0f) {
                sweep.pos.x = box.pos.x;
                sweep.pos.y = box.pos.y;
                auto hit = intersectAABB(box);
                if (hit) {
                    sweep.hit = std::move(hit);
                    sweep.hit->time = 0.0f;
                    sweep.time = 0.0f;
                } else {
                    sweep.time = 1.0f;
                }
                return sweep;
            }

            // Call into intersectSegment with the moving box's half size as padding
            sweep.hit = intersectSegment(box.pos, delta, box.half.x, box.half.y);
            if (sweep.hit) {
                sweep.time = TheEngine::clamp(sweep.hit->time - EPSILON, 0.0f, 1.0f);
                sweep.pos.x = box.pos.x + delta.x * sweep.time;
                sweep.pos.y = box.pos.y + delta.y * sweep.time;
                
                Vec2 direction = delta;
                direction.normalize();
                sweep.hit->pos.x = TheEngine::clamp(
                    sweep.hit->pos.x + direction.x * box.half.x,
                    pos.x - half.x, pos.x + half.x);
                sweep.hit->pos.y = TheEngine::clamp(
                    sweep.hit->pos.y + direction.y * box.half.y,
                    pos.y - half.y, pos.y + half.y);
            } else {
                sweep.pos.x = box.pos.x + delta.x;
                sweep.pos.y = box.pos.y + delta.y;
                sweep.time = 1.0f;
            }
            return sweep;
        }

        // Sweep this AABB through multiple static colliders
        Sweep AABB::sweepInto(const std::vector<AABB>& staticColliders, const Vec2& delta) const {
            Sweep nearest;
            nearest.time = 1.0f;
            nearest.pos.x = pos.x + delta.x;
            nearest.pos.y = pos.y + delta.y;

            for (const auto& collider : staticColliders) {
                Sweep sweep = collider.sweepAABB(*this, delta);
                if (sweep.time < nearest.time) {
                    nearest = std::move(sweep);
                }
            }
            return nearest;
        }
    }
}

