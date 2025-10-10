#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/FBXLoader.h>
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

std::vector<std::shared_ptr<Mesh>> Mesh::CreateFromOBJMultiMaterial(GraphicsDevice& device, const std::string& path)
{
    std::ifstream in(path);
    if (!in) return {};

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
    
    // Map material name -> texture path and material data
    std::unordered_map<std::string, std::string> mtlNameToTex;
    std::unordered_map<std::string, std::string> mtlNameToMapKd;
    std::string currentUseMtl;

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
            if (tag == "newmtl") { 
                ls >> currentMtl; 
            }
            else if (tag == "map_Kd") {
                std::string texPath; ls >> texPath;
                if (!currentMtl.empty()) {
                    mtlNameToMapKd[currentMtl] = texPath;
                    mtlNameToTex[currentMtl] = texPath;
                }
            }
        }
    };

    // Store vertices for each material
    std::unordered_map<std::string, std::vector<Vertex>> materialVertices;
    std::unordered_map<std::string, std::vector<ui32>> materialIndices;
    std::unordered_map<std::string, ui32> materialVertexCount;

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
                    
                    // Add to current material's vertex list
                    materialVertices[currentUseMtl].push_back({ pos, nor, uv, Color::WHITE });
                    materialIndices[currentUseMtl].push_back((ui32)materialVertices[currentUseMtl].size()-1);
                }
            }
        }
    }

    // Create meshes for each material
    std::vector<std::shared_ptr<Mesh>> meshes;
    
    for (const auto& [materialName, vertices] : materialVertices) {
        if (vertices.empty()) continue;
        
        auto m = std::make_shared<Mesh>();
        m->m_vertexCount = (ui32)vertices.size();
        m->m_indexCount = (ui32)materialIndices[materialName].size();
        m->m_vb = device.createVertexBuffer({ vertices.data(), m->m_vertexCount, sizeof(Vertex) });
        m->m_ib = device.createIndexBuffer({ materialIndices[materialName].data(), m->m_indexCount, sizeof(ui32) });
        
        // Load texture for this material
        auto it = mtlNameToTex.find(materialName);
        if (it != mtlNameToTex.end()) {
            // Use absolute path to ensure texture is found
            std::string texturePath = it->second;
            std::string fullPath;
            
            // If it's already an absolute path, use it as is
            if (texturePath.find(':') != std::string::npos || texturePath[0] == '/') {
                fullPath = texturePath;
            } else {
                // Convert relative path to absolute path
                fullPath = "D:/TheEngine/TheEngine/" + baseDir + texturePath;
            }
            
            std::wstring wpath;
            wpath.assign(fullPath.begin(), fullPath.end());
            auto tex = dx3d::Texture2D::LoadTexture2D(device.getD3DDevice(), wpath.c_str());
            if (tex) {
                m->setTexture(tex);
            } else {
                m->setTexture(dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice()));
            }
        } else {
            m->setTexture(dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice()));
        }
        
        meshes.push_back(m);
    }
    
    return meshes;
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

std::shared_ptr<Mesh> Mesh::CreatePlane(GraphicsDevice& device, float width, float height)
{
    const float w = width * 0.5f;
    const float h = height * 0.5f;
    
    // Create a plane with 4 vertices (2 triangles) - facing up
    const Vertex verts[] = {
        // Position, Normal, UV, Color
        { { -w, 0, -h }, { 0, 1, 0 }, { 0, 0 }, Color::WHITE }, // Bottom-left
        { {  w, 0, -h }, { 0, 1, 0 }, { 1, 0 }, Color::WHITE }, // Bottom-right
        { {  w, 0,  h }, { 0, 1, 0 }, { 1, 1 }, Color::WHITE }, // Top-right
        { { -w, 0,  h }, { 0, 1, 0 }, { 0, 1 }, Color::WHITE }  // Top-left
    };

    const ui32 idx[] = {
        0, 2, 1,  // First triangle (flipped winding)
        0, 3, 2   // Second triangle (flipped winding)
    };

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)std::size(verts);
    m->m_indexCount = (ui32)std::size(idx);
    m->m_vb = device.createVertexBuffer({ verts, m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ idx, m->m_indexCount, sizeof(ui32) });
    // Ensure a non-black base color by setting a default white texture
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = width; m->m_height = height;
    return m;
}

