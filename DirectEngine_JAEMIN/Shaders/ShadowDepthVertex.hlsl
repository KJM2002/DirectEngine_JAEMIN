cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Projection;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

float4 main(VSInput input) : SV_POSITION
{
    float4 worldPosition = mul(float4(input.position, 1.0f), World);
    float4 viewPosition = mul(worldPosition, View);
    return mul(viewPosition, Projection);
}
