#include <DX3D/Graphics/Mesh.h>

using namespace dx3d;

std::shared_ptr<Mesh> Mesh::CreateQuadColored(GraphicsDevice& device, float w, float h)
{
    const float hw = w * 0.5f, hh = h * 0.5f;

    const Vertex verts[] = {
        { { -hw, -hh, 0.0f }, { 0, 0, 1 }, { 0, 1 }, Color::RED },
        { { -hw,  hh, 0.0f }, { 0, 0, 1 }, { 0, 0 }, Color::BLUE },
        { {  hw,  hh, 0.0f }, { 0, 0, 1 }, { 1, 0 }, Color::RED },
        { {  hw, -hh, 0.0f }, { 0, 0, 1 }, { 1, 1 }, Color::BLUE },
    };
    const ui32 idx[] = { 0,1,2, 0,2,3 };

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)std::size(verts);
    m->m_indexCount = (ui32)std::size(idx);
    m->m_vb = device.createVertexBuffer({ verts, m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ idx, m->m_indexCount, sizeof(ui32) });
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = w;
    m->m_height = h;
    return m;
}
std::shared_ptr<Mesh> Mesh::CreateQuadSolidColored(GraphicsDevice& device, float w, float h, Vec4 Color)
{
    const float hw = w * 0.5f;
    const float hh = h * 0.5f;

    const Vertex verts[] = {
        { { -hw, -hh, 0.0f }, { 0, 0, 1 }, { 0, 1 }, Color },
        { { -hw,  hh, 0.0f }, { 0, 0, 1 }, { 0, 0 }, Color },
        { {  hw,  hh, 0.0f }, { 0, 0, 1 }, { 1, 0 }, Color },
        { {  hw, -hh, 0.0f }, { 0, 0, 1 }, { 1, 1 }, Color },
    };

    const ui32 idx[] = { 0, 1, 2, 0, 2, 3 };

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)std::size(verts);
    m->m_indexCount = (ui32)std::size(idx);
    m->m_vb = device.createVertexBuffer({ verts, m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ idx, m->m_indexCount, sizeof(ui32) });
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = w;
    m->m_height = h;
    return m;
}
std::shared_ptr<Mesh> Mesh::CreateQuadTextured(GraphicsDevice& device, float w, float h)
{
    const float hw = w * 0.5f, hh = h * 0.5f;
    const Vertex verts[] = {
        { { -hw, -hh, 0.0f }, { 0, 0, 1 }, { 0, 1 }, { 1, 1, 1, 1 } },
        { { -hw,  hh, 0.0f }, { 0, 0, 1 }, { 0, 0 }, { 1, 1, 1, 1 } },
        { {  hw,  hh, 0.0f }, { 0, 0, 1 }, { 1, 0 }, { 1, 1, 1, 1 } },
        { {  hw, -hh, 0.0f }, { 0, 0, 1 }, { 1, 1 }, { 1, 1, 1, 1 } },
    };
    const ui32 idx[] = { 0,1,2, 0,2,3 };
    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)std::size(verts);
    m->m_indexCount = (ui32)std::size(idx);
    m->m_vb = device.createVertexBuffer({ verts, m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ idx, m->m_indexCount, sizeof(ui32) });
    m->m_width = w;
    m->m_height = h;
    m->m_device = &device; // Store device reference for UV updates
    return m;
}


void Mesh::draw(DeviceContext& ctx) const
{
    ctx.setVertexBuffer(*m_vb);
    if (m_ib)
        ctx.setIndexBuffer(*m_ib);

    // Bind the texture if the mesh is textured
    if (m_texture) {
        ctx.setPSShaderResource(0, m_texture->getSRV());
        ctx.setPSSampler(0, nullptr); // Uses default sampler
    }

    // Draw
    if (m_ib)
        ctx.drawIndexedTriangleList(m_indexCount, 0);
    else
        ctx.drawTriangleList(m_vertexCount, 0);
}
void Mesh::setSpriteFrame(int frameX, int frameY, int totalFramesX, int totalFramesY)
{
    if (totalFramesX <= 0 || totalFramesY <= 0) return;

    // Calculate UV coordinates for the specific frame
    float frameWidth = 1.0f / totalFramesX;
    float frameHeight = 1.0f / totalFramesY;

    m_currentU = frameX * frameWidth;
    m_currentV = frameY * frameHeight;
    m_currentUWidth = frameWidth;
    m_currentVHeight = frameHeight;

    updateUVCoordinates();
}

void Mesh::setSpriteFrameByIndex(int frameIndex, int totalFramesX, int totalFramesY)
{
    if (totalFramesX <= 0 || totalFramesY <= 0) return;

    int frameX = frameIndex % totalFramesX;
    int frameY = frameIndex / totalFramesX;
    setSpriteFrame(frameX, frameY, totalFramesX, totalFramesY);
}
void Mesh::setCustomUVRect(float u, float v, float uWidth, float vHeight)
{
    m_currentU = u;
    m_currentV = v;
    m_currentUWidth = uWidth;
    m_currentVHeight = vHeight;

    updateUVCoordinates();
}

void Mesh::updateUVCoordinates()
{
    if (!m_vb || !m_device) return;

    // Create new vertices with updated UV coordinates
    const float hw = m_width * 0.5f, hh = m_height * 0.5f;

    // Calculate the UV corners
    float uMin = m_currentU;
    float vMin = m_currentV;
    float uMax = m_currentU + m_currentUWidth;
    float vMax = m_currentV + m_currentVHeight;

    const Vertex verts[] = {
        { { -hw, -hh, 0.0f }, { 0, 0, 1 }, { uMin, vMax }, { 1, 1, 1, 1 } }, // Bottom-left
        { { -hw,  hh, 0.0f }, { 0, 0, 1 }, { uMin, vMin }, { 1, 1, 1, 1 } }, // Top-left
        { {  hw,  hh, 0.0f }, { 0, 0, 1 }, { uMax, vMin }, { 1, 1, 1, 1 } }, // Top-right
        { {  hw, -hh, 0.0f }, { 0, 0, 1 }, { uMax, vMax }, { 1, 1, 1, 1 } }, // Bottom-right
    };

    // Update the vertex buffer with new UV coordinates
    // Note: This assumes you have a way to update vertex buffer data
    // You might need to add this method to your GraphicsDevice/VertexBuffer class
    m_vb = m_device->createVertexBuffer({ verts, m_vertexCount, sizeof(Vertex) });
}