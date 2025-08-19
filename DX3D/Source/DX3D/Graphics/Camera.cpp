// Camera.cpp
#include "Camera.h"
#include <cmath>

namespace dx3d {

    Camera::Camera()
        : m_position(0, 0, -1)  // Start camera back a bit
        , m_target(0, 0, 0)     // Look at origin
        , m_up(0, 1, 0)         // Y-up
    {
        updateViewMatrix();
        // Default to a reasonable 2D orthographic projection
        setOrthographic(800, 600);
    }

    void Camera::lookAt(const Vec3& target, const Vec3& up) {
        m_target = target;
        m_up = up;
        updateViewMatrix();
    }

    void Camera::translate(float x, float y, float z) {
        m_position = m_position + Vec3(x, y, z);
        m_target = m_target + Vec3(x, y, z);  // Move target with camera
        updateViewMatrix();
    }

    void Camera::translate(const Vec3& delta) {
        translate(delta.x, delta.y, delta.z);
    }

    void Camera::setOrthographic(float width, float height, float nearPlane, float farPlane) {
        // Create orthographic projection centered around origin
        m_projectionMatrix = Mat4::orthographic(width, height, nearPlane, farPlane);
    }

    void Camera::setPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane) {
        // For now, we'll implement a simple perspective matrix
        // This is a basic implementation - you might want to enhance it later
        Mat4 result;
        float tanHalfFov = std::tan(fovY * 0.5f);

        result.data()[0] = 1.0f / (aspectRatio * tanHalfFov);
        result.data()[5] = 1.0f / tanHalfFov;
        result.data()[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.data()[11] = -1.0f;
        result.data()[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result.data()[15] = 0.0f;

        m_projectionMatrix = result;
    }

    void Camera::setup2D(float screenWidth, float screenHeight) {
        // For 2D rendering, set up orthographic projection
        // Position camera so (0,0) is top-left corner of screen
        setPosition(screenWidth * 0.5f, screenHeight * 0.5f, -1.0f);
        lookAt(Vec3(screenWidth * 0.5f, screenHeight * 0.5f, 0.0f));
        setOrthographic(screenWidth, screenHeight);
    }

    void Camera::updateViewMatrix() {
        // Create a basic look-at matrix
        // This is a simplified implementation
        Vec3 forward = Vec3(
            m_target.x - m_position.x,
            m_target.y - m_position.y,
            m_target.z - m_position.z
        );

        // Normalize forward vector (simplified)
        float length = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        if (length > 0.001f) {
            forward.x /= length;
            forward.y /= length;
            forward.z /= length;
        }

        // For now, create a simple view matrix
        // This is a basic implementation that works for simple 2D cases
        Mat4 translation = Mat4::translation(Vec3(-m_position.x, -m_position.y, -m_position.z));
        m_viewMatrix = translation;
    }

}