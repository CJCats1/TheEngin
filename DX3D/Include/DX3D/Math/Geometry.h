#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Types.h>
#include <vector>
#include <string>
#include <algorithm>

namespace dx3d {
    static float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    struct MeshDimensions {
        f32 width{ 0 }, height{ 0 }, depth{ 0 };
    };

    enum class Primitive {
        Triangles,
        Lines
    };

    class Rect
    {
    public:
        Rect() = default;
        Rect(i32 width, i32 height) : left(0), top(0), width(width), height(height) {}
        Rect(i32 left, i32 top, i32 width, i32 height) : left(left), top(top), width(width), height(height) {}
    public:
        i32 left{}, top{}, width{}, height{};
    };

    struct Vertex
    {
        Vec3 pos;
        Vec3 normal;
        Vec2 uv;
        Vec4 color;
    };
}

namespace dx3d {
    namespace geom {
        struct HalfPlane { Vec2 n; float d; };

        float cross(const Vec2& o, const Vec2& a, const Vec2& b);

        std::vector<Vec2> clipPolygonWithHalfPlane(
            const std::vector<Vec2>& poly,
            const HalfPlane& hp
        );

        std::vector<Vec2> computeVoronoiCell(
            const Vec2& site,
            const std::vector<Vec2>& allSites,
            const Vec2& boundsCenter,
            const Vec2& boundsSize
        );

        std::vector<Vec2> computeConvexHull(const std::vector<Vec2>& points);
    }

    struct AABB {
        Vec2 center;
        Vec2 halfSize;

        AABB() : center(0.0f, 0.0f), halfSize(0.0f, 0.0f) {}
        AABB(const Vec2& center, const Vec2& halfSize) : center(center), halfSize(halfSize) {}
        AABB(const Vec2& center, float width, float height)
            : center(center), halfSize(width * 0.5f, height * 0.5f) {}

        Vec2 getMin() const { return center - halfSize; }
        Vec2 getMax() const { return center + halfSize; }
        
        float getWidth() const { return halfSize.x * 2.0f; }
        float getHeight() const { return halfSize.y * 2.0f; }

        float getLeft() const { return center.x - halfSize.x; }
        float getRight() const { return center.x + halfSize.x; }
        float getTop() const { return center.y + halfSize.y; }
        float getBottom() const { return center.y - halfSize.y; }

        bool intersects(const AABB& other) const {
            bool AisToTheRightOfB = getLeft() > other.getRight();
            bool AisToTheLeftOfB = getRight() < other.getLeft();
            bool AisAboveB = getBottom() < other.getTop();
            bool AisBelowB = getTop() > other.getBottom();
            
            return !(AisToTheRightOfB
                || AisToTheLeftOfB
                || AisAboveB
                || AisBelowB);
        }

        bool contains(const Vec2& point) const {
            Vec2 min = getMin();
            Vec2 max = getMax();
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y;
        }
    };
}