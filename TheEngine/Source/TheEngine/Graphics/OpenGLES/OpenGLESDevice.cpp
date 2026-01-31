#include <TheEngine/Graphics/OpenGLES/OpenGLESDevice.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESSwapChain.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESContext.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESShaderBinary.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESPipelineState.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESVertexShaderSignature.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESVertexBuffer.h>
#include <TheEngine/Graphics/OpenGLES/OpenGLESIndexBuffer.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <string>

using namespace TheEngine;

OpenGLESDevice::OpenGLESDevice(const GraphicsDeviceDesc& desc)
	: m_logger(desc.base.logger)
{
}

SwapChainPtr OpenGLESDevice::createSwapChain(const SwapChainDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESSwapChain>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

DeviceContextPtr OpenGLESDevice::createDeviceContext()
{
	return std::make_shared<OpenGLESContext>(m_logger);
}

ShaderBinaryPtr OpenGLESDevice::compileShader(const ShaderCompileDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	const char* stageDefine = (desc.shaderType == ShaderType::VertexShader) ? "THEENGINE_VERTEX" : "THEENGINE_FRAGMENT";
	std::string header = "#version 300 es\n";
	header += "precision mediump float;\n";
	header += "#define ";
	header += stageDefine;
	header += "\n";
	std::string sourceWithHeader = header;
	sourceWithHeader.append(static_cast<const char*>(desc.shaderSourceCode), desc.shaderSourceCodeSize);

	ShaderCompileDesc glDesc = desc;
	glDesc.shaderSourceCode = sourceWithHeader.data();
	glDesc.shaderSourceCodeSize = sourceWithHeader.size();
	return std::make_shared<OpenGLESShaderBinary>(glDesc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

GraphicsPipelineStatePtr OpenGLESDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESPipelineState>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

VertexBufferPtr OpenGLESDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESVertexBuffer>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

VertexShaderSignaturePtr OpenGLESDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESVertexShaderSignature>(desc);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

IndexBufferPtr OpenGLESDevice::createIndexBuffer(const IndexBufferDesc& desc)
{
#if defined(THEENGINE_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESIndexBuffer>(desc, m_logger);
#else
	(void)desc;
	THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

void OpenGLESDevice::executeCommandList(IRenderContext& context)
{
	(void)context;
}

NativeGraphicsHandle OpenGLESDevice::getNativeDeviceHandle() const noexcept
{
	return nullptr;
}
