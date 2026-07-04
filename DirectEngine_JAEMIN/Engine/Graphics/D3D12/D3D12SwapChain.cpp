#include "D3D12SwapChain.h"

#include "Engine/Core/Log.h"

#include <Windows.h>

namespace Engine::Graphics::D3D12
{
    D3D12SwapChain::D3D12SwapChain(IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* commandQueue)
        : m_factory(factory)
        , m_device(device)
        , m_commandQueue(commandQueue)
    {
    }

    bool D3D12SwapChain::Initialize(void* nativeWindowHandle, std::uint32_t width, std::uint32_t height)
    {
        if (!m_factory || !m_device || !m_commandQueue || !nativeWindowHandle || width == 0 || height == 0)
        {
            return false;
        }

        m_width = width;
        m_height = height;

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = BackBufferCount;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ComPtr<IDXGISwapChain1> swapChain;
        HWND hwnd = static_cast<HWND>(nativeWindowHandle);
        if (Failed(m_factory->CreateSwapChainForHwnd(m_commandQueue, hwnd, &desc, nullptr, nullptr, swapChain.GetAddressOf())))
        {
            Core::Log::Error("Failed to create D3D12 swap chain.");
            return false;
        }

        if (Failed(swapChain.As(&m_swapChain)))
        {
            Core::Log::Error("Failed to query IDXGISwapChain3.");
            return false;
        }

        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        return CreateRenderTargetViews();
    }

    void D3D12SwapChain::Shutdown()
    {
        ReleaseRenderTargets();
        m_rtvHeap.Reset();
        m_swapChain.Reset();
    }

    void D3D12SwapChain::Present()
    {
        if (!m_swapChain)
        {
            return;
        }

        m_swapChain->Present(1, 0);
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void D3D12SwapChain::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (!m_swapChain || width == 0 || height == 0)
        {
            return;
        }

        ReleaseRenderTargets();
        if (Failed(m_swapChain->ResizeBuffers(BackBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0)))
        {
            Core::Log::Error("Failed to resize D3D12 swap chain.");
            return;
        }

        m_width = width;
        m_height = height;
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        CreateRenderTargetViews();
    }

    ID3D12Resource* D3D12SwapChain::GetCurrentBackBuffer() const
    {
        return m_backBuffers[m_currentBackBufferIndex].Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentRenderTargetView() const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
        if (!m_rtvHeap)
        {
            return handle;
        }

        handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(m_currentBackBufferIndex) * m_rtvDescriptorSize;
        return handle;
    }

    bool D3D12SwapChain::CreateRenderTargetViews()
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = BackBufferCount;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (Failed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf()))))
        {
            Core::Log::Error("Failed to create D3D12 RTV descriptor heap.");
            return false;
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (std::uint32_t i = 0; i < BackBufferCount; ++i)
        {
            if (Failed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBuffers[i].GetAddressOf()))))
            {
                Core::Log::Error("Failed to get D3D12 back buffer.");
                return false;
            }

            m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
            handle.ptr += m_rtvDescriptorSize;
        }

        return true;
    }

    void D3D12SwapChain::ReleaseRenderTargets()
    {
        for (auto& backBuffer : m_backBuffers)
        {
            backBuffer.Reset();
        }
    }
}
