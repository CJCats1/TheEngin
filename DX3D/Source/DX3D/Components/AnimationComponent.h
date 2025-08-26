#pragma once
#include <DX3D/Math/Geometry.h>
#include <functional>

namespace dx3d {
    class Entity;

    class AnimationComponent {
    public:
        AnimationComponent() = default;

        // Set a custom update function
        void setUpdateFunction(std::function<void(Entity&, float)> updateFunc) {
            m_updateFunction = updateFunc;
        }
        // Update this component
        void update(Entity& entity, float dt) {
            if (m_updateFunction) {
                m_updateFunction(entity, dt);
            }
        }
    private:
        std::function<void(Entity&, float)> m_updateFunction;
    };
    // Movement component for simple movement behaviors
    class MovementComponent {
    public:
        MovementComponent(float speed = 100.0f) : m_speed(speed) {}

        void setSpeed(float speed) { m_speed = speed; }
        float getSpeed() const { return m_speed; }

        void setVelocity(const Vec2& velocity) { m_velocity = velocity; }
        const Vec2& getVelocity() const { return m_velocity; }

        void update(Entity& entity, float dt);

    private:
        float m_speed;
        Vec2 m_velocity{ 0.0f, 0.0f };
    };
}