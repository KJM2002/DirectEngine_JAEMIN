#pragma once

#include "D3D11Common.h"
#include "Engine/Graphics/RHI/RHISwapChain.h"

namespace Engine::Graphics::D3D11
{
    class D3D11Context;
    class D3D11Device;

    class D3D11SwapChain final : public RHI::RHISwapChain
    {
    public:
        D3D11SwapChain(ID3D11Device* device, IDXGIFactory* factory, D3D11Context* context, void* nativeWindowHandle, std::uint32_t width, std::uint32_t height);
        ~D3D11SwapChain() override = default;

        bool Initialize(void* nativeWindowHandle, std::uint32_t width, std::uint32_t height) override;
        void Shutdown() override;
        void Present() override;
        void Resize(std::uint32_t width, std::uint32_t height) override;

    private:
        friend class D3D11Device;

        ID3D11RenderTargetView* GetRenderTargetView() const;
        ID3D11DepthStencilView* GetDepthStencilView() const;
        bool CreateBackBufferViews(std::uint32_t width, std::uint32_t height);

        ComPtr<ID3D11Device> m_device;
        ComPtr<IDXGIFactory> m_factory;
        ComPtr<IDXGISwapChain> m_swapChain;
        D3D11Context* m_context = nullptr;
        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11Texture2D> m_depthStencilTexture;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        bool m_enableVSync = true;
    };
}
