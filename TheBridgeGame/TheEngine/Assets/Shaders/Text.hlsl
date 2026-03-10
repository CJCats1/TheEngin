// Text.hlsl - Normal text shader
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
    float3 position : POSITION0;
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
    
    // Normal matrix transformation
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    o.position = mul(viewPos, projectionMatrix);
    
    o.uv = input.uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    // Sample the text texture
    float4 texColor = tex.Sample(samp, input.uv);
    
    // Return the texture color (premultiplied alpha)
    return texColor;
}