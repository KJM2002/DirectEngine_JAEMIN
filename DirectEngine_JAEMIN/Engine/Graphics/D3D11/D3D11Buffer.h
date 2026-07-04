#pragma once

#include "D3D11Common.h"
#include "Engine/Graphics/RHI/RHIBuffer.h"

namespace Engine::Graphics::D3D11
{
    class D3D11Buffer final : public RHI::RHIBuffer
    {
    public:
        D3D11Buffer(RHI::BufferDesc desc, ComPtr<ID3D11Buffer> buffer);
        ~D3D11Buffer() override = default;

        const RHI::BufferDesc& GetDesc() const override;
        RHI::BufferType GetType() const override;
        std::uint32_t GetSize() const override;
        std::uint32_t GetStride() const override;
        std::uint32_t GetCount() const override;

        ID3D11Buffer* GetNativeBuffer() const;

    private:
        RHI::BufferDesc m_desc;
        ComPtr<ID3D11Buffer> m_buffer;
    };
}
