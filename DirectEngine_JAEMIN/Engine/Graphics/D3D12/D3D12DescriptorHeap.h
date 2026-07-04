#pragma once

#include "D3D12Common.h"

namespace Engine::Graphics::D3D12
{
    class D3D12DescriptorHeap
    {
    public:
        bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count, bool shaderVisible);
        void Shutdown();
        ID3D12DescriptorHeap* GetNativeHeap() const;
        std::uint32_t GetDescriptorSize() const;

    private:
        ComPtr<ID3D12DescriptorHeap> m_heap;
        std::uint32_t m_descriptorSize = 0;
    };
}
