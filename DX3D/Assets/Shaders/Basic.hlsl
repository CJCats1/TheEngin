Texture2D tex : register(t0);
SamplerState samp : register(s0);

// Use row_major so you don't have to transpose on CPU if you already fill row-major.
cbuffer TransformBuffer : register(b0)
{
    row_major matrix worldMatrix; // from TransformComponent::getWorldMatrix2D()
    row_major matrix viewMatrix; // identity
    row_major matrix projectionMatrix; // orthographic pixels->clip, see C++ below
};

// tint: rgb color, a = strength (0..1)
cbuffer TintBuffer : register(b1)
{
    float4 tintColor = float4(1, 1, 1, 0);
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(float4(input.position, 1.0f), worldMatrix); // v * M with row_major
    float4 vp = mul(wp, viewMatrix); // (identity)
    o.position = mul(vp, projectionMatrix); // orthographic pixels->clip
    o.uv = input.uv;
    o.color = input.color;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 texColor = tex.Sample(samp, input.uv);
    float3 blended = lerp(texColor.rgb, tintColor.rgb, tintColor.a);
    return float4(blended, texColor.a) * input.color;
}
