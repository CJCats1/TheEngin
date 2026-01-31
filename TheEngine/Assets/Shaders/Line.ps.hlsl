// Line Pixel Shader
// Optimized for rendering lines with per-vertex colors
// No texture sampling - pure color output

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

float4 PSMain(PSInput input) : SV_Target
{
    // Simple pass-through of vertex color
    // No texture sampling, no complex lighting
    // Just pure color output for maximum performance
    return input.color;
}
