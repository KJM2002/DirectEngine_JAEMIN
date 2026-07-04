#include "D3D12Device.h"

#include "Engine/Core/Log.h"

namespace Engine::Graphics::D3D12
{
    bool D3D12Device::Initialize()
    {
        UINT factoryFlags = 0;
#if defined(_DEBUG)
        ComPtr<ID3D12Debug> debugController;
        if (Succeeded(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
            factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif

        if (Failed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(m_factory.GetAddressOf()))))
        {
            Core::Log::Error("Failed to create D3D12 DXGI factory.");
            return false;
        }

        if (Failed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()))))
        {
            Core::Log::Error("Failed to create D3D12 device.");
            return false;
        }

        if (!m_commandQueue.Initialize(m_device.Get()) || !m_commandList.Initialize(m_device.Get()) || !m_rtvHeap.Initialize(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false) || !m_fence.Initialize(m_device.Get()))
        {
            Core::Log::Error("Failed to initialize D3D12 command/descriptor/fence objects.");
            return false;
        }

        m_context.Initialize(&m_commandList, &m_commandQueue);
        Core::Log::Info("D3D12 backend scaffold initialized.");
        return true;
    }

    void D3D12Device::Shutdown()
    {
        m_fence.Shutdown();
        m_rtvHeap.Shutdown();
        m_commandList.Shutdown();
        m_commandQueue.Shutdown();
        m_device.Reset();
        m_factory.Reset();
    }

    RHI::RHIContext& D3D12Device::GetContext()
    {
        return m_context;
    }

    std::unique_ptr<RHI::RHISwapChain> D3D12Device::CreateSwapChain(const RHI::SwapChainDesc& desc)
    {
        auto swapChain = std::make_unique<D3D12SwapChain>(m_factory.Get(), m_device.Get(), m_commandQueue.GetNativeQueue());
        if (!swapChain->Initialize(desc.nativeWindowHandle, desc.width, desc.height))
        {
            return nullptr;
        }

        m_context.SetSwapChain(swapChain.get());
        return swapChain;
    }

    std::unique_ptr<RHI::RHIDevice> CreateD3D12Device()
    {
        return std::make_unique<D3D12Device>();
    }
}
