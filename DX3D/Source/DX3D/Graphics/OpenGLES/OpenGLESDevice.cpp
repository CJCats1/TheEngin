#include <DX3D/Graphics/OpenGLES/OpenGLESDevice.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESSwapChain.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESContext.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESShaderBinary.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESPipelineState.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESVertexShaderSignature.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESVertexBuffer.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESIndexBuffer.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <string>

using namespace dx3d;

OpenGLESDevice::OpenGLESDevice(const GraphicsDeviceDesc& desc)
	: m_logger(desc.base.logger)
{
}

SwapChainPtr OpenGLESDevice::createSwapChain(const SwapChainDesc& desc)
{
#if defined(DX3D_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESSwapChain>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
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
#if defined(DX3D_PLATFORM_ANDROID)
	const char* stageDefine = (desc.shaderType == ShaderType::VertexShader) ? "DX3D_VERTEX" : "DX3D_FRAGMENT";
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
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

GraphicsPipelineStatePtr OpenGLESDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
#if defined(DX3D_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESPipelineState>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

VertexBufferPtr OpenGLESDevice::createVertexBuffer(const VertexBufferDesc& desc)
{
#if defined(DX3D_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESVertexBuffer>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

VertexShaderSignaturePtr OpenGLESDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
{
#if defined(DX3D_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESVertexShaderSignature>(desc);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
		"OpenGLES backend only supported on Android.");
	return nullptr;
#endif
}

IndexBufferPtr OpenGLESDevice::createIndexBuffer(const IndexBufferDesc& desc)
{
#if defined(DX3D_PLATFORM_ANDROID)
	return std::make_shared<OpenGLESIndexBuffer>(desc, m_logger);
#else
	(void)desc;
	DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
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
