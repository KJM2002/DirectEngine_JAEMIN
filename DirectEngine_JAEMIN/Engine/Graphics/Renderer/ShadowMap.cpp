#include "ShadowMap.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Graphics/Renderer/Mesh.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/Scene.h"

namespace Engine::Renderer
{
    bool ShadowMap::Initialize(RHI::RHIDevice& device, std::uint32_t width, std::uint32_t height)
    {
        m_width = width;
        m_height = height;
        UpdateLightMatrices({ 0.4f, -1.0f, 0.35f });

        m_depthTexture = device.CreateDepthTarget(width, height, RHI::TextureFormat::D32, true);
        m_vertexShader = device.CreateVertexShader(L"Shaders/ShadowDepthVertex.hlsl", "main");
        m_pixelShader = device.CreatePixelShader(L"Shaders/ShadowDepthPixel.hlsl", "main");
        if (!m_depthTexture || !m_vertexShader || !m_pixelShader)
        {
            Core::Log::Error("Failed to initialize shadow map resources.");
            return false;
        }

        m_ready = true;
        Core::Log::Info("ShadowMap initialized with depth target and depth-only shaders.");
        return true;
    }

    void ShadowMap::Shutdown()
    {
        m_width = 0;
        m_height = 0;
        m_ready = false;
        m_depthTexture.reset();
        m_vertexShader.reset();
        m_pixelShader.reset();
    }

    void ShadowMap::UpdateLightMatrices(const Math::Vector3& lightDirection)
    {
        DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&lightDirection));
        DirectX::XMVECTOR eye = DirectX::XMVectorScale(direction, -8.0f);
        DirectX::XMVECTOR target = DirectX::XMVectorZero();
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, target, up);
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(12.0f, 12.0f, 0.1f, 30.0f);
        DirectX::XMStoreFloat4x4(&m_lightView, view);
        DirectX::XMStoreFloat4x4(&m_lightProjection, projection);
    }

    void ShadowMap::BeginShadowPass(RHI::RHIContext& context)
    {
        RHI::ViewportDesc viewport;
        viewport.width = static_cast<float>(m_width);
        viewport.height = static_cast<float>(m_height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        context.SetViewport(viewport);
        context.SetTexture(RHI::ShaderStage::Pixel, 1, nullptr);
        context.SetRenderTarget(nullptr, m_depthTexture);
        context.ClearDepthStencil(1.0f, 0);
    }

    void ShadowMap::EndShadowPass(RHI::RHIContext& context)
    {
        (void)context;
    }

    ShadowPassStats ShadowMap::CollectCasters(const Scene::Scene& scene) const
    {
        ShadowPassStats stats;
        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            const Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMesh())
            {
                continue;
            }

            ++stats.casterCount;
            stats.triangleCount += meshRenderer->GetMesh()->GetIndexCount() / 3;
        }
        return stats;
    }

    std::uint32_t ShadowMap::GetWidth() const
    {
        return m_width;
    }

    std::uint32_t ShadowMap::GetHeight() const
    {
        return m_height;
    }

    bool ShadowMap::IsReady() const
    {
        return m_ready;
    }

    const std::shared_ptr<RHI::RHITexture>& ShadowMap::GetDepthTexture() const
    {
        return m_depthTexture;
    }

    const std::shared_ptr<RHI::RHIShader>& ShadowMap::GetVertexShader() const
    {
        return m_vertexShader;
    }

    const std::shared_ptr<RHI::RHIShader>& ShadowMap::GetPixelShader() const
    {
        return m_pixelShader;
    }

    const Math::Matrix4x4& ShadowMap::GetLightView() const
    {
        return m_lightView;
    }

    const Math::Matrix4x4& ShadowMap::GetLightProjection() const
    {
        return m_lightProjection;
    }
}
