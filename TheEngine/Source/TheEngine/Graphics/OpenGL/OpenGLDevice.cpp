#include <TheEngine/Graphics/OpenGL/OpenGLDevice.h>
#include <TheEngine/Graphics/OpenGL/OpenGLSwapChain.h>
#include <TheEngine/Graphics/OpenGL/OpenGLContext.h>
#include <TheEngine/Graphics/OpenGL/OpenGLShaderBinary.h>
#include <TheEngine/Graphics/OpenGL/OpenGLPipelineState.h>
#include <TheEngine/Graphics/OpenGL/OpenGLVertexShaderSignature.h>
#include <TheEngine/Graphics/OpenGL/OpenGLVertexBuffer.h>
#include <TheEngine/Graphics/OpenGL/OpenGLIndexBuffer.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <string>

using namespace TheEngine;

OpenGLDevice::OpenGLDevice(const GraphicsDeviceDesc& desc)
	: m_logger(desc.base.logger)
{
}

SwapChainPtr OpenGLDevice::createSwapChain(const SwapChainDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	return std::make_shared<OpenGLSwapChain>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
	return nullptr;
#endif
}

DeviceContextPtr OpenGLDevice::createDeviceContext()
{
	return std::make_shared<OpenGLContext>(m_logger);
}

ShaderBinaryPtr OpenGLDevice::compileShader(const ShaderCompileDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const char* stageDefine = (desc.shaderType == ShaderType::VertexShader) ? "THEENGINE_VERTEX" : "THEENGINE_FRAGMENT";
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
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
	return nullptr;
#endif
}

GraphicsPipelineStatePtr OpenGLDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	return std::make_shared<OpenGLPipelineState>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
	return nullptr;
#endif
}

VertexBufferPtr OpenGLDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	return std::make_shared<OpenGLVertexBuffer>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
	return nullptr;
#endif
}

VertexShaderSignaturePtr OpenGLDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	return std::make_shared<OpenGLVertexShaderSignature>(desc);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
	return nullptr;
#endif
}

IndexBufferPtr OpenGLDevice::createIndexBuffer(const IndexBufferDesc& desc)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	return std::make_shared<OpenGLIndexBuffer>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGL backend not enabled. Define THEENGINE_ENABLE_OPENGL.");
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
