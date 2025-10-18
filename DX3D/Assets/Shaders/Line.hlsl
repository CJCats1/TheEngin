// Combined Line Shader (Vertex + Pixel)
// Optimized for rendering lines with per-vertex colors
// Features:
// - Per-vertex colors
// - Global tint support
// - No texture sampling (pure color output)
// - Optimized for line rendering performance

cbuffer TransformBuffer : register(b0)
{
    row_major matrix viewMatrix;
    row_major matrix projectionMatrix;
};

// Global tint for all lines (usually set to white for no tinting)
cbuffer TintBuffer : register(b1)
{
    float4 tintColor = float4(1, 1, 1, 1);
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;    // Not used for lines, but kept for compatibility
    float2 uv : TEXCOORD0;      // Not used for lines, but kept for compatibility
    float4 color : COLOR0;      // Per-vertex line color
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    // Transform position through view and projection matrices
    float4 worldPos = float4(input.position, 1.0f);
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projectionMatrix);
    
    // Pass through vertex color with global tint applied
    output.color = input.color * tintColor;
    
    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Simple pass-through of vertex color
    // No texture sampling, no complex lighting
    // Just pure color output for maximum performance
    
    // Future enhancements could include:
    // - Anti-aliasing for smoother lines
    // - Distance-based alpha for fade effects
    // - Custom line styles (dashed, dotted, etc.)
    
    return input.color;
}
