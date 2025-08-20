// Fixed Camera.cpp with proper 2D setup
#include "Camera.h"
#include <cmath>

namespace dx3d {
    Camera::Camera()
        : m_position(0, 0, 5)  // Changed: Position camera in front of sprites
        , m_target(0, 0, 0)
        , m_up(0, 1, 0)
    {
        updateViewMatrix();
        setOrthographic(800, 600);
    }

    void Camera::lookAt(const Vec3& target, const Vec3& up) {
        m_target = target;
        m_up = up;
        updateViewMatrix();
    }

    void Camera::translate(float x, float y, float z) {
        Vec3 delta(x, y, z);
        m_position = m_position + delta;
        m_target = m_target + delta;
        updateViewMatrix();
    }

    void Camera::translate(const Vec3& delta) {
        translate(delta.x, delta.y, delta.z);
    }

    void Camera::setOrthographic(float width, float height, float nearPlane, float farPlane) {
        // Fixed orthographic projection matrix
        float scale = 200.0f; // Same scale factor
        float left = -width / (2.0f * scale);   // Fixed: divide by 2 for proper centering
        float right = width / (2.0f * scale);
        float bottom = -height / (2.0f * scale);
        float top = height / (2.0f * scale);

        Mat4 result;
        float* data = result.data();

        // Initialize to zero
        for (int i = 0; i < 16; i++) data[i] = 0.0f;

        // DirectX orthographic projection matrix (column-major)
        // Column 0 (X scale)
        data[0] = 2.0f / (right - left);
        data[1] = 0.0f;
        data[2] = 0.0f;
        data[3] = 0.0f;

        // Column 1 (Y scale)  
        data[4] = 0.0f;
        data[5] = 2.0f / (top - bottom);
        data[6] = 0.0f;
        data[7] = 0.0f;

        // Column 2 (Z scale) - Fixed DirectX depth mapping
        data[8] = 0.0f;
        data[9] = 0.0f;
        data[10] = 1.0f / (farPlane - nearPlane);  // Fixed: positive for DirectX
        data[11] = 0.0f;

        // Column 3 (Translation)
        data[12] = -(right + left) / (right - left);  // Should be 0 for centered
        data[13] = -(top + bottom) / (top - bottom);  // Should be 0 for centered
        data[14] = -nearPlane / (farPlane - nearPlane);  // Fixed depth offset
        data[15] = 1.0f;

        m_projectionMatrix = result;
    }

    void Camera::setPerspective(float fovY, float aspectRatio, float nearPlane, float farPlane) {
        Mat4 result;
        float* data = result.data();

        // Initialize to zero
        for (int i = 0; i < 16; i++) data[i] = 0.0f;

        float tanHalfFov = std::tan(fovY * 0.5f);

        // Column-major perspective matrix
        data[0] = 1.0f / (aspectRatio * tanHalfFov);
        data[5] = 1.0f / tanHalfFov;
        data[10] = farPlane / (farPlane - nearPlane);  // Fixed for DirectX
        data[11] = 1.0f;  // Changed to positive for DirectX
        data[14] = -(farPlane * nearPlane) / (farPlane - nearPlane);

        m_projectionMatrix = result;
    }

    void Camera::setup2D(float screenWidth, float screenHeight) {
        if (screenWidth <= 0.0f) screenWidth = 800.0f;
        if (screenHeight <= 0.0f) screenHeight = 600.0f;

        // FIXED: Proper 2D camera setup
        setPosition(0.0f, 0.0f, 0.0f);  // Camera at origin
        lookAt(Vec3(0.0f, 0.0f, 1.0f)); // Looking toward positive Z (where sprites are)

        // FIXED: Proper orthographic projection for 2D
        float scale = 200.0f;
        float left = -screenWidth / (2.0f * scale);
        float right = screenWidth / (2.0f * scale);
        float bottom = -screenHeight / (2.0f * scale);
        float top = screenHeight / (2.0f * scale);
        float nearPlane = 0.1f;  // FIXED: Closer near plane
        float farPlane = 10.0f;

        // Create orthographic matrix manually for clarity
        Mat4 result;
        float* data = result.data();

        // Initialize to zero
        for (int i = 0; i < 16; i++) data[i] = 0.0f;

        // Orthographic projection matrix (column-major)
        data[0] = 2.0f / (right - left);     // X scale
        data[5] = 2.0f / (top - bottom);     // Y scale  
        data[10] = 1.0f / (farPlane - nearPlane);  // Z scale (DirectX style)
        data[12] = -(right + left) / (right - left);   // X offset (should be 0)
        data[13] = -(top + bottom) / (top - bottom);   // Y offset (should be 0)  
        data[14] = -nearPlane / (farPlane - nearPlane); // Z offset
        data[15] = 1.0f;

        m_projectionMatrix = result;

        printf("2D Camera setup:\n");
        printf("  Screen: %.1fx%.1f, Scale: %.1f\n", screenWidth, screenHeight, scale);
        printf("  Ortho bounds: [%.3f,%.3f] x [%.3f,%.3f] x [%.1f,%.1f]\n",
            left, right, bottom, top, nearPlane, farPlane);
    }
    void Camera::updateViewMatrix() {
        // Create proper look-at matrix for 2D
        Vec3 forward = (m_target - m_position).normalized();
        Vec3 right = forward.cross(m_up).normalized();
        Vec3 up = right.cross(forward).normalized();

        Mat4 result;
        float* data = result.data();

        // Initialize to zero
        for (int i = 0; i < 16; i++) data[i] = 0.0f;

        // Column-major view matrix
        // Column 0 (right vector)
        data[0] = right.x;
        data[1] = up.x;
        data[2] = -forward.x;  // Negative because we look down -Z
        data[3] = 0.0f;

        // Column 1 (up vector)
        data[4] = right.y;
        data[5] = up.y;
        data[6] = -forward.y;
        data[7] = 0.0f;

        // Column 2 (forward vector)
        data[8] = right.z;
        data[9] = up.z;
        data[10] = -forward.z;
        data[11] = 0.0f;

        // Column 3 (translation)
        data[12] = -right.dot(m_position);
        data[13] = -up.dot(m_position);
        data[14] = forward.dot(m_position);  // Positive because of -forward above
        data[15] = 1.0f;

        m_viewMatrix = result;
    }

    
}