#pragma once

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12Context.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12SwapChain.h"
#include "Engine/Graphics/RHI/RHIDevice.h"

namespace Engine::Graphics::D3D12
{
    class D3D12Device final : public RHI::RHIDevice
    {
    public:
        bool Initialize() override;
        void Shutdown() override;
        RHI::RHIContext& GetContext() override;
        std::shared_ptr<RHI::RHIBuffer> CreateBuffer(const RHI::BufferDesc&, const void*) override { return nullptr; }
        std::shared_ptr<RHI::RHIBuffer> CreateVertexBuffer(const void*, std::uint32_t, std::uint32_t) override { return nullptr; }
        std::shared_ptr<RHI::RHIBuffer> CreateIndexBuffer(const void*, std::uint32_t, std::uint32_t) override { return nullptr; }
        std::shared_ptr<RHI::RHIBuffer> CreateConstantBuffer(std::uint32_t) override { return nullptr; }
        std::shared_ptr<RHI::RHIShader> CreateShader(const RHI::ShaderDesc&) override { return nullptr; }
        std::shared_ptr<RHI::RHIShader> CreateVertexShader(const std::wstring&, const std::string&) override { return nullptr; }
        std::shared_ptr<RHI::RHIShader> CreatePixelShader(const std::wstring&, const std::string&) override { return nullptr; }
        std::shared_ptr<RHI::RHITexture> CreateTexture2D(const RHI::TextureDesc&) override { return nullptr; }
        std::shared_ptr<RHI::RHITexture> CreateTexture2D(const Resource::ImageData&) override { return nullptr; }
        std::shared_ptr<RHI::RHITexture> CreateRenderTarget(std::uint32_t, std::uint32_t, RHI::TextureFormat) override { return nullptr; }
        std::shared_ptr<RHI::RHITexture> CreateDepthTarget(std::uint32_t, std::uint32_t, RHI::TextureFormat, bool) override { return nullptr; }
        std::unique_ptr<RHI::RHISwapChain> CreateSwapChain(const RHI::SwapChainDesc& desc) override;

    private:
        ComPtr<IDXGIFactory6> m_factory;
        ComPtr<ID3D12Device> m_device;
        D3D12Context m_context;
        D3D12CommandQueue m_commandQueue;
        D3D12CommandList m_commandList;
        D3D12DescriptorHeap m_rtvHeap;
        D3D12Fence m_fence;
    };
}
