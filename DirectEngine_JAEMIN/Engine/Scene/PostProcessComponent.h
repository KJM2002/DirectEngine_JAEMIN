#pragma once

#include "Component.h"
#include "Engine/Graphics/Renderer/PostProcess.h"

namespace Engine::Scene
{
    class PostProcessComponent : public Component
    {
    public:
        bool enabled = true;
        Renderer::PostProcessEffect effect = Renderer::PostProcessEffect::None;
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

        Renderer::PostProcessSettings ToSettings() const;
    };
}
