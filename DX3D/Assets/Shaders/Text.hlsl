// Text.hlsl — simple textured quad for screen-space text (no lighting)

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer TransformBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VSInput
{
    float3 position : POSITION0; // From Mesh::CreateQuadTextured
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    o.position = mul(viewPos, projectionMatrix);
    o.uv = input.uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    // Texture is premultiplied alpha (PBGRA); your blend state should be:
    // Src = ONE, Dest = INV_SRC_ALPHA
    return tex.Sample(samp, input.uv);
}
