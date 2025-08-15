#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Math/Geometry.h>
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

	//auto myTexture = std::make_shared<dx3d::Texture2D>();
	//auto quadTex = Mesh::CreateQuadTextured(device, 1.0f, 1.0f);
	//quadTex->setTexture(myTexture);


	auto quadColor = Mesh::CreateQuadSolidColored(device, 1.0f, 1.0f, Color::RED);
	m_meshes.push_back(quadColor);

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

	m_graphicsDevice->executeCommandList(context);
	swapChain.present();
}
