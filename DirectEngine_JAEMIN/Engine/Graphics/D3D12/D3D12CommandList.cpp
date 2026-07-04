#include "D3D12CommandList.h"

namespace Engine::Graphics::D3D12
{
    bool D3D12CommandList::Initialize(ID3D12Device* device)
    {
        if (!device)
        {
            return false;
        }

        if (Failed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_allocator.GetAddressOf()))))
        {
            return false;
        }

        if (Failed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_allocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf()))))
        {
            return false;
        }

        m_commandList->Close();
        return true;
    }

    void D3D12CommandList::Shutdown()
    {
        m_commandList.Reset();
        m_allocator.Reset();
    }

    bool D3D12CommandList::Begin()
    {
        if (!m_allocator || !m_commandList)
        {
            return false;
        }

        return Succeeded(m_allocator->Reset()) && Succeeded(m_commandList->Reset(m_allocator.Get(), nullptr));
    }

    bool D3D12CommandList::Close()
    {
        return m_commandList && Succeeded(m_commandList->Close());
    }

    ID3D12GraphicsCommandList* D3D12CommandList::GetNativeCommandList() const
    {
        return m_commandList.Get();
    }
}
