#include <TheEngine/Graphics/VertexShaderSignature.h>
#include <TheEngine/Graphics/ShaderBinary.h>
#include <TheEngine/Graphics/GraphicsUtils.h>
#include <d3dcompiler.h>
#include <ranges>

TheEngine::VertexShaderSignature::VertexShaderSignature(const VertexShaderSignatureDesc& desc, const GraphicsResourceDesc& gDesc) :
	GraphicsResource(gDesc), m_vsBinary(desc.vsBinary)
{
	if (!desc.vsBinary) THEENGINE_LOG_THROW_INVALID_ARG("No shader binary provided.");
	if (desc.vsBinary->getType() != ShaderType::VertexShader)
		THEENGINE_LOG_THROW_INVALID_ARG("The 'vsBinary' member is not a valid vertex shader binary.");

	auto vsData = m_vsBinary->getData();

	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(D3DReflect(
		vsData.data,
		vsData.dataSize,
		IID_PPV_ARGS(&m_shaderReflection)),
		"D3DReflect failed.");

	D3D11_SHADER_DESC shaderDesc{};
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_shaderReflection->GetDesc(&shaderDesc),
		"ID3D11ShaderReflection::GetDesc failed.");

    // Reflect declared inputs and filter out system-value semantics (e.g., SV_VertexID)
    D3D11_SIGNATURE_PARAMETER_DESC params[D3D11_STANDARD_VERTEX_ELEMENT_COUNT]{};
    for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
    {
        THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_shaderReflection->GetInputParameterDesc(i, &params[i]),
            "ID3D11ShaderReflection::GetInputParameterDesc failed.");
    }

    UINT outCount = 0;
    for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
    {
        const auto& param = params[i];
        // Skip system-value inputs (SV_*) which are not fed by the input assembler
        if (param.SystemValueType != D3D_NAME_UNDEFINED) continue;
        if (param.SemanticName && param.SemanticName[0] == 'S' && param.SemanticName[1] == 'V' && param.SemanticName[2] == '_') continue;

        m_elements[outCount++] = {
            param.SemanticName,
            param.SemanticIndex,
            GraphicsUtils::GetDXGIFormatFromMask(param.ComponentType,param.Mask),
            0,
            D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        };
    }
    m_numElements = outCount;
}

TheEngine::BinaryData TheEngine::VertexShaderSignature::getShaderBinaryData() const noexcept
{
	return m_vsBinary->getData();
}

TheEngine::BinaryData TheEngine::VertexShaderSignature::getInputElementsData() const noexcept
{
	return
	{
		m_elements,
		m_numElements
	};
}