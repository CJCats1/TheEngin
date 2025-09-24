#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <cmath>

using namespace dx3d;

LineRenderer::LineRenderer(GraphicsDevice& device) 
    : m_device(device) {
}

void LineRenderer::addLine(const Vec2& start, const Vec2& end, const Vec4& color, float thickness) {
    Line line;
    line.start = start;
    line.end = end;
    line.color = color;
    line.thickness = thickness;
    m_lines.push_back(line);
    m_bufferDirty = true;
}

void LineRenderer::addLine(const Line& line) {
    m_lines.push_back(line);
    m_bufferDirty = true;
}

void LineRenderer::addRect(const Vec2& position, const Vec2& size, const Vec4& color, float thickness) {
    Vec2 halfSize = size * 0.5f;
    Vec2 topLeft = position - halfSize;
    Vec2 bottomRight = position + halfSize;
    
    // Add four lines to form a rectangle
    this->addLine(Vec2(topLeft.x, topLeft.y), Vec2(bottomRight.x, topLeft.y), color, thickness); // Top
    this->addLine(Vec2(bottomRight.x, topLeft.y), Vec2(bottomRight.x, bottomRight.y), color, thickness); // Right
    this->addLine(Vec2(bottomRight.x, bottomRight.y), Vec2(topLeft.x, bottomRight.y), color, thickness); // Bottom
    this->addLine(Vec2(topLeft.x, bottomRight.y), Vec2(topLeft.x, topLeft.y), color, thickness); // Left
}

void LineRenderer::addCircle(const Vec2& center, float radius, const Vec4& color, float thickness, int segments) {
    if (segments < 3) segments = 3;
    
    float angleStep = 2.0f * 3.14159265f / segments;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        Vec2 start = center + Vec2(cos(angle1), sin(angle1)) * radius;
        Vec2 end = center + Vec2(cos(angle2), sin(angle2)) * radius;
        
        this->addLine(start, end, color, thickness);
    }
}
void LineRenderer::setCamera(const Camera2D* camera) {
    if (camera) {
        m_viewMatrix = camera->getViewMatrix();
        m_projMatrix = camera->getProjectionMatrix();
        m_camera = camera;
    }
}
void LineRenderer::clear() {
    m_lines.clear();
    m_vertices.clear();
    m_indices.clear();
    m_bufferDirty = true;
}

void LineRenderer::updateBuffer() {
    if (!m_bufferDirty) return;
    
    m_vertices.clear();
    m_indices.clear();
    
    for (const auto& line : m_lines) {
        generateLineVertices(line);
    }
    
    if (!m_vertices.empty()) {
        // Update vertex buffer
        m_vertexBuffer = m_device.createVertexBuffer({
            m_vertices.data(),
            static_cast<ui32>(m_vertices.size()),
            sizeof(Vertex),
            true // dynamic buffer
        });
        
        // Update index buffer
        if (!m_indices.empty()) {
            m_indexBuffer = m_device.createIndexBuffer({
                m_indices.data(),
                static_cast<ui32>(m_indices.size()),
                sizeof(ui32)
            });
        }
    } else {
        // Clear buffers if no vertices
        m_vertexBuffer.reset();
        m_indexBuffer.reset();
    }
    
    m_bufferDirty = false;
}

void LineRenderer::draw(DeviceContext& ctx) {
    if (!m_visible || m_lines.empty()) return;

    updateBuffer();

    if (m_vertexBuffer && !m_vertices.empty()) {
        // Only set camera matrices if we have a camera assigned
        // Otherwise, use the matrices already set by the scene
        if (m_camera) {
            ctx.setViewMatrix(m_camera->getViewMatrix());
            ctx.setProjectionMatrix(m_camera->getProjectionMatrix());
        }
        // If no camera assigned, use whatever matrices the scene has already set

        ctx.setVertexBuffer(*m_vertexBuffer);
        
        // Bind default sampler even though we don't use textures
        // This prevents D3D11 warnings about unbound samplers
        ctx.setPSSampler(0, ctx.getDefaultSampler());

        if (m_indexBuffer) {
            ctx.setIndexBuffer(*m_indexBuffer);
            ctx.drawIndexedLineList(static_cast<ui32>(m_indices.size()), 0);
        }
        else {
            ctx.drawTriangleList(static_cast<ui32>(m_vertices.size()), 0);
        }
    }
}
void LineRenderer::generateLineVertices(const Line& line) {
    Vec2 direction = line.end - line.start;
    float length = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.001f) return; // Skip zero-length lines
    
    Vec2 normalized = direction / length;
    Vec2 perpendicular = Vec2(-normalized.y, normalized.x) * (line.thickness * 0.5f);
    
    // Create a quad for the line
    // Apply local position offset only if local positioning is enabled
    // This prevents world coordinates from being corrupted by local sprite positioning
    Vec2 start = m_useLocalPositioning ? line.start + m_position : line.start;
    Vec2 end = m_useLocalPositioning ? line.end + m_position : line.end;
    
    // Convert to screen space if needed
    if (m_useScreenSpace) {
        float screenWidth = GraphicsEngine::getWindowWidth();
        float screenHeight = GraphicsEngine::getWindowHeight();
        
        // Convert from world coordinates to screen coordinates
        // Map world coordinates to screen space (0,0 = top-left, screenWidth,screenHeight = bottom-right)
        start.x = (start.x + screenWidth * 0.5f);
        start.y = (screenHeight * 0.5f - start.y); // Flip Y for screen space
        
        end.x = (end.x + screenWidth * 0.5f);
        end.y = (screenHeight * 0.5f - end.y); // Flip Y for screen space
    }
    
    ui32 startIndex = static_cast<ui32>(m_vertices.size());
    
    // Four vertices for the quad - using proper Vec3 constructor
    m_vertices.push_back({ Vec3(start.x - perpendicular.x, start.y - perpendicular.y, 0.0f), Vec3(0, 0, 1), Vec2(0, 0), line.color });
    m_vertices.push_back({ Vec3(start.x + perpendicular.x, start.y + perpendicular.y, 0.0f), Vec3(0, 0, 1), Vec2(0, 1), line.color });
    m_vertices.push_back({ Vec3(end.x + perpendicular.x, end.y + perpendicular.y, 0.0f), Vec3(0, 0, 1), Vec2(1, 1), line.color });
    m_vertices.push_back({ Vec3(end.x - perpendicular.x, end.y - perpendicular.y, 0.0f), Vec3(0, 0, 1), Vec2(1, 0), line.color });
    
    // Create indices for two triangles
    m_indices.push_back(startIndex);
    m_indices.push_back(startIndex + 1);
    m_indices.push_back(startIndex + 2);
    
    m_indices.push_back(startIndex);
    m_indices.push_back(startIndex + 2);
    m_indices.push_back(startIndex + 3);
}

void LineRenderer::createBuffers() {
    // Don't create empty buffers - wait until we have actual data
    // This method is kept for compatibility but does nothing
}