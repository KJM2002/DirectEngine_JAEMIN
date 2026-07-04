#include "PostProcessComponent.h"

namespace Engine::Scene
{
    Renderer::PostProcessSettings PostProcessComponent::ToSettings() const
    {
        Renderer::PostProcessSettings settings;
        settings.enabled = enabled;
        settings.effect = effect;
        settings.grayscaleEnabled = grayscaleEnabled || effect == Renderer::PostProcessEffect::Grayscale;
        settings.vignetteEnabled = vignetteEnabled || effect == Renderer::PostProcessEffect::Vignette;
        settings.toneMappingEnabled = toneMappingEnabled || effect == Renderer::PostProcessEffect::ToneMapping;
        settings.colorAdjustEnabled = colorAdjustEnabled;
        settings.grayscaleIntensity = grayscaleIntensity;
        settings.exposure = exposure;
        settings.gamma = gamma;
        settings.vignetteStrength = vignetteStrength;
        settings.vignetteRadius = vignetteRadius;
        settings.vignetteSoftness = vignetteSoftness;
        settings.brightness = brightness;
        settings.contrast = contrast;
        settings.saturation = saturation;
        return settings;
    }
}
