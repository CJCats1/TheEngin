#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    class ColliderComponent {
    public:
        ColliderComponent(float width, float height)
            : m_width(width), m_height(height) {
        }

        bool containsPoint(const Vec2& worldPoint, const Vec3& spritePosition) const {
            float halfWidth = m_width * 0.5f;
            float halfHeight = m_height * 0.5f;

            return (worldPoint.x >= spritePosition.x - halfWidth &&
                worldPoint.x <= spritePosition.x + halfWidth &&
                worldPoint.y >= spritePosition.y - halfHeight &&
                worldPoint.y <= spritePosition.y + halfHeight);
        }

        float getWidth() const { return m_width; }
        float getHeight() const { return m_height; }

    private:
        float m_width;
        float m_height;
    };
}
