#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <d3d11.h>
#include <wrl.h>

namespace dx3d
{
    class IndexBuffer : public IRenderIndexBuffer
    {
    public:
        IndexBuffer(ID3D11Buffer* buf, ui32 count, ui32 stride)
            : m_buf(buf), m_count(count), m_stride(stride) {
        }

        ID3D11Buffer* getNative() const { return m_buf.Get(); }
        ui32 getCount() const noexcept override { return m_count; }
        ui32 getStride() const noexcept override { return m_stride; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buf;
        ui32 m_count{};
        ui32 m_stride{};
    };
}
