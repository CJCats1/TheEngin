#include <DX3D/Graphics/ShadowMap.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <iostream>

namespace dx3d
{
    ShadowMap::ShadowMap(const GraphicsResourceDesc& gDesc, uint32_t width, uint32_t height)
        : GraphicsResource(gDesc), m_width(width), m_height(height)
    {
        createDepthTexture();
        createDepthViews();
    }

    void ShadowMap::createDepthTexture()
    {
        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = m_width;
        depthDesc.Height = m_height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24-bit depth, 8-bit stencil
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        depthDesc.CPUAccessFlags = 0;
        depthDesc.MiscFlags = 0;

        HRESULT hr = m_device.CreateTexture2D(&depthDesc, nullptr, m_depthTexture.GetAddressOf());
        if (FAILED(hr)) {
            std::cerr << "Failed to create shadow map depth texture" << std::endl;
            throw std::runtime_error("Failed to create shadow map depth texture");
        }
    }

    void ShadowMap::createDepthViews()
    {
        // Create depth stencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        HRESULT hr = m_device.CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_depthDSV.GetAddressOf());
        if (FAILED(hr)) {
            std::cerr << "Failed to create shadow map depth stencil view" << std::endl;
            throw std::runtime_error("Failed to create shadow map depth stencil view");
        }

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // Read depth as R24
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = m_device.CreateShaderResourceView(m_depthTexture.Get(), &srvDesc, m_depthSRV.GetAddressOf());
        if (FAILED(hr)) {
            std::cerr << "Failed to create shadow map shader resource view" << std::endl;
            throw std::runtime_error("Failed to create shadow map shader resource view");
        }
    }

    void ShadowMap::clear(ID3D11DeviceContext* context)
    {
        context->ClearDepthStencilView(m_depthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    void ShadowMap::setAsRenderTarget(ID3D11DeviceContext* context)
    {
        // Set depth stencil view as the only render target
        context->OMSetRenderTargets(0, nullptr, m_depthDSV.Get());
    }

    void ShadowMap::setViewport(ID3D11DeviceContext* context)
    {
        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        
        context->RSSetViewports(1, &viewport);
    }

    Microsoft::WRL::ComPtr<ID3D11SamplerState> ShadowMap::createShadowSampler(ID3D11Device* device)
    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        samplerDesc.BorderColor[0] = 1.0f; // White border (no shadow)
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
        HRESULT hr = device->CreateSamplerState(&samplerDesc, sampler.GetAddressOf());
        if (FAILED(hr)) {
            std::cerr << "Failed to create shadow map sampler" << std::endl;
            throw std::runtime_error("Failed to create shadow map sampler");
        }

        return sampler;
    }
}
