#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Math/Vec3.h>
#include <fstream>

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

	const Vertex vertexList[] =
	{
		{ {-0.5f,-0.5f,0.0f},{1,0,0,1} },
		{ {-0.5,0.5f,0.0f}  ,{0,1,0,1} },
		{ {0.5f,0.5f,0.0f}  ,{0,0,1,1} },

		{ {0.5f,0.5f,0.0f}  ,{0,0,1,1} },
		{ {0.5f,-0.5f,0.0f} ,{1,0,1,1} },
		{ {-0.5f,-0.5f,0.0f},{1,0,0,1} }
	};

	m_vb = device.createVertexBuffer({ vertexList, std::size(vertexList),sizeof(Vertex) });

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
	//what the hell are these magic numbers. a triangle? 
	context.clearAndSetBackBuffer(swapChain, { 0.27f,0.39f,0.55f,1.0f });
	context.setGraphicsPipelineState(*m_pipeline);

	context.setViewportSize(swapChain.getSize());

	auto& vb = *m_vb;
	context.setVertexBuffer(vb);
	context.drawTriangleList(vb.getVertexListSize(), 0u);

	auto& device = *m_graphicsDevice;
	device.executeCommandList(context);
	swapChain.present();
}
