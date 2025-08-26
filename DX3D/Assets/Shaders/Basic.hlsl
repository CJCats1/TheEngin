Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer TransformBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer TintBuffer : register(b1) // use b1 since b0 is your transform buffer
{
    float4 tintColor = float4(1, 1, 1, 1); // default: no tint
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
    VSOutput output;
    
    // Column-major multiplication: mul(matrix, vector)
    float4 worldPos = mul(worldMatrix, float4(input.position, 1.0f));
    float4 viewPos = mul(viewMatrix, worldPos);
    output.position = mul(projectionMatrix, viewPos);
    
    output.uv = input.uv;
    output.color = input.color;
    
    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 texColor = tex.Sample(samp, input.uv);
    return texColor * input.color * tintColor;
}