#include <DX3D/Graphics/SkyboxRenderer.h>
#include <DX3D/Graphics/Texture2D.h>
#include <wrl.h>
#include <d3d11.h>

using namespace dx3d;

void SkyboxRenderer::render(IRenderDevice& device, IRenderContext& context, Mesh& skybox,
	IRenderPipelineState* pipeline, const Mat4& viewMatrixNoTranslation, bool visible)
{
	if (!visible)
	{
		return;
	}

	auto* d3dDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
	auto* d3dContext = static_cast<ID3D11DeviceContext*>(context.getNativeContextHandle());
	if (!d3dDevice || !d3dContext)
	{
		return;
	}

	if (pipeline)
	{
		context.setGraphicsPipelineState(*pipeline);
	}

	static Microsoft::WRL::ComPtr<ID3D11DepthStencilState> s_skyboxDepthState;
	static Microsoft::WRL::ComPtr<ID3D11RasterizerState> s_skyboxRasterState;
	static Microsoft::WRL::ComPtr<ID3D11SamplerState> s_skyboxSampler;

	if (!s_skyboxDepthState)
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;
		d3dDevice->CreateDepthStencilState(&depthDesc, s_skyboxDepthState.GetAddressOf());
	}

	if (!s_skyboxRasterState)
	{
		D3D11_RASTERIZER_DESC rastDesc{};
		rastDesc.FillMode = D3D11_FILL_SOLID;
		rastDesc.CullMode = D3D11_CULL_FRONT;
		rastDesc.FrontCounterClockwise = FALSE;
		rastDesc.DepthClipEnable = TRUE;
		d3dDevice->CreateRasterizerState(&rastDesc, s_skyboxRasterState.GetAddressOf());
	}

	if (!s_skyboxSampler)
	{
		D3D11_SAMPLER_DESC samp{};
		samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samp.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samp.MinLOD = 0;
		samp.MaxLOD = D3D11_FLOAT32_MAX;
		d3dDevice->CreateSamplerState(&samp, s_skyboxSampler.GetAddressOf());
	}

	d3dContext->OMSetDepthStencilState(s_skyboxDepthState.Get(), 0);
	d3dContext->RSSetState(s_skyboxRasterState.Get());

	context.setViewMatrix(viewMatrixNoTranslation);
	context.setWorldMatrix(Mat4::identity());

	if (auto tex = skybox.getTexture())
	{
		context.setTexture(0, tex->getNativeView());
		context.setSampler(0, s_skyboxSampler.Get());
	}

	skybox.draw(context);

	d3dContext->OMSetDepthStencilState(nullptr, 0);
	d3dContext->RSSetState(nullptr);
}
