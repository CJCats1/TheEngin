// Compute shader for metaball field calculation
// Calculates the metaball field at each pixel and stores it in a texture

struct MetaballData
{
    float3 position;
    float radius;
    float4 color;
};

// Structured buffer containing all metaball data
StructuredBuffer<MetaballData> metaballBuffer : register(t0);

// Output texture for the metaball field
RWTexture2D<float4> fieldTexture : register(u0);

// Constant buffer
cbuffer MetaballConstants : register(b0)
{
    float2 textureSize;
    float2 worldOrigin;
    float2 worldSize;
    int metaballCount;
    float threshold;
    float smoothing;
};

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    // Check bounds
    if (id.x >= (uint)textureSize.x || id.y >= (uint)textureSize.y)
        return;
    
    // Convert pixel coordinates to world coordinates
    float2 uv = (float2(id.xy) + 0.5f) / textureSize;
    float2 worldPos = worldOrigin + uv * worldSize;
    
    // Calculate metaball field
    float field = 0.0f;
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    for (int i = 0; i < metaballCount; ++i)
    {
        MetaballData ball = metaballBuffer[i];
        
        // Calculate distance to metaball center
        float2 toBall = worldPos.xy - ball.position.xy;
        float dist = length(toBall);
        
        // Metaball influence function
        float influence = 0.0f;
        if (dist < ball.radius)
        {
            float t = dist / ball.radius;
            // Smooth falloff function
            influence = 1.0f - (3.0f * t * t - 2.0f * t * t * t);
        }
        
        field += influence;
        
        // Blend colors based on influence
        if (influence > 0.0f)
        {
            color += ball.color * influence;
            totalWeight += influence;
        }
    }
    
    // Normalize color
    if (totalWeight > 0.0f)
    {
        color /= totalWeight;
    }
    
    // Apply threshold to create surface
    float surface = smoothstep(threshold - smoothing, threshold + smoothing, field);
    
    // Store result
    fieldTexture[id.xy] = float4(color.rgb, surface);
}
