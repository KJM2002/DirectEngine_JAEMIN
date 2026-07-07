#include "SelectionOutlineRenderer.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIBuffer.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Graphics/RHI/RHITexture.h"
#include "Engine/Graphics/Renderer/Mesh.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/MeshRendererComponent.h"

#include <algorithm>

namespace Engine::Renderer
{
    bool SelectionOutlineRenderer::Initialize(RHI::RHIDevice& device, std::uint32_t width, std::uint32_t height)
    {
        m_device = &device;

        const FullscreenVertex vertices[] =
        {
            { { -1.0f, -1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, {} },
            { { -1.0f,  1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f }, {} },
            { {  1.0f,  1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f }, {} },
            { {  1.0f, -1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }, {} }
        };
        const std::uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

        m_fullscreenVertexBuffer = device.CreateVertexBuffer(vertices, sizeof(vertices), sizeof(FullscreenVertex));
        m_fullscreenIndexBuffer = device.CreateIndexBuffer(indices, sizeof(indices), 6);
        m_transformBuffer = device.CreateConstantBuffer(sizeof(TransformBufferData));
        m_compositeBuffer = device.CreateConstantBuffer(sizeof(CompositeBufferData));
        m_maskVertexShader = device.CreateVertexShader(L"Shaders/SelectionOutlineMaskVertex.hlsl", "main");
        m_maskPixelShader = device.CreatePixelShader(L"Shaders/SelectionOutlineMaskPixel.hlsl", "main");
        m_fullscreenVertexShader = device.CreateVertexShader(L"Shaders/PostProcessVertex.hlsl", "main");
        m_outlinePixelShader = device.CreatePixelShader(L"Shaders/SelectionOutlinePixel.hlsl", "main");

        if (!m_fullscreenVertexBuffer || !m_fullscreenIndexBuffer || !m_transformBuffer || !m_compositeBuffer
            || !m_maskVertexShader || !m_maskPixelShader || !m_fullscreenVertexShader || !m_outlinePixelShader)
        {
            Core::Log::Error("Failed to initialize selection outline shader resources.");
            return false;
        }

        return CreateTargets(width, height);
    }

    void SelectionOutlineRenderer::Shutdown()
    {
        m_outlinePixelShader.reset();
        m_fullscreenVertexShader.reset();
        m_maskPixelShader.reset();
        m_maskVertexShader.reset();
        m_compositeBuffer.reset();
        m_transformBuffer.reset();
        m_fullscreenIndexBuffer.reset();
        m_fullscreenVertexBuffer.reset();
        m_compositeTarget.reset();
        m_maskTarget.reset();
        m_width = 0;
        m_height = 0;
        m_device = nullptr;
    }

    bool SelectionOutlineRenderer::Resize(std::uint32_t width, std::uint32_t height)
    {
        return CreateTargets(width, height);
    }

    bool SelectionOutlineRenderer::RenderMask(
        RHI::RHIContext& context,
        const std::vector<Scene::GameObject*>& selectedObjects,
        const Scene::Camera& camera,
        float aspectRatio,
        const std::shared_ptr<RHI::RHITexture>& depthTarget)
    {
        if (!m_maskTarget || selectedObjects.empty())
        {
            return false;
        }

        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 2, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 3, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 4, nullptr);
        context.SetRenderTarget(m_maskTarget, depthTarget);
        context.ClearRenderTarget(0.0f, 0.0f, 0.0f, 0.0f);
        context.ClearDepthStencil(1.0f, 0);

        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);

        TransformBufferData transform = {};
        DirectX::XMStoreFloat4x4(&transform.view, camera.GetViewMatrix());
        DirectX::XMStoreFloat4x4(&transform.projection, camera.GetProjectionMatrix(aspectRatio));

        bool drewAnyMesh = false;
        for (const Scene::GameObject* object : selectedObjects)
        {
            if (!object)
            {
                continue;
            }

            const Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->IsEnabled() || !meshRenderer->GetMesh())
            {
                continue;
            }

            const Mesh& mesh = *meshRenderer->GetMesh();
            if (!mesh.GetVertexBuffer() || !mesh.GetIndexBuffer())
            {
                continue;
            }

