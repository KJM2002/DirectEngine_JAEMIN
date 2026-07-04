#pragma once

#include "RHICommon.h"

#include <memory>
#include <string>

namespace Engine::Resource
{
    struct ImageData;
}

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIContext;
    class RHIShader;
    class RHISwapChain;
    class RHITexture;

    // Creates GPU-facing resources through an API-neutral contract.
    class RHIDevice
    {
    public:
        virtual ~RHIDevice() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;
        virtual RHIContext& GetContext() = 0;
        virtual std::shared_ptr<RHIBuffer> CreateBuffer(const BufferDesc& desc, const void* initialData) = 0;
        virtual std::shared_ptr<RHIBuffer> CreateVertexBuffer(const void* data, std::uint32_t size, std::uint32_t stride) = 0;
        virtual std::shared_ptr<RHIBuffer> CreateIndexBuffer(const void* data, std::uint32_t size, std::uint32_t indexCount) = 0;
        virtual std::shared_ptr<RHIBuffer> CreateConstantBuffer(std::uint32_t size) = 0;
        virtual std::shared_ptr<RHIShader> CreateShader(const ShaderDesc& desc) = 0;
        virtual std::shared_ptr<RHIShader> CreateVertexShader(const std::wstring& filePath, const std::string& entryPoint) = 0;
        virtual std::shared_ptr<RHIShader> CreatePixelShader(const std::wstring& filePath, const std::string& entryPoint) = 0;
        virtual std::shared_ptr<RHITexture> CreateTexture2D(const TextureDesc& desc) = 0;
        virtual std::shared_ptr<RHITexture> CreateTexture2D(const Resource::ImageData& image) = 0;
        virtual std::shared_ptr<RHITexture> CreateRenderTarget(std::uint32_t width, std::uint32_t height, TextureFormat format) = 0;
        virtual std::shared_ptr<RHITexture> CreateDepthTarget(std::uint32_t width, std::uint32_t height, TextureFormat format, bool shaderResource) = 0;
        virtual std::unique_ptr<RHISwapChain> CreateSwapChain(const SwapChainDesc& desc) = 0;
        virtual void* GetNativeHandleForDebugUI() const { return nullptr; }
    };
}
