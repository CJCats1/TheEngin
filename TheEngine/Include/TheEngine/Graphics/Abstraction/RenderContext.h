#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/GraphicsHandles.h>
#include <TheEngine/Graphics/Abstraction/RenderSwapChain.h>
#include <vector>

namespace TheEngine
{
    class IRenderPipelineState;
    class IRenderVertexBuffer;
    class IRenderIndexBuffer;

    class THEENGINE_API IRenderContext
    {
    public:
        virtual ~IRenderContext() = default;

        virtual void clearAndSetBackBuffer(const IRenderSwapChain& swapChain, const Vec4& color) = 0;
        virtual void setGraphicsPipelineState(const IRenderPipelineState& pipeline) = 0;
        virtual void setVertexBuffer(const IRenderVertexBuffer& buffer) = 0;
        virtual void setViewportSize(const Rect& size) = 0;
        virtual void drawTriangleList(ui32 vertexCount, ui32 startVertexLocation) = 0;

        virtual void setIndexBuffer(IRenderIndexBuffer& ib, IndexFormat fmt = IndexFormat::UInt32, ui32 offset = 0) = 0;
        virtual void drawIndexedTriangleList(ui32 indexCount, ui32 startIndex) = 0;
        virtual void drawIndexedLineList(ui32 indexCount, ui32 startIndex) = 0;

        virtual void setTexture(ui32 slot, NativeGraphicsHandle srv) = 0;
        virtual void setSampler(ui32 slot, NativeGraphicsHandle sampler) = 0;

        virtual void setWorldMatrix(const Mat4& worldMatrix) = 0;
        virtual void setViewMatrix(const Mat4& viewMatrix) = 0;
        virtual void setProjectionMatrix(const Mat4& projectionMatrix) = 0;
        virtual void updateTransformBuffer() = 0;

        virtual void enableAlphaBlending() = 0;
        virtual void disableAlphaBlending() = 0;
        virtual void enableTransparentDepth() = 0;
        virtual void enableDefaultDepth() = 0;
        virtual void setScreenSpaceMatrices(float screenWidth, float screenHeight) = 0;
        virtual void restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix) = 0;
        virtual void setTint(const Vec4& tint) = 0;

        virtual void setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient) = 0;
        virtual void setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors,
            const std::vector<float>& intensities) = 0;
        virtual void setMaterial(const Vec3& specColor, float shininess, float ambient) = 0;
        virtual void setCameraPosition(const Vec3& pos) = 0;
        virtual void setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness) = 0;
        virtual void setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
            float innerAngleRadians, float outerAngleRadians, const Vec3& color, float intensity) = 0;

        virtual void disableDepthTest() = 0;
        virtual void enableDepthTest() = 0;

        virtual void setPSConstants0(const void* data, ui32 byteSize) = 0;
        virtual void setShadowMap(NativeGraphicsHandle shadowMap, NativeGraphicsHandle shadowSampler) = 0;
        virtual void setShadowMaps(NativeGraphicsHandle const* shadowMaps, ui32 count, NativeGraphicsHandle shadowSampler) = 0;
        virtual void setShadowMatrix(const Mat4& lightViewProj) = 0;
        virtual void setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices) = 0;

        virtual NativeGraphicsHandle getDefaultSamplerHandle() const = 0;
        virtual NativeGraphicsHandle getNativeContextHandle() const noexcept = 0;
        virtual void clearShaderResourceBindings() = 0;
    };

    using RenderContextPtr = std::shared_ptr<IRenderContext>;
}
