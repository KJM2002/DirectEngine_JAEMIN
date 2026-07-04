#include "D3D12DescriptorHeap.h"

namespace Engine::Graphics::D3D12
{
    bool D3D12DescriptorHeap::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, std::uint32_t count, bool shaderVisible)
    {
        if (!device || count == 0)
        {
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = type;
        desc.NumDescriptors = count;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (Failed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_heap.GetAddressOf()))))
        {
            return false;
        }

        m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
        return true;
    }

    void D3D12DescriptorHeap::Shutdown()
    {
        m_heap.Reset();
        m_descriptorSize = 0;
    }

    ID3D12DescriptorHeap* D3D12DescriptorHeap::GetNativeHeap() const
    {
        return m_heap.Get();
    }

    std::uint32_t D3D12DescriptorHeap::GetDescriptorSize() const
    {
        return m_descriptorSize;
    }
}
