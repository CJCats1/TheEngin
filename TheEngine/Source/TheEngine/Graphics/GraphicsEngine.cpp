#include <TheEngine/Graphics/GraphicsEngine.h>
#if defined(_WIN32)
#include <TheEngine/Graphics/GraphicsDevice.h>
#endif
#if defined(_WIN32)
#include <TheEngine/Graphics/DeviceContext.h>
#include <TheEngine/Graphics/SwapChain.h>
#include <TheEngine/Graphics/VertexBuffer.h>
#endif
#include <fstream>
#include <TheEngine/Graphics/Texture2D.h>
#include <TheEngine\Math\Backend\MathBackend.h>
#include <fstream>
#include <TheEngine\Graphics\Abstraction\RenderBackendFactory.h>
#include <cstdlib>
#include <string_view>
#include <algorithm>
#include <cctype>
#if defined(THEENGINE_PLATFORM_ANDROID)
#include <TheEngine/Core/AndroidPlatform.h>
#endif
using namespace TheEngine;

namespace
{
    RenderBackendType readBackendFromConfig(RenderBackendType defaultType)
    {
#if defined(THEENGINE_PLATFORM_ANDROID)
        // On Android we always use OpenGL; ignore config.
        (void)defaultType;
        return RenderBackendType::OpenGL;
#else
        std::ifstream file("TheEngineConfig.ini");
        if (!file)
            return defaultType;

        auto trim = [](std::string& s)
        {
            const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
            auto itFront = std::find_if_not(s.begin(), s.end(), isSpace);
            auto itBack = std::find_if_not(s.rbegin(), s.rend(), isSpace).base();
            if (itFront >= itBack)
            {
                s.clear();
                return;
            }
            s.assign(itFront, itBack);
        };

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            auto firstNonSpace = line.find_first_not_of(" \t");
            if (firstNonSpace == std::string::npos)
                continue;
            const char c = line[firstNonSpace];
            if (c == '#' || c == ';')
                continue;

            const auto eqPos = line.find('=');
            if (eqPos == std::string::npos)
                continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            trim(key);
            trim(value);

            if (key.empty() || value.empty())
                continue;

            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

            if (key == "backend" || key == "renderer")
            {
                if (value == "opengl" || value == "gl")
                    return RenderBackendType::OpenGL;
                if (value == "dx11" || value == "directx11" || value == "direct3d11" || value == "d3d11")
                    return RenderBackendType::DirectX11;
            }
        }

        return defaultType;
#endif
    }

	bool loadShaderFile(const char* path, std::string& out)
	{
#if defined(THEENGINE_PLATFORM_ANDROID)
		auto data = platform::readAsset(path);
		if (data.empty())
		{
			return false;
		}
		out.assign(reinterpret_cast<const char*>(data.data()), data.size());
		return true;
#else
		std::ifstream shaderStream(path);
		if (!shaderStream)
		{
			return false;
		}
		out.assign(std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>());
		return true;
#endif
	}
}
float TheEngine::GraphicsEngine::m_windowHeight = 720.0f;
float TheEngine::GraphicsEngine::m_windowWidth = 1280.0f;

TheEngine::GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
{
    std::cout << "GraphicsEngine: Starting initialization..." << std::endl;
	RenderBackendType backendType = readBackendFromConfig(desc.backendType);
#if defined(THEENGINE_PLATFORM_ANDROID)
    backendType = RenderBackendType::OpenGL;
#endif
#if defined(_MSC_VER)
	char* env = nullptr;
	size_t envLen = 0;
	if (_dupenv_s(&env, &envLen, "THEENGINE_BACKEND") == 0 && env)
	{
		std::string_view backendValue(env);
		if (backendValue == "opengl" || backendValue == "OpenGL")
			backendType = RenderBackendType::OpenGL;
		else if (backendValue == "dx11" || backendValue == "directx11" || backendValue == "DirectX11")
			backendType = RenderBackendType::DirectX11;
	}
	if (env)
	{
		std::free(env);
	}
#else
	if (const char* env = std::getenv("THEENGINE_BACKEND"))
	{
		std::string_view backendValue(env);
		if (backendValue == "opengl" || backendValue == "OpenGL")
			backendType = RenderBackendType::OpenGL;
		else if (backendValue == "dx11" || backendValue == "directx11" || backendValue == "DirectX11")
			backendType = RenderBackendType::DirectX11;
	}
#endif

    m_backendType = backendType;
#if !defined(THEENGINE_PLATFORM_ANDROID)
    // On desktop, use GLM for OpenGL as it's available via vcpkg
    if (m_backendType == RenderBackendType::OpenGL)
    {
        math::backend::setBackend(math::backend::MathBackendType::Glm);
    }
    else
    {
        math::backend::setBackend(math::backend::MathBackendType::Default);
    }
#else
    // On Android, GLM isn't available, so always use default backend
    math::backend::setBackend(math::backend::MathBackendType::Default);
#endif
    m_renderBackend = createRenderBackend(backendType, m_logger);
    m_graphicsDevice = m_renderBackend->createDevice(GraphicsDeviceDesc{ m_logger });
    auto& device = *m_graphicsDevice;
    m_deviceContext = device.createDeviceContext();
    m_imguiBackend = m_renderBackend->createImGuiBackend();
    std::cout << "GraphicsEngine: Device and context created" << std::endl;
    if (m_backendType != RenderBackendType::OpenGL)
    {
        initializePipelines();
    }
}

