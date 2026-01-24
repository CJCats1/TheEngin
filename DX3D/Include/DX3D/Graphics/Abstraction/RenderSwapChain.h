#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/GraphicsHandles.h>

namespace dx3d
{
    class IRenderSwapChain
    {
    public:
        virtual ~IRenderSwapChain() = default;
        virtual Rect getSize() const noexcept = 0;
        virtual void present(bool vsync = false) = 0;
        virtual void resize(int width, int height) = 0;
        virtual NativeGraphicsHandle getNativeSwapChainHandle() const noexcept = 0;
    };

    using RenderSwapChainPtr = std::shared_ptr<IRenderSwapChain>;
}
