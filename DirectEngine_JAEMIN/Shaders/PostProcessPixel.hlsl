Texture2D SceneColor : register(t0);
SamplerState SceneSampler : register(s0);

cbuffer PostProcessBuffer : register(b4)
{
    int GrayscaleEnabled;
    int VignetteEnabled;
    int ToneMappingEnabled;
    int ColorAdjustEnabled;
    float GrayscaleIntensity;
    float Exposure;
    float Gamma;
    float VignetteStrength;
    float VignetteRadius;
    float VignetteSoftness;
    float Brightness;
    float Contrast;
    float Saturation;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = SceneColor.Sample(SceneSampler, input.uv);
    float3 result = color.rgb;

    if (GrayscaleEnabled != 0)
    {
        float gray = dot(result, float3(0.299f, 0.587f, 0.114f));
        result = lerp(result, float3(gray, gray, gray), saturate(GrayscaleIntensity));
    }

    if (ColorAdjustEnabled != 0)
    {
        result += Brightness;
        result = (result - 0.5f) * Contrast + 0.5f;
        float luminance = dot(result, float3(0.299f, 0.587f, 0.114f));
        result = lerp(float3(luminance, luminance, luminance), result, Saturation);
    }

    if (VignetteEnabled != 0)
    {
        float2 centered = input.uv * 2.0f - 1.0f;
        float distanceFromCenter = length(centered);
        float edge = max(0.0001f, VignetteRadius);
        float softness = max(0.0001f, VignetteSoftness);
        float vignette = 1.0f - smoothstep(edge, edge + softness, distanceFromCenter) * VignetteStrength;
        vignette = saturate(vignette);
        result *= vignette;
    }

    if (ToneMappingEnabled != 0)
    {
        result *= Exposure;
        result = result / (result + float3(1.0f, 1.0f, 1.0f));
        result = pow(saturate(result), 1.0f / max(0.0001f, Gamma));
    }

    return float4(result, color.a);
}
