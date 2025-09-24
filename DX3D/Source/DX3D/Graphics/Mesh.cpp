#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>

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

std::shared_ptr<Mesh> Mesh::CreateFromOBJ(GraphicsDevice& device, const std::string& path)
{
    std::ifstream in(path);
    if (!in) return nullptr;

    // Helper: directory of OBJ to resolve relative MTL/texture paths
    auto getDir = [](const std::string& p) -> std::string {
        size_t s = p.find_last_of("/\\");
        return (s == std::string::npos) ? std::string() : p.substr(0, s + 1);
    };
    const std::string baseDir = getDir(path);

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    struct Idx { int v{-1}, vt{-1}, vn{-1}; };
    std::vector<Vertex> outVerts;
    std::vector<ui32> indices;

    // Minimal MTL support: map material name -> diffuse texture path
    std::unordered_map<std::string, std::string> mtlNameToTex;
    std::string currentUseMtl;
    std::string chosenDiffuseTexPath; // first used material with map_Kd wins

    auto loadMTL = [&](const std::string& mtlFilename){
        std::ifstream min(baseDir + mtlFilename);
        if (!min) return;
        std::string line;
        std::string currentMtl;
        while (std::getline(min, line))
        {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ls(line);
            std::string tag; ls >> tag;
            if (tag == "newmtl") { ls >> currentMtl; }
            else if (tag == "map_Kd") {
                std::string texPath; ls >> texPath;
                if (!currentMtl.empty()) mtlNameToTex[currentMtl] = texPath;
            }
        }
    };

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string tag; ls >> tag;
        if (tag == "v") { float x,y,z; ls >> x >> y >> z; positions.push_back({x,y,z}); }
        else if (tag == "vt") { float u,v; ls >> u >> v; uvs.push_back({u,1.0f - v}); }
        else if (tag == "vn") { float x,y,z; ls >> x >> y >> z; normals.push_back({x,y,z}); }
        else if (tag == "mtllib") { std::string mtlFile; ls >> mtlFile; loadMTL(mtlFile); }
        else if (tag == "usemtl") {
            ls >> currentUseMtl;
            if (chosenDiffuseTexPath.empty()) {
                auto it = mtlNameToTex.find(currentUseMtl);
                if (it != mtlNameToTex.end()) chosenDiffuseTexPath = it->second;
            }
        }
        else if (tag == "f") {
            std::vector<Idx> face;
            std::string vert;
            while (ls >> vert) {
                Idx idx{}; int v=-1, vt=-1, vn=-1;
                // Parse v/vt/vn or v//vn or v/vt
                size_t p1 = vert.find('/');
                size_t p2 = vert.find('/', p1 == std::string::npos ? p1 : p1+1);
                try {
                    if (p1 == std::string::npos) { v = std::stoi(vert); }
                    else {
                        v = std::stoi(vert.substr(0, p1));
                        if (p2 == std::string::npos) {
                            vt = std::stoi(vert.substr(p1+1));
                        } else {
                            if (p2 > p1+1) vt = std::stoi(vert.substr(p1+1, p2 - (p1+1)));
                            if (p2+1 < vert.size()) vn = std::stoi(vert.substr(p2+1));
                        }
                    }
                } catch(...) { }
                idx.v = v; idx.vt = vt; idx.vn = vn; face.push_back(idx);
            }
            // Triangulate fan if quad/ngon
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                Idx tri[3] = { face[0], face[i], face[i+1] };
                for (int k=0;k<3;k++)
                {
                    int v = tri[k].v; int vt = tri[k].vt; int vn = tri[k].vn;
                    if (v < 0) continue; // skip invalid
                    Vec3 pos = positions[(size_t)(v-1)];
                    Vec2 uv = (vt>0 && (size_t)(vt-1) < uvs.size()) ? uvs[(size_t)(vt-1)] : Vec2(0,0);
                    Vec3 nor = (vn>0 && (size_t)(vn-1) < normals.size()) ? normals[(size_t)(vn-1)] : Vec3(0,0,1);
                    outVerts.push_back({ pos, nor, uv, Color::WHITE });
                    indices.push_back((ui32)outVerts.size()-1);
                }
            }
        }
    }

    if (outVerts.empty()) return nullptr;

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)outVerts.size();
    m->m_indexCount = (ui32)indices.size();
    m->m_vb = device.createVertexBuffer({ outVerts.data(), m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ indices.data(), m->m_indexCount, sizeof(ui32) });
    if (!chosenDiffuseTexPath.empty()) {
        std::wstring wpath;
        const std::string full = baseDir + chosenDiffuseTexPath;
        wpath.assign(full.begin(), full.end());
        auto tex = dx3d::Texture2D::LoadTexture2D(device.getD3DDevice(), wpath.c_str());
        if (tex) m->setTexture(tex);
        else m->setTexture(dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice()));
    } else {
        m->setTexture(dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice()));
    }
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


