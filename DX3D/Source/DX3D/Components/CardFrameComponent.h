#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    // Frame component for card rigid body reference - similar to FrameComponent in PhysicsTetrisScene
    class CardFrameComponent {
    public:
        CardFrameComponent(Vec2 position = Vec2(0.0f, 0.0f), float rotation = 0.0f)
            : m_position(position), m_rotation(rotation), m_startPosition(position), m_startRotation(rotation) {
        }

        Vec2 getPosition() const { return m_position; }
        void setPosition(const Vec2& pos) { m_position = pos; }
        float getRotation() const { return m_rotation; }
        void setRotation(float rot) { m_rotation = rot; }

        Vec2 getStartPosition() const { return m_startPosition; }
        float getStartRotation() const { return m_startRotation; }

        void reset() {
            m_position = m_startPosition;
            m_rotation = m_startRotation;
        }

        // Move the frame to a new position (used when dragging cards)
        void moveTo(const Vec2& newPosition) {
            m_position = newPosition;
        }

        // Get the target position for a card based on this frame
        Vec2 getTargetPosition() const {
            return m_position; // For now, cards target the frame position directly
        }

    private:
        Vec2 m_position;
        float m_rotation;
        Vec2 m_startPosition;
        float m_startRotation;
    };
}
