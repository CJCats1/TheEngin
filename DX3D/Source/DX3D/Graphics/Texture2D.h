#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <memory>
namespace dx3d
{
    class Texture2D {
    public:
        Texture2D(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
            : m_srv(srv) {
        }

        ID3D11ShaderResourceView* getSRV() const { return m_srv.Get(); }
        //needto implement this 
        inline std::shared_ptr<Texture2D> LoadTexture2D(ID3D11Device* device, const wchar_t* filePath)
        {
            
        }
    private:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    };
}