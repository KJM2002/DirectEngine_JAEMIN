#pragma once

#include "D3D11Common.h"
#include "Engine/Graphics/RHI/RHIContext.h"

namespace Engine::Graphics::D3D11
{
    class D3D11Device;

    class D3D11Context final : public RHI::RHIContext
    {
    public:
        explicit D3D11Context(ComPtr<ID3D11DeviceContext> context);
        ~D3D11Context() override = default;

        void ClearRenderTarget(float red, float green, float blue, float alpha) override;
        void ClearDepthStencil(float depth, std::uint8_t stencil) override;
        void SetDefaultRenderTarget() override;
        void SetRenderTarget(const std::shared_ptr<RHI::RHITexture>& colorTarget, const std::shared_ptr<RHI::RHITexture>& depthTarget) override;
        void SetViewport(const RHI::ViewportDesc& viewport) override;
        void SetPrimitiveTopology(RHI::PrimitiveTopology topology) override;
        void SetVertexBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer) override;
        void SetIndexBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer) override;
        void SetVertexShader(const std::shared_ptr<RHI::RHIShader>& shader) override;
        void SetPixelShader(const std::shared_ptr<RHI::RHIShader>& shader) override;
        void SetConstantBuffer(RHI::ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHI::RHIBuffer>& buffer) override;
        void SetTexture(RHI::ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHI::RHITexture>& texture) override;
        void UpdateConstantBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer, const void* data, std::uint32_t size) override;
        void Draw(std::uint32_t vertexCount, std::uint32_t startVertex) override;
        void DrawIndexed(std::uint32_t indexCount) override;
        void* GetNativeHandleForDebugUI() const override;
        ID3D11DeviceContext* GetNativeContext() const;

    private:
        friend class D3D11Device;
        friend class D3D11SwapChain;

        void SetRenderTargets(ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);
        void SetDefaultRenderTargets(ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);

        ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11RenderTargetView> m_defaultRenderTargetView;
        ComPtr<ID3D11DepthStencilView> m_defaultDepthStencilView;
    };
}
