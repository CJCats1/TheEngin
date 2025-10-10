// Simple shadow map debug shader
Texture2D shadowMap : register(t0);
SamplerState shadowSampler : register(s0);

struct VSInput
{
    float3 position : POSITION0;
    float2 uv       : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.position = float4(input.position, 1.0f);
    o.uv = input.uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Sample the shadow map depth
    float depth = shadowMap.Sample(shadowSampler, input.uv).r;
    
    // Visualize depth as grayscale
    return float4(depth, depth, depth, 1.0f);
}
