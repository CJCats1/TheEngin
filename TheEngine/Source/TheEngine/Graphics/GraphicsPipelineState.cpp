#include <TheEngine/Graphics/GraphicsPipelineState.h>
#include <TheEngine/Graphics/ShaderBinary.h>
#include <TheEngine/Graphics/VertexShaderSignature.h>

TheEngine::GraphicsPipelineState::GraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const GraphicsResourceDesc& gDesc) : GraphicsResource(gDesc)
{
	if (desc.ps.getType() != ShaderType::PixelShader)
		THEENGINE_LOG_THROW_INVALID_ARG("The ps member is not a valid pixel shader binary. whoops");
	auto vs = desc.vs.getShaderBinaryData();
	auto ps = desc.ps.getData();
	auto vsInputElements = desc.vs.getInputElementsData();

    // Some shaders (e.g., full-screen tri using SV_VertexID) have no inputs.
    // In that case, skip creating an input layout and bind nullptr at draw time.
    if (vsInputElements.dataSize > 0)
    {
        THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL
        (
            m_device.CreateInputLayout
            (
                static_cast<const D3D11_INPUT_ELEMENT_DESC*>(vsInputElements.data),
                static_cast<ui32>(vsInputElements.dataSize),
                vs.data,
                vs.dataSize,
                &m_layout
            ),
            "CreateInputLayout failed."
        );
    }
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(
	m_device.CreateVertexShader(vs.data,vs.dataSize,nullptr, &m_vs),
		"CreateVertexShader failed."
	);
	THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreatePixelShader(ps.data,ps.dataSize,nullptr,&m_ps),
		"CreatePixelShader failed");
}
