#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include<iostream>

dx3d::DeviceContext::DeviceContext(const GraphicsResourceDesc& gDesc): GraphicsResource(gDesc)
{

	DX3DGraphicsLogThrowOnFail(m_device.CreateDeferredContext(0,&m_context),
		"CreateDeferredContext failed.")

	createConstantBuffers();
	createBlendStates();
	createDepthStates();
}
void dx3d::DeviceContext::createBlendStates()
{
	// Alpha blend (for transparent textures)
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DX3DGraphicsLogThrowOnFail(m_device.CreateBlendState(&blendDesc, m_alphaBlendState.GetAddressOf()),
		"Failed to create alpha blend state");

	// No blend (default opaque)
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	DX3DGraphicsLogThrowOnFail(m_device.CreateBlendState(&blendDesc, m_noBlendState.GetAddressOf()),
		"Failed to create no-blend state");
}

void dx3d::DeviceContext::createDepthStates()
{
	// Default depth (writes enabled)
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DX3DGraphicsLogThrowOnFail(m_device.CreateDepthStencilState(&depthDesc, m_defaultDepthState.GetAddressOf()),
		"Failed to create default depth state");

	// Transparent depth (disable writes)
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DX3DGraphicsLogThrowOnFail(m_device.CreateDepthStencilState(&depthDesc, m_transparentDepthState.GetAddressOf()),
		"Failed to create transparent depth state");
}

// Enable alpha blending
void dx3d::DeviceContext::enableAlphaBlending()
{
	float blendFactor[4] = { 0,0,0,0 };
	m_context->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
}

// Disable alpha blending (opaque)
void dx3d::DeviceContext::disableAlphaBlending()
{
	float blendFactor[4] = { 0,0,0,0 };
	m_context->OMSetBlendState(m_noBlendState.Get(), blendFactor, 0xFFFFFFFF);
}

// Use depth state for transparent objects
void dx3d::DeviceContext::enableTransparentDepth()
{
	m_context->OMSetDepthStencilState(m_transparentDepthState.Get(), 0);
}

// Reset to default depth
void dx3d::DeviceContext::enableDefaultDepth()
{
	m_context->OMSetDepthStencilState(m_defaultDepthState.Get(), 0);
}
// 2. Implementation in DeviceContext.cpp
void dx3d::DeviceContext::setScreenSpaceMatrices(float screenWidth, float screenHeight) {
	// Manual orthographic matrix for screen space
	// Maps (0,0) to top-left, (width,height) to bottom-right
	Mat4 projection;
	float* m = projection.data();

	// Clear matrix
	for (int i = 0; i < 16; i++) m[i] = 0.0f;

	// Orthographic projection: pixel space to NDC
	m[0] = 2.0f / screenWidth;   // X scale
	m[5] = -2.0f / screenHeight; // Y scale (negative to flip Y)
	m[10] = -1.0f;               // Z scale 
	m[12] = -1.0f;               // X offset
	m[13] = 1.0f;                // Y offset  
	m[14] = 0.0f;                // Z offset
	m[15] = 1.0f;                // W

	setViewMatrix(Mat4::identity());
	setProjectionMatrix(projection);

}

