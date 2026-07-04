#pragma once

#include "D3D11Common.h"
#include "D3D11Context.h"
#include "Engine/Graphics/RHI/RHIDevice.h"

namespace Engine::Graphics::D3D11
{
    class D3D11Device final : public RHI::RHIDevice
    {
    public:
        D3D11Device();
        ~D3D11Device() override;

        bool Initialize() override;
        void Shutdown() override;
        D3D11Context& GetContext() override;

        std::shared_ptr<RHI::RHIBuffer> CreateBuffer(const RHI::BufferDesc& desc, const void* initialData) override;
        std::shared_ptr<RHI::RHIBuffer> CreateVertexBuffer(const void* data, std::uint32_t size, std::uint32_t stride) override;
        std::shared_ptr<RHI::RHIBuffer> CreateIndexBuffer(const void* data, std::uint32_t size, std::uint32_t indexCount) override;
        std::shared_ptr<RHI::RHIBuffer> CreateConstantBuffer(std::uint32_t size) override;
        std::shared_ptr<RHI::RHIShader> CreateShader(const RHI::ShaderDesc& desc) override;
        std::shared_ptr<RHI::RHIShader> CreateVertexShader(const std::wstring& filePath, const std::string& entryPoint) override;
        std::shared_ptr<RHI::RHIShader> CreatePixelShader(const std::wstring& filePath, const std::string& entryPoint) override;
        std::shared_ptr<RHI::RHITexture> CreateTexture2D(const RHI::TextureDesc& desc) override;
        std::shared_ptr<RHI::RHITexture> CreateTexture2D(const Resource::ImageData& image) override;
        std::shared_ptr<RHI::RHITexture> CreateRenderTarget(std::uint32_t width, std::uint32_t height, RHI::TextureFormat format) override;
        std::shared_ptr<RHI::RHITexture> CreateDepthTarget(std::uint32_t width, std::uint32_t height, RHI::TextureFormat format, bool shaderResource) override;
        std::unique_ptr<RHI::RHISwapChain> CreateSwapChain(const RHI::SwapChainDesc& desc) override;
        void* GetNativeHandleForDebugUI() const override;

    private:
        ComPtr<IDXGIFactory> CreateDXGIFactoryFromDevice() const;

        ComPtr<ID3D11Device> m_device;
        std::unique_ptr<D3D11Context> m_context;
    };
}
