#include <DX3D/Graphics/OpenGL/OpenGLDevice.h>
#include <DX3D/Graphics/OpenGL/OpenGLSwapChain.h>
#include <DX3D/Graphics/OpenGL/OpenGLContext.h>
#include <DX3D/Graphics/OpenGL/OpenGLShaderBinary.h>
#include <DX3D/Graphics/OpenGL/OpenGLPipelineState.h>
#include <DX3D/Graphics/OpenGL/OpenGLVertexShaderSignature.h>
#include <DX3D/Graphics/OpenGL/OpenGLVertexBuffer.h>
#include <DX3D/Graphics/OpenGL/OpenGLIndexBuffer.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <string>

using namespace dx3d;

OpenGLDevice::OpenGLDevice(const GraphicsDeviceDesc& desc)
	: m_logger(desc.base.logger)
{
}

SwapChainPtr OpenGLDevice::createSwapChain(const SwapChainDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLSwapChain>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

DeviceContextPtr OpenGLDevice::createDeviceContext()
{
	return std::make_shared<OpenGLContext>(m_logger);
}

ShaderBinaryPtr OpenGLDevice::compileShader(const ShaderCompileDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	const char* stageDefine = (desc.shaderType == ShaderType::VertexShader) ? "DX3D_VERTEX" : "DX3D_FRAGMENT";
	std::string header = "#version 330 core\n#define ";
	header += stageDefine;
	header += "\n";
	std::string sourceWithHeader = header;
	sourceWithHeader.append(static_cast<const char*>(desc.shaderSourceCode), desc.shaderSourceCodeSize);

	ShaderCompileDesc glDesc = desc;
	glDesc.shaderSourceCode = sourceWithHeader.data();
	glDesc.shaderSourceCodeSize = sourceWithHeader.size();
	return std::make_shared<OpenGLShaderBinary>(glDesc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

GraphicsPipelineStatePtr OpenGLDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLPipelineState>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

VertexBufferPtr OpenGLDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLVertexBuffer>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

VertexShaderSignaturePtr OpenGLDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLVertexShaderSignature>(desc);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

IndexBufferPtr OpenGLDevice::createIndexBuffer(const IndexBufferDesc& desc)
{
#if defined(DX3D_ENABLE_OPENGL)
	return std::make_shared<OpenGLIndexBuffer>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
	return nullptr;
#endif
}

void OpenGLDevice::executeCommandList(IRenderContext& context)
{
	(void)context;
}

NativeGraphicsHandle OpenGLDevice::getNativeDeviceHandle() const noexcept
{
	return nullptr;
}
