#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/RenderBackend.h>
#include <memory>

namespace TheEngine
{
    std::unique_ptr<IRenderBackend> createRenderBackend(RenderBackendType type, Logger& logger);
}
