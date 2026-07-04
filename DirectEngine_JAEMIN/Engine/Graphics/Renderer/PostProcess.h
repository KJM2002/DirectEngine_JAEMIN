#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace Engine::RHI
{
    class RHIBuffer;
    class RHIContext;
    class RHIDevice;
    class RHIShader;
    class RHITexture;
}

namespace Engine::Renderer
{
    enum class PostProcessEffect
    {
        None,
        Grayscale,
        Vignette,
        ToneMapping
    };

    struct PostProcessSettings
    {
        bool enabled = false;
        PostProcessEffect effect = PostProcessEffect::None;
        bool grayscaleEnabled = false;
        bool vignetteEnabled = false;
        bool toneMappingEnabled = false;
        bool colorAdjustEnabled = false;
        float grayscaleIntensity = 1.0f;
        float exposure = 1.0f;
        float gamma = 2.2f;
        float vignetteStrength = 0.35f;
        float vignetteRadius = 0.85f;
        float vignetteSoftness = 0.45f;
        float brightness = 0.0f;
        float contrast = 1.0f;
        float saturation = 1.0f;

        bool HasAnyEffectEnabled() const
        {
            return grayscaleEnabled || vignetteEnabled || toneMappingEnabled || colorAdjustEnabled || effect != PostProcessEffect::None;
        }
    };

    class PostProcessStack
    {
    public:
        bool Initialize(RHI::RHIDevice& device);
        void Shutdown();
        void SetSettings(const PostProcessSettings& settings);
        const PostProcessSettings& GetSettings() const;
        void Apply(RHI::RHIContext& context, const std::shared_ptr<RHI::RHITexture>& sceneColor);

    private:
        PostProcessSettings m_settings;
        std::shared_ptr<RHI::RHIBuffer> m_vertexBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_indexBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_constantBuffer;
        std::shared_ptr<RHI::RHIShader> m_vertexShader;
        std::shared_ptr<RHI::RHIShader> m_pixelShader;
    };
}
