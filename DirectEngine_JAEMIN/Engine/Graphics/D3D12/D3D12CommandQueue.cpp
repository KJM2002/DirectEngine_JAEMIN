#include "D3D12CommandQueue.h"

namespace Engine::Graphics::D3D12
{
    bool D3D12CommandQueue::Initialize(ID3D12Device* device)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        return device && Succeeded(device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_queue.GetAddressOf())));
    }

    void D3D12CommandQueue::Shutdown()
    {
        m_queue.Reset();
    }

    void D3D12CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList)
    {
        if (!m_queue || !commandList)
        {
            return;
        }

        ID3D12CommandList* commandLists[] = { commandList };
        m_queue->ExecuteCommandLists(1, commandLists);
    }

    ID3D12CommandQueue* D3D12CommandQueue::GetNativeQueue() const
    {
        return m_queue.Get();
    }
}
