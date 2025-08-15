#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/GraphicsDevice.h>



namespace dx3d
{
	class DeviceContext final : public GraphicsResource
	{
	public:
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
	private:
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context{};

		friend class GraphicsDevice;
	};

}