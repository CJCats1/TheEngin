#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/SpriteComponent.h> 

namespace dx3d
{
	class GraphicsEngine final : public Base
	{
	public: 
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;

		GraphicsDevice& getGraphicsDevice() noexcept;
		void render(SwapChain& swapChain);
		void addMesh(const std::shared_ptr<dx3d::Mesh>& mesh)
		{
			m_meshes.push_back(mesh);
		}
		void addSprite(std::unique_ptr<dx3d::SpriteComponent> sprite)
		{
			m_sprites.push_back(std::move(sprite));
		}

		void clearSprites()
		{
			m_sprites.clear();
		}
	private:
		std::vector<std::unique_ptr<dx3d::SpriteComponent>> m_sprites;
		std::vector<std::shared_ptr<dx3d::Mesh>> m_meshes;
		std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
		DeviceContextPtr m_deviceContext{};
		GraphicsPipelineStatePtr m_pipeline{};
		VertexBufferPtr m_vb{};
	};
}