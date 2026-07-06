struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
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
    float3 CameraPosition;
    float MaterialPadding2;
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
SamplerState RoughnessSampler : register(s2);
SamplerState MetallicSampler : register(s3);
SamplerState NormalSampler : register(s4);

cbuffer ShadowBuffer : register(b3)
{
    row_major float4x4 LightView;
    row_major float4x4 LightProjection;
    float4 ShadowParams;
};

static const float PI = 3.14159265f;

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

float DistributionGGX(float3 normal, float3 halfway, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = saturate(dot(normal, halfway));
    float nDotH2 = nDotH * nDotH;
    float denom = nDotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 0.000001f);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return nDotV / max(nDotV * (1.0f - k) + k, 0.000001f);
}

float GeometrySmith(float3 normal, float3 viewDirection, float3 lightDirection, float roughness)
{
    float nDotV = saturate(dot(normal, viewDirection));
    float nDotL = saturate(dot(normal, lightDirection));
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 ResolveNormal(PSInput input)
{
    float3 normal = normalize(input.normal);
    if (UseNormalTexture == 0)
    {
        return normal;
    }

    float3 tangent = normalize(input.tangent - normal * dot(input.tangent, normal));
    if (dot(tangent, tangent) < 0.0001f)
    {
        return normal;
    }

    float3 bitangent = normalize(cross(normal, tangent));
    float3 tangentNormal = NormalTexture.Sample(NormalSampler, input.uv).xyz * 2.0f - 1.0f;
    tangentNormal = normalize(tangentNormal);
    return normalize(tangentNormal.x * tangent + tangentNormal.y * bitangent + tangentNormal.z * normal);
}

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = ResolveNormal(input);
    float3 viewDirection = normalize(CameraPosition - input.worldPosition);
    float shadow = CalculateShadow(input.shadowPosition);

    float4 textureColor = UseBaseColorTexture != 0 ? DiffuseTexture.Sample(DiffuseSampler, input.uv) : float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 baseColor = input.color * BaseColor * textureColor;
    float3 albedo = saturate(baseColor.rgb);
    float roughness = saturate(UseRoughnessTexture != 0 ? RoughnessTexture.Sample(RoughnessSampler, input.uv).r : Roughness);
    float metallic = saturate(UseMetallicTexture != 0 ? MetallicTexture.Sample(MetallicSampler, input.uv).r : Metallic);
    roughness = max(roughness, 0.045f);

    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 directLighting = float3(0.0f, 0.0f, 0.0f);

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

        float3 halfway = normalize(viewDirection + lightDirection);
        float nDotL = saturate(dot(normal, lightDirection));
        float nDotV = saturate(dot(normal, viewDirection));
        float hDotV = saturate(dot(halfway, viewDirection));

        float ndf = DistributionGGX(normal, halfway, roughness);
        float geometry = GeometrySmith(normal, viewDirection, lightDirection, roughness);
        float3 fresnel = FresnelSchlick(hDotV, f0);
        float3 specular = (ndf * geometry * fresnel) / max(4.0f * nDotV * nDotL, 0.0001f);
        float3 diffuse = (1.0f - fresnel) * (1.0f - metallic) * albedo / PI;
        directLighting += (diffuse + specular) * lightColor * attenuation * nDotL * shadow;
    }

    float3 ambient = AmbientColor.rgb * albedo * (1.0f - metallic);
    float3 finalColor = ambient + directLighting;
    return float4(saturate(finalColor), baseColor.a);
}
