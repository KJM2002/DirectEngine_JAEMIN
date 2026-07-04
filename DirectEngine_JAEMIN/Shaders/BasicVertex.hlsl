cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
};

cbuffer ShadowBuffer : register(b3)
{
    row_major float4x4 LightView;
    row_major float4x4 LightProjection;
    float4 ShadowParams;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float4 shadowPosition : TEXCOORD2;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), World);
    float4 viewPosition = mul(worldPosition, View);
    float4 lightViewPosition = mul(worldPosition, LightView);
    output.position = mul(viewPosition, Projection);
    output.shadowPosition = mul(lightViewPosition, LightProjection);
    output.normal = normalize(mul(float4(input.normal, 0.0f), World).xyz);
    output.worldPosition = worldPosition.xyz;
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
