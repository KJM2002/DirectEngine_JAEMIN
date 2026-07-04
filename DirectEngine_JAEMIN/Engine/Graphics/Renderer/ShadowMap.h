#pragma once

#include "Engine/Math/MathTypes.h"

#include <cstdint>
#include <memory>

namespace Engine::RHI
{
    class RHIContext;
    class RHIDevice;
    class RHIShader;
    class RHITexture;
}

namespace Engine::Scene
{
    class Scene;
}

namespace Engine::Renderer
{
    struct ShadowPassStats
    {
        std::uint32_t casterCount = 0;
        std::uint32_t triangleCount = 0;
    };

    class ShadowMap
    {
    public:
        bool Initialize(RHI::RHIDevice& device, std::uint32_t width = 2048, std::uint32_t height = 2048);
        void Shutdown();
        void UpdateLightMatrices(const Math::Vector3& lightDirection);
        void BeginShadowPass(RHI::RHIContext& context);
        void EndShadowPass(RHI::RHIContext& context);
        ShadowPassStats CollectCasters(const Scene::Scene& scene) const;

        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;
        bool IsReady() const;
        const std::shared_ptr<RHI::RHITexture>& GetDepthTexture() const;
        const std::shared_ptr<RHI::RHIShader>& GetVertexShader() const;
        const std::shared_ptr<RHI::RHIShader>& GetPixelShader() const;
        const Math::Matrix4x4& GetLightView() const;
        const Math::Matrix4x4& GetLightProjection() const;

    private:
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        bool m_ready = false;
        std::shared_ptr<RHI::RHITexture> m_depthTexture;
        std::shared_ptr<RHI::RHIShader> m_vertexShader;
        std::shared_ptr<RHI::RHIShader> m_pixelShader;
        Math::Matrix4x4 m_lightView = {};
        Math::Matrix4x4 m_lightProjection = {};
    };
}
