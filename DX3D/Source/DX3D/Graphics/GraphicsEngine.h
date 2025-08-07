#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>

namespace dx3d
{
	class GraphicsEngine final : public Base
	{
	public: 
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;

		GraphicsDevice& getGraphicsDevice() noexcept;
		void render(SwapChain& swapChain);
	private:
		struct Vertex
		{
			Vec3 position;
			Vec4 color;
		};
	private:
		std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
		DeviceContextPtr m_deviceContext{};
		GraphicsPipelineStatePtr m_pipeline{};
		VertexBufferPtr m_vb{};
	};
}