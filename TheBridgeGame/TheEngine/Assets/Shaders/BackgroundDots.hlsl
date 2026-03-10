cbuffer DotParams : register(b0)
{
    float2 viewportSize;   // (width, height)
    float  dotSpacing;     // pixels between dot centers
    float  dotRadius;      // radius in pixels
    float4 baseColor;      // background color
    float4 dotColor;       // dot color
};

struct VSOut
{
    float4 position : SV_Position;
};

// Fullscreen triangle via SV_VertexID (no vertex buffer needed from our engine, but
// we will still feed a quad; the PS only relies on SV_Position).
VSOut VSMain(uint vertexId : SV_VertexID)
{
    float2 pos;
    // Fullscreen triangle covering NDC
    pos = float2((vertexId == 2) ? 3.0f : -1.0f,
                 (vertexId == 1) ? 3.0f : -1.0f);

    VSOut o;
    o.position = float4(pos, 0.0f, 1.0f);
    return o;
}

float sdCircle(float2 p, float r)
{
    return length(p) - r;
}

float4 PSMain(VSOut input) : SV_Target
{
    // Convert to pixel coordinates. SV_Position.xy is already in pixel space
    float2 pix = input.position.xy;

    // Find the nearest grid cell center
    float2 cell = floor(pix / dotSpacing) * dotSpacing + dotSpacing * 0.5f;
    float2 local = pix - cell;

    // Distance from center
    float d = sdCircle(local, dotRadius);

    // Smooth edge for subtle antialiasing
    float edge = fwidth(d) * 1.5f;
    float mask = 1.0f - saturate(smoothstep(0.0f, edge, d));

    float3 color = lerp(baseColor.rgb, dotColor.rgb, mask * dotColor.a);
    return float4(color, 1.0f);
}


