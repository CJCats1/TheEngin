#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>

namespace dx3d
{
	class OpenGLVertexShaderSignature final : public IRenderVertexShaderSignature
	{
	public:
		explicit OpenGLVertexShaderSignature(const VertexShaderSignatureDesc& desc)
			: m_vsBinary(desc.vsBinary)
		{
		}

		BinaryData getShaderBinaryData() const noexcept override
		{
			return m_vsBinary ? m_vsBinary->getData() : BinaryData{};
		}

		BinaryData getInputElementsData() const noexcept override
		{
			return {};
		}

	private:
		ShaderBinaryPtr m_vsBinary{};
	};
}
