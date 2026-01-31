#pragma once
#include <TheEngine/Graphics/GraphicsResource.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>

namespace TheEngine
{
	class ShaderBinary final : public GraphicsResource, public IRenderShaderBinary
	{
	public: 
		ShaderBinary(const ShaderCompileDesc& desc,const GraphicsResourceDesc& gDesc);
		BinaryData getData() const noexcept override;
		ShaderType getType() const noexcept override;
	private:
		Microsoft::WRL::ComPtr<ID3DBlob> m_blob{};
		ShaderType m_type{};
	};
}