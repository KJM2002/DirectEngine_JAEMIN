#include "PostProcess.h"

#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIBuffer.h"
#include "Engine/Graphics/RHI/RHIContext.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Graphics/RHI/RHIShader.h"
#include "Engine/Graphics/RHI/RHITexture.h"
#include "Engine/Math/MathTypes.h"

namespace Engine::Renderer
{
    namespace
    {
        struct PostProcessVertex
        {
            Math::Vector3 position;
            Math::Vector3 normal;
            Math::Vector4 color;
            Math::Vector2 uv;
        };

        struct PostProcessBufferData
        {
            std::int32_t grayscaleEnabled = 0;
            std::int32_t vignetteEnabled = 0;
            std::int32_t toneMappingEnabled = 0;
            std::int32_t colorAdjustEnabled = 0;
            float grayscaleIntensity = 1.0f;
            float exposure = 1.0f;
            float gamma = 2.2f;
            float vignetteStrength = 0.35f;
            float vignetteRadius = 0.85f;
            float vignetteSoftness = 0.45f;
            float brightness = 0.0f;
            float contrast = 1.0f;
            float saturation = 1.0f;
        };
    }

    bool PostProcessStack::Initialize(RHI::RHIDevice& device)
    {
        const PostProcessVertex vertices[] =
        {
            { { -1.0f, -1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { { -1.0f,  1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { {  1.0f,  1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  1.0f, -1.0f, 0.0f }, {}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
        };
        const std::uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

        m_vertexBuffer = device.CreateVertexBuffer(vertices, sizeof(vertices), sizeof(PostProcessVertex));
        m_indexBuffer = device.CreateIndexBuffer(indices, sizeof(indices), 6);
        m_constantBuffer = device.CreateConstantBuffer(sizeof(PostProcessBufferData));
        m_vertexShader = device.CreateVertexShader(L"Shaders/PostProcessVertex.hlsl", "main");
        m_pixelShader = device.CreatePixelShader(L"Shaders/PostProcessPixel.hlsl", "main");
        if (!m_vertexBuffer || !m_indexBuffer || !m_constantBuffer || !m_vertexShader || !m_pixelShader)
        {
            Core::Log::Error("Failed to initialize post process resources.");
            return false;
        }

        m_settings.enabled = false;
        m_settings.effect = PostProcessEffect::None;
        Core::Log::Info("PostProcessStack initialized with fullscreen pass resources.");
        return true;
    }

    void PostProcessStack::Shutdown()
    {
        m_pixelShader.reset();
        m_vertexShader.reset();
        m_constantBuffer.reset();
        m_indexBuffer.reset();
        m_vertexBuffer.reset();
        m_settings = {};
    }

    void PostProcessStack::SetSettings(const PostProcessSettings& settings)
    {
        m_settings = settings;
    }

    const PostProcessSettings& PostProcessStack::GetSettings() const
    {
        return m_settings;
    }

    void PostProcessStack::Apply(RHI::RHIContext& context, const std::shared_ptr<RHI::RHITexture>& sceneColor)
    {
        if (!m_settings.enabled || !m_settings.HasAnyEffectEnabled() || !sceneColor)
        {
            return;
        }

        PostProcessBufferData bufferData;
        bufferData.grayscaleEnabled = (m_settings.grayscaleEnabled || m_settings.effect == PostProcessEffect::Grayscale) ? 1 : 0;
        bufferData.vignetteEnabled = (m_settings.vignetteEnabled || m_settings.effect == PostProcessEffect::Vignette) ? 1 : 0;
        bufferData.toneMappingEnabled = (m_settings.toneMappingEnabled || m_settings.effect == PostProcessEffect::ToneMapping) ? 1 : 0;
        bufferData.colorAdjustEnabled = m_settings.colorAdjustEnabled ? 1 : 0;
        bufferData.grayscaleIntensity = m_settings.grayscaleIntensity;
        bufferData.exposure = m_settings.exposure;
        bufferData.gamma = m_settings.gamma;
        bufferData.vignetteStrength = m_settings.vignetteStrength;
        bufferData.vignetteRadius = m_settings.vignetteRadius;
        bufferData.vignetteSoftness = m_settings.vignetteSoftness;
        bufferData.brightness = m_settings.brightness;
        bufferData.contrast = m_settings.contrast;
        bufferData.saturation = m_settings.saturation;

        context.UpdateConstantBuffer(m_constantBuffer, &bufferData, sizeof(bufferData));
        context.SetConstantBuffer(RHI::ShaderStage::Pixel, 4, m_constantBuffer);
        context.SetPrimitiveTopology(RHI::PrimitiveTopology::TriangleList);
        context.SetTexture(RHI::ShaderStage::Pixel, 0, sceneColor);
        context.SetVertexBuffer(m_vertexBuffer);
        context.SetIndexBuffer(m_indexBuffer);
        context.SetVertexShader(m_vertexShader);
        context.SetPixelShader(m_pixelShader);
        context.DrawIndexed(6);
    }
}
