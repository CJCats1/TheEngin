Texture2D tex : register(t0);
SamplerState samp : register(s0);

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
    output.position = float4(input.position, 1);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 texColor = tex.Sample(samp, input.uv);
    return texColor * input.color;
}