void dx3d::DeviceContext::restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix) {
	setViewMatrix(viewMatrix);
	setProjectionMatrix(projectionMatrix);
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
	D3D11_BUFFER_DESC tintDesc{};
	tintDesc.Usage = D3D11_USAGE_DYNAMIC;
	tintDesc.ByteWidth = sizeof(Vec4); // RGBA color
	tintDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	tintDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&tintDesc, nullptr, m_tintBuffer.GetAddressOf()),
		"Failed to create tint constant buffer"
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

    // Light buffer (PS b2)
	D3D11_BUFFER_DESC lightDesc{};
	lightDesc.Usage = D3D11_USAGE_DYNAMIC;
    lightDesc.ByteWidth = 16 + (32 * 10); // 16-byte header + 10 lights (each 32 bytes)
	lightDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&lightDesc, nullptr, m_lightBuffer.GetAddressOf()),
		"Failed to create light constant buffer"
	);

	// Material buffer (PS b3)
	D3D11_BUFFER_DESC matDesc{};
	matDesc.Usage = D3D11_USAGE_DYNAMIC;
	matDesc.ByteWidth = 32; // specularColor(12)+shininess(4)+ambient(4)+pad(12)
	matDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&matDesc, nullptr, m_materialBuffer.GetAddressOf()),
		"Failed to create material constant buffer"
	);

	// Camera buffer (PS b4)
	D3D11_BUFFER_DESC camDesc{};
	camDesc.Usage = D3D11_USAGE_DYNAMIC;
	camDesc.ByteWidth = 16; // float3 + pad
	camDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	camDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&camDesc, nullptr, m_cameraBuffer.GetAddressOf()),
		"Failed to create camera constant buffer"
	);

	// PBR buffer (PS b5)
	D3D11_BUFFER_DESC pbrDesc{};
	pbrDesc.Usage = D3D11_USAGE_DYNAMIC;
	pbrDesc.ByteWidth = 48; // enabled(4) + albedo(12) + metallic(4) + roughness(4) + pad(24)
	pbrDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	pbrDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&pbrDesc, nullptr, m_pbrBuffer.GetAddressOf()),
		"Failed to create PBR constant buffer"
	);

	// Spotlight buffer (PS b6)
	D3D11_BUFFER_DESC spotDesc{};
	spotDesc.Usage = D3D11_USAGE_DYNAMIC;
	spotDesc.ByteWidth = 80; // increased size to match shader expectations
	spotDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	spotDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&spotDesc, nullptr, m_spotlightBuffer.GetAddressOf()),
		"Failed to create spotlight constant buffer"
	);

    // Shadow buffer (PS b7)
	D3D11_BUFFER_DESC shadowDesc{};
	shadowDesc.Usage = D3D11_USAGE_DYNAMIC;
    // 10 matrices (10*64) + count (4) padded to 16 bytes
    shadowDesc.ByteWidth = 64 * 10 + 16;
	shadowDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	shadowDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateBuffer(&shadowDesc, nullptr, m_shadowBuffer.GetAddressOf()),
		"Failed to create shadow constant buffer"
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
void dx3d::DeviceContext::setTint(const Vec4& tint)
{
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_tintBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) {
		memcpy(mapped.pData, &tint, sizeof(Vec4));
		m_context->Unmap(m_tintBuffer.Get(), 0);
	}

	m_context->PSSetConstantBuffers(1, 1, m_tintBuffer.GetAddressOf()); // b1 matches shader
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

