#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Camera.h>

namespace dx3d
{
	class DeviceContext final : public GraphicsResource
	{
	public:
		struct TransformData {
			Mat4 worldMatrix;
			Mat4 viewMatrix;
			Mat4 projectionMatrix;
		};
		explicit DeviceContext(const GraphicsResourceDesc& gDesc);
		void clearAndSetBackBuffer(const SwapChain& swapChain, const Vec4& color);
		void setGraphicsPipelineState(const GraphicsPipelineState& pipeline);
		void setVertexBuffer(const VertexBuffer& buffer);
		void setViewportSize(const Rect& size);
		void drawTriangleList(ui32 vertexCount, ui32 startVertexLocation);

		void setIndexBuffer(IndexBuffer& ib, DXGI_FORMAT fmt = DXGI_FORMAT_R32_UINT, ui32 offset = 0);
		void drawIndexedTriangleList(ui32 indexCount, ui32 startIndex);
		void drawIndexedLineList(ui32 indexCount, ui32 startIndex);
		void setPSShaderResource(ui32 slot, ID3D11ShaderResourceView* srv);

		void setWorldMatrix(const Mat4& worldMatrix);
		void setPSSampler(ui32 slot, ID3D11SamplerState* sampler);

		void setViewMatrix(const Mat4& viewMatrix);
		void setProjectionMatrix(const Mat4& projectionMatrix);
		void updateTransformBuffer();

		void enableAlphaBlending();
		void disableAlphaBlending();
		void enableTransparentDepth();
		void enableDefaultDepth();
		void setScreenSpaceMatrices(float screenWidth, float screenHeight);
		void restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix);
		TransformData getTransformData() { return m_currentTransforms; };
		void setTint(const Vec4& tint);

		// Lighting (PS b2)
		void setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient);
		void setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors, const std::vector<float>& intensities);
		void setMaterial(const Vec3& specColor, float shininess, float ambient);
		void setCameraPosition(const Vec3& pos);

	void disableDepthTest();
	void enableDepthTest();

	// Utility: set a small PS constant buffer at slot 0
	void setPSConstants0(const void* data, ui32 byteSize);
	
	ID3D11SamplerState* getDefaultSampler() const { return m_defaultSampler.Get(); }

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
		friend class GraphicsDevice;
		void createConstantBuffers();
		void createBlendStates();
		void createDepthStates();
	};

}