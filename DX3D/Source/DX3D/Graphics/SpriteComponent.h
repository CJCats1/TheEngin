// SpriteComponent.h
#pragma once
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/TransformComponent.h>
#include <memory>
#include <string>

namespace dx3d {
    class SpriteComponent {
    public:
        SpriteComponent(GraphicsDevice& device, const std::wstring& texturePath,
            float width = 1.0f, float height = 1.0f);

        // Alternative constructor for existing texture
        SpriteComponent(GraphicsDevice& device, std::shared_ptr<Texture2D> texture,
            float width = 1.0f, float height = 1.0f);
        SpriteComponent(GraphicsDevice& device, std::shared_ptr<Mesh> mesh,
            std::shared_ptr<Texture2D> texture = nullptr);

        ~SpriteComponent() = default;

        // Transform access (direct access to transform component)
        TransformComponent& transform() { return m_transform; }
        const TransformComponent& transform() const { return m_transform; }

        // Convenience methods that delegate to transform
        void setPosition(float x, float y, float z = 0.0f) { m_transform.setPosition(x, y, z); }
        void setPosition(const Vec3& pos) { m_transform.setPosition(pos); }
        void setPosition(const Vec2& pos) { m_transform.setPosition(pos); }

        void translate(float x, float y, float z = 0.0f) { m_transform.translate(x, y, z); }
        void translate(const Vec3& delta) { m_transform.translate(delta); }
        void translate(const Vec2& delta) { m_transform.translate(delta); }

        void setRotation(float x, float y = 0.0f, float z = 0.0f) { m_transform.setRotation(x, y, z); }
        void setRotation(const Vec3& rot) { m_transform.setRotation(rot); }
        void setRotationZ(float z) { m_transform.setRotationZ(z); }

        void rotate(float x, float y = 0.0f, float z = 0.0f) { m_transform.rotate(x, y, z); }
        void rotate(const Vec3& delta) { m_transform.rotate(delta); }
        void rotateZ(float deltaZ) { m_transform.rotateZ(deltaZ); }

        void setScale(float x, float y, float z = 1.0f) { m_transform.setScale(x, y, z); }
        void setScale(const Vec3& scale) { m_transform.setScale(scale); }
        void setScale(float uniformScale) { m_transform.setScale(uniformScale); }
        void setScale2D(float uniformScale) { m_transform.setScale2D(uniformScale); }

        void scaleBy(float factor) { m_transform.scaleBy(factor); }
        void scaleBy(const Vec3& factor) { m_transform.scaleBy(factor); }

        // Getters that delegate to transform
        Vec3 getPosition() const { return m_transform.getPosition(); }
        Vec3 getRotation() const { return m_transform.getRotation(); }
        Vec3 getScale() const { return m_transform.getScale(); }
        Vec2 getPosition2D() const { return m_transform.getPosition2D(); }
        float getRotationZ() const { return m_transform.getRotationZ(); }
        Mat4 getWorldMatrix() const { return m_transform.getWorldMatrix2D(); } // Use 2D optimized version

        // Mesh and texture accessors
        std::shared_ptr<Mesh> getMesh() const { return m_mesh; }
        std::shared_ptr<Texture2D> getTexture() const { return m_texture; }
        void setTexture(std::shared_ptr<Texture2D> texture);

        // Utility
        bool isValid() const { return m_mesh && m_texture; }
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        // Rendering
        void draw(DeviceContext& ctx) const;
        GraphicsDevice& getGraphicsDevice() { return m_device; }
        void setTint(const Vec4& tint) { m_tint = tint; }
        Vec4 getTint() const { return m_tint; }

        bool m_useScreenSpace = false;   // default: world space
        Vec2 m_screenPosition = { 0, 0 };  // pixel coordinates when in screen space

        void setScreenPosition(float x, float y) { m_screenPosition = { x, y }; }
        Vec2 getScreenPosition() const { return m_screenPosition; }
        void enableScreenSpace(bool enable = true) {
            m_useScreenSpace = enable;
        }

