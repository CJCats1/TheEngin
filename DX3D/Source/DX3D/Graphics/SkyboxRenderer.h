#pragma once
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <DX3D/Graphics/Abstraction/RenderContext.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <DX3D/Graphics/Mesh.h>

namespace dx3d
{
	class SkyboxRenderer
	{
	public:
		static void render(IRenderDevice& device, IRenderContext& context, Mesh& skybox,
			IRenderPipelineState* pipeline, const Mat4& viewMatrixNoTranslation, bool visible);
	};
}
