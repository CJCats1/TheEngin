// Depth-only shader for shadow map generation
cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 worldMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(float4(input.position, 1.0f), worldMatrix);
    float4 vp = mul(wp, viewMatrix);
    o.position = mul(vp, projectionMatrix);
    return o;
}

// No pixel shader needed for depth-only rendering
// The depth buffer will be written automatically
