#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <fstream>
#include <DX3D/Graphics/Texture2D.h>
using namespace dx3d;

dx3d::GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
{
	m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{m_logger});
	auto& device = *m_graphicsDevice;
	m_deviceContext = device.createDeviceContext();

	constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic.hlsl";
	std::ifstream shaderStream(shaderFilePath);
	if (!shaderStream) DX3DLogThrowError("Failed to open shader file.");
	std::string shaderFileData{
		std::istreambuf_iterator<char>(shaderStream),
		std::istreambuf_iterator<char>()
	};

	auto shaderSourceCode = shaderFileData.c_str();
	auto shaderSourceCodeSize = shaderFileData.length();

	auto vs = device.compileShader({shaderFilePath,shaderSourceCode,shaderSourceCodeSize,
		"VSMain",ShaderType::VertexShader});
	auto ps = device.compileShader({shaderFilePath, shaderSourceCode, shaderSourceCodeSize,
		"PSMain",ShaderType::PixelShader});
	auto vsSig = device.createVertexShaderSignature({ vs });

	m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

	auto myTexture = dx3d::Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/cat.jpg");
	if (!myTexture) {
	    // File not found or failed to load - use white texture as fallback
	    myTexture = dx3d::Texture2D::CreateWhiteTexture(device.getD3DDevice());
	    printf("Failed to load cat.jpg - using white texture\n");
	}
	auto catSprite = std::make_unique<dx3d::SpriteComponent>(
		device,
		L"DX3D/Assets/Textures/cat.jpg",  // Fixed path to match your working one
		1.0f, 1.0f  // width, height
	);

	// Set positions and transforms but they dont work right now 
	catSprite->setPosition(5.0f, 0.0f, 0.0f);
	catSprite->setScale(1.0f);
	m_sprites.push_back(std::move(catSprite));
}

dx3d::GraphicsEngine::~GraphicsEngine()
{
}

GraphicsDevice& dx3d::GraphicsEngine::getGraphicsDevice() noexcept
{
	return *m_graphicsDevice;
}

void dx3d::GraphicsEngine::render(SwapChain& swapChain)
{
	auto& context = *m_deviceContext;

	context.clearAndSetBackBuffer(swapChain, { 0.27f, 0.39f, 0.55f, 1.0f });
	context.setGraphicsPipelineState(*m_pipeline);
	context.setViewportSize(swapChain.getSize());

	for (auto& mesh : m_meshes)
	{
		mesh->draw(context);
	}
	for (auto& sprite : m_sprites)
	{
		if (sprite->isVisible() && sprite->isValid()) {
			Mat4 worldMatrix = sprite->getWorldMatrix();
			Vec3 pos = sprite->getPosition();

			sprite->draw(context);
		}
	}
	m_graphicsDevice->executeCommandList(context);
	swapChain.present();
}
