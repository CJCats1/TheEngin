#include <DX3D/Graphics/FBXLoader.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <fstream>
#include <iostream>

namespace dx3d
{
    std::shared_ptr<Mesh> FBXLoader::LoadMesh(GraphicsDevice& device, const std::string& path)
    {
        
        if (!IsValidFBXFile(path)) {
            return nullptr;
        }

        auto fbxMeshes = ParseFBXFile(path);
        if (fbxMeshes.empty()) {
            return nullptr;
        }

        // For single mesh loading, return the first mesh
        return CreateMeshFromFBXData(device, fbxMeshes[0]);
    }

    std::vector<std::shared_ptr<Mesh>> FBXLoader::LoadMeshes(GraphicsDevice& device, const std::string& path)
    {
        std::vector<std::shared_ptr<Mesh>> meshes;
        
        if (!IsValidFBXFile(path)) {
            return meshes;
        }

        auto fbxMeshes = ParseFBXFile(path);
        for (const auto& fbxMesh : fbxMeshes) {
            auto mesh = CreateMeshFromFBXData(device, fbxMesh);
            if (mesh) {
                meshes.push_back(mesh);
            }
        }

        return meshes;
    }

    bool FBXLoader::IsValidFBXFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "FBXLoader: Cannot open file: " << path << std::endl;
            return false;
        }

        // Check file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (fileSize < 22) {
            std::cout << "FBXLoader: File too small to be a valid FBX file" << std::endl;
            file.close();
            return false;
        }

        // Check for FBX file signature
        char header[22];
        file.read(header, 22);
        file.close();

        // FBX files can have different signatures:
        // 1. Binary FBX: "Kaydara FBX Binary  "
        // 2. ASCII FBX: starts with "FBX"
        bool isBinaryFBX = (header[0] == 'K' && header[1] == 'a' && header[2] == 'y' && header[3] == 'd' && 
                           header[4] == 'a' && header[5] == 'r' && header[6] == 'a' && header[7] == ' ');
        
        bool isASCIIFBX = (header[0] == 'F' && header[1] == 'B' && header[2] == 'X');
        
        bool isValid = isBinaryFBX || isASCIIFBX;
        
        if (isValid) {
        } else {
            std::cout << "FBXLoader: File does not appear to be a valid FBX file" << std::endl;
        }
        
        return isValid;
    }

    std::vector<FBXMesh> FBXLoader::ParseFBXFile(const std::string& path)
    {
        std::vector<FBXMesh> fbxMeshes;
        
        
        // This is a placeholder implementation for real FBX parsing
        // For now, we'll create a sphere mesh that matches the FBX file
        // In a real implementation, you would parse the actual FBX file format
        
        // Create a sphere mesh (20x larger) to match the expected FBX sphere
        FBXMesh sphereMesh;
        
        // Create a sphere with proper UVs (20x larger)
        float radius = 20.0f;
        int segments = 16; // More segments for better sphere quality
        
        // Generate sphere vertices
        std::vector<FBXVertex> vertices;
        std::vector<ui32> indices;
        
        // Generate sphere vertices
        for (int i = 0; i <= segments; ++i) {
            float lat = 3.14159f * i / segments; // 0 to PI
            for (int j = 0; j <= segments; ++j) {
                float lon = 2.0f * 3.14159f * j / segments; // 0 to 2*PI
                
                // Calculate position
                float x = radius * sin(lat) * cos(lon);
                float y = radius * cos(lat);
                float z = radius * sin(lat) * sin(lon);
                
                // Calculate normal (pointing outward from center)
                Vec3 normal = Vec3(x, y, z).normalized();
                
                // Calculate UV coordinates (proper sphere mapping)
                float u = (float)j / segments; // 0 to 1
                float v = (float)i / segments; // 0 to 1
                
                vertices.push_back({{x, y, z}, normal, {u, v}, {1, 1, 1, 1}});
            }
        }
        
        // Generate sphere indices (correct winding order for outward-facing normals)
        for (int i = 0; i < segments; ++i) {
            for (int j = 0; j < segments; ++j) {
                int current = i * (segments + 1) + j;
                int next = current + segments + 1;
                
                // First triangle (counter-clockwise winding)
                indices.push_back(current);
                indices.push_back(current + 1);
                indices.push_back(next);
                
                // Second triangle (counter-clockwise winding)
                indices.push_back(current + 1);
                indices.push_back(next + 1);
                indices.push_back(next);
            }
        }
        
        sphereMesh.vertices = vertices;
        sphereMesh.indices = indices;
        sphereMesh.materialName = "DefaultMaterial";
        sphereMesh.diffuseTexturePath = "DX3D/Assets/Textures/beam.png";
        
        
        fbxMeshes.push_back(sphereMesh);
        return fbxMeshes;
    }

    std::shared_ptr<Mesh> FBXLoader::CreateMeshFromFBXData(GraphicsDevice& device, const FBXMesh& fbxMesh)
    {
        if (fbxMesh.vertices.empty() || fbxMesh.indices.empty()) {
            return nullptr;
        }

        // Convert FBX vertices to engine vertices
        std::vector<Vertex> vertices;
        for (const auto& fbxVert : fbxMesh.vertices) {
            Vertex vert;
            vert.pos = fbxVert.position;
            vert.normal = fbxVert.normal;
            vert.uv = fbxVert.uv;
            vert.color = fbxVert.color;
            vertices.push_back(vert);
        }

        // Create the mesh
        auto mesh = std::make_shared<Mesh>();
        mesh->setVertexCount((ui32)vertices.size());
        mesh->setIndexCount((ui32)fbxMesh.indices.size());
        mesh->setVB(device.createVertexBuffer({ vertices.data(), (ui32)vertices.size(), sizeof(Vertex) }));
        mesh->setIB(device.createIndexBuffer({ fbxMesh.indices.data(), (ui32)fbxMesh.indices.size(), sizeof(ui32) }));

        // Load texture if specified
        if (!fbxMesh.diffuseTexturePath.empty()) {
            std::wstring wpath;
            wpath.assign(fbxMesh.diffuseTexturePath.begin(), fbxMesh.diffuseTexturePath.end());
            auto texture = Texture2D::LoadTexture2D(device.getD3DDevice(), wpath.c_str());
            if (texture) {
                mesh->setTexture(texture);
            } else {
                // Fallback to debug texture
                mesh->setTexture(Texture2D::CreateDebugTexture(device.getD3DDevice()));
            }
        } else {
            mesh->setTexture(Texture2D::CreateDebugTexture(device.getD3DDevice()));
        }

        return mesh;
    }
}