        bool isScreenSpace() const {
            return m_useScreenSpace;
        }
		float getWidth() const { return m_width; }
		float getHeight() const { return m_height; }

        // SPRITESHEET FUNCTIONALITY
        // Set up spritesheet parameters
        void setupSpritesheet(int totalFramesX, int totalFramesY) {
            m_spritesheetFramesX = totalFramesX;
            m_spritesheetFramesY = totalFramesY;
            m_isSpritesheetEnabled = true;
        }

        // Set frame by X,Y coordinates
        void setSpriteFrame(int frameX, int frameY) {
            if (!m_isSpritesheetEnabled || !m_mesh) return;
            m_currentFrameX = frameX;
            m_currentFrameY = frameY;
            m_mesh->setSpriteFrame(frameX, frameY, m_spritesheetFramesX, m_spritesheetFramesY);
        }

        // Set frame by index (0-based, left-to-right, top-to-bottom)
        void setSpriteFrameByIndex(int frameIndex) {
            if (!m_isSpritesheetEnabled || !m_mesh) return;
            m_currentFrameIndex = frameIndex;
            m_mesh->setSpriteFrameByIndex(frameIndex, m_spritesheetFramesX, m_spritesheetFramesY);

            // Update frame X,Y for consistency
            m_currentFrameX = frameIndex % m_spritesheetFramesX;
            m_currentFrameY = frameIndex / m_spritesheetFramesX;
        }

        // Animation support
        void startAnimation(int startFrame, int endFrame, float frameRate) {
            m_animationStartFrame = startFrame;
            m_animationEndFrame = endFrame;
            m_animationFrameRate = frameRate;
            m_animationTimer = 0.0f;
            m_isAnimating = true;
            setSpriteFrameByIndex(startFrame);
        }

        void stopAnimation() {
            m_isAnimating = false;
        }

        void updateAnimation(float deltaTime) {
            if (!m_isAnimating) return;

            m_animationTimer += deltaTime;
            float frameDuration = 1.0f / m_animationFrameRate;

            if (m_animationTimer >= frameDuration) {
                m_animationTimer = 0.0f;
                m_currentFrameIndex++;

                if (m_currentFrameIndex > m_animationEndFrame) {
                    if (m_animationLoop) {
                        m_currentFrameIndex = m_animationStartFrame;
                    }
                    else {
                        m_currentFrameIndex = m_animationEndFrame;
                        m_isAnimating = false;
                    }
                }

                setSpriteFrameByIndex(m_currentFrameIndex);
            }
        }

        void setAnimationLoop(bool loop) { m_animationLoop = loop; }
        bool isAnimating() const { return m_isAnimating; }

        // Getters for spritesheet info
        int getCurrentFrameX() const { return m_currentFrameX; }
        int getCurrentFrameY() const { return m_currentFrameY; }
        int getCurrentFrameIndex() const { return m_currentFrameIndex; }
        int getSpritesheetFramesX() const { return m_spritesheetFramesX; }
        int getSpritesheetFramesY() const { return m_spritesheetFramesY; }
        bool isSpritesheetEnabled() const { return m_isSpritesheetEnabled; }

    private:
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Texture2D> m_texture;
        TransformComponent m_transform;
        bool m_visible = true;
        GraphicsDevice& m_device;
        Vec4 m_tint = { 1,1,1,0 };
        float m_width = 1.0f;
        float m_height = 1.0f;
        // Spritesheet members
        bool m_isSpritesheetEnabled = false;
        int m_spritesheetFramesX = 1;
        int m_spritesheetFramesY = 1;
        int m_currentFrameX = 0;
        int m_currentFrameY = 0;
        int m_currentFrameIndex = 0;

        // Animation members
        bool m_isAnimating = false;
        bool m_animationLoop = true;
        int m_animationStartFrame = 0;
        int m_animationEndFrame = 0;
        float m_animationFrameRate = 10.0f; // frames per second
        float m_animationTimer = 0.0f;

        void initialize(GraphicsDevice& device, float width, float height);
    };
}