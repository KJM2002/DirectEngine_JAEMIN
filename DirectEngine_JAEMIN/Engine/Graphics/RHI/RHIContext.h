#pragma once

#include "RHICommon.h"

#include <cstdint>
#include <memory>

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIShader;
    class RHITexture;

    // Records or submits draw commands depending on the active graphics backend.
    class RHIContext
    {
    public:
        virtual ~RHIContext() = default;

        virtual void ClearRenderTarget(float red, float green, float blue, float alpha) = 0;
        virtual void ClearDepthStencil(float depth, std::uint8_t stencil) = 0;
        virtual void SetDefaultRenderTarget() = 0;
        virtual void SetRenderTarget(const std::shared_ptr<RHITexture>& colorTarget, const std::shared_ptr<RHITexture>& depthTarget) = 0;
        virtual void SetViewport(const ViewportDesc& viewport) = 0;
        virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
        virtual void SetVertexBuffer(const std::shared_ptr<RHIBuffer>& buffer) = 0;
        virtual void SetIndexBuffer(const std::shared_ptr<RHIBuffer>& buffer) = 0;
        virtual void SetVertexShader(const std::shared_ptr<RHIShader>& shader) = 0;
        virtual void SetPixelShader(const std::shared_ptr<RHIShader>& shader) = 0;
        virtual void SetConstantBuffer(ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHIBuffer>& buffer) = 0;
        virtual void SetTexture(ShaderStage stage, std::uint32_t slot, const std::shared_ptr<RHITexture>& texture) = 0;
        virtual void TransitionResource(const std::shared_ptr<RHITexture>& texture, ResourceState before, ResourceState after) { (void)texture; (void)before; (void)after; }
        virtual void UpdateConstantBuffer(const std::shared_ptr<RHIBuffer>& buffer, const void* data, std::uint32_t size) = 0;
        virtual void Draw(std::uint32_t vertexCount, std::uint32_t startVertex) = 0;
        virtual void DrawIndexed(std::uint32_t indexCount) = 0;
        virtual void* GetNativeHandleForDebugUI() const { return nullptr; }
    };
}
