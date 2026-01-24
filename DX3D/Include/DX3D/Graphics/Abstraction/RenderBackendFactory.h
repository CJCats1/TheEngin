#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/RenderBackend.h>
#include <memory>

namespace dx3d
{
    std::unique_ptr<IRenderBackend> createRenderBackend(RenderBackendType type, Logger& logger);
}
