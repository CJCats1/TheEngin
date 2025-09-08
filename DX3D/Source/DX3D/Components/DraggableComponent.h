#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    class DraggableComponent {
    public:
        DraggableComponent() = default;

        bool isDragging() const { return m_isDragging; }
        void setDragging(bool dragging) { m_isDragging = dragging; }

        Vec2 getDragOffset() const { return m_dragOffset; }
        void setDragOffset(const Vec2& offset) { m_dragOffset = offset; }

        Vec3 getOriginalPosition() const { return m_originalPosition; }
        void setOriginalPosition(const Vec3& pos) { m_originalPosition = pos; }

        bool isEnabled() const { return m_enabled; }
        void setEnabled(bool enabled) { m_enabled = enabled; }

    private:
        bool m_isDragging = false;
        bool m_enabled = true;
        Vec2 m_dragOffset{ 0, 0 }; // Offset from mouse to sprite center when dragging starts
        Vec3 m_originalPosition{ 0, 0, 0 }; // Position when drag started
    };
}