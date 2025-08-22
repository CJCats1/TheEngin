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
    auto whiteTexture = dx3d::Texture2D::CreateWhiteTexture(device.getD3DDevice());
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
    auto whiteTexture = dx3d::Texture2D::CreateWhiteTexture(device.getD3DDevice());
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
        // ADD THIS LINE to fix the sampler warning:
        ctx.setPSSampler(0, nullptr); // Uses default sampler
    }

    // Draw
    if (m_ib)
        ctx.drawIndexedTriangleList(m_indexCount, 0);
    else
        ctx.drawTriangleList(m_vertexCount, 0);
}
