Texture2D SceneColor : register(t0);
Texture2D SelectionMask : register(t1);
SamplerState SceneSampler : register(s0);
SamplerState MaskSampler : register(s1);

cbuffer SelectionOutlineBuffer : register(b5)
{
    float4 OutlineColor;
    float2 TexelSize;
    float OutlineWidth;
    float Opacity;
    int MaskReady;
    float3 Padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float SampleMask(float2 uv)
{
    return SelectionMask.Sample(MaskSampler, uv).r;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 scene = SceneColor.Sample(SceneSampler, input.uv);
    if (MaskReady == 0)
    {
        return scene;
    }

    float center = SampleMask(input.uv);
    float neighbor = 0.0f;
    int radius = clamp((int)ceil(OutlineWidth), 1, 16);

    [unroll]
    for (int i = 1; i <= 16; ++i)
    {
        if (i <= radius)
        {
            float2 offset = TexelSize * (float)i;
            neighbor = max(neighbor, SampleMask(input.uv + float2( offset.x, 0.0f)));
            neighbor = max(neighbor, SampleMask(input.uv + float2(-offset.x, 0.0f)));
            neighbor = max(neighbor, SampleMask(input.uv + float2(0.0f,  offset.y)));
            neighbor = max(neighbor, SampleMask(input.uv + float2(0.0f, -offset.y)));
            neighbor = max(neighbor, SampleMask(input.uv + float2( offset.x,  offset.y)));
            neighbor = max(neighbor, SampleMask(input.uv + float2(-offset.x,  offset.y)));
            neighbor = max(neighbor, SampleMask(input.uv + float2( offset.x, -offset.y)));
            neighbor = max(neighbor, SampleMask(input.uv + float2(-offset.x, -offset.y)));
        }
    }

    float outline = saturate(neighbor - center);
    float alpha = outline * saturate(Opacity) * OutlineColor.a;
    float3 color = lerp(scene.rgb, OutlineColor.rgb, alpha);
    return float4(color, scene.a);
}
