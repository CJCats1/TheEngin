#pragma once
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    class Camera {
    public:
        Camera();

        // Camera positioning and orientation
        void setPosition(float x, float y, float z) { m_position = Vec3(x, y, z); updateViewMatrix(); }
        void setPosition(const Vec3& pos) { m_position = pos; updateViewMatrix(); }
        void lookAt(const Vec3& target, const Vec3& up = Vec3(0, 1, 0));

        // Movement
        void translate(float x, float y, float z);
        void translate(const Vec3& delta);

        // Projection settings
        void setOrthographic(float width, float height, float nearPlane = 0.1f, float farPlane = 100.0f);
        void setPerspective(float fovY, float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f);

        // Getters
        const Vec3& getPosition() const { return m_position; }
        const Vec3& getTarget() const { return m_target; }
        const Vec3& getUp() const { return m_up; }
        const Mat4& getViewMatrix() const { return m_viewMatrix; }
        const Mat4& getProjectionMatrix() const { return m_projectionMatrix; }

        // For 2D sprite rendering, useful helper
        void setup2D(float screenWidth, float screenHeight);

    private:
        Vec3 m_position;
        Vec3 m_target;
        Vec3 m_up;

        Mat4 m_viewMatrix;
        Mat4 m_projectionMatrix;

        void updateViewMatrix();
    };
}