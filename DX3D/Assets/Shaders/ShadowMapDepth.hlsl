// Depth-only pixel shader for shadow map generation
// This shader outputs nothing (depth-only rendering)

struct VSOutput
{
    float4 position : SV_Position;
};

// Depth-only pixel shader - no color output needed
void PSMain(VSOutput input)
{
    // No pixel shader output needed for depth-only rendering
    // The depth buffer will be written automatically
}

// No pixel shader needed for depth-only rendering
// The depth buffer will automatically capture the depth values