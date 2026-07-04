#pragma once

#include "D3D12Common.h"

namespace Engine::Graphics::D3D12
{
    class D3D12CommandQueue
    {
    public:
        bool Initialize(ID3D12Device* device);
        void Shutdown();
        void ExecuteCommandList(ID3D12CommandList* commandList);
        ID3D12CommandQueue* GetNativeQueue() const;

    private:
        ComPtr<ID3D12CommandQueue> m_queue;
    };
}
