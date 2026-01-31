#include <TheEngine/Graphics/VertexBuffer.h>

TheEngine::VertexBuffer::VertexBuffer(const VertexBufferDesc& desc, const GraphicsResourceDesc& gDesc) :
	GraphicsResource(gDesc), m_vertexSize(desc.vertexSize), m_vertexListSize(desc.vertexListSize), m_isDynamic(desc.isDynamic)
{
	if (!desc.vertexList) THEENGINE_LOG_THROW_INVALID_ARG("No vertex list provided.");
	if (!desc.vertexListSize) THEENGINE_LOG_THROW_INVALID_ARG("Vertex list size must be non-zero.");
	if (!desc.vertexSize) THEENGINE_LOG_THROW_INVALID_ARG("Vertex size must be non-zero.");

	D3D11_BUFFER_DESC buffDesc{};
	buffDesc.ByteWidth = desc.vertexListSize * desc.vertexSize;
	buffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	if (m_isDynamic) {
		// Dynamic buffer for updating UV coordinates (spritesheets)
		buffDesc.Usage = D3D11_USAGE_DYNAMIC;
		buffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else {
		// Static buffer for better performance when no updates needed
		buffDesc.Usage = D3D11_USAGE_IMMUTABLE;
		buffDesc.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = desc.vertexList;

	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(
		m_device.CreateBuffer(&buffDesc, &initData, &m_buffer),
		"CreateBuffer failed.");
}

TheEngine::ui32 TheEngine::VertexBuffer::getVertexListSize() const noexcept
{
	return m_vertexListSize;
}

bool TheEngine::VertexBuffer::updateVertexData(const void* newData, ui32 dataSize)
{
	if (!m_isDynamic || !newData) {
		return false;
	}

	// Verify data size matches expected size
	ui32 expectedSize = m_vertexListSize * m_vertexSize;
	if (dataSize != expectedSize) {
		return false;
	}

	// Get the immediate context from the device
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	m_device.GetImmediateContext(context.GetAddressOf());

	// Map the buffer for writing
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(hr)) {
		return false;
	}

	// Copy the new data
	memcpy(mappedResource.pData, newData, dataSize);

	// Unmap the buffer
	context->Unmap(m_buffer.Get(), 0);

	return true;
}