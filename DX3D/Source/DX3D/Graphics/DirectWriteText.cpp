#include "DirectWriteText.h"
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <vector>
#include<iostream>
#include <DX3D/Graphics/GraphicsEngine.h>

namespace dx3d {

    // Static members
    std::unique_ptr<DirectWriteRenderer> TextSystem::s_renderer = nullptr;
    bool TextSystem::s_initialized = false;

    DirectWriteRenderer::DirectWriteRenderer(GraphicsDevice& device)
        : m_device(device) {
    }

    DirectWriteRenderer::~DirectWriteRenderer() {
        shutdown();
    }
    bool DirectWriteRenderer::initialize() {
        HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
        );
        if (FAILED(hr)) {
            std::cout << "DirectWriteRenderer: DWriteCreateFactory FAILED (hr = "
                << std::hex << hr << ")\n";
            return false;
        }

        std::cout << "DirectWriteRenderer: DWriteCreateFactory SUCCESS\n";

        if (!initializeWIC()) {
            std::cout << "DirectWriteRenderer: WIC initialization FAILED\n";
            return false;
        }

        if (!initializeDirect2D()) {
            std::cout << "DirectWriteRenderer: Direct2D initialization FAILED\n";
            return false;
        }

        return true;
    }
    void DirectWriteRenderer::shutdown() {
        m_d2dContext.Reset();
        m_d2dDevice.Reset();
        m_d2dFactory.Reset();
        m_writeFactory.Reset();
        m_wicFactory.Reset();
    }

    bool DirectWriteRenderer::initializeWIC() {
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_wicFactory)
        );
        return SUCCEEDED(hr);
    }

    bool DirectWriteRenderer::initializeDirectWrite() {
        HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
        );
        return SUCCEEDED(hr);
    }

    bool DirectWriteRenderer::initializeDirect2D() {
        HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );
        if (FAILED(hr)) {
            std::cout << "D2D1CreateFactory FAILED: 0x" << std::hex << hr << "\n";
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_device.getD3DDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        if (FAILED(hr)) {
            std::cout << "QueryInterface IDXGIDevice FAILED: 0x" << std::hex << hr << "\n";
            return false;
        }

        hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
        if (FAILED(hr)) {
            std::cout << "CreateDevice FAILED: 0x" << std::hex << hr << "\n";
            return false;
        }

        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
        if (FAILED(hr)) {
            std::cout << "CreateDeviceContext FAILED: 0x" << std::hex << hr << "\n";
            return false;
        }

        // WIC initialization
        return initializeWIC();
    }

    std::shared_ptr<Texture2D> DirectWriteRenderer::renderTextToTexture(
        const std::wstring& text,
        const std::wstring& fontFamily,
        float fontSize,
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_FONT_STYLE fontStyle,
        const Vec4& color,
        UINT32 maxWidth,
        UINT32 maxHeight) {

        if (text.empty()) return nullptr;

        // Create text format
        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
        HRESULT hr = m_writeFactory->CreateTextFormat(
            fontFamily.c_str(),
            nullptr,
            fontWeight,
            fontStyle,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"en-us",
            &textFormat
        );
        if (FAILED(hr)) return nullptr;

        // Create text layout to measure the text
        Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
        hr = m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            textFormat.Get(),
            static_cast<float>(maxWidth),
            static_cast<float>(maxHeight),
            &textLayout
        );
        if (FAILED(hr)) return nullptr;

        // Get text metrics
        DWRITE_TEXT_METRICS textMetrics;
        hr = textLayout->GetMetrics(&textMetrics);
        if (FAILED(hr)) return nullptr;

        // Calculate texture dimensions (add padding)
        UINT32 textureWidth = static_cast<UINT32>(textMetrics.width) + 4;
        UINT32 textureHeight = static_cast<UINT32>(textMetrics.height) + 4;

        // Ensure minimum size
        textureWidth = std::max(textureWidth, 1u);
        textureHeight = std::max(textureHeight, 1u);

        // Create WIC bitmap
        Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
        hr = m_wicFactory->CreateBitmap(
            textureWidth,
            textureHeight,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnDemand,
            &wicBitmap
        );
        if (FAILED(hr)) return nullptr;

        // Create render target
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget;
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
        props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);

        hr = m_d2dFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), &props, &renderTarget);
        if (FAILED(hr)) return nullptr;

        // Create brush
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        hr = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(color.x, color.y, color.z, color.w),
            &brush
        );
        if (FAILED(hr)) return nullptr;

        // Render text
        renderTarget->BeginDraw();
        renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
        renderTarget->DrawTextLayout(
            D2D1::Point2F(2.0f, 2.0f), // Small padding
            textLayout.Get(),
            brush.Get()
        );
        hr = renderTarget->EndDraw();
        if (FAILED(hr)) return nullptr;

        // Convert WIC bitmap to D3D11 texture
        Microsoft::WRL::ComPtr<IWICBitmapLock> lock;
        WICRect rect = { 0, 0, static_cast<INT>(textureWidth), static_cast<INT>(textureHeight) };
        hr = wicBitmap->Lock(&rect, WICBitmapLockRead, &lock);
        if (FAILED(hr)) return nullptr;

        UINT bufferSize;
        BYTE* buffer;
        hr = lock->GetDataPointer(&bufferSize, &buffer);
        if (FAILED(hr)) return nullptr;

        UINT stride;
        hr = lock->GetStride(&stride);
        if (FAILED(hr)) return nullptr;

        // Create D3D11 texture
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = textureWidth;
        texDesc.Height = textureHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = buffer;
        data.SysMemPitch = stride;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> d3dTexture;
        hr = m_device.getD3DDevice()->CreateTexture2D(&texDesc, &data, &d3dTexture);
        if (FAILED(hr)) return nullptr;

        // Create shader resource view
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = m_device.getD3DDevice()->CreateShaderResourceView(d3dTexture.Get(), nullptr, &srv);
        if (FAILED(hr)) return nullptr;

        return std::make_shared<Texture2D>(srv);
    }

    Vec2 DirectWriteRenderer::measureText(
        const std::wstring& text,
        const std::wstring& fontFamily,
        float fontSize,
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_FONT_STYLE fontStyle,
        UINT32 maxWidth) {

        if (text.empty()) return Vec2(0.0f, 0.0f);

        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
        HRESULT hr = m_writeFactory->CreateTextFormat(
            fontFamily.c_str(),
            nullptr,
            fontWeight,
            fontStyle,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"en-us",
            &textFormat
        );
        if (FAILED(hr)) return Vec2(0.0f, 0.0f);

        Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
        hr = m_writeFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            textFormat.Get(),
            static_cast<float>(maxWidth),
            FLT_MAX,
            &textLayout
        );
        if (FAILED(hr)) return Vec2(0.0f, 0.0f);

        DWRITE_TEXT_METRICS textMetrics;
        hr = textLayout->GetMetrics(&textMetrics);
        if (FAILED(hr)) return Vec2(0.0f, 0.0f);

        return Vec2(textMetrics.width, textMetrics.height);
    }

    // TextComponent implementation
    TextComponent::TextComponent(GraphicsDevice& device, DirectWriteRenderer& textRenderer,
        const std::wstring& text, float fontSize)
        : m_device(device), m_textRenderer(textRenderer), m_text(text), m_fontFamily(L"Arial"),
        m_fontSize(fontSize), m_fontWeight(DWRITE_FONT_WEIGHT_NORMAL),
        m_fontStyle(DWRITE_FONT_STYLE_NORMAL), m_color(1.0f, 1.0f, 1.0f, 1.0f),
        m_maxWidth(1024), m_maxHeight(512), m_visible(true), m_needsRebuild(true),
        m_useScreenSpace(true), m_screenPosition(0.0f, 0.0f) {
    }
    void TextComponent::setScreenPosition(float x, float y) {
        m_screenPosition = Vec2(x, y);
        m_useScreenSpace = true;
    }

    void TextComponent::setScreenPosition(const Vec2& pos) {
        m_screenPosition = pos;
        m_useScreenSpace = true;
    }
    void TextComponent::setText(const std::wstring& text) {
        if (m_text != text) {
            m_text = text;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setFontSize(float size) {
        if (m_fontSize != size) {
            m_fontSize = size;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setFontFamily(const std::wstring& fontFamily) {
        if (m_fontFamily != fontFamily) {
            m_fontFamily = fontFamily;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setFontWeight(DWRITE_FONT_WEIGHT weight) {
        if (m_fontWeight != weight) {
            m_fontWeight = weight;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setFontStyle(DWRITE_FONT_STYLE style) {
        if (m_fontStyle != style) {
            m_fontStyle = style;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setColor(const Vec4& color) {
        if (m_color.x != color.x || m_color.y != color.y || m_color.z != color.z || m_color.w != color.w) {
            m_color = color;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setMaxWidth(UINT32 maxWidth) {
        if (m_maxWidth != maxWidth) {
            m_maxWidth = maxWidth;
            m_needsRebuild = true;
        }
    }

    void TextComponent::setMaxHeight(UINT32 maxHeight) {
        if (m_maxHeight != maxHeight) {
            m_maxHeight = maxHeight;
            m_needsRebuild = true;
        }
    }

    Vec2 TextComponent::getTextSize() const {
        return m_textRenderer.measureText(m_text, m_fontFamily, m_fontSize, m_fontWeight, m_fontStyle, m_maxWidth);
    }

    void TextComponent::rebuildTexture() const {
        std::cout << "=== rebuildTexture() called ===" << std::endl;
        float screenWidth = GraphicsEngine::getWindowWidth();

        if (m_text.empty()) {
            std::cout << "Text is empty, clearing texture and mesh" << std::endl;
            m_textTexture.reset();
            m_textMesh.reset();
            m_needsRebuild = false;
            return;
        }

        // Render text to texture
        m_textTexture = m_textRenderer.renderTextToTexture(
            m_text, m_fontFamily, m_fontSize, m_fontWeight, m_fontStyle, m_color, m_maxWidth, m_maxHeight
        );

        if (!m_textTexture) {
            std::cout << "ERROR: Failed to create text texture!" << std::endl;
            m_needsRebuild = false;
            return;
        }

        // Get text dimensions for mesh sizing
        Vec2 textSize = getTextSize();
        std::cout << "Measured text size: " << textSize.x << "x" << textSize.y << std::endl;
        float worldScale = 1.0f; // Your current divisor, but make it configurable

        m_textMesh = Mesh::CreateQuadTextured(m_device,
            textSize.x / worldScale,
            textSize.y / worldScale);

        if (!m_textMesh) {
            std::cout << "ERROR: Failed to create text mesh!" << std::endl;
            m_needsRebuild = false;
            return;
        }

        // Assign texture
        m_textMesh->setTexture(m_textTexture);

        m_needsRebuild = false;
        std::cout << "=== rebuildTexture() complete ===" << std::endl;
    }

    void TextComponent::draw(DeviceContext& ctx) const {
        if (!isVisible() || m_text.empty()) return;

        // Make sure texture + mesh are built
        if (m_needsRebuild) rebuildTexture();
        if (!m_textMesh || !m_textTexture) return;

        float screenWidth = GraphicsEngine::getWindowWidth();
        float screenHeight = GraphicsEngine::getWindowHeight();

        ctx.enableAlphaBlending();
        ctx.enableTransparentDepth();

        if (m_useScreenSpace) {
            // Same normalized → world mapping as SpriteComponent
            float normalizedX = m_screenPosition.x - 0.5f; // [0,1] → [-0.5,0.5]
            float normalizedY = m_screenPosition.y - 0.5f;

            // Scale to world units relative to text size
            Vec2 textSize = getTextSize();
            float worldX = normalizedX * (screenWidth);
            float worldY = normalizedY * (screenHeight);

            // Build world matrix
            Mat4 worldMatrix = Mat4::translation(Vec3(worldX, worldY, 0.0f));

            // Identity view + ortho projection (just like sprites)
            Mat4 viewMatrix = Mat4::identity();
            Mat4 projMatrix = Mat4::orthographic(screenWidth, screenHeight, -100.0f, 100.0f);

            ctx.setWorldMatrix(worldMatrix);
            ctx.setViewMatrix(viewMatrix);
            ctx.setProjectionMatrix(projMatrix);
            printf("Screen-space: norm(%.2f, %.2f) \n",
                m_screenPosition.x, m_screenPosition.y);
        }
        else {
            // Default to using transform’s world matrix
            ctx.setWorldMatrix(m_transform.getWorldMatrix());
        }

        // Tint handling — add like sprites if needed
        ctx.setTint(m_color);

        // Draw text mesh
        m_textMesh->draw(ctx);

        ctx.disableAlphaBlending();
        ctx.enableDefaultDepth();
    }

    // TextSystem implementation
    void TextSystem::initialize(GraphicsDevice& device) {
        std::cout << "[DEBUG] TextSystem::initialize() CALLED\n";
        if (!s_initialized) {
            s_renderer = std::make_unique<DirectWriteRenderer>(device);
            if (s_renderer->initialize()) {
                s_initialized = true;
                std::cout << "[DEBUG] TextSystem initialized successfully\n";
            }
            else {
                s_renderer.reset();
                std::cout << "[DEBUG] TextSystem FAILED to initialize\n";
            }
        }
        else {
            std::cout << "[DEBUG] TextSystem already initialized (skipping)\n";
        }
    }


    void TextSystem::shutdown() {
        if (s_initialized) {
            s_renderer.reset();
            s_initialized = false;
        }
    }

    DirectWriteRenderer& TextSystem::getRenderer() {
        return *s_renderer;
    }

    bool TextSystem::isInitialized() {
        return s_initialized;
    }
}