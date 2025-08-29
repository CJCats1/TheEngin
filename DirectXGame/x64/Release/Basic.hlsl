Texture2D tex : register(t0);
SamplerState samp : register(s0);

// Transform matrices
cbuffer TransformBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// Tint color: rgb = color, a = strength (0 = no tint, 1 = full tint)
cbuffer TintBuffer : register(b1)
{
    float4 tintColor = float4(1, 1, 1, 0); // default: no tint
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

    // Transform vertex position
    float4 worldPos = mul(worldMatrix, float4(input.position, 1.0f));
    float4 viewPos = mul(viewMatrix, worldPos);
    output.position = mul(projectionMatrix, viewPos);

    output.uv = input.uv;
    output.color = input.color;

    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Sample the texture
    float4 texColor = tex.Sample(samp, input.uv);
    
    // Branchless tinting - when tintColor.a = 0, this becomes texColor.rgb
    float3 blendedRGB = lerp(texColor.rgb, tintColor.rgb, tintColor.a);
    
    // Keep texture alpha
    float finalAlpha = texColor.a;
    
    // Multiply by vertex color
    return float4(blendedRGB, finalAlpha) * input.color;
}