#include "D3D11Context.h"

#include "D3D11Buffer.h"
#include "D3D11Shader.h"
#include "D3D11Texture.h"

#include <cstring>

namespace Engine::Graphics::D3D11
{
    D3D11Context::D3D11Context(ComPtr<ID3D11DeviceContext> context)
        : m_context(context)
    {
    }

    void D3D11Context::SetRenderTargets(ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView)
    {
        m_renderTargetView = renderTargetView;
        m_depthStencilView = depthStencilView;

        ID3D11RenderTargetView* nativeRenderTargetView = m_renderTargetView.Get();
        const UINT renderTargetCount = nativeRenderTargetView != nullptr ? 1u : 0u;
        m_context->OMSetRenderTargets(renderTargetCount, nativeRenderTargetView ? &nativeRenderTargetView : nullptr, m_depthStencilView.Get());
    }

    void D3D11Context::SetDefaultRenderTargets(ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView)
    {
        m_defaultRenderTargetView = renderTargetView;
        m_defaultDepthStencilView = depthStencilView;
        SetRenderTargets(renderTargetView, depthStencilView);
    }

    void D3D11Context::ClearRenderTarget(float red, float green, float blue, float alpha)
    {
        if (!m_renderTargetView)
        {
            return;
        }

        ID3D11RenderTargetView* nativeRenderTargetView = m_renderTargetView.Get();
        m_context->OMSetRenderTargets(1, &nativeRenderTargetView, m_depthStencilView.Get());

        const float color[] = { red, green, blue, alpha };
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), color);
    }

    void D3D11Context::ClearDepthStencil(float depth, std::uint8_t stencil)
    {
        if (!m_depthStencilView)
        {
            return;
        }

        m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
    }

    void D3D11Context::SetDefaultRenderTarget()
    {
        SetRenderTargets(m_defaultRenderTargetView.Get(), m_defaultDepthStencilView.Get());
    }

    void D3D11Context::SetRenderTarget(const std::shared_ptr<RHI::RHITexture>& colorTarget, const std::shared_ptr<RHI::RHITexture>& depthTarget)
    {
        ID3D11RenderTargetView* renderTargetView = nullptr;
        ID3D11DepthStencilView* depthStencilView = nullptr;

        if (colorTarget)
        {
            const auto& texture = static_cast<const D3D11Texture&>(*colorTarget);
            renderTargetView = texture.GetRenderTargetView();
        }

        if (depthTarget)
        {
            const auto& texture = static_cast<const D3D11Texture&>(*depthTarget);
            depthStencilView = texture.GetDepthStencilView();
        }

        SetRenderTargets(renderTargetView, depthStencilView);
    }

    void D3D11Context::SetViewport(const RHI::ViewportDesc& viewport)
    {
        D3D11_VIEWPORT nativeViewport = {};
        nativeViewport.TopLeftX = viewport.x;
        nativeViewport.TopLeftY = viewport.y;
        nativeViewport.Width = viewport.width;
        nativeViewport.Height = viewport.height;
        nativeViewport.MinDepth = viewport.minDepth;
        nativeViewport.MaxDepth = viewport.maxDepth;
        m_context->RSSetViewports(1, &nativeViewport);
    }

    void D3D11Context::SetPrimitiveTopology(RHI::PrimitiveTopology topology)
    {
        D3D11_PRIMITIVE_TOPOLOGY nativeTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        if (topology == RHI::PrimitiveTopology::LineList)
        {
            nativeTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        }

        m_context->IASetPrimitiveTopology(nativeTopology);
    }

    void D3D11Context::SetVertexBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer)
    {
        if (!buffer || buffer->GetType() != RHI::BufferType::Vertex)
        {
            return;
        }

        const auto& d3dBuffer = static_cast<const D3D11Buffer&>(*buffer);
        ID3D11Buffer* nativeBuffer = d3dBuffer.GetNativeBuffer();
        UINT stride = d3dBuffer.GetStride();
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &nativeBuffer, &stride, &offset);
    }

    void D3D11Context::SetIndexBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer)
    {
        if (!buffer || buffer->GetType() != RHI::BufferType::Index)
        {
            return;
        }

        const auto& d3dBuffer = static_cast<const D3D11Buffer&>(*buffer);
        m_context->IASetIndexBuffer(d3dBuffer.GetNativeBuffer(), DXGI_FORMAT_R32_UINT, 0);
    }

    void D3D11Context::SetVertexShader(const std::shared_ptr<RHI::RHIShader>& shader)
    {
        if (!shader || shader->GetStage() != RHI::ShaderStage::Vertex)
        {
            return;
        }

        const auto& d3dShader = static_cast<const D3D11Shader&>(*shader);
        m_context->IASetInputLayout(d3dShader.GetInputLayout());
        m_context->VSSetShader(d3dShader.GetVertexShader(), nullptr, 0);
    }

    void D3D11Context::SetPixelShader(const std::shared_ptr<RHI::RHIShader>& shader)
    {
        if (!shader || shader->GetStage() != RHI::ShaderStage::Pixel)
        {
            return;
        }

        const auto& d3dShader = static_cast<const D3D11Shader&>(*shader);
        m_context->PSSetShader(d3dShader.GetPixelShader(), nullptr, 0);
    }

    void D3D11Context::SetConstantBuffer(RHI::ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHI::RHIBuffer>& buffer)
    {
        if (!buffer || buffer->GetType() != RHI::BufferType::Constant)
        {
            return;
        }

        const auto& d3dBuffer = static_cast<const D3D11Buffer&>(*buffer);
        ID3D11Buffer* nativeBuffer = d3dBuffer.GetNativeBuffer();

        if (stage == RHI::ShaderStage::Vertex)
        {
            m_context->VSSetConstantBuffers(slot, 1, &nativeBuffer);
        }
        else if (stage == RHI::ShaderStage::Pixel)
        {
            m_context->PSSetConstantBuffers(slot, 1, &nativeBuffer);
        }
    }

    void D3D11Context::SetTexture(RHI::ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHI::RHITexture>& texture)
    {
        if (stage != RHI::ShaderStage::Pixel)
        {
            return;
        }

        if (!texture)
        {
            ID3D11ShaderResourceView* shaderResourceView = nullptr;
            ID3D11SamplerState* samplerState = nullptr;
            m_context->PSSetShaderResources(slot, 1, &shaderResourceView);
            m_context->PSSetSamplers(slot, 1, &samplerState);
            return;
        }

        const auto& d3dTexture = static_cast<const D3D11Texture&>(*texture);
        ID3D11ShaderResourceView* shaderResourceView = d3dTexture.GetShaderResourceView();
        ID3D11SamplerState* samplerState = d3dTexture.GetSamplerState();
        m_context->PSSetShaderResources(slot, 1, &shaderResourceView);
        m_context->PSSetSamplers(slot, 1, &samplerState);
    }

    void D3D11Context::UpdateConstantBuffer(const std::shared_ptr<RHI::RHIBuffer>& buffer, const void* data, std::uint32_t size)
    {
        if (!buffer || buffer->GetType() != RHI::BufferType::Constant || data == nullptr || size == 0)
        {
            return;
        }

        const auto& d3dBuffer = static_cast<const D3D11Buffer&>(*buffer);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        HRESULT result = m_context->Map(d3dBuffer.GetNativeBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (Failed(result))
        {
            return;
        }

        const std::uint32_t bytesToCopy = size < buffer->GetSize() ? size : buffer->GetSize();
        std::memcpy(mapped.pData, data, bytesToCopy);
        m_context->Unmap(d3dBuffer.GetNativeBuffer(), 0);
    }

    void D3D11Context::Draw(std::uint32_t vertexCount, std::uint32_t startVertex)
    {
        m_context->Draw(vertexCount, startVertex);
    }

    void D3D11Context::DrawIndexed(std::uint32_t indexCount)
    {
        m_context->DrawIndexed(indexCount, 0, 0);
    }

    void* D3D11Context::GetNativeHandleForDebugUI() const
    {
        return m_context.Get();
    }

    ID3D11DeviceContext* D3D11Context::GetNativeContext() const
    {
        return m_context.Get();
    }
}
