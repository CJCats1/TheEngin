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
    m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{ m_logger });
    auto& device = *m_graphicsDevice;
    m_deviceContext = device.createDeviceContext();

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
        constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic3D.hlsl";
        std::ifstream shaderStream(shaderFilePath);
        if (!shaderStream) DX3DLogThrowError("Failed to open Basic3D.hlsl.");
        std::string shaderFileData{ std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>() };
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();

        auto vs = device.compileShader({ shaderFilePath, src, len, "VSMain", ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, "PSMain", ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs });
        m_pipeline3D = device.createGraphicsPipelineState({ *vsSig, *ps });
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
        context.setGraphicsPipelineState(*m_backgroundDotsPipeline);
        // Build constants
        struct Params { float viewportSize[2]; float dotSpacing; float dotRadius; float baseColor[4]; float dotColor[4]; } p{};
        p.viewportSize[0] = swapChain.getSize().width;
        p.viewportSize[1] = swapChain.getSize().height;
        p.dotSpacing = 40.0f;   // pixels
        p.dotRadius = 1.2f;     // pixels
        p.baseColor[0] = 0.27f; p.baseColor[1] = 0.39f; p.baseColor[2] = 0.55f; p.baseColor[3] = 1.0f; // same clear color
        p.dotColor[0] = 0.20f; p.dotColor[1] = 0.32f; p.dotColor[2] = 0.46f; p.dotColor[3] = 0.6f;    // darker blue, alpha as strength
        context.setPSConstants0(&p, sizeof(p));

        // Disable depth for background
        context.disableDepthTest();
        // Issue a trivial draw (VS uses SV_VertexID so we can draw 3 vertices)
        context.drawTriangleList(3, 0);
        // Restore default
        context.enableDepthTest();
    }

    context.setGraphicsPipelineState(*m_pipeline);
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
