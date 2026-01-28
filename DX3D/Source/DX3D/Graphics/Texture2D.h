#pragma once
#include <memory>
#if defined(_WIN32)
#include <wrl.h>
#include <d3d11.h>
#include <wincodec.h>
#include <comdef.h>
#else
struct ID3D11ShaderResourceView;
struct ID3D11Device;
#endif
#include <iostream>
#include <cstdint>
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <DX3D/Graphics/Abstraction/GraphicsHandles.h>

#if defined(DX3D_ENABLE_OPENGL)
#include <glad/glad.h>
#endif
#if defined(DX3D_PLATFORM_ANDROID)
#include <GLES3/gl3.h>
#include <DX3D/Core/AndroidPlatform.h>
#include <Libraries/stb/stb_image.h>
#include <android/log.h>
#endif
namespace dx3d
{
    class Texture2D {
    public:
        enum class Backend {
            DirectX11,
            OpenGL
        };

#if defined(_WIN32)
        Texture2D(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
            : m_backend(Backend::DirectX11), m_srv(srv) {
        }
#endif

#if defined(DX3D_ENABLE_OPENGL) || defined(DX3D_PLATFORM_ANDROID)
        explicit Texture2D(unsigned int glTexture)
            : m_backend(Backend::OpenGL), m_glTexture(glTexture) {
        }
#endif

        ~Texture2D()
        {
#if defined(DX3D_ENABLE_OPENGL) || defined(DX3D_PLATFORM_ANDROID)
            if (m_backend == Backend::OpenGL && m_glTexture)
            {
                glDeleteTextures(1, &m_glTexture);
                m_glTexture = 0;
            }
#endif
        }

        ID3D11ShaderResourceView* getSRV() const
        {
#if defined(_WIN32)
            return m_srv.Get();
#else
            return nullptr;
#endif
        }
        NativeGraphicsHandle getNativeView() const {
            if (m_backend == Backend::OpenGL) {
                return reinterpret_cast<NativeGraphicsHandle>(static_cast<uintptr_t>(m_glTexture));
            }
#if defined(_WIN32)
            return m_srv.Get();
#else
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> LoadTexture2D(IRenderDevice& device, const wchar_t* filePath)
        {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            if (nativeDevice) {
                return LoadTexture2D(nativeDevice, filePath);
            }
#endif
#if defined(DX3D_PLATFORM_ANDROID)
            return LoadTexture2DOpenGLES(filePath);
#elif defined(DX3D_ENABLE_OPENGL)
            return LoadTexture2DOpenGL(filePath);
#else
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> LoadTexture2D(IRenderDevice* device, const wchar_t* filePath)
        {
            return device ? LoadTexture2D(*device, filePath) : nullptr;
        }

#if defined(_WIN32)
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
#endif

#if defined(_WIN32)
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
#endif

        static std::shared_ptr<Texture2D> CreateDebugTexture(IRenderDevice& device)
        {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            if (nativeDevice) {
                return CreateDebugTexture(nativeDevice);
            }
#endif
#if defined(DX3D_PLATFORM_ANDROID)
            return CreateDebugTextureOpenGLES();
#elif defined(DX3D_ENABLE_OPENGL)
            return CreateDebugTextureOpenGL();
#else
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> CreateDebugTexture(IRenderDevice* device)
        {
            return device ? CreateDebugTexture(*device) : nullptr;
        }

        // Create a cubemap with solid colors for testing
#if defined(_WIN32)
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
#endif

        static std::shared_ptr<Texture2D> CreateSkyboxCubemap(IRenderDevice& device)
        {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            return nativeDevice ? CreateSkyboxCubemap(nativeDevice) : nullptr;
#else
            (void)device;
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> CreateSkyboxCubemap(IRenderDevice* device)
        {
            return device ? CreateSkyboxCubemap(*device) : nullptr;
        }

        // Load cubemap from a single 4x3 cross-layout image file
#if defined(_WIN32)
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
#endif

        static std::shared_ptr<Texture2D> LoadSkyboxCubemap(IRenderDevice& device, const wchar_t* filePath)
        {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            return nativeDevice ? LoadSkyboxCubemap(nativeDevice, filePath) : nullptr;
#else
            (void)device;
            (void)filePath;
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> LoadSkyboxCubemap(IRenderDevice* device, const wchar_t* filePath)
        {
            return device ? LoadSkyboxCubemap(*device, filePath) : nullptr;
        }

       
#if defined(_WIN32)
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
#endif

        static std::shared_ptr<Texture2D> CreateNoiseTexture(IRenderDevice& device, int size = 256) {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            return nativeDevice ? CreateNoiseTexture(nativeDevice, size) : nullptr;
#else
            (void)device;
            (void)size;
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> CreateNoiseTexture(IRenderDevice* device, int size = 256) {
            return device ? CreateNoiseTexture(*device, size) : nullptr;
        }
        
        // Create a sun texture with glow effect
#if defined(_WIN32)
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
#endif

        static std::shared_ptr<Texture2D> CreateSunTexture(IRenderDevice& device, int size = 256) {
#if defined(_WIN32)
            auto* nativeDevice = static_cast<ID3D11Device*>(device.getNativeDeviceHandle());
            return nativeDevice ? CreateSunTexture(nativeDevice, size) : nullptr;
#else
            (void)device;
            (void)size;
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> CreateSunTexture(IRenderDevice* device, int size = 256) {
            return device ? CreateSunTexture(*device, size) : nullptr;
        }

    private:
#if defined(DX3D_PLATFORM_ANDROID)
        static std::string wideToUtf8Path(const wchar_t* path)
        {
            if (!path)
            {
                return {};
            }
            std::wstring wpath(path);
            std::string out;
            out.reserve(wpath.size());
            for (wchar_t ch : wpath)
            {
                if (ch <= 0x7F)
                {
                    out.push_back(static_cast<char>(ch));
                }
                else
                {
                    out.push_back('?');
                }
            }
            return out;
        }

        static std::shared_ptr<Texture2D> LoadTexture2DOpenGLES(const wchar_t* filePath)
        {
            const std::string path = wideToUtf8Path(filePath);
            if (path.empty())
            {
                __android_log_print(ANDROID_LOG_ERROR, "LoadTexture2D", "Failed: empty path");
                return nullptr;
            }
            
            __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Loading: %s", path.c_str());
            
            auto data = platform::readAsset(path.c_str());
            if (data.empty())
            {
                __android_log_print(ANDROID_LOG_ERROR, "LoadTexture2D", "Failed: asset not found or empty (%s)", path.c_str());
                return nullptr;
            }
            
            __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Asset loaded, size: %zu bytes", data.size());

            int width = 0;
            int height = 0;
            int channels = 0;
            stbi_uc* pixels = stbi_load_from_memory(
                data.data(),
                static_cast<int>(data.size()),
                &width,
                &height,
                &channels,
                STBI_rgb_alpha
            );
            if (!pixels || width <= 0 || height <= 0)
            {
                __android_log_print(ANDROID_LOG_ERROR, "LoadTexture2D", "Failed: stbi_load_from_memory returned nullptr or invalid dimensions (%dx%d)", width, height);
                if (pixels)
                {
                    stbi_image_free(pixels);
                }
                return nullptr;
            }
            
            __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Image decoded: %dx%d, channels: %d", width, height, channels);

            unsigned int textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // Use GL_RGBA8 as internal format for OpenGL ES 3.0 compatibility
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, pixels);
            
            // Check for OpenGL errors after texture creation
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                __android_log_print(ANDROID_LOG_ERROR, "LoadTexture2D", "OpenGL error after glTexImage2D: 0x%x", err);
            }
            
            // Verify texture is valid by checking if it's actually bound
            GLint boundAfter = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundAfter);
            if (static_cast<unsigned int>(boundAfter) != textureId) {
                __android_log_print(ANDROID_LOG_ERROR, "LoadTexture2D", "Texture binding verification failed! Expected %u, got %d", textureId, boundAfter);
            }
            
            // Verify texture parameters (use known dimensions since GL_TEXTURE_WIDTH/HEIGHT not available in OpenGL ES)
            __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Texture verified: ID=%u, size=%dx%d", textureId, width, height);
            
            glBindTexture(GL_TEXTURE_2D, 0);

            stbi_image_free(pixels);
            
            __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Success! texture ID: %u", textureId);

            return std::make_shared<Texture2D>(textureId);
        }
#endif

        static std::shared_ptr<Texture2D> LoadTexture2DOpenGL(const wchar_t* filePath)
        {
#if defined(DX3D_ENABLE_OPENGL)
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory2,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory)
            );
            if (FAILED(hr)) return nullptr;

            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            hr = wicFactory->CreateDecoderFromFilename(
                filePath,
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                &decoder
            );
            if (FAILED(hr)) return nullptr;

            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) return nullptr;

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

            UINT width = 0, height = 0;
            hr = converter->GetSize(&width, &height);
            if (FAILED(hr)) return nullptr;

            const UINT stride = width * 4;
            const UINT imageSize = stride * height;
            std::unique_ptr<BYTE[]> pixels(new BYTE[imageSize]);

            hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.get());
            if (FAILED(hr)) return nullptr;

            unsigned int textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(width),
                static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::make_shared<Texture2D>(textureId);
#else
            (void)filePath;
            return nullptr;
#endif
        }

        static std::shared_ptr<Texture2D> CreateDebugTextureOpenGL()
        {
#if defined(DX3D_ENABLE_OPENGL)
            const int size = 8;
            const UINT width = size;
            const UINT height = size;
            std::unique_ptr<UINT[]> pixels(new UINT[width * height]);
            const UINT magenta = 0xFFFF00FF;
            const UINT black = 0xFF000000;

            for (UINT y = 0; y < height; y++)
            {
                for (UINT x = 0; x < width; x++)
                {
                    bool isCyan = ((x / (size / 2)) + (y / (size / 2))) % 2 == 0;
                    pixels[y * width + x] = isCyan ? magenta : black;
                }
            }

            unsigned int textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(width),
                static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::make_shared<Texture2D>(textureId);
#else
            return nullptr;
#endif
        }

#if defined(DX3D_PLATFORM_ANDROID)
        static std::shared_ptr<Texture2D> CreateDebugTextureOpenGLES()
        {
            const int size = 8;
            const int width = size;
            const int height = size;
            std::unique_ptr<unsigned int[]> pixels(new unsigned int[width * height]);
            const unsigned int magenta = 0xFFFF00FF;
            const unsigned int black = 0xFF000000;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const bool isMagenta = ((x / (size / 2)) + (y / (size / 2))) % 2 == 0;
                    pixels[y * width + x] = isMagenta ? magenta : black;
                }
            }

            unsigned int textureId = 0;
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
            glBindTexture(GL_TEXTURE_2D, 0);

            return std::make_shared<Texture2D>(textureId);
        }
#endif

        Backend m_backend{ Backend::DirectX11 };
#if defined(_WIN32)
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
#else
        void* m_srv{};
#endif
        unsigned int m_glTexture{};
    };
}