std::shared_ptr<Mesh> Mesh::CreateSphere(GraphicsDevice& device, float radius, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<ui32> indices;

    // Generate sphere vertices
    for (int i = 0; i <= segments; ++i) {
        float lat = 3.14159f * i / segments; // latitude angle
        for (int j = 0; j <= segments; ++j) {
            float lon = 2.0f * 3.14159f * j / segments; // longitude angle
            
            float x = radius * std::sin(lat) * std::cos(lon);
            float y = radius * std::cos(lat);
            float z = radius * std::sin(lat) * std::sin(lon);
            
            Vec3 position(x, y, z);
            Vec3 normal = position.normalized();
            Vec2 uv(j / (float)segments, i / (float)segments);
            
            vertices.push_back({ position, normal, uv, Color::WHITE });
        }
    }

    // Generate sphere indices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int current = i * (segments + 1) + j;
            int next = current + segments + 1;

            // First triangle
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            // Second triangle
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)vertices.size();
    m->m_indexCount = (ui32)indices.size();
    m->m_vb = device.createVertexBuffer({ vertices.data(), m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ indices.data(), m->m_indexCount, sizeof(ui32) });
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = radius * 2.0f; m->m_height = radius * 2.0f;
    return m;
}

std::shared_ptr<Mesh> Mesh::CreateCylinder(GraphicsDevice& device, float radius, float height, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<ui32> indices;

    float halfHeight = height * 0.5f;

    // Generate cylinder vertices
    // Top and bottom centers
    vertices.push_back({ { 0, halfHeight, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f }, Color::WHITE }); // Top center
    vertices.push_back({ { 0, -halfHeight, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f }, Color::WHITE }); // Bottom center

    // Side vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = radius * std::cos(angle);
        float z = radius * std::sin(angle);
        
        Vec3 normal(x / radius, 0, z / radius);
        
        // Top ring
        vertices.push_back({ { x, halfHeight, z }, normal, { i / (float)segments, 0 }, Color::WHITE });
        // Bottom ring
        vertices.push_back({ { x, -halfHeight, z }, normal, { i / (float)segments, 1 }, Color::WHITE });
    }

    // Generate cylinder indices
    // Top cap
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0); // Top center
        indices.push_back(2 + i * 2); // Top ring vertex
        indices.push_back(2 + ((i + 1) % segments) * 2); // Next top ring vertex
    }

    // Bottom cap
    for (int i = 0; i < segments; ++i) {
        indices.push_back(1); // Bottom center
        indices.push_back(2 + ((i + 1) % segments) * 2 + 1); // Next bottom ring vertex
        indices.push_back(2 + i * 2 + 1); // Bottom ring vertex
    }

    // Side faces
    for (int i = 0; i < segments; ++i) {
        int top1 = 2 + i * 2;
        int top2 = 2 + ((i + 1) % segments) * 2;
        int bottom1 = 2 + i * 2 + 1;
        int bottom2 = 2 + ((i + 1) % segments) * 2 + 1;

        // First triangle
        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);

        // Second triangle
        indices.push_back(top2);
        indices.push_back(bottom1);
        indices.push_back(bottom2);
    }

    auto m = std::make_shared<Mesh>();
    m->m_vertexCount = (ui32)vertices.size();
    m->m_indexCount = (ui32)indices.size();
    m->m_vb = device.createVertexBuffer({ vertices.data(), m->m_vertexCount, sizeof(Vertex) });
    m->m_ib = device.createIndexBuffer({ indices.data(), m->m_indexCount, sizeof(ui32) });
    auto whiteTexture = dx3d::Texture2D::CreateDebugTexture(device.getD3DDevice());
    m->setTexture(whiteTexture);
    m->m_width = radius * 2.0f; m->m_height = height;
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
    if (m_ib) {
        ctx.drawIndexedTriangleList(m_indexCount, 0);
    } else
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

// FBX Loading Functions
// Note: These are basic implementations that can be enhanced with proper FBX parsing
// For production use, consider integrating Assimp or Autodesk FBX SDK

std::shared_ptr<Mesh> Mesh::CreateFromFBX(GraphicsDevice& device, const std::string& path)
{
    // Use the FBXLoader to load the mesh
    auto mesh = FBXLoader::LoadMesh(device, path);
    
    // If FBX loading fails, fallback to a cube
    if (!mesh) {
        return CreateCube(device, 1.0f);
    }
    
    return mesh;
}

std::vector<std::shared_ptr<Mesh>> Mesh::CreateFromFBXMultiMaterial(GraphicsDevice& device, const std::string& path)
{
    // Use the FBXLoader to load multiple meshes
    auto meshes = FBXLoader::LoadMeshes(device, path);
    
    // If FBX loading fails, fallback to a single cube
    if (meshes.empty()) {
        meshes.push_back(CreateCube(device, 1.0f));
    }
    
    return meshes;
}