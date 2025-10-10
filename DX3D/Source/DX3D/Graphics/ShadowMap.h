#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <memory>
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Math/Geometry.h>

namespace dx3d
{
    class ShadowMap : public GraphicsResource
    {
    public:
        ShadowMap(const GraphicsResourceDesc& gDesc, uint32_t width = 1024, uint32_t height = 1024);
        
        // Get the depth texture as a shader resource
        ID3D11ShaderResourceView* getDepthSRV() const { return m_depthSRV.Get(); }
        
        // Get the depth stencil view for rendering
        ID3D11DepthStencilView* getDepthDSV() const { return m_depthDSV.Get(); }
        
        // Get dimensions
        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        
        // Clear the shadow map
        void clear(ID3D11DeviceContext* context);
        
        // Set as render target for shadow map generation
        void setAsRenderTarget(ID3D11DeviceContext* context);
        
        // Set viewport for shadow map rendering
        void setViewport(ID3D11DeviceContext* context);
        
        // Create a comparison sampler for shadow map sampling
        static Microsoft::WRL::ComPtr<ID3D11SamplerState> createShadowSampler(ID3D11Device* device);

    private:
        uint32_t m_width;
        uint32_t m_height;
        
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthDSV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;
        
        void createDepthTexture();
        void createDepthViews();
    };
}
