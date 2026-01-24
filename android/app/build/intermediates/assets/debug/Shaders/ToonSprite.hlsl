Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer TransformBuffer : register(b0)
{
    row_major matrix worldMatrix;
    row_major matrix viewMatrix;
    row_major matrix projectionMatrix;
};

// tintColor: keep same binding as Basic.hlsl to avoid C++ changes.
// We'll still use it for interior tint blending. Outline color is black.
cbuffer TintBuffer : register(b1)
{
    float4 tintColor = float4(1, 1, 1, 0);
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(float4(input.position, 1.0f), worldMatrix);
    float4 vp = mul(wp, viewMatrix);
    o.position = mul(vp, projectionMatrix);
    o.uv = input.uv;
    o.color = input.color;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Base sprite color with existing tint blending semantics
    float4 texColor = tex.Sample(samp, input.uv);
    float3 baseRgb = lerp(texColor.rgb, tintColor.rgb, saturate(tintColor.a));
    float baseAlpha = texColor.a;

    // Edge detection around alpha transitions
    uint w, h;
    tex.GetDimensions(w, h);
    float2 texel = 1.0 / float2(max(1.0, (float)w), max(1.0, (float)h));

    // Sample 8-neighborhood
    float aC = baseAlpha;
    float a1 = tex.Sample(samp, input.uv + float2( texel.x, 0)).a;
    float a2 = tex.Sample(samp, input.uv + float2(-texel.x, 0)).a;
    float a3 = tex.Sample(samp, input.uv + float2(0,  texel.y)).a;
    float a4 = tex.Sample(samp, input.uv + float2(0, -texel.y)).a;
    float a5 = tex.Sample(samp, input.uv + texel).a;
    float a6 = tex.Sample(samp, input.uv - texel).a;
    float a7 = tex.Sample(samp, input.uv + float2(texel.x, -texel.y)).a;
    float a8 = tex.Sample(samp, input.uv + float2(-texel.x, texel.y)).a;

    // Edge mask: pixel near boundary where any neighbor differs substantially
    float neighborMax = max(max(max(a1, a2), max(a3, a4)), max(max(a5, a6), max(a7, a8)));
    float neighborMin = min(min(min(a1, a2), min(a3, a4)), min(min(a5, a6), min(a7, a8)));

    // Consider an edge if alpha transitions across threshold in the neighborhood
    float thresh = 0.5;
    float isInside = aC > thresh ? 1.0 : 0.0;
    float neighborAcross = (neighborMax > thresh && neighborMin < thresh) ? 1.0 : 0.0;

    // Build soft outline mask around edges (also capture just-outside edge)
    // Outside edge: current alpha < thresh but any neighbor > thresh
    float outsideEdge = (aC <= thresh && neighborMax > thresh) ? 1.0 : 0.0;
    // Inside edge: current alpha > thresh and any neighbor < thresh
    float insideEdge  = (aC >  thresh && neighborMin < thresh) ? 1.0 : 0.0;
    float outlineMask = saturate(max(outsideEdge, insideEdge) * neighborAcross);

    // Expand outline slightly by sampling a larger ring
    float2 texel2 = texel * 2.0;
    float aFar = max(
        max(tex.Sample(samp, input.uv + float2( texel2.x, 0)).a,
            tex.Sample(samp, input.uv + float2(-texel2.x, 0)).a),
        max(tex.Sample(samp, input.uv + float2(0,  texel2.y)).a,
            tex.Sample(samp, input.uv + float2(0, -texel2.y)).a));
    float outsideEdge2 = (aC <= thresh && aFar > thresh) ? 1.0 : 0.0;
    outlineMask = saturate(max(outlineMask, outsideEdge2 * 0.6));

    // Composite: draw outline behind base sprite
    float3 outlineColor = float3(0.05, 0.05, 0.05); // near black

    // Blend outline where base alpha is low; preserve interior color
    float3 colorOut = baseRgb;
    float  alphaOut = baseAlpha;
    // Where base is transparent but near edge, show solid outline
    colorOut = lerp(colorOut, outlineColor, outlineMask * (1.0 - baseAlpha));
    alphaOut = max(alphaOut, outlineMask * 1.0);

    return float4(colorOut, alphaOut) * input.color;
}


