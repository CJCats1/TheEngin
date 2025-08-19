#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/IndexBuffer.h>


dx3d::DeviceContext::DeviceContext(const GraphicsResourceDesc& gDesc): GraphicsResource(gDesc)
{

	DX3DGraphicsLogThrowOnFail(m_device.CreateDeferredContext(0,&m_context),
		"CreateDeferredContext failed.")

	createConstantBuffers();
}

void dx3d::DeviceContext::createConstantBuffers()
{
	// Create constant buffer for transform matrices (now larger)
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(TransformData); // Now includes all 3 matrices
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&bufferDesc, nullptr, m_worldMatrixBuffer.GetAddressOf()),
		"Failed to create transform constant buffer"
	);

	// Create default sampler state
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	DX3DGraphicsLogThrowOnFail(
		m_device.CreateSamplerState(&samplerDesc, m_defaultSampler.GetAddressOf()),
		"Failed to create default sampler state"
	);
	// Initialize with identity matrices
	m_currentTransforms.worldMatrix = Mat4::identity();
	m_currentTransforms.viewMatrix = Mat4::identity();
	m_currentTransforms.projectionMatrix = Mat4::identity();

	// Set initial transform buffer
	updateTransformBuffer();

	// Bind default sampler
	m_context->PSSetSamplers(0, 1, m_defaultSampler.GetAddressOf());
}

void dx3d::DeviceContext::setWorldMatrix(const Mat4& worldMatrix)
{
	m_currentTransforms.worldMatrix = worldMatrix;
	updateTransformBuffer();
}

void dx3d::DeviceContext::setViewMatrix(const Mat4& viewMatrix)
{
	m_currentTransforms.viewMatrix = viewMatrix;
	updateTransformBuffer();
}

void dx3d::DeviceContext::setProjectionMatrix(const Mat4& projectionMatrix)
{
	m_currentTransforms.projectionMatrix = projectionMatrix;
	updateTransformBuffer();
}

void dx3d::DeviceContext::setPSSampler(ui32 slot, ID3D11SamplerState* sampler)
{
	m_context->PSSetSamplers(slot, 1, &sampler);
}
void dx3d::DeviceContext::clearAndSetBackBuffer(const SwapChain& swapChain, const Vec4& color)
{
	f32 fColor[] = {color.x,color.y,color.z,color.w};
	auto rtv = swapChain.m_rtv.Get();
	m_context->ClearRenderTargetView(rtv,fColor);
	m_context->OMSetRenderTargets(1,&rtv,nullptr);
}

void dx3d::DeviceContext::setGraphicsPipelineState(const GraphicsPipelineState& pipeline)
{
	m_context->IASetInputLayout(pipeline.m_layout.Get());
	m_context->VSSetShader(pipeline.m_vs.Get(), nullptr, 0);
	m_context->PSSetShader(pipeline.m_ps.Get(), nullptr, 0);
}

void dx3d::DeviceContext::setVertexBuffer(const VertexBuffer& buffer)
{
	auto stride = buffer.m_vertexSize;
	auto buf = buffer.m_buffer.Get();
	auto offset = 0u;
	m_context->IASetVertexBuffers(0,1,&buf,&stride,&offset);
}

void dx3d::DeviceContext::setViewportSize(const Rect& size)
{
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<f32>(size.width);
	vp.Height = static_cast<f32>(size.height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	m_context->RSSetViewports(1,&vp);
}

void dx3d::DeviceContext::drawTriangleList(ui32 vertexCount, ui32 startVertexLocation)
{
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->Draw(vertexCount, startVertexLocation);
}

void dx3d::DeviceContext::setIndexBuffer(IndexBuffer& ib, DXGI_FORMAT fmt, ui32 offset)
{
	m_context->IASetIndexBuffer(ib.getNative(), fmt, offset);
}

void dx3d::DeviceContext::drawIndexedTriangleList(ui32 indexCount, ui32 startIndex)
{
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->DrawIndexed(static_cast<UINT>(indexCount),
		static_cast<UINT>(startIndex),
		0);
}

void dx3d::DeviceContext::drawIndexedLineList(ui32 indexCount, ui32 startIndex)
{
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	m_context->DrawIndexed(static_cast<UINT>(indexCount),
		static_cast<UINT>(startIndex),
		0);
}
void dx3d::DeviceContext::setPSShaderResource(ui32 slot, ID3D11ShaderResourceView* srv)
{
	m_context->PSSetShaderResources(slot, 1, &srv);
}

void dx3d::DeviceContext::updateTransformBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = m_context->Map(m_worldMatrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr)) {
		memcpy(mappedResource.pData, &m_currentTransforms, sizeof(TransformData));
		m_context->Unmap(m_worldMatrixBuffer.Get(), 0);

		// Bind the constant buffer to vertex shader
		m_context->VSSetConstantBuffers(0, 1, m_worldMatrixBuffer.GetAddressOf());
	}
}