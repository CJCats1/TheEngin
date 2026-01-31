#include <TheEngine/Graphics/ShaderBinary.h>
#include <TheEngine/Graphics/GraphicsUtils.h>
#include <d3dcompiler.h>
#include <iostream>
TheEngine::ShaderBinary::ShaderBinary(const ShaderCompileDesc& desc, const GraphicsResourceDesc& gDesc): 
	GraphicsResource(gDesc),m_type(desc.shaderType)
{
	if (!desc.shaderSourceName) THEENGINE_LOG_THROW_INVALID_ARG("No shader source name provided.");
	if (!desc.shaderSourceCode) THEENGINE_LOG_THROW_INVALID_ARG("No shader source code provided.");
	if (!desc.shaderSourceCodeSize) THEENGINE_LOG_THROW_INVALID_ARG("No shader source code size provided.");
	if (!desc.shaderEntryPoint) THEENGINE_LOG_THROW_INVALID_ARG("No shader entry point provided.");

	std::cout << "ShaderBinary: Compiling shader " << desc.shaderSourceName << " with entry point " << desc.shaderEntryPoint << std::endl;

	UINT compileFlags{};

#ifdef _DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif  
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{};
	HRESULT hr = D3DCompile(
		desc.shaderSourceCode,
		desc.shaderSourceCodeSize,
		desc.shaderSourceName,
		nullptr,
		nullptr,
		desc.shaderEntryPoint,
		TheEngine::GraphicsUtils::GetShaderModelTarget(desc.shaderType),
		compileFlags,
		0,
		&m_blob,
		&errorBlob
	);
	TheEngine::GraphicsLogUtils::CheckShaderCompile(m_logger, hr, errorBlob.Get());
	
	std::cout << "ShaderBinary: Shader compiled successfully" << std::endl;
}

TheEngine::BinaryData TheEngine::ShaderBinary::getData() const noexcept
{
	return
	{
		m_blob->GetBufferPointer(),
		m_blob->GetBufferSize()
	};
}

TheEngine::ShaderType TheEngine::ShaderBinary::getType() const noexcept
{
	return m_type;
}
