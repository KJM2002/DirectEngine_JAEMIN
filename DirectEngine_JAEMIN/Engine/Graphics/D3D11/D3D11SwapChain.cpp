#include "D3D11SwapChain.h"

#include "D3D11Context.h"

#include <Windows.h>

namespace Engine::Graphics::D3D11
{
    D3D11SwapChain::D3D11SwapChain(ID3D11Device* device, IDXGIFactory* factory, D3D11Context* context, void* nativeWindowHandle, std::uint32_t width, std::uint32_t height)
        : m_device(device)
        , m_factory(factory)
        , m_context(context)
    {
        Initialize(nativeWindowHandle, width, height);
    }

    bool D3D11SwapChain::Initialize(void* nativeWindowHandle, std::uint32_t width, std::uint32_t height)
    {
        if (m_device == nullptr || m_factory == nullptr || nativeWindowHandle == nullptr)
        {
            return false;
        }

        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferCount = 1;
        desc.BufferDesc.Width = width;
        desc.BufferDesc.Height = height;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = static_cast<HWND>(nativeWindowHandle);
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        HRESULT result = m_factory->CreateSwapChain(m_device.Get(), &desc, m_swapChain.GetAddressOf());
        if (Failed(result))
        {
            return false;
        }

        return CreateBackBufferViews(width, height);
    }

    void D3D11SwapChain::Shutdown()
    {
        m_depthStencilView.Reset();
        m_depthStencilTexture.Reset();
        m_renderTargetView.Reset();
        m_swapChain.Reset();
    }

    void D3D11SwapChain::Present()
    {
        if (m_swapChain)
        {
            m_swapChain->Present(m_enableVSync ? 1u : 0u, 0);
        }
    }

    void D3D11SwapChain::Resize(std::uint32_t width, std::uint32_t height)
    {
        if (!m_swapChain || width == 0 || height == 0)
        {
            return;
        }

        if (m_context)
        {
            m_context->SetDefaultRenderTargets(nullptr, nullptr);
        }

        m_depthStencilView.Reset();
        m_depthStencilTexture.Reset();
        m_renderTargetView.Reset();

        HRESULT result = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (Failed(result))
        {
            return;
        }

        if (CreateBackBufferViews(width, height) && m_context)
        {
            m_context->SetDefaultRenderTargets(m_renderTargetView.Get(), m_depthStencilView.Get());
        }
    }

    ID3D11RenderTargetView* D3D11SwapChain::GetRenderTargetView() const
    {
        return m_renderTargetView.Get();
    }

    ID3D11DepthStencilView* D3D11SwapChain::GetDepthStencilView() const
    {
        return m_depthStencilView.Get();
    }

    bool D3D11SwapChain::CreateBackBufferViews(std::uint32_t width, std::uint32_t height)
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
        if (Failed(result))
        {
            return false;
        }

        result = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
        if (Failed(result))
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = width;
        depthDesc.Height = height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        result = m_device->CreateTexture2D(&depthDesc, nullptr, m_depthStencilTexture.GetAddressOf());
        if (Failed(result))
        {
            return false;
        }

        result = m_device->CreateDepthStencilView(m_depthStencilTexture.Get(), nullptr, m_depthStencilView.GetAddressOf());
        return Succeeded(result);
    }
}
