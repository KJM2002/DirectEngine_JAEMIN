#pragma once

#include "D3D12Common.h"
#include "Engine/Graphics/RHI/RHIContext.h"

namespace Engine::Graphics::D3D12
{
    class D3D12CommandList;
    class D3D12CommandQueue;
    class D3D12SwapChain;

    class D3D12Context final : public RHI::RHIContext
    {
    public:
        void Initialize(D3D12CommandList* commandList, D3D12CommandQueue* commandQueue) { m_commandList = commandList; m_commandQueue = commandQueue; }
        void SetSwapChain(D3D12SwapChain* swapChain) { m_swapChain = swapChain; }
        void ClearRenderTarget(float red, float green, float blue, float alpha) override;
        void ClearDepthStencil(float, std::uint8_t) override {}
        void SetDefaultRenderTarget() override {}
        void SetRenderTarget(const std::shared_ptr<RHI::RHITexture>&, const std::shared_ptr<RHI::RHITexture>&) override {}
        void SetViewport(const RHI::ViewportDesc&) override {}
        void SetPrimitiveTopology(RHI::PrimitiveTopology) override {}
        void SetVertexBuffer(const std::shared_ptr<RHI::RHIBuffer>&) override {}
        void SetIndexBuffer(const std::shared_ptr<RHI::RHIBuffer>&) override {}
        void SetVertexShader(const std::shared_ptr<RHI::RHIShader>&) override {}
        void SetPixelShader(const std::shared_ptr<RHI::RHIShader>&) override {}
        void SetConstantBuffer(RHI::ShaderStage, std::uint32_t, const std::shared_ptr<RHI::RHIBuffer>&) override {}
        void SetTexture(RHI::ShaderStage, std::uint32_t, const std::shared_ptr<RHI::RHITexture>&) override {}
        void UpdateConstantBuffer(const std::shared_ptr<RHI::RHIBuffer>&, const void*, std::uint32_t) override {}
        void Draw(std::uint32_t, std::uint32_t) override {}
        void DrawIndexed(std::uint32_t) override {}

    private:
        D3D12CommandList* m_commandList = nullptr;
        D3D12CommandQueue* m_commandQueue = nullptr;
        D3D12SwapChain* m_swapChain = nullptr;
    };
}