void TheEngine::GraphicsEngine::initializePipelines()
{
    if (m_pipelinesInitialized)
    {
        return;
    }
    m_pipelinesInitialized = true;
    auto& device = *m_graphicsDevice;
    const bool isOpenGL = (m_backendType == RenderBackendType::OpenGL);
    const char* glslFallbackPath = "TheEngine/Assets/Shaders/OpenGLFallback.glsl";
    const auto shaderPath = [isOpenGL, glslFallbackPath](const char* hlslPath)
    {
        return isOpenGL ? glslFallbackPath : hlslPath;
    };
    const char* vsEntry = isOpenGL ? "main" : "VSMain";
    const char* psEntry = isOpenGL ? "main" : "PSMain";

    // ---- Default world-space pipeline (Basic.hlsl) ----
    {
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/Basic.hlsl");
        std::string shaderFileData{};
        if (!loadShaderFile(shaderFilePath, shaderFileData))
            THEENGINE_LOG_THROW_ERROR("Failed to open shader file.");
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();

        auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs });
        m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
    }

    // ---- 3D world-space pipeline (Basic3D.hlsl) ----
    {
        std::cout << "GraphicsEngine: Creating 3D pipeline..." << std::endl;
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/Basic3D.hlsl");
        std::string shaderFileData{};
        if (!loadShaderFile(shaderFilePath, shaderFileData)) {
            std::cout << "ERROR: Failed to open Basic3D.hlsl" << std::endl;
            THEENGINE_LOG_THROW_ERROR("Failed to open Basic3D.hlsl.");
        }
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();
        std::cout << "Basic3D.hlsl loaded, size: " << len << " bytes" << std::endl;

        auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
        
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
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/Text.hlsl");
        std::string shaderFileData{};
        if (!loadShaderFile(shaderFilePath, shaderFileData))
            THEENGINE_LOG_THROW_ERROR("Failed to open Text.hlsl.");
        auto* src = shaderFileData.c_str();
        auto   len = shaderFileData.length();

        auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
        auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs }); // reflects POSITION0/TEXCOORD0
        m_textPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
    }

    // ---- Background dots pipeline (BackgroundDots.hlsl / OpenGLBackgroundDots.glsl) ----
    {
        const char* shaderFilePath = isOpenGL
            ? "TheEngine/Assets/Shaders/OpenGLBackgroundDots.glsl"
            : "TheEngine/Assets/Shaders/BackgroundDots.hlsl";
        std::string shaderFileData{};
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();

            // Compile with explicit VS/PS entry points
            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
            // No input layout needed for SV_VertexID; create empty signature
            auto vsSig = device.createVertexShaderSignature({ vs });
            m_backgroundDotsPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

            if (!isOpenGL)
            {
                // Prepare a simple fullscreen quad (used just to issue 3 verts via Draw)
                m_fullscreenQuad = Mesh::CreateQuadColored(device, 2.0f, 2.0f);
            }
        }
        else {
            // Optional: fallback silently if shader missing
            m_backgroundDotsPipeline.reset();
        }
    }

    // ---- Toon/world-space pipeline (ToonSprite.hlsl) ----
    {
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/ToonSprite.hlsl");
        std::string shaderFileData{};
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();

            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            m_toonPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
        }
        else {
            m_toonPipeline.reset();
        }
    }

    // ---- Simple shadow map pipeline (SimpleShadowMap.hlsl) ----
    {
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/SimpleShadowMap.hlsl");
        std::string shaderFileData{};
        
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            
            // Create pipeline with vertex shader and pixel shader
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
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
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/SimpleShadowDebug.hlsl");
        std::string shaderFileData{};
        
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto   len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
            auto vsSig = device.createVertexShaderSignature({ vs });
            
            m_shadowMapDebugPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
            std::cout << "Simple shadow map debug pipeline created successfully" << std::endl;
        }
        else {
            std::cout << "Failed to create simple shadow map debug pipeline - shader file not found" << std::endl;
            m_shadowMapDebugPipeline.reset();
        }
    }

    // ---- Line rendering pipeline (Line.hlsl) ----
    {
        std::cout << "GraphicsEngine: Creating line pipeline..." << std::endl;
        const char* shaderFilePath = isOpenGL
            ? "TheEngine/Assets/Shaders/OpenGLLine.glsl"
            : "TheEngine/Assets/Shaders/Line.hlsl";
        std::string shaderFileData{};
        
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
            
            if (vs && ps) {
                auto vsSig = device.createVertexShaderSignature({ vs });
                m_linePipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
                std::cout << "Line pipeline created successfully" << std::endl;
            } else {
                std::cout << "ERROR: Failed to compile line shaders" << std::endl;
                m_linePipeline.reset();
            }
        }
        else {
            std::cout << "Failed to create line pipeline - shader file not found" << std::endl;
            m_linePipeline.reset();
        }
    }

    // Create skybox pipeline
    {
        std::cout << "Creating skybox pipeline..." << std::endl;
        const char* shaderFilePath = shaderPath("TheEngine/Assets/Shaders/Skybox.hlsl");
        std::string shaderFileData{};
        
        if (loadShaderFile(shaderFilePath, shaderFileData)) {
            auto* src = shaderFileData.c_str();
            auto len = shaderFileData.length();
            
            auto vs = device.compileShader({ shaderFilePath, src, len, vsEntry, ShaderType::VertexShader });
            auto ps = device.compileShader({ shaderFilePath, src, len, psEntry, ShaderType::PixelShader });
            
            if (vs && ps) {
                auto vsSig = device.createVertexShaderSignature({ vs });
                if (vsSig) {
                    m_skyboxPipeline = device.createGraphicsPipelineState({ *vsSig, *ps });
                    std::cout << "Skybox pipeline created successfully" << std::endl;
                } else {
                    std::cout << "ERROR: Failed to create skybox vertex shader signature" << std::endl;
                    m_skyboxPipeline.reset();
                }
            } else {
                std::cout << "ERROR: Failed to compile skybox shaders" << std::endl;
                m_skyboxPipeline.reset();
            }
        } else {
            std::cout << "Failed to create skybox pipeline - shader file not found" << std::endl;
            m_skyboxPipeline.reset();
        }
    }
}


