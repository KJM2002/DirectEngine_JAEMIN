#include "D3D12Fence.h"

namespace Engine::Graphics::D3D12
{
    bool D3D12Fence::Initialize(ID3D12Device* device)
    {
        m_value = 0;
        return device && Succeeded(device->CreateFence(m_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));
    }

    void D3D12Fence::Shutdown()
    {
        m_fence.Reset();
        m_value = 0;
    }
}
