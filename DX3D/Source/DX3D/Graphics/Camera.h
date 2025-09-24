#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/TransformComponent.h>

namespace dx3d {
    class Camera3D {
    public:
        Camera3D(float fovYRadians, float aspect, float nearZ = 0.1f, float farZ = 1000.0f)
            : m_fovY(fovYRadians), m_aspect(aspect), m_nearZ(nearZ), m_farZ(farZ) {
            m_position = Vec3(0.0f, 0.0f, -5.0f);
            m_target = Vec3(0.0f, 0.0f, 0.0f);
            m_up = Vec3(0.0f, 1.0f, 0.0f);
        }

        // Setters
        void setPerspective(float fovYRadians, float aspect, float nearZ, float farZ) {
            m_fovY = fovYRadians; m_aspect = aspect; m_nearZ = nearZ; m_farZ = farZ;
        }
        void setPosition(const Vec3& p) { m_position = p; }
        void setTarget(const Vec3& t) { m_target = t; }
        void setUp(const Vec3& u) { m_up = u; }

        // Getters
        const Vec3& getPosition() const { return m_position; }
        const Vec3& getTarget() const { return m_target; }
        const Vec3& getUp() const { return m_up; }
        float getFovY() const { return m_fovY; }
        float getAspect() const { return m_aspect; }
        float getNearZ() const { return m_nearZ; }
        float getFarZ() const { return m_farZ; }

        // Matrices
        Mat4 getViewMatrix() const { return Mat4::lookAt(m_position, m_target, m_up); }
        Mat4 getProjectionMatrix() const { return Mat4::perspective(m_fovY, m_aspect, m_nearZ, m_farZ); }
        Mat4 getViewProjectionMatrix() const { return getProjectionMatrix() * getViewMatrix(); }

        // Controls helpers
        void move(const Vec3& delta) { m_position = m_position + delta; m_target = m_target + delta; }

    private:
        Vec3 m_position;
        Vec3 m_target;
        Vec3 m_up;
        float m_fovY;
        float m_aspect;
        float m_nearZ;
        float m_farZ;
    };
    class Camera2D {
    public:
        Camera2D(float screenWidth, float screenHeight);
        ~Camera2D() = default;

        // Transform access (direct access to transform component)
        TransformComponent& transform() { return m_transform; }
        const TransformComponent& transform() const { return m_transform; }

        // Camera-specific controls (delegates to transform)
        void setPosition(float x, float y) { m_transform.setPosition(x, y, 0.0f); }
        void setPosition(const Vec2& position) { m_transform.setPosition(position); }
        void move(float deltaX, float deltaY) { m_transform.translate(deltaX, deltaY, 0.0f); }
        void move(const Vec2& delta) { m_transform.translate(delta); }

        void setRotation(float rotation) { m_transform.setRotationZ(rotation); }
        void rotate(float deltaRotation) { m_transform.rotateZ(deltaRotation); }

        // Camera-specific zoom controls
        void setZoom(float zoom);
        void zoom(float deltaZoom);

        // Getters
        Vec2 getPosition() const { return m_transform.getPosition2D(); }
        float getZoom() const { return m_zoom; }
        float getRotation() const { return m_transform.getRotationZ(); }

        // Matrix getters
        Mat4 getViewMatrix() const;
        Mat4 getProjectionMatrix() const { return m_projectionMatrix; }
        Mat4 getViewProjectionMatrix() const;

        // Screen space conversions
        Vec2 screenToWorld(const Vec2& screenPos) const;
        Vec2 worldToScreen(const Vec2& worldPos) const;

        // Screen bounds in world space
        struct Bounds {
            float left, right, top, bottom;
        };
        Bounds getWorldBounds() const;

        // Update screen size (call when window resizes)
        void setScreenSize(float width, float height);

    private:
        TransformComponent m_transform;
        float m_zoom;
        float m_screenWidth;
        float m_screenHeight;
        Mat4 m_projectionMatrix;

        void updateProjectionMatrix();
    };

    inline Camera2D::Camera2D(float screenWidth, float screenHeight)
        : m_zoom(1.0f)
        , m_screenWidth(screenWidth)
        , m_screenHeight(screenHeight) {
        m_transform.setPosition(0.0f, 0.0f, 0.0f);
        updateProjectionMatrix();
    }

    inline void Camera2D::setZoom(float zoom) {
        m_zoom = clamp(zoom, 0.1f, 10.0f); // Reasonable zoom limits
    }

