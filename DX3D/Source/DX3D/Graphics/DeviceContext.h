#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/Abstraction/RenderContext.h>

namespace dx3d
{
	class DeviceContext final : public GraphicsResource, public IRenderContext
	{
	public:
		struct TransformData {
			Mat4 worldMatrix;
			Mat4 viewMatrix;
			Mat4 projectionMatrix;
		};
		explicit DeviceContext(const GraphicsResourceDesc& gDesc);
		void clearAndSetBackBuffer(const IRenderSwapChain& swapChain, const Vec4& color) override;
		void setGraphicsPipelineState(const IRenderPipelineState& pipeline) override;
		void setVertexBuffer(const IRenderVertexBuffer& buffer) override;
		void setViewportSize(const Rect& size) override;
		void drawTriangleList(ui32 vertexCount, ui32 startVertexLocation) override;

		void setIndexBuffer(IRenderIndexBuffer& ib, IndexFormat fmt = IndexFormat::UInt32, ui32 offset = 0) override;
		void drawIndexedTriangleList(ui32 indexCount, ui32 startIndex) override;
		void drawIndexedLineList(ui32 indexCount, ui32 startIndex) override;
		void setTexture(ui32 slot, NativeGraphicsHandle srv) override;

		void setWorldMatrix(const Mat4& worldMatrix) override;
		void setSampler(ui32 slot, NativeGraphicsHandle sampler) override;

		void setViewMatrix(const Mat4& viewMatrix) override;
		void setProjectionMatrix(const Mat4& projectionMatrix) override;
		void updateTransformBuffer() override;

		void enableAlphaBlending() override;
		void disableAlphaBlending() override;
		void enableTransparentDepth() override;
		void enableDefaultDepth() override;
		void setScreenSpaceMatrices(float screenWidth, float screenHeight) override;
		void restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix) override;
		TransformData getTransformData() { return m_currentTransforms; };
		void setTint(const Vec4& tint) override;

		// Lighting (PS b2)
		void setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient) override;
		void setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors, const std::vector<float>& intensities) override;
		void setMaterial(const Vec3& specColor, float shininess, float ambient) override;
		void setCameraPosition(const Vec3& pos) override;

		// PBR controls (PS b5)
		void setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness) override;
		// Spotlight (PS b6)
		void setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
			float innerAngleRadians, float outerAngleRadians, const Vec3& color, float intensity) override;

	void disableDepthTest() override;
	void enableDepthTest() override;

	// Utility: set a small PS constant buffer at slot 0
	void setPSConstants0(const void* data, ui32 byteSize) override;
	
	// Shadow mapping support
		void setShadowMap(NativeGraphicsHandle shadowMap, NativeGraphicsHandle shadowSampler) override;
		void setShadowMaps(NativeGraphicsHandle const* shadowMaps, ui32 count, NativeGraphicsHandle shadowSampler) override;
		void setShadowMatrix(const Mat4& lightViewProj) override;
		void setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices) override;
	
	NativeGraphicsHandle getDefaultSamplerHandle() const override { return m_defaultSampler.Get(); }
	NativeGraphicsHandle getNativeContextHandle() const noexcept override { return m_context.Get(); }
	void clearShaderResourceBindings() override;

	private:
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context{};
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_defaultSampler;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_worldMatrixBuffer;
		Microsoft::WRL::ComPtr<ID3D11BlendState> m_alphaBlendState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> m_noBlendState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_transparentDepthState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_defaultDepthState;

		TransformData m_currentTransforms;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_tintBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_materialBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_cameraBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pbrBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_spotlightBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_shadowBuffer;
		friend class GraphicsDevice;
		void createConstantBuffers();
		void createBlendStates();
		void createDepthStates();
	};

}