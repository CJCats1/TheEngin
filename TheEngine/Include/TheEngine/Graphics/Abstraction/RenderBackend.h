#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/RenderDevice.h>
#include <memory>

namespace TheEngine
{
    class IImGuiBackend
    {
    public:
        virtual ~IImGuiBackend() = default;
        virtual void initialize(void* windowHandle, IRenderDevice& device, IRenderContext& context) = 0;
        virtual void shutdown() = 0;
        virtual void newFrame() = 0;
        virtual void renderDrawData() = 0;
        virtual void invalidateDeviceObjects() = 0;
        virtual void createDeviceObjects() = 0;
        virtual void renderPlatformWindows() = 0;
    };

    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;
        virtual RenderDevicePtr createDevice(const GraphicsDeviceDesc& desc) = 0;
        virtual std::unique_ptr<IImGuiBackend> createImGuiBackend() = 0;
    };
}