std::shared_ptr<Mesh> Mesh::CreateCube(GraphicsDevice& device, float size)
{
    const float s = size * 0.5f;
    // 24 unique vertices (per-face normals/uvs)
    const Vertex verts[] = {
        // +X
        { { s,-s,-s }, { 1,0,0 }, { 0,1 }, Color::WHITE },
        { { s, s,-s }, { 1,0,0 }, { 0,0 }, Color::WHITE },
        { { s, s, s }, { 1,0,0 }, { 1,0 }, Color::WHITE },
        { { s,-s, s }, { 1,0,0 }, { 1,1 }, Color::WHITE },
        // -X
        { { -s,-s, s }, { -1,0,0 }, { 0,1 }, Color::WHITE },
        { { -s, s, s }, { -1,0,0 }, { 0,0 }, Color::WHITE },
        { { -s, s,-s }, { -1,0,0 }, { 1,0 }, Color::WHITE },
        { { -s,-s,-s }, { -1,0,0 }, { 1,1 }, Color::WHITE },
        // +Y
        { { -s, s,-s }, { 0,1,0 }, { 0,1 }, Color::WHITE },
        { { -s, s, s }, { 0,1,0 }, { 0,0 }, Color::WHITE },
        { {  s, s, s }, { 0,1,0 }, { 1,0 }, Color::WHITE },
        { {  s, s,-s }, { 0,1,0 }, { 1,1 }, Color::WHITE },
        // -Y
        { { -s,-s, s }, { 0,-1,0 }, { 0,1 }, Color::WHITE },
        { { -s,-s,-s }, { 0,-1,0 }, { 0,0 }, Color::WHITE },
        { {  s,-s,-s }, { 0,-1,0 }, { 1,0 }, Color::WHITE },
        { {  s,-s, s }, { 0,-1,0 }, { 1,1 }, Color::WHITE },
        // +Z
        { { -s,-s, s }, { 0,0,1 }, { 0,1 }, Color::WHITE },
        { {  s,-s, s }, { 0,0,1 }, { 1,1 }, Color::WHITE },
        { {  s, s, s }, { 0,0,1 }, { 1,0 }, Color::WHITE },
        { { -s, s, s }, { 0,0,1 }, { 0,0 }, Color::WHITE },
        // -Z
        { {  s,-s,-s }, { 0,0,-1 }, { 0,1 }, Color::WHITE },
        { { -s,-s,-s }, { 0,0,-1 }, { 1,1 }, Color::WHITE },
        { { -s, s,-s }, { 0,0,-1 }, { 1,0 }, Color::WHITE },
        { {  s, s,-s }, { 0,0,-1 }, { 0,0 }, Color::WHITE },
    };

    const ui32 idx[] = {
        0,1,2, 0,2,3,      // +X
        4,5,6, 4,6,7,      // -X
        8,9,10, 8,10,11,   // +Y
        12,13,14, 12,14,15,// -Y
        16,17,18, 16,18,19,// +Z
        20,21,22, 20,22,23 // -Z
    };

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)std::size(verts);
    m->m_indexCount = (ui32)std::size(idx);
    m->m_vb = device.createVertexBuffer({ verts, m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ idx, m->m_indexCount, sizeof(ui32) });
    // Ensure a non-black base color by setting a default white texture
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = size; m->m_height = size;
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
    }
    
    // Always bind default sampler to prevent D3D11 warnings
    ctx.setPSSampler(0, ctx.getDefaultSampler());

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