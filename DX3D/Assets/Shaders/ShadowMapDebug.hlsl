// Enhanced debug shader for visualizing shadow map depth
Texture2D shadowMap : register(t0);
SamplerState shadowSampler : register(s0);

// Debug visualization modes
cbuffer DebugParams : register(b0)
{
    int debugMode;      // 0=grayscale, 1=heatmap, 2=linearized, 3=raw
    float nearPlane;    // Near plane distance for linearization
    float farPlane;     // Far plane distance for linearization
    float padding;      // Padding for alignment
};

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
    // For screen space quad, just pass through the position
    o.position = float4(input.position, 1.0f);
    o.uv = input.uv;
    return o;
}

// Convert depth to linear depth
float LinearizeDepth(float depth, float near, float far)
{
    return (2.0f * near * far) / (far + near - depth * (far - near));
}

// Heat map color based on depth value
float3 HeatMapColor(float t)
{
    // t should be in range [0, 1]
    t = saturate(t);
    
    if (t < 0.25f)
    {
        // Blue to cyan
        float localT = t / 0.25f;
        return lerp(float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 1.0f), localT);
    }
    else if (t < 0.5f)
    {
        // Cyan to green
        float localT = (t - 0.25f) / 0.25f;
        return lerp(float3(0.0f, 1.0f, 1.0f), float3(0.0f, 1.0f, 0.0f), localT);
    }
    else if (t < 0.75f)
    {
        // Green to yellow
        float localT = (t - 0.5f) / 0.25f;
        return lerp(float3(0.0f, 1.0f, 0.0f), float3(1.0f, 1.0f, 0.0f), localT);
    }
    else
    {
        // Yellow to red
        float localT = (t - 0.75f) / 0.25f;
        return lerp(float3(1.0f, 1.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), localT);
    }
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Sample the shadow map depth
    float depth = shadowMap.Sample(shadowSampler, input.uv).r;
    
    float3 color = float3(0.0f, 0.0f, 0.0f);
    
    if (debugMode == 0) // Grayscale
    {
        // Standard grayscale visualization
        color = float3(depth, depth, depth);
    }
    else if (debugMode == 1) // Heat map
    {
        // Use heat map for better depth visualization
        color = HeatMapColor(depth);
    }
    else if (debugMode == 2) // Linearized depth
    {
        // Linearize the depth for better understanding
        float linearDepth = LinearizeDepth(depth, nearPlane, farPlane);
        // Normalize to [0, 1] range for visualization
        float normalizedDepth = saturate(linearDepth / farPlane);
        color = HeatMapColor(normalizedDepth);
    }
    else if (debugMode == 3) // Raw depth with contrast
    {
        // Apply contrast to make depth differences more visible
        float contrastDepth = pow(depth, 2.2f); // Gamma correction for better contrast
        color = float3(contrastDepth, contrastDepth, contrastDepth);
    }
    else // Default to grayscale
    {
        color = float3(depth, depth, depth);
    }
    
    return float4(color, 1.0f);
}
