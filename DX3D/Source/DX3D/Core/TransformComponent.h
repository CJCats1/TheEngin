#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    class TransformComponent {
    public:
        TransformComponent() = default;
        TransformComponent(const Vec3& position, const Vec3& rotation = Vec3(0.0f, 0.0f, 0.0f), const Vec3& scale = Vec3(1.0f, 1.0f, 1.0f))
            : m_position(position), m_rotation(rotation), m_scale(scale) {
        }

        // Position
        void setPosition(f32 x, f32 y, f32 z = 0.0f) {
            m_position = Vec3(x, y, z);
            m_dirty = true;
        }
        void setPosition(const Vec3& pos) {
            m_position = pos;
            m_dirty = true;
        }
        void setPosition(const Vec2& pos) {
            m_position = Vec3(pos.x, pos.y, m_position.z);
            m_dirty = true;
        }

        void translate(f32 x, f32 y, f32 z = 0.0f) {
            m_position.x += x;
            m_position.y += y;
            m_position.z += z;
            m_dirty = true;
        }
        void translate(const Vec3& delta) {
            m_position = m_position + delta;
            m_dirty = true;
        }
        void translate(const Vec2& delta) {
            m_position.x += delta.x;
            m_position.y += delta.y;
            m_dirty = true;
        }

        // Rotation (in radians)
        void setRotation(f32 x, f32 y = 0.0f, f32 z = 0.0f) {
            m_rotation = Vec3(x, y, z);
            m_dirty = true;
        }
        void setRotation(const Vec3& rot) {
            m_rotation = rot;
            m_dirty = true;
        }
        void setRotationZ(f32 z) {
            m_rotation.z = z;
            m_dirty = true;
        }

        void rotate(f32 x, f32 y = 0.0f, f32 z = 0.0f) {
            m_rotation.x += x;
            m_rotation.y += y;
            m_rotation.z += z;
            m_dirty = true;
        }
        void rotate(const Vec3& delta) {
            m_rotation = m_rotation + delta;
            m_dirty = true;
        }
        void rotateZ(f32 deltaZ) {
            m_rotation.z += deltaZ;
            m_dirty = true;
        }

        // Scale
        void setScale(f32 x, f32 y, f32 z = 1.0f) {
            m_scale = Vec3(x, y, z);
            m_dirty = true;
        }
        void setScale(const Vec3& scale) {
            m_scale = scale;
            m_dirty = true;
        }
        void setScale(f32 uniformScale) {
            m_scale = Vec3(uniformScale, uniformScale, uniformScale);
            m_dirty = true;
        }
        void setScale2D(f32 uniformScale) {
            m_scale = Vec3(uniformScale, uniformScale, 1.0f);
            m_dirty = true;
        }

        void scaleBy(f32 factor) {
            m_scale = m_scale * factor;
            m_dirty = true;
        }
        void scaleBy(const Vec3& factor) {
            m_scale.x *= factor.x;
            m_scale.y *= factor.y;
            m_scale.z *= factor.z;
            m_dirty = true;
        }

        // Getters
        const Vec3& getPosition() const { return m_position; }
        const Vec3& getRotation() const { return m_rotation; }
        const Vec3& getScale() const { return m_scale; }

        Vec2 getPosition2D() const { return Vec2{ m_position.x, m_position.y }; }
        f32 getRotationZ() const { return m_rotation.z; }

        // Matrix operations
        Mat4 getWorldMatrix() const {
            if (m_dirty) {
                updateMatrix();
            }
            return m_worldMatrix;
        }

        // Alternative matrix calculation for different transform orders
        Mat4 getWorldMatrix2D() const {
            if (m_dirty) {
                updateMatrix2D();
            }
            return m_worldMatrix;
        }

        // Reset to identity
        void reset() {
            m_position = Vec3(0.0f, 0.0f, 0.0f);
            m_rotation = Vec3(0.0f, 0.0f, 0.0f);
            m_scale = Vec3(1.0f, 1.0f, 1.0f);
            m_dirty = true;
        }

        // Force matrix recalculation
        void markDirty() { m_dirty = true; }

    private:
        Vec3 m_position = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 m_rotation = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 m_scale = Vec3(1.0f, 1.0f, 1.0f);

        mutable Mat4 m_worldMatrix = Mat4::identity();
        mutable bool m_dirty = true;

        void updateMatrix() const {
            Mat4 scaleMatrix = Mat4::scale(m_scale);
            Mat4 rotationMatrix = Mat4::rotationX(m_rotation.x) *
                Mat4::rotationY(m_rotation.y) *
                Mat4::rotationZ(m_rotation.z);
            Mat4 translationMatrix = Mat4::translation(m_position);

            // Standard transform order: Scale -> Rotate -> Translate
            m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
            m_dirty = false;
        }

        void updateMatrix2D() const {
            // Optimized for 2D transforms (only Z rotation)
            Mat4 scaleMatrix = Mat4::scale(m_scale);
            Mat4 rotationMatrix = Mat4::rotationZ(m_rotation.z);
            Mat4 translationMatrix = Mat4::translation(m_position);

            m_worldMatrix = scaleMatrix* rotationMatrix   * translationMatrix;
            m_dirty = false;
        }
    };
}