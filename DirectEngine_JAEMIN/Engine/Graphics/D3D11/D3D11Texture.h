#pragma once

#include "D3D11Common.h"
#include "Engine/Graphics/RHI/RHITexture.h"

namespace Engine::Graphics::D3D11
{
    class D3D11Texture final : public RHI::RHITexture
    {
    public:
        D3D11Texture(
            const RHI::TextureDesc& desc,
            ComPtr<ID3D11Texture2D> texture,
            ComPtr<ID3D11ShaderResourceView> shaderResourceView,
            ComPtr<ID3D11SamplerState> samplerState,
            ComPtr<ID3D11RenderTargetView> renderTargetView = nullptr,
            ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr);

        RHI::TextureFormat GetFormat() const override;
        std::uint32_t GetWidth() const override;
        std::uint32_t GetHeight() const override;

        ID3D11ShaderResourceView* GetShaderResourceView() const;
        ID3D11SamplerState* GetSamplerState() const;
        ID3D11RenderTargetView* GetRenderTargetView() const;
        ID3D11DepthStencilView* GetDepthStencilView() const;

    private:
        RHI::TextureDesc m_desc;
        ComPtr<ID3D11Texture2D> m_texture;
        ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        ComPtr<ID3D11SamplerState> m_samplerState;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    };
}
