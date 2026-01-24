#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <d3dcompiler.h>

namespace dx3d
{
	class VertexShaderSignature final : public GraphicsResource, public IRenderVertexShaderSignature
	{
	public:
		VertexShaderSignature(const VertexShaderSignatureDesc& desc, const GraphicsResourceDesc& gDesc);
		BinaryData getShaderBinaryData() const noexcept override;
		BinaryData getInputElementsData() const noexcept override;
	private:
		ShaderBinaryPtr m_vsBinary{};
		Microsoft::WRL::ComPtr<ID3D11ShaderReflection> m_shaderReflection{};
		D3D11_INPUT_ELEMENT_DESC m_elements[D3D11_STANDARD_VERTEX_ELEMENT_COUNT]{};
		ui32 m_numElements{};
	};
}