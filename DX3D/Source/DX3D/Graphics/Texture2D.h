#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <memory>
#include <wincodec.h> // For WIC
#include <comdef.h>
#include <iostream>
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

            if (FAILED(hr))
            {
                return nullptr;
            }

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

        static std::shared_ptr<Texture2D> CreateDebugTexture(ID3D11Device* device)
        {
            std::cout << "DebugTextureUsed" << std::endl;
			int size = 8; // 8x8 
            // Small checkerboard texture (default 8x8)
            UINT width = size;
            UINT height = size;

            std::unique_ptr<UINT[]> pixels(new UINT[width * height]);

            UINT magenta = 0xFFFF00FF;   
            UINT black = 0xFF000000;
            UINT transparent = 0x00000000;

            // Fill checkerboard pattern
            for (UINT y = 0; y < height; y++)
            {
                for (UINT x = 0; x < width; x++)
                {
                    bool isCyan = ((x / (size / 2)) + (y / (size / 2))) % 2 == 0;
                    pixels[y * width + x] = isCyan ? magenta : black;
                }
            }

            // Describe texture
            D3D11_TEXTURE2D_DESC texDesc{};
            texDesc.Width = width;
            texDesc.Height = height;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            // Subresource data
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = pixels.get();
            data.SysMemPitch = width * sizeof(UINT);

            // Create texture
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HRESULT hr = device->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            // Create SRV
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            return std::make_shared<Texture2D>(srv);
        }

        // Create a cubemap with solid colors for testing
        static std::shared_ptr<Texture2D> CreateSkyboxCubemap(ID3D11Device* device) {
            // Create a simple cubemap with solid colors
            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = 512;
            texDesc.Height = 512;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 6; // 6 faces for cubemap
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.SampleDesc.Quality = 0;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

            // Create solid color data for each face
            std::vector<D3D11_SUBRESOURCE_DATA> initData(6);
            std::vector<std::vector<unsigned char>> faceData(6);
            
            for (int face = 0; face < 6; ++face) {
                faceData[face].resize(512 * 512 * 4);
                
                // Different colors for each face
                unsigned char r, g, b;
                switch (face) {
                    case 0: r = 255; g = 0; b = 0; break;   // Right - Red
                    case 1: r = 0; g = 255; b = 0; break;   // Left - Green
                    case 2: r = 0; g = 0; b = 255; break;  // Top - Blue
                    case 3: r = 255; g = 255; b = 0; break; // Bottom - Yellow
                    case 4: r = 255; g = 0; b = 255; break; // Back - Magenta
                    case 5: r = 0; g = 255; b = 255; break; // Front - Cyan
                }
                
                for (size_t i = 0; i < faceData[face].size(); i += 4) {
                    faceData[face][i] = r;     // R
                    faceData[face][i + 1] = g; // G
                    faceData[face][i + 2] = b; // B
                    faceData[face][i + 3] = 255; // A
                }
                
                initData[face].pSysMem = faceData[face].data();
                initData[face].SysMemPitch = 512 * 4;
                initData[face].SysMemSlicePitch = 0;
            }

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HRESULT hr = device->CreateTexture2D(&texDesc, initData.data(), tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            // Create SRV
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = texDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = 1;
            srvDesc.TextureCube.MostDetailedMip = 0;
            
            hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            return std::make_shared<Texture2D>(srv);
        }

        // Load cubemap from a single 4x3 cross-layout image file
        static std::shared_ptr<Texture2D> LoadSkyboxCubemap(ID3D11Device* device, const wchar_t* filePath) {
            // Decode with WIC to 32bpp RGBA
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
            if (FAILED(hr)) return nullptr;

            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            hr = wicFactory->CreateDecoderFromFilename(filePath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
            if (FAILED(hr)) return nullptr;

            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) return nullptr;

            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
            hr = wicFactory->CreateFormatConverter(&converter);
            if (FAILED(hr)) return nullptr;

            hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
            if (FAILED(hr)) return nullptr;

            UINT width = 0, height = 0;
            hr = converter->GetSize(&width, &height);
            if (FAILED(hr) || width == 0 || height == 0) return nullptr;

            // Expect 4x3 cross; derive face size
            if (width % 4 != 0 || height % 3 != 0) {
                // Fallback: not a cross; try simple 2D texture for now
                return LoadTexture2D(device, filePath);
            }
            UINT faceSize = width / 4; // also equals height / 3
            if (faceSize * 3 != height) return LoadTexture2D(device, filePath);

            // Read full image pixels
            const UINT stride = width * 4;
            const UINT imageSize = stride * height;
            std::unique_ptr<BYTE[]> pixels(new BYTE[imageSize]);
            hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.get());
            if (FAILED(hr)) return nullptr;

            // Helper to copy a rect from source into a face buffer with rotation
            enum class Rot { R0, R90, R180, R270 };
            auto copyRectRot = [&](UINT srcX, UINT srcY, Rot rot, std::vector<unsigned char>& out) {
                out.resize(faceSize * faceSize * 4);
                for (UINT y = 0; y < faceSize; ++y) {
                    for (UINT x = 0; x < faceSize; ++x) {
                        UINT sx = 0, sy = 0;
                        switch (rot) {
                            case Rot::R0:   sx = srcX + x;                 sy = srcY + y;                 break;
                            case Rot::R90:  sx = srcX + y;                 sy = srcY + (faceSize - 1 - x); break; // CW
                            case Rot::R180: sx = srcX + (faceSize - 1 - x); sy = srcY + (faceSize - 1 - y); break;
                            case Rot::R270: sx = srcX + (faceSize - 1 - y); sy = srcY + x;                 break; // CCW
                        }
                        const BYTE* srcPx = pixels.get() + sy * stride + sx * 4;
                        BYTE* dstPx = out.data() + (y * faceSize + x) * 4;
                        dstPx[0] = srcPx[0];
                        dstPx[1] = srcPx[1];
                        dstPx[2] = srcPx[2];
                        dstPx[3] = srcPx[3];
                    }
                }
            };

            // Cross layout mapping → D3D cubemap face order: +X, -X, +Y, -Y, +Z, -Z
            // Standard layout positions (tile coords):
            //       (1,0)
            // (0,1) (1,1) (2,1) (3,1)
            //       (1,2)
            // where each tile is faceSize x faceSize
            std::vector<std::vector<unsigned char>> faceData(6);
            // Apply rotations typical for a horizontal cross (common DDS/Unity layout)
            // Adjust these if the source uses a different convention.
            // Face order: +X, -X, +Y, -Y, +Z, -Z
            // Tile coords: (2,1), (0,1), (1,0), (1,2), (1,1), (3,1)

            // +X at (2,1), rotate 90 CW
            copyRectRot(faceSize * 2, faceSize * 1, Rot::R0,  faceData[0]);
            // -X at (0,1), rotate 90 CCW
            copyRectRot(faceSize * 0, faceSize * 1, Rot::R0, faceData[1]);
            // +Y (top) at (1,0), no rotation
            copyRectRot(faceSize * 1, faceSize * 0, Rot::R0,   faceData[2]);
            // -Y (bottom) at (1,2), rotate 180
            copyRectRot(faceSize * 1, faceSize * 2, Rot::R0, faceData[3]);
            // +Z (front) at (1,1), no rotation
            copyRectRot(faceSize * 1, faceSize * 1, Rot::R0,   faceData[4]);
            // -Z (back) at (3,1), rotate 180 so horizon aligns when looking back
            copyRectRot(faceSize * 3, faceSize * 1, Rot::R0, faceData[5]);

            // Create cubemap texture
            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = faceSize;
            texDesc.Height = faceSize;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 6;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

            std::vector<D3D11_SUBRESOURCE_DATA> init(6);
            for (int i = 0; i < 6; ++i) {
                init[i].pSysMem = faceData[i].data();
                init[i].SysMemPitch = faceSize * 4;
                init[i].SysMemSlicePitch = 0;
            }

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            hr = device->CreateTexture2D(&texDesc, init.data(), tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            // Create SRV as cubemap
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = texDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = 1;
            srvDesc.TextureCube.MostDetailedMip = 0;

            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;

            return std::make_shared<Texture2D>(srv);
        }

       
        static std::shared_ptr<Texture2D> CreateNoiseTexture(ID3D11Device* device, int size = 256) {
            std::unique_ptr<BYTE[]> pixels(new BYTE[size * size * 4]);
            
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int index = (y * size + x) * 4;
                    
                    // Generate Perlin-like noise
                    float noise = 0.0f;
                    float frequency = 0.1f;
                    float amplitude = 1.0f;
                    
                    for (int i = 0; i < 4; ++i) {
                        noise += sin(x * frequency) * cos(y * frequency) * amplitude;
                        frequency *= 2.0f;
                        amplitude *= 0.5f;
                    }
                    
                    // Normalize to 0-1
                    noise = (noise + 1.0f) * 0.5f;
                    noise = std::max(0.0f, std::min(1.0f, noise));
                    
                    BYTE noiseValue = (BYTE)(noise * 255);
                    pixels[index] = noiseValue;     // R
                    pixels[index + 1] = noiseValue; // G
                    pixels[index + 2] = noiseValue; // B
                    pixels[index + 3] = 255;        // A
                }
            }
            
            // Create texture
            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = size;
            texDesc.Height = size;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = pixels.get();
            data.SysMemPitch = size * 4;
            
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HRESULT hr = device->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;
            
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;
            
            return std::make_shared<Texture2D>(srv);
        }
        
        // Create a sun texture with glow effect
        static std::shared_ptr<Texture2D> CreateSunTexture(ID3D11Device* device, int size = 256) {
            std::unique_ptr<BYTE[]> pixels(new BYTE[size * size * 4]);
            
            for (int y = 0; y < size; ++y) {
                for (int x = 0; x < size; ++x) {
                    int index = (y * size + x) * 4;
                    
                    // Calculate distance from center
                    float centerX = size / 2.0f;
                    float centerY = size / 2.0f;
                    float dist = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
                    float maxDist = size / 2.0f;
                    
                    // Create sun with bright center and soft glow
                    float normalizedDist = dist / maxDist;
                    
                    // Bright white center
                    float centerRadius = 0.15f;
                    float glowRadius = 0.8f;
                    
                    float intensity = 0.0f;
                    if (normalizedDist <= centerRadius) {
                        // Bright white center
                        intensity = 1.0f;
                    } else if (normalizedDist <= glowRadius) {
                        // Soft glow falloff
                        float glowFactor = (glowRadius - normalizedDist) / (glowRadius - centerRadius);
                        intensity = glowFactor * glowFactor; // Quadratic falloff for softer glow
                    }
                    
                    // Add some noise for realistic sun surface
                    float noise = (sin(x * 0.3f) * cos(y * 0.3f) + 1.0f) * 0.5f;
                    intensity *= (0.8f + 0.2f * noise);
                    
                    // Clamp intensity
                    intensity = std::max(0.0f, std::min(1.0f, intensity));
                    
                    // Sun color: bright white with slight yellow tint
                    float r = intensity;
                    float g = intensity * 0.95f; // Slight yellow tint
                    float b = intensity * 0.8f;  // More yellow
                    float a = intensity;
                    
                    pixels[index] = (BYTE)(r * 255);     // R
                    pixels[index + 1] = (BYTE)(g * 255); // G
                    pixels[index + 2] = (BYTE)(b * 255); // B
                    pixels[index + 3] = (BYTE)(a * 255); // A
                }
            }
            
            // Create texture
            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = size;
            texDesc.Height = size;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = pixels.get();
            data.SysMemPitch = size * 4;
            
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HRESULT hr = device->CreateTexture2D(&texDesc, &data, tex.GetAddressOf());
            if (FAILED(hr)) return nullptr;
            
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            hr = device->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf());
            if (FAILED(hr)) return nullptr;
            
            return std::make_shared<Texture2D>(srv);
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    };
}