#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
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
        static std::shared_ptr<Mesh> CreateQuadColored(GraphicsDevice& device, float w, float h);
        static std::shared_ptr<Mesh> CreateQuadSolidColored(GraphicsDevice& device, float w, float h, Vec4 color);
        static std::shared_ptr<Mesh> CreateQuadTextured(GraphicsDevice& device, float w, float h);
        static std::shared_ptr<Mesh> CreateCube(GraphicsDevice& device, float size);
        static std::shared_ptr<Mesh> CreatePlane(GraphicsDevice& device, float width, float height);
        static std::shared_ptr<Mesh> CreateSphere(GraphicsDevice& device, float radius, int segments = 16);
        static std::shared_ptr<Mesh> CreateCylinder(GraphicsDevice& device, float radius, float height, int segments = 16);
        static std::shared_ptr<Mesh> CreateFromOBJ(GraphicsDevice& device, const std::string& path);
        static std::vector<std::shared_ptr<Mesh>> CreateFromOBJMultiMaterial(GraphicsDevice& device, const std::string& path);
        static std::shared_ptr<Mesh> CreateFromFBX(GraphicsDevice& device, const std::string& path);
        static std::vector<std::shared_ptr<Mesh>> CreateFromFBXMultiMaterial(GraphicsDevice& device, const std::string& path);

        // Spritesheet support
        void setSpriteFrame(int frameX, int frameY, int totalFramesX, int totalFramesY);
        void setSpriteFrameByIndex(int frameIndex, int totalFramesX, int totalFramesY);
        void setCustomUVRect(float u, float v, float uWidth, float vHeight);

        void setTexture(std::shared_ptr<Texture2D> texture) { m_texture = texture; }
        bool isTextured() const { return (bool)m_texture; }
        void draw(DeviceContext& ctx) const;
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
		void setVB(std::shared_ptr<VertexBuffer> vb) { m_vb = vb; }
		void setIB(std::shared_ptr<IndexBuffer> ib) { m_ib = ib; }
		std::shared_ptr<Texture2D> getTexture() const { return m_texture; }
    private:
        std::shared_ptr<VertexBuffer> m_vb;
        std::shared_ptr<IndexBuffer>  m_ib;
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
        GraphicsDevice* m_device = nullptr;

        // Helper method to update UV coordinates
        void updateUVCoordinates();
    };
}