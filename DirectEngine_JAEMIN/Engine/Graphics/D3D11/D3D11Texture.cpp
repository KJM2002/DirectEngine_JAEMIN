#include "D3D11Texture.h"

#include <utility>

namespace Engine::Graphics::D3D11
{
    D3D11Texture::D3D11Texture(
        const RHI::TextureDesc& desc,
        ComPtr<ID3D11Texture2D> texture,
        ComPtr<ID3D11ShaderResourceView> shaderResourceView,
        ComPtr<ID3D11SamplerState> samplerState,
        ComPtr<ID3D11RenderTargetView> renderTargetView,
        ComPtr<ID3D11DepthStencilView> depthStencilView)
        : m_desc(desc)
        , m_texture(std::move(texture))
        , m_shaderResourceView(std::move(shaderResourceView))
        , m_samplerState(std::move(samplerState))
        , m_renderTargetView(std::move(renderTargetView))
        , m_depthStencilView(std::move(depthStencilView))
    {
        m_desc.initialData = nullptr;
    }

    RHI::TextureFormat D3D11Texture::GetFormat() const
    {
        return m_desc.format;
    }

    std::uint32_t D3D11Texture::GetWidth() const
    {
        return m_desc.width;
    }

    std::uint32_t D3D11Texture::GetHeight() const
    {
        return m_desc.height;
    }

    ID3D11ShaderResourceView* D3D11Texture::GetShaderResourceView() const
    {
        return m_shaderResourceView.Get();
    }

    ID3D11SamplerState* D3D11Texture::GetSamplerState() const
    {
        return m_samplerState.Get();
    }

    ID3D11RenderTargetView* D3D11Texture::GetRenderTargetView() const
    {
        return m_renderTargetView.Get();
    }

    ID3D11DepthStencilView* D3D11Texture::GetDepthStencilView() const
    {
        return m_depthStencilView.Get();
    }
}
