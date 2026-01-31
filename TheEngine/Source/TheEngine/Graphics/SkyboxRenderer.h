#pragma once
#include <TheEngine/Graphics/Abstraction/RenderDevice.h>
#include <TheEngine/Graphics/Abstraction/RenderContext.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>
#include <TheEngine/Graphics/Mesh.h>

namespace TheEngine
{
	class SkyboxRenderer
	{
	public:
		static void render(IRenderDevice& device, IRenderContext& context, Mesh& skybox,
			IRenderPipelineState* pipeline, const Mat4& viewMatrixNoTranslation, bool visible);
	};
}
