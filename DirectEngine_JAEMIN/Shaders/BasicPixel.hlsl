struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float4 shadowPosition : TEXCOORD2;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer MaterialBuffer : register(b1)
{
    float4 BaseColor;
    int UseBaseColorTexture;
    int UseRoughnessTexture;
    int UseMetallicTexture;
    int UseNormalTexture;
    float Roughness;
    float Metallic;
    float MaterialPadding0;
    float MaterialPadding1;
};

cbuffer LightBuffer : register(b2)
{
    float4 Lights[64];
    float4 AmbientColor;
    uint LightCount;
    float3 LightPadding;
};

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);
Texture2D ShadowMapTexture : register(t1);
SamplerComparisonState ShadowSampler : register(s1);
Texture2D RoughnessTexture : register(t2);
Texture2D MetallicTexture : register(t3);
Texture2D NormalTexture : register(t4);

cbuffer ShadowBuffer : register(b3)
{
    row_major float4x4 LightView;
    row_major float4x4 LightProjection;
    float4 ShadowParams;
};

float CalculateShadow(float4 shadowPosition)
{
    if (ShadowParams.x < 0.5f)
    {
        return 1.0f;
    }

    float3 projected = shadowPosition.xyz / shadowPosition.w;
    float2 uv = projected.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return 1.0f;
    }

    float depth = projected.z - ShadowParams.y;
    return ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, uv, depth);
}

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lighting = AmbientColor.rgb;
    float shadow = CalculateShadow(input.shadowPosition);

    [loop]
    for (uint i = 0; i < LightCount; ++i)
    {
        uint baseIndex = i * 4;
        float4 positionRange = Lights[baseIndex + 0];
        float4 directionType = Lights[baseIndex + 1];
        float4 colorIntensity = Lights[baseIndex + 2];
        float4 spotAngles = Lights[baseIndex + 3];

        float3 lightColor = colorIntensity.rgb * colorIntensity.a;
        float lightType = directionType.w;
        float3 lightDirection = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (lightType < 0.5f)
        {
            lightDirection = normalize(-directionType.xyz);
        }
        else
        {
            float3 toLight = positionRange.xyz - input.worldPosition;
            float distanceToLight = length(toLight);
            lightDirection = distanceToLight > 0.0001f ? toLight / distanceToLight : float3(0.0f, 1.0f, 0.0f);
            attenuation = saturate(1.0f - distanceToLight / max(positionRange.w, 0.0001f));
            attenuation *= attenuation;

            if (lightType > 1.5f)
            {
                float3 spotForward = normalize(directionType.xyz);
                float spotDot = dot(normalize(input.worldPosition - positionRange.xyz), spotForward);
                float cone = saturate((spotDot - spotAngles.y) / max(spotAngles.x - spotAngles.y, 0.0001f));
                attenuation *= cone;
            }
        }

        float diffuseAmount = saturate(dot(normal, lightDirection));
        lighting += lightColor * diffuseAmount * attenuation * shadow;
    }

    float roughness = UseRoughnessTexture != 0 ? RoughnessTexture.Sample(DiffuseSampler, input.uv).r : Roughness;
    float metallic = UseMetallicTexture != 0 ? MetallicTexture.Sample(DiffuseSampler, input.uv).r : Metallic;
    lighting *= lerp(1.15f, 0.75f, saturate(roughness));

    float4 textureColor = UseBaseColorTexture != 0 ? DiffuseTexture.Sample(DiffuseSampler, input.uv) : float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 albedo = input.color * BaseColor * textureColor;
    float3 finalColor = lerp(albedo.rgb * lighting, albedo.rgb * (lighting * 0.45f + 0.55f), saturate(metallic));
    return float4(finalColor, albedo.a);
}