TheEngine::GraphicsEngine::~GraphicsEngine()
{
}

IRenderDevice& TheEngine::GraphicsEngine::getGraphicsDevice() noexcept
{
	return *m_graphicsDevice;
}

IImGuiBackend& TheEngine::GraphicsEngine::getImGuiBackend() noexcept
{
	return *m_imguiBackend;
}

void TheEngine::GraphicsEngine::beginFrame(IRenderSwapChain& swapChain)
{
	auto& context = *m_deviceContext;
	context.clearAndSetBackBuffer(swapChain, { 0.27f, 0.39f, 0.55f, 1.0f });
    context.setViewportSize(swapChain.getSize());

    // Optional dotted background pass in screen space
    // - DirectX11: requires fullscreen quad mesh
    // - OpenGL: uses gl_VertexID in shader, so no quad is needed
    const bool canRenderDots =
        m_backgroundDotsPipeline &&
        (m_fullscreenQuad || m_backendType == RenderBackendType::OpenGL);
    if (canRenderDots)
    {
        renderBackgroundDots(context, m_backgroundDotsPipeline.get(), 
            swapChain.getSize().width, swapChain.getSize().height);
    }

    context.setGraphicsPipelineState(*m_pipeline);
}

void GraphicsEngine::renderBackgroundDots(IRenderContext& context, IRenderPipelineState* backgroundDotsPipeline,
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

void TheEngine::GraphicsEngine::endFrame(IRenderSwapChain& swapChain)
{
	m_graphicsDevice->executeCommandList(*m_deviceContext);
	swapChain.present();
}

IRenderContext& TheEngine::GraphicsEngine::getContext()
{
	return *m_deviceContext;
}