void dx3d::DeviceContext::setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient)
{
	struct Header { ui32 count; float pad[3]; };
	struct PackedLight { Vec3 dir; float intensity; Vec3 color; float pad0; };
    struct Buffer { Header h; PackedLight l[10]; };
	Buffer buf{}; buf.h.count = 1;
	buf.l[0] = { direction,intensity,color,0.0f };

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &buf, sizeof(Buffer)); m_context->Unmap(m_lightBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(2, 1, m_lightBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors, const std::vector<float>& intensities)
{
	struct Header { ui32 count; float pad[3]; };
	struct PackedLight { Vec3 dir; float intensity; Vec3 color; float pad0; };
    struct Buffer { Header h; PackedLight l[10]; };
    Buffer buf{}; buf.h.count = (ui32)std::min<size_t>(10, std::min(dirs.size(), std::min(colors.size(), intensities.size())));
	for (ui32 i = 0; i < buf.h.count; ++i) buf.l[i] = { dirs[i], intensities[i], colors[i], 0.0f };

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_lightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &buf, sizeof(Buffer)); m_context->Unmap(m_lightBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(2, 1, m_lightBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setMaterial(const Vec3& specColor, float shininess, float ambient)
{
	struct Mat { Vec3 spec; float shininess; float ambient; float pad[3]; };
	Mat m{ specColor, shininess, ambient };
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_materialBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &m, sizeof(Mat)); m_context->Unmap(m_materialBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(3, 1, m_materialBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setCameraPosition(const Vec3& pos)
{
	struct Cam { Vec3 pos; float pad; };
	Cam c{ pos };
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_cameraBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &c, sizeof(Cam)); m_context->Unmap(m_cameraBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(4, 1, m_cameraBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness)
{
	struct PBRBufferData {
		ui32 usePBR; float pad0[3];
		Vec3 albedo; float metallic;
		float roughness; float pad1[3];
	};
	PBRBufferData p{}; p.usePBR = enabled ? 1u : 0u; p.albedo = albedo; p.metallic = metallic; p.roughness = roughness;
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_pbrBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &p, sizeof(PBRBufferData)); m_context->Unmap(m_pbrBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(5, 1, m_pbrBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
	float innerAngleRadians, float outerAngleRadians, const Vec3& color, float intensity)
{
	struct SpotBuf {
		ui32 enabled; float pad0[3];
		Vec3 pos; float range;
		Vec3 dir; float innerCos;
		float outerCos; Vec3 col;
		float intensity; float pad1[3];
	};
	SpotBuf s{};
	s.enabled = enabled ? 1u : 0u;
	s.pos = position; s.range = range;
	Vec3 nd = direction.normalized(); s.dir = nd; s.innerCos = std::cos(innerAngleRadians);
	s.outerCos = std::cos(outerAngleRadians); s.col = color; s.intensity = intensity;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_spotlightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) { memcpy(mapped.pData, &s, sizeof(SpotBuf)); m_context->Unmap(m_spotlightBuffer.Get(), 0); }
	m_context->PSSetConstantBuffers(6, 1, m_spotlightBuffer.GetAddressOf());
}
void dx3d::DeviceContext::clearAndSetBackBuffer(const SwapChain& swapChain, const Vec4& color)
{
	f32 fColor[] = {color.x,color.y,color.z,color.w};
	auto rtv = swapChain.m_rtv.Get();
	m_context->ClearRenderTargetView(rtv,fColor);
	if (swapChain.m_dsv)
	{
		m_context->ClearDepthStencilView(swapChain.m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		m_context->OMSetRenderTargets(1,&rtv, swapChain.m_dsv.Get());
	}
	else
	{
		m_context->OMSetRenderTargets(1,&rtv,nullptr);
	}
}

void dx3d::DeviceContext::setGraphicsPipelineState(const GraphicsPipelineState& pipeline)
{
    // Allow null input layout for shaders that use SV_VertexID
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
void dx3d::DeviceContext::disableDepthTest()
{
	// Disable depth
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> noDepthState;
	DX3DGraphicsLogThrowOnFail(
		m_device.CreateDepthStencilState(&depthDesc, noDepthState.GetAddressOf()),
		"Failed to create no-depth state"
	);

	m_context->OMSetDepthStencilState(noDepthState.Get(), 0);
}

// Re-enable default depth
void dx3d::DeviceContext::enableDepthTest()
{
	enableDefaultDepth(); 
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

void dx3d::DeviceContext::setPSConstants0(const void* data, ui32 byteSize)
{
    // Create or resize a small dynamic buffer on demand and bind to PS b0
    static Microsoft::WRL::ComPtr<ID3D11Buffer> s_psB0;
    static ui32 s_capacity = 0;
    if (byteSize == 0) {
        ID3D11Buffer* buf = s_psB0.Get();
        m_context->PSSetConstantBuffers(0, 1, &buf);
        return;
    }
    ui32 aligned = (byteSize + 15u) & ~15u;
    if (!s_psB0 || aligned > s_capacity) {
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = aligned;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        s_psB0.Reset();
        DX3DGraphicsLogThrowOnFail(m_device.CreateBuffer(&bd, nullptr, s_psB0.GetAddressOf()),
            "Failed to create PS constant buffer b0");
        s_capacity = aligned;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(m_context->Map(s_psB0.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, data, byteSize);
        m_context->Unmap(s_psB0.Get(), 0);
    }
    ID3D11Buffer* buf = s_psB0.Get();
    m_context->PSSetConstantBuffers(0, 1, &buf);
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

void dx3d::DeviceContext::setShadowMap(ID3D11ShaderResourceView* shadowMap, ID3D11SamplerState* shadowSampler)
{
	// Bind shadow map texture to slot 1 (slot 0 is for regular textures)
	m_context->PSSetShaderResources(1, 1, &shadowMap);
	
	// Bind shadow sampler to slot 1 (slot 0 is for regular samplers)
	m_context->PSSetSamplers(1, 1, &shadowSampler);
}

void dx3d::DeviceContext::setShadowMaps(ID3D11ShaderResourceView* const* shadowMaps, ui32 count, ID3D11SamplerState* shadowSampler)
{
    // Bind up to 10 SRVs starting at slot 1
    ui32 clamped = std::min(count, 10u);
    m_context->PSSetShaderResources(1, clamped, shadowMaps);
    m_context->PSSetSamplers(1, 1, &shadowSampler);
}

void dx3d::DeviceContext::setShadowMatrix(const Mat4& lightViewProj)
{
	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT hr = m_context->Map(m_shadowBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr)) {
        // Write first matrix, zero the rest, set count=1
        struct ShadowCB { Mat4 mats[10]; ui32 count; float pad[3]; } cb{};
        cb.mats[0] = lightViewProj;
        cb.count = 1;
        memcpy(mapped.pData, &cb, sizeof(ShadowCB));
		m_context->Unmap(m_shadowBuffer.Get(), 0);
	}
	m_context->PSSetConstantBuffers(7, 1, m_shadowBuffer.GetAddressOf());
}

void dx3d::DeviceContext::setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices)
{
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_context->Map(m_shadowBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        struct ShadowCB { Mat4 mats[10]; ui32 count; float pad[3]; } cb{};
        ui32 n = static_cast<ui32>(std::min<size_t>(10, lightViewProjMatrices.size()));
        for (ui32 i = 0; i < n; ++i) cb.mats[i] = lightViewProjMatrices[i];
        cb.count = n;
        memcpy(mapped.pData, &cb, sizeof(ShadowCB));
        m_context->Unmap(m_shadowBuffer.Get(), 0);
    }
    m_context->PSSetConstantBuffers(7, 1, m_shadowBuffer.GetAddressOf());
}