/*MIT License

C++ 3D Game Tutorial Series (https://github.com/PardCode/CPP-3D-Game-Tutorial-Series)

Copyright (c) 2019-2025, PardCode

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {

    enum class FirmGuyShape { Circle, Rectangle };

    class FirmGuyComponent {
    public:
        FirmGuyComponent() = default;

        // shape
        void setCircle(float radius) { m_shape = FirmGuyShape::Circle; m_radius = radius; }
        void setRectangle(const Vec2& halfExtents) { m_shape = FirmGuyShape::Rectangle; m_halfExtents = halfExtents; }
        FirmGuyShape getShape() const { return m_shape; }

        // transform/kinematics
        void setPosition(const Vec2& p) { m_position = p; }
        Vec2 getPosition() const { return m_position; }
        void setVelocity(const Vec2& v) { m_velocity = v; }
        Vec2 getVelocity() const { return m_velocity; }
        void setAngle(float radians) { m_angle = radians; }
        float getAngle() const { return m_angle; }

        // physical params
        void setMass(float m) { m_mass = (m <= 0.0f ? 1.0f : m); }
        float getMass() const { return m_mass; }
        void setRestitution(float r) { m_restitution = r; }
        float getRestitution() const { return m_restitution; }
        void setFriction(float f) { m_friction = f; }
        float getFriction() const { return m_friction; }
        void setGravityScale(float g) { m_gravityScale = g; }
        float getGravityScale() const { return m_gravityScale; }
        bool isStatic() const { return m_isStatic; }
        void setStatic(bool s) { m_isStatic = s; }

        // shape params
        float getRadius() const { return m_radius; }
        Vec2 getHalfExtents() const { return m_halfExtents; }

    private:
        FirmGuyShape m_shape{ FirmGuyShape::Circle };
        Vec2 m_position{ 0.0f, 0.0f };
        Vec2 m_velocity{ 0.0f, 0.0f };
        float m_angle{ 0.0f }; // rotation for rectangles (and optional circles)

        float m_mass{ 1.0f };
        float m_restitution{ 0.2f }; // bounciness 0..1
        float m_friction{ 0.98f };   // simple velocity damping
        float m_gravityScale{ 1.0f };
        bool m_isStatic{ false };

        // shape specific
        float m_radius{ 0.5f };
        Vec2 m_halfExtents{ 0.5f, 0.5f };
    };
}


