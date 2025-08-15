#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <memory>
#include <wincodec.h> // For WIC
#include <comdef.h>

namespace dx3d
{
    class Texture2D {
    public:
        Texture2D(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
            : m_srv(srv) {
        }

        ID3D11ShaderResourceView* getSRV() const { return m_srv.Get(); }

        static std::shared_ptr<Texture2D> LoadTexture2D(ID3D11Device* device, const wchar_t* filePath)
        {
            // Initialize COM
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);

            // Create WIC factory
            Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory2,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory)
            );

            if (FAILED(hr)) return nullptr;

            // Create decoder
            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            hr = wicFactory->CreateDecoderFromFilename(
                filePath,
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                &decoder
            );

            if (FAILED(hr)) return nullptr;

            // Get first frame
            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) return nullptr;

            // Convert to RGBA format
            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            hr = wicFactory->CreateFormatConverter(&converter);
            if (FAILED(hr)) return nullptr;

            hr = converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom
            );

            if (FAILED(hr)) return nullptr;

            // Get image dimensions
            UINT width, height;
            hr = converter->GetSize(&width, &height);
            if (FAILED(hr)) return nullptr;

            // Create buffer for pixel data
            UINT stride = width * 4; // 4 bytes per pixel (RGBA)
            UINT imageSize = stride * height;
            std::unique_ptr<BYTE[]> pixels(new BYTE[imageSize]);

            // Copy pixel data
            hr = converter->CopyPixels(
                nullptr,
                stride,
                imageSize,
                pixels.get()
            );

            if (FAILED(hr)) return nullptr;

            // Create D3D11 texture
            D3D11_TEXTURE2D_DESC texDesc{};
            texDesc.Width = width;
            texDesc.Height = height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = pixels.get();
            data.SysMemPitch = stride;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            hr = device->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            // Create shader resource view
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            return std::make_shared<Texture2D>(srv);
        }

        static std::shared_ptr<Texture2D> CreateWhiteTexture(ID3D11Device* device)
        {
            D3D11_TEXTURE2D_DESC texDesc{};
            texDesc.Width = 1;
            texDesc.Height = 1;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            UINT whitePixel[1] = { 0xFFFFFFFF }; // RGBA = 1,1,1,1
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = whitePixel;
            data.SysMemPitch = sizeof(UINT);

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            device->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());

            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());

            return std::make_shared<Texture2D>(srv);
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    };
}