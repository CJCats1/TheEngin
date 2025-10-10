#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <fstream>
#include <DX3D/Graphics/Texture2D.h>
#include <fstream>
using namespace dx3d;
float dx3d::GraphicsEngine::m_windowHeight = 720.0f;
float dx3d::GraphicsEngine::m_windowWidth = 1280.0f;

dx3d::GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
{
    std::cout << "GraphicsEngine: Starting initialization..." << std::endl;
    m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{ m_logger });
    auto& device = *m_graphicsDevice;
    m_deviceContext = device.createDeviceContext();
    std::cout << "GraphicsEngine: Device and context created" << std::endl;

    // ---- Default world-space pipeline (Basic.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (!shaderStream) DX3DLogThrowError("Failed to open shader file.");
        std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();

        auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs });
        m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
    }

    // ---- 3D world-space pipeline (Basic3D.hlsl) ----
    {
        std::cout << "GraphicsEngine: Creating 3D pipeline..." << std::endl;
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic3D.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (!shaderStream) {
            std::cout << "ERROR: Failed to open Basic3D.hlsl" << std::endl;
            DX3DLogThrowError("Failed to open Basic3D.hlsl.");
        }
        std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();
        std::cout << "Basic3D.hlsl loaded, size: " << len << " bytes" << std::endl;

        auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
        
        // Debug: Check if shaders compiled successfully
        std::cout << "About to check shader compilation results..." << std::endl;
        if (vs) {
            std::cout << "3D vertex shader compiled successfully" << std::endl;
        } else {
            std::cout << "ERROR: Failed to compile 3D vertex shader" << std::endl;
        }
        
        if (ps) {
            std::cout << "3D pixel shader compiled successfully" << std::endl;
        } else {
            std::cout << "ERROR: Failed to compile 3D pixel shader" << std::endl;
        }
        
        auto vsSig = device.createVertexShaderSignature({ vs });
        m_pipeline3D = device.createGraphicsPipelineState({ *vsSig, *ps });
        
        // Debug: Check if 3D pipeline was created successfully
        if (m_pipeline3D) {
            std::cout << "3D pipeline created successfully" << std::endl;
        } else {
            std::cout << "ERROR: Failed to create 3D pipeline" << std::endl;
        }
    }

    // ---- Text screen-space pipeline (Text.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Text.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (!shaderStream) DX3DLogThrowError("Failed to open Text.hlsl.");
        std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();

        auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs }); // reflects POSITION0/TEXCOORD0
        m_textPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
    }

    // ---- Background dots pipeline (BackgroundDots.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/BackgroundDots.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (shaderStream) {
            std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();

            // Compile with explicit VS/PS entry points
            auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
            // No input layout needed for SV_VertexID; create empty signature
            auto vsSig = device.createVertexShaderSignature({ vs });
            m_backgroundDotsPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

            // Prepare a simple fullscreen quad (used just to issue 3 verts via Draw)
            m_fullscreenQuad = Mesh::CreateQuadColored(device, 2.0f, 2.0f);
        }
        else {
            // Optional: fallback silently if shader missing
            m_backgroundDotsPipeline.reset();
        }
    }

    // ---- Toon/world-space pipeline (ToonSprite.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/ToonSprite.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (shaderStream) {
            std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();

            auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            m_toonPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
        }
        else {
            m_toonPipeline.reset();
        }
    }

    // ---- Simple shadow map pipeline (SimpleShadowMap.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/SimpleShadowMap.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        
        if (shaderStream) {
            std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            
            // Create pipeline with vertex shader and pixel shader
            auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
            m_shadowMapPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
            std::cout << "Simple shadow map pipeline created successfully" << std::endl;
        }
        else {
            std::cout << "Failed to create simple shadow map pipeline - shader file not found" << std::endl;
            m_shadowMapPipeline.reset();
        }
    }

    // ---- Simple shadow map debug pipeline (SimpleShadowDebug.hlsl) ----
    {
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/SimpleShadowDebug.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        
        if (shaderStream) {
            std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            
            m_shadowMapDebugPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
            std::cout << "Simple shadow map debug pipeline created successfully" << std::endl;
        }
        else {
            std::cout << "Failed to create simple shadow map debug pipeline - shader file not found" << std::endl;
            m_shadowMapDebugPipeline.reset();
        }
    }
}


dx3d::GraphicsEngine::~GraphicsEngine()
{
}

GraphicsDevice& dx3d::GraphicsEngine::getGraphicsDevice() noexcept
{
	return *m_graphicsDevice;
}

void dx3d::GraphicsEngine::beginFrame(SwapChain& swapChain)
{
	auto& context = *m_deviceContext;
	context.clearAndSetBackBuffer(swapChain, { 0.27f, 0.39f, 0.55f, 1.0f });
    context.setViewportSize(swapChain.getSize());

    // Optional dotted background pass in screen space
    if (m_backgroundDotsPipeline && m_fullscreenQuad)
    {
        renderBackgroundDots(context, m_backgroundDotsPipeline.get(), 
            swapChain.getSize().width, swapChain.getSize().height);
    }

    context.setGraphicsPipelineState(*m_pipeline);
}

void GraphicsEngine::renderBackgroundDots(DeviceContext& context, GraphicsPipelineState* backgroundDotsPipeline, 
    float screenWidth, float screenHeight, float dotSpacing, float dotRadius,
    const Vec4& baseColor, const Vec4& dotColor)
{
    if (!backgroundDotsPipeline) return;
    
    context.setGraphicsPipelineState(*backgroundDotsPipeline);
    
    // Build constants for background dots shader
    struct DotParams { 
        float viewportSize[2]; 
        float dotSpacing; 
        float dotRadius; 
        float baseColor[4]; 
        float dotColor[4]; 
    } params{};
    
    params.viewportSize[0] = screenWidth;
    params.viewportSize[1] = screenHeight;
    params.dotSpacing = dotSpacing;
    params.dotRadius = dotRadius;
    params.baseColor[0] = baseColor.x;
    params.baseColor[1] = baseColor.y;
    params.baseColor[2] = baseColor.z;
    params.baseColor[3] = baseColor.w;
    params.dotColor[0] = dotColor.x;
    params.dotColor[1] = dotColor.y;
    params.dotColor[2] = dotColor.z;
    params.dotColor[3] = dotColor.w;
    
    context.setPSConstants0(&params, sizeof(params));
    
    // Disable depth for background
    context.disableDepthTest();
    // Draw fullscreen triangle (3 vertices)
    context.drawTriangleList(3, 0);
    // Restore depth testing
    context.enableDepthTest();
}

void dx3d::GraphicsEngine::endFrame(SwapChain& swapChain)
{
	m_graphicsDevice->executeCommandList(*m_deviceContext);
	swapChain.present();
}

DeviceContext& dx3d::GraphicsEngine::getContext()
{
	return *m_deviceContext;
}
