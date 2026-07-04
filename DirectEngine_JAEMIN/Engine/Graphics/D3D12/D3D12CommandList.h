#pragma once

#include "D3D12Common.h"

namespace Engine::Graphics::D3D12
{
    class D3D12CommandList
    {
    public:
        bool Initialize(ID3D12Device* device);
        void Shutdown();
        bool Begin();
        bool Close();
        ID3D12GraphicsCommandList* GetNativeCommandList() const;

    private:
        ComPtr<ID3D12CommandAllocator> m_allocator;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
    };
}
