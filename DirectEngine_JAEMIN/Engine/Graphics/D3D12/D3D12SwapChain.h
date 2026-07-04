#pragma once

#include "D3D12Common.h"
#include "Engine/Graphics/RHI/RHISwapChain.h"

#include <array>
#include <cstdint>

namespace Engine::Graphics::D3D12
{
    class D3D12SwapChain final : public RHI::RHISwapChain
    {
    public:
        D3D12SwapChain(IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* commandQueue);

        bool Initialize(void* nativeWindowHandle, std::uint32_t width, std::uint32_t height) override;
        void Shutdown() override;
        void Present() override;
        void Resize(std::uint32_t width, std::uint32_t height) override;

        ID3D12Resource* GetCurrentBackBuffer() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

    private:
        bool CreateRenderTargetViews();
        void ReleaseRenderTargets();

        static constexpr std::uint32_t BackBufferCount = 2;

        IDXGIFactory6* m_factory = nullptr;
        ID3D12Device* m_device = nullptr;
        ID3D12CommandQueue* m_commandQueue = nullptr;
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        std::array<ComPtr<ID3D12Resource>, BackBufferCount> m_backBuffers;
        std::uint32_t m_rtvDescriptorSize = 0;
        std::uint32_t m_currentBackBufferIndex = 0;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
    };
}
