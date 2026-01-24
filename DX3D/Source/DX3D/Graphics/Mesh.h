#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <DX3D/Graphics/Abstraction/RenderContext.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <DX3D/Graphics/Texture2D.h>
#include <memory>

namespace dx3d
{
    struct Color
    {
        static const Vec4 BLACK;
        static const Vec4 WHITE;
        static const Vec4 RED;
        static const Vec4 GREEN;
        static const Vec4 BLUE;
    };

    // Define the static constants
    inline const Vec4 Color::BLACK = Vec4(0, 0, 0, 1);
    inline const Vec4 Color::WHITE = Vec4(1, 1, 1, 1);
    inline const Vec4 Color::RED = Vec4(1, 0, 0, 1);
    inline const Vec4 Color::GREEN = Vec4(0, 1, 0, 1);
    inline const Vec4 Color::BLUE = Vec4(0, 0, 1, 1);

    class Mesh {
    public:
        static std::shared_ptr<Mesh> CreateQuadColored(IRenderDevice& device, float w, float h);
        static std::shared_ptr<Mesh> CreateQuadSolidColored(IRenderDevice& device, float w, float h, Vec4 color);
        static std::shared_ptr<Mesh> CreateQuadTextured(IRenderDevice& device, float w, float h);
        static std::shared_ptr<Mesh> CreateCube(IRenderDevice& device, float size);
        static std::shared_ptr<Mesh> CreatePlane(IRenderDevice& device, float width, float height);
        static std::shared_ptr<Mesh> CreateSphere(IRenderDevice& device, float radius, int segments = 16);
        static std::shared_ptr<Mesh> CreateCylinder(IRenderDevice& device, float radius, float height, int segments = 16);
        static std::shared_ptr<Mesh> CreateFromOBJ(IRenderDevice& device, const std::string& path);
        static std::vector<std::shared_ptr<Mesh>> CreateFromOBJMultiMaterial(IRenderDevice& device, const std::string& path);
        static std::shared_ptr<Mesh> CreateFromFBX(IRenderDevice& device, const std::string& path);
        static std::vector<std::shared_ptr<Mesh>> CreateFromFBXMultiMaterial(IRenderDevice& device, const std::string& path);

        // Spritesheet support
        void setSpriteFrame(int frameX, int frameY, int totalFramesX, int totalFramesY);
        void setSpriteFrameByIndex(int frameIndex, int totalFramesX, int totalFramesY);
        void setCustomUVRect(float u, float v, float uWidth, float vHeight);

        void setTexture(std::shared_ptr<Texture2D> texture) { m_texture = texture; }
        bool isTextured() const { return (bool)m_texture; }
        void draw(IRenderContext& ctx) const;
        float getWidth() const { return m_width; }
        float getHeight() const { return m_height; }

        // UV accessors
        float getCurrentU() const { return m_currentU; }
        float getCurrentV() const { return m_currentV; }
        float getCurrentUWidth() const { return m_currentUWidth; }
        float getCurrentVHeight() const { return m_currentVHeight; }

		ui32 getVertexCount() const { return m_vertexCount; }
		void setVertexCount(ui32 theVertexCount) { m_vertexCount = theVertexCount; }
		void setIndexCount(ui32 theIndexCount) { m_indexCount = theIndexCount; }

		ui32 getIndexCount() const { return m_indexCount; }
		void setVB(VertexBufferPtr vb) { m_vb = vb; }
		void setIB(IndexBufferPtr ib) { m_ib = ib; }
		std::shared_ptr<Texture2D> getTexture() const { return m_texture; }
    private:
        VertexBufferPtr m_vb;
        IndexBufferPtr  m_ib;
        Primitive m_prim{ Primitive::Triangles };
        std::shared_ptr<Texture2D>    m_texture;
        ui32 m_vertexCount{ 0 };
        ui32 m_indexCount{ 0 };
        float m_width = 0.0f;
        float m_height = 0.0f;

        // Store current UV coordinates for sprite sheet support
        float m_currentU = 0.0f;
        float m_currentV = 0.0f;
        float m_currentUWidth = 1.0f;
        float m_currentVHeight = 1.0f;

        // Keep reference to device for updating vertex buffer
        IRenderDevice* m_device = nullptr;

        // Helper method to update UV coordinates
        void updateUVCoordinates();
    };
}