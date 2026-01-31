#pragma once
#include <string>
#include <vector>
#include <memory>
#include <TheEngine/Math/Geometry.h>

namespace TheEngine
{
    class IRenderDevice;
    class Mesh;

    // Forward declaration for FBX data structures
    struct FBXVertex
    {
        Vec3 position;
        Vec3 normal;
        Vec2 uv;
        Vec4 color;
    };

    struct FBXMesh
    {
        std::vector<FBXVertex> vertices;
        std::vector<ui32> indices;
        std::string materialName;
        std::string diffuseTexturePath;
    };

    class FBXLoader
    {
    public:
        // Load a single mesh from FBX file
        static std::shared_ptr<Mesh> LoadMesh(IRenderDevice& device, const std::string& path);
        
        // Load multiple meshes from FBX file (for multi-material models)
        static std::vector<std::shared_ptr<Mesh>> LoadMeshes(IRenderDevice& device, const std::string& path);
        
        // Check if FBX file exists and is valid
        static bool IsValidFBXFile(const std::string& path);
        
    private:
        // Parse FBX file and extract mesh data
        static std::vector<FBXMesh> ParseFBXFile(const std::string& path);
        
        // Convert FBX mesh data to engine Mesh
        static std::shared_ptr<Mesh> CreateMeshFromFBXData(IRenderDevice& device, const FBXMesh& fbxMesh);
    };
}
