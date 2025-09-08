#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{
	class VertexBuffer final : public GraphicsResource
	{
	public:
		VertexBuffer(const VertexBufferDesc& desc, const GraphicsResourceDesc& gDesc);
		ui32 getVertexListSize() const noexcept;
		ui32 getVertexSize() const noexcept { return m_vertexSize; }

		// Method to update vertex buffer data (needed for spritesheet UV updates)
		bool updateVertexData(const void* newData, ui32 dataSize);

		// Get the native D3D11 buffer (for advanced operations)
		ID3D11Buffer* getNativeBuffer() const { return m_buffer.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
		ui32 m_vertexSize{};
		ui32 m_vertexListSize{};
		bool m_isDynamic = false; // Track if buffer supports updates
		friend class DeviceContext;
		friend class GraphicsDevice;
	};
}