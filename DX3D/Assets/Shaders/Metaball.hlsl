// Metaball rendering shader for FLIP fluid simulation
// Implements the john-wigg.dev texture-based metaball approach

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

// Constant buffer
cbuffer MetaballBuffer : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix;
    float3 cameraPos;
    float time;
    float2 worldOrigin;
    float2 worldSize;
    float threshold;
    float smoothing;
};

// Metaball field texture (accumulated from all particles)
Texture2D fieldTexture : register(t0);
SamplerState fieldSampler : register(s0);

// Color gradient texture for final rendering
Texture2D gradientTexture : register(t1);
SamplerState gradientSampler : register(s1);

// Noise texture for foam effects
Texture2D noiseTexture : register(t2);
SamplerState noiseSampler : register(s2);

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    
    // Transform to world space
    float4 worldPos = mul(float4(input.position, 1.0f), viewProjMatrix);
    output.worldPos = worldPos.xyz / worldPos.w;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 screenUV = input.uv;
    float3 worldPos = input.worldPos;
    
    // Convert world position to texture coordinates
    float2 texUV = (worldPos.xy - worldOrigin) / worldSize;
    
    // Sample the accumulated metaball field
    // This contains the sum of all radial gradient textures
    float4 fieldData = fieldTexture.Sample(fieldSampler, texUV);
    float fieldValue = fieldData.r; // Use red channel as field strength
    
    // Apply threshold to create surface (john-wigg.dev approach)
    float surface = smoothstep(threshold - smoothing, threshold + smoothing, fieldValue);
    
    // Sample color gradient based on field strength
    float3 finalColor = gradientTexture.Sample(gradientSampler, float2(fieldValue, 0.0f)).rgb;
    
    // Add noise for foam effects
    float noise = noiseTexture.Sample(noiseSampler, screenUV * 4.0f + time * 0.1f).r;
    noise = noise * 0.1f; // Subtle noise
    
    // Final color with surface threshold
    finalColor += noise * surface;
    float alpha = surface;
    
    return float4(finalColor, alpha);
}