    inline void Camera2D::zoom(float deltaZoom) {
        setZoom(m_zoom + deltaZoom);
    }

    inline Mat4 Camera2D::getViewMatrix() const {
        Vec2 pos = getPosition();
        float rotation = getRotation();

        // Create view matrix: Scale by zoom, rotate, then translate by negative camera position
        Mat4 translation = Mat4::translation(Vec3(-pos.x, -pos.y, 0.0f));
        Mat4 rotationMatrix = Mat4::rotationZ(-rotation); // Negative because we're moving the world
        Mat4 scale = Mat4::scale(Vec3(m_zoom, m_zoom, 1.0f));

        return translation  * rotationMatrix * scale;
    }

    inline Mat4 Camera2D::getViewProjectionMatrix() const {
        return m_projectionMatrix * getViewMatrix();
    }

    inline Vec2 Camera2D::screenToWorld(const Vec2& screenPos) const {
        Vec2 pos = getPosition();
        float rotation = getRotation();

        // Convert screen coordinates to normalized device coordinates (-1 to 1)
        float ndcX = (screenPos.x / m_screenWidth) * 2.0f - 1.0f;
        float ndcY = 1.0f - (screenPos.y / m_screenHeight) * 2.0f; // Flip Y axis

        // Apply inverse zoom
        ndcX /= m_zoom;
        ndcY /= m_zoom;

        // Apply inverse rotation
        float cos_r = std::cos(-rotation);
        float sin_r = std::sin(-rotation);
        float rotatedX = ndcX * cos_r - ndcY * sin_r;
        float rotatedY = ndcX * sin_r + ndcY * cos_r;

        // Scale to world units (half screen size in world units at zoom 1.0)
        float worldX = rotatedX * (m_screenWidth * 0.5f) + pos.x;
        float worldY = rotatedY * (m_screenHeight * 0.5f) + pos.y;

        return Vec2(worldX, worldY);
    }

    inline Vec2 Camera2D::worldToScreen(const Vec2& worldPos) const {
        Vec2 pos = getPosition();
        float rotation = getRotation();

        // Translate by camera position
        float translatedX = worldPos.x - pos.x;
        float translatedY = worldPos.y - pos.y;

        // Apply rotation
        float cos_r = std::cos(rotation);
        float sin_r = std::sin(rotation);
        float rotatedX = translatedX * cos_r - translatedY * sin_r;
        float rotatedY = translatedX * sin_r + translatedY * cos_r;

        // Apply zoom and convert to NDC
        float ndcX = (rotatedX * m_zoom) / (m_screenWidth * 0.5f);
        float ndcY = (rotatedY * m_zoom) / (m_screenHeight * 0.5f);

        // Convert from NDC to screen coordinates
        float screenX = (ndcX + 1.0f) * 0.5f * m_screenWidth;
        float screenY = (1.0f - ndcY) * 0.5f * m_screenHeight;

        return Vec2(screenX, screenY);
    }

    inline Camera2D::Bounds Camera2D::getWorldBounds() const {
        // Get the four corners of the screen in world space
        Vec2 topLeft = screenToWorld(Vec2(0, 0));
        Vec2 topRight = screenToWorld(Vec2(m_screenWidth, 0));
        Vec2 bottomLeft = screenToWorld(Vec2(0, m_screenHeight));
        Vec2 bottomRight = screenToWorld(Vec2(m_screenWidth, m_screenHeight));

        // Find the min/max bounds
        float left = std::min({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
        float right = std::max({ topLeft.x, topRight.x, bottomLeft.x, bottomRight.x });
        float top = std::min({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });
        float bottom = std::max({ topLeft.y, topRight.y, bottomLeft.y, bottomRight.y });

        return { left, right, top, bottom };
    }

    inline void Camera2D::setScreenSize(float width, float height) {
        m_screenWidth = width;
        m_screenHeight = height;
        updateProjectionMatrix();
    }

    inline void Camera2D::updateProjectionMatrix() {
        // Create orthographic projection for 2D
        // Map screen pixels to world units (1 pixel = 1 world unit at zoom 1.0)
        float halfWidth = m_screenWidth * 0.5f;
        float halfHeight = m_screenHeight * 0.5f;

        m_projectionMatrix = Mat4::orthographic(
            m_screenWidth,  // width
            m_screenHeight, // height
            -1000.0f,        // near plane (match sprite component)
            1000.0f          // far plane (match sprite component)
        );
    }

}