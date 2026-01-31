#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>

namespace TheEngine
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
