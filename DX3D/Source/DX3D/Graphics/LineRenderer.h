#pragma once
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Camera.h>

#include <vector>
#include <memory>

namespace dx3d {

    struct Line {
        Vec2 start;
        Vec2 end;
        Vec4 color;
        float thickness;
    };

    class LineRenderer {
    public:
        LineRenderer(GraphicsDevice& device);
        ~LineRenderer() = default;

        // Add lines to be rendered
        void addLine(const Vec2& start, const Vec2& end, const Vec4& color = Color::WHITE, float thickness = 1.0f);
        void addLine(const Line& line);
        void addRect(const Vec2& position, const Vec2& size, const Vec4& color = Color::WHITE, float thickness = 1.0f);
        void addCircle(const Vec2& center, float radius, const Vec4& color = Color::WHITE, float thickness = 1.0f, int segments = 16);

        // Clear all lines
        void clear();

        // Update vertex buffer with current lines
        void updateBuffer();

        // Render all lines
        void draw(DeviceContext& ctx);

        // Camera integration - make this public so the scene can call it
        void setCamera(const Camera2D* camera);

        // Properties
        bool isVisible() const { return m_visible; }
        void setVisible(bool visible) { m_visible = visible; }

        // Position and transform
        void setPosition(const Vec2& position) { m_position = position; }
        void setPosition(float x, float y) { m_position = Vec2(x, y); }
        Vec2 getPosition() const { return m_position; }

        // Screen space rendering
        void enableScreenSpace(bool enable = true) { m_useScreenSpace = enable; }
        bool isScreenSpace() const { return m_useScreenSpace; }

        // Get device reference
        GraphicsDevice& getDevice() { return m_device; }

    private:
        GraphicsDevice& m_device;
        std::vector<Line> m_lines;
        std::vector<Vertex> m_vertices;
        std::vector<ui32> m_indices;

        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;

        bool m_visible = true;
        bool m_useScreenSpace = false;
        Vec2 m_position = { 0.0f, 0.0f };
        bool m_bufferDirty = true;
        Mat4 m_viewMatrix;
        Mat4 m_projMatrix;
        const Camera2D* m_camera = nullptr;

        // Helper methods
        void generateLineVertices(const Line& line);
        void createBuffers();
    };

}