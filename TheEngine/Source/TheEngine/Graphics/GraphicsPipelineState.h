#pragma once
#include <TheEngine/Graphics/GraphicsResource.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>

namespace TheEngine
{
	class GraphicsPipelineState final : public GraphicsResource, public IRenderPipelineState
	{
	public: 
		GraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const GraphicsResourceDesc& gDesc);
	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs{};
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps{};
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout{};
		friend class DeviceContext;
	};
}