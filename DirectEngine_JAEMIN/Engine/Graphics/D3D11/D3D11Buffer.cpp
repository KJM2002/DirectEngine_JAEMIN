#include "D3D11Buffer.h"

namespace Engine::Graphics::D3D11
{
    D3D11Buffer::D3D11Buffer(RHI::BufferDesc desc, ComPtr<ID3D11Buffer> buffer)
        : m_desc(desc)
        , m_buffer(buffer)
    {
    }

    const RHI::BufferDesc& D3D11Buffer::GetDesc() const
    {
        return m_desc;
    }

    RHI::BufferType D3D11Buffer::GetType() const
    {
        return m_desc.type;
    }

    std::uint32_t D3D11Buffer::GetSize() const
    {
        return static_cast<std::uint32_t>(m_desc.byteSize);
    }

    std::uint32_t D3D11Buffer::GetStride() const
    {
        return m_desc.stride;
    }

    std::uint32_t D3D11Buffer::GetCount() const
    {
        return m_desc.count;
    }

    ID3D11Buffer* D3D11Buffer::GetNativeBuffer() const
    {
        return m_buffer.Get();
    }
}
