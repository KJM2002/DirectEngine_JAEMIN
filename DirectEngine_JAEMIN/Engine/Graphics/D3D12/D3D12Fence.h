#pragma once

#include "D3D12Common.h"

namespace Engine::Graphics::D3D12
{
    class D3D12Fence
    {
    public:
        bool Initialize(ID3D12Device* device);
        void Shutdown();

    private:
        ComPtr<ID3D12Fence> m_fence;
        std::uint64_t m_value = 0;
    };
}
