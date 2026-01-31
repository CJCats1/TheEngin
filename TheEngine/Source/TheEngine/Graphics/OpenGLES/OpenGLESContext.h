#pragma once
#include <TheEngine/Graphics/Abstraction/RenderContext.h>
#include <TheEngine/Core/Logger.h>

namespace TheEngine
{
	class OpenGLESContext final : public IRenderContext
	{
	public:
		explicit OpenGLESContext(Logger& logger);

		void clearAndSetBackBuffer(const IRenderSwapChain& swapChain, const Vec4& color) override;
		void setGraphicsPipelineState(const IRenderPipelineState& pipeline) override;
		void setVertexBuffer(const IRenderVertexBuffer& buffer) override;
		void setIndexBuffer(IRenderIndexBuffer& ib, IndexFormat fmt, ui32 offset) override;
		void setViewportSize(const Rect& size) override;
		void drawTriangleList(ui32 vertexCount, ui32 startVertexLocation) override;
		void drawIndexedTriangleList(ui32 indexCount, ui32 startIndex) override;
		void drawIndexedLineList(ui32 indexCount, ui32 startIndex) override;
		void setTexture(ui32 slot, NativeGraphicsHandle srv) override;
		void setSampler(ui32 slot, NativeGraphicsHandle sampler) override;
		void setWorldMatrix(const Mat4& worldMatrix) override;
		void setViewMatrix(const Mat4& viewMatrix) override;
		void setProjectionMatrix(const Mat4& projectionMatrix) override;
		void updateTransformBuffer() override;
		void enableAlphaBlending() override;
		void disableAlphaBlending() override;
		void enableTransparentDepth() override;
		void enableDefaultDepth() override;
		void setScreenSpaceMatrices(float screenWidth, float screenHeight) override;
		void restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix) override;
		void setTint(const Vec4& tint) override;
		void setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient) override;
		void setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors, const std::vector<float>& intensities) override;
		void setMaterial(const Vec3& specColor, float shininess, float ambient) override;
		void setCameraPosition(const Vec3& pos) override;
		void setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness) override;
		void setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
			float innerAngleRadians, float outerAngleRadians, const Vec3& color, float intensity) override;
		void disableDepthTest() override;
		void enableDepthTest() override;
		void setShadowMap(NativeGraphicsHandle shadowMap, NativeGraphicsHandle shadowSampler) override;
		void setShadowMaps(NativeGraphicsHandle const* shadowMaps, ui32 count, NativeGraphicsHandle shadowSampler) override;
		void setShadowMatrix(const Mat4& lightViewProj) override;
		void setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices) override;
		NativeGraphicsHandle getDefaultSamplerHandle() const override;
		NativeGraphicsHandle getNativeContextHandle() const noexcept override;
		void setPSConstants0(const void* data, ui32 byteSize) override;
		void clearShaderResourceBindings() override;

	private:
		Logger& m_logger;
		unsigned int m_program{};
		unsigned int m_boundVao{};
		unsigned int m_indexType{};
		Mat4 m_worldMatrix{};
		Mat4 m_viewMatrix{};
		Mat4 m_projectionMatrix{};
		Vec4 m_tint{ 1.0f, 1.0f, 1.0f, 1.0f };
	};
}