            DirectX::XMStoreFloat4x4(&transform.world, object->GetTransform().GetWorldMatrix());
            context.UpdateConstantBuffer(m_transformBuffer, &transform, sizeof(transform));
            context.SetConstantBuffer(RHI::ShaderStage::Vertex, 0, m_transformBuffer);
            context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
            context.SetVertexBuffer(mesh.GetVertexBuffer());
            context.SetIndexBuffer(mesh.GetIndexBuffer());
            context.SetVertexShader(m_maskVertexShader);
            context.SetPixelShader(m_maskPixelShader);
            context.DrawIndexed(mesh.GetIndexCount());
            drewAnyMesh = true;
        }

        return drewAnyMesh;
    }

    void SelectionOutlineRenderer::Composite(
        RHI::RHIContext& context,
        const std::shared_ptr<RHI::RHITexture>& sceneColor,
        const std::shared_ptr<RHI::RHITexture>& outputTarget,
        const SelectionOutlineSettings& settings,
        bool maskReady)
    {
        if (!outputTarget)
        {
            return;
        }

        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        context.SetRenderTarget(outputTarget, nullptr);
        DrawComposite(context, sceneColor, settings, maskReady);
    }

    void SelectionOutlineRenderer::CompositeToDefault(
        RHI::RHIContext& context,
        const std::shared_ptr<RHI::RHITexture>& sceneColor,
        const SelectionOutlineSettings& settings,
        bool maskReady)
    {
        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        context.SetDefaultRenderTarget();
        context.ClearDepthStencil(1.0f, 0);
        DrawComposite(context, sceneColor, settings, maskReady);
    }

    const std::shared_ptr<RHI::RHITexture>& SelectionOutlineRenderer::GetCompositeTarget() const
    {
        return m_compositeTarget;
    }

    bool SelectionOutlineRenderer::CreateTargets(std::uint32_t width, std::uint32_t height)
    {
        if (!m_device || width == 0 || height == 0)
        {
            return false;
        }

        RHI::TextureDesc maskDesc;
        maskDesc.width = width;
        maskDesc.height = height;
        maskDesc.format = RHI::TextureFormat::RGBA8;
        maskDesc.shaderResource = true;
        maskDesc.renderTarget = true;
        maskDesc.filter = RHI::TextureFilter::Point;
        maskDesc.addressMode = RHI::TextureAddressMode::Clamp;

        m_maskTarget = m_device->CreateTexture2D(maskDesc);
        m_compositeTarget = m_device->CreateRenderTarget(width, height, RHI::TextureFormat::RGBA8);
        if (!m_maskTarget || !m_compositeTarget)
        {
            Core::Log::Error("Failed to create selection outline render targets.");
            return false;
        }

        m_width = width;
        m_height = height;
        return true;
    }

    void SelectionOutlineRenderer::DrawComposite(
        RHI::RHIContext& context,
        const std::shared_ptr<RHI::RHITexture>& sceneColor,
        const SelectionOutlineSettings& settings,
        bool maskReady)
    {
        if (!sceneColor || !m_fullscreenVertexBuffer || !m_fullscreenIndexBuffer || !m_compositeBuffer)
        {
            return;
        }

        CompositeBufferData bufferData;
        bufferData.outlineColor = settings.color;
        bufferData.texelSize =
        {
            m_width > 0 ? 1.0f / static_cast<float>(m_width) : 1.0f,
            m_height > 0 ? 1.0f / static_cast<float>(m_height) : 1.0f
        };
        bufferData.outlineWidth = std::clamp(settings.width, 1.0f, 16.0f);
        bufferData.opacity = std::clamp(settings.opacity, 0.0f, 1.0f);
        bufferData.maskReady = maskReady ? 1 : 0;

        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);
        context.UpdateConstantBuffer(m_compositeBuffer, &bufferData, sizeof(bufferData));
        context.SetConstantBuffer(RHI::ShaderStage::Pixel, 5, m_compositeBuffer);
        context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
        context.SetTexture(RHI::ShaderStage::Pixel, 0, sceneColor);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, maskReady ? m_maskTarget : nullptr);
        context.SetVertexBuffer(m_fullscreenVertexBuffer);
        context.SetIndexBuffer(m_fullscreenIndexBuffer);
        context.SetVertexShader(m_fullscreenVertexShader);
        context.SetPixelShader(m_outlinePixelShader);
        context.DrawIndexed(6);
        context.SetTexture(RHI::ShaderStage::Pixel, 0, nullptr);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
    }
}
