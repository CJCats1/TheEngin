// Volumetric Cloud Shader - Based on SebLague's Cloud Rendering Technique
// Implements ray marching through a volume to render realistic clouds

cbuffer CloudBuffer : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldMatrix;
    float3 cameraPosition;
    float time;
    float3 cloudColor;
    float cloudDensity;
    float cloudCoverage;
    float cloudSpeed;
    float3 boundsMin;
    float3 boundsMax;
    float numSteps;
    float stepSize;
    float lightIntensity;
    float3 lightDirection;
    float3 lightColor;
}

// Worley noise texture for cloud density
Texture2D<float4> worleyTexture : register(t0);
SamplerState worleySampler : register(s0);

// Depth texture for proper depth testing
Texture2D<float> depthTexture : register(t1);
SamplerState depthSampler : register(s1);

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 viewVector : TEXCOORD3;
};

// Ray-box intersection function
float2 rayBoxDst(float3 boundsMin, float3 boundsMax, float3 rayOrigin, float3 rayDir)
{
    float3 t0 = (boundsMin - rayOrigin) / rayDir;
    float3 t1 = (boundsMax - rayOrigin) / rayDir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    
    float dstA = max(max(tmin.x, tmin.y), tmin.z);
    float dstB = min(tmax.x, min(tmax.y, tmax.z));
    
    float dstToBox = max(0.0, dstA);
    float dstInsideBox = max(0.0, dstB - dstToBox);
    
    return float2(dstToBox, dstInsideBox);
}

// Sample cloud density at a world position
float sampleDensity(float3 worldPos)
{
    // Transform world position to cloud space
    float3 cloudPos = worldPos * 0.01; // Scale down for cloud detail
    
    // Add wind movement
    cloudPos.x += time * cloudSpeed * 0.1;
    cloudPos.z += time * cloudSpeed * 0.05;
    
    // Sample Worley noise for base density
    float2 noiseUV = cloudPos.xz;
    float4 worleySample = worleyTexture.SampleLevel(worleySampler, noiseUV, 0);
    float baseDensity = worleySample.r;
    
    // Add height-based density falloff
    float height = (worldPos.y - boundsMin.y) / (boundsMax.y - boundsMin.y);
    float heightFalloff = smoothstep(0.0, 0.3, height) * (1.0 - smoothstep(0.7, 1.0, height));
    
    // Combine density with coverage
    float density = baseDensity * heightFalloff * cloudCoverage;
    
    return saturate(density * cloudDensity);
}

// Calculate light transmittance through the cloud
float calculateTransmittance(float3 rayOrigin, float3 rayDir, float3 lightDir, float3 samplePos)
{
    float transmittance = 1.0;
    float lightStepSize = stepSize * 2.0; // Larger steps for light calculation
    
    // March towards light source
    float lightDistance = 0.0;
    float maxLightDistance = 50.0; // Maximum distance to sample light
    
    while (lightDistance < maxLightDistance)
    {
        float3 lightSamplePos = samplePos + lightDir * lightDistance;
        
        // Check if we're still inside the cloud bounds
        if (any(lightSamplePos < boundsMin) || any(lightSamplePos > boundsMax))
            break;
            
        float lightDensity = sampleDensity(lightSamplePos);
        transmittance *= exp(-lightDensity * lightStepSize);
        
        lightDistance += lightStepSize;
        
        // Early exit if transmittance is very low
        if (transmittance < 0.01)
            break;
    }
    
    return transmittance;
}

VSInput VSMain(VSInput input)
{
    return input;
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
    output.position = mul(mul(worldPos, viewMatrix), projectionMatrix);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.viewVector = worldPos.xyz - cameraPosition;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 rayOrigin = cameraPosition;
    float3 rayDir = normalize(input.viewVector);
    
    // Sample depth for proper depth testing
    float nonLinearDepth = depthTexture.Sample(depthSampler, input.uv);
    float depth = LinearEyeDepth(nonLinearDepth) * length(input.viewVector);
    
    // Calculate ray-box intersection
    float2 rayBoxInfo = rayBoxDst(boundsMin, boundsMax, rayOrigin, rayDir);
    float dstToBox = rayBoxInfo.x;
    float dstInsideBox = rayBoxInfo.y;
    
    // Early exit if ray doesn't intersect the cloud volume
    if (dstInsideBox <= 0.0)
        return float4(0, 0, 0, 0);
    
    // Set up ray marching parameters
    float dstTravelled = 0.0;
    float stepSize = dstInsideBox / numSteps;
    float dstLimit = min(depth - dstToBox, dstInsideBox);
    
    // Accumulate density and color
    float totalDensity = 0.0;
    float3 totalColor = float3(0, 0, 0);
    
    // March through the volume
    while (dstTravelled < dstLimit)
    {
        float3 rayPos = rayOrigin + rayDir * (dstToBox + dstTravelled);
        float density = sampleDensity(rayPos);
        
        if (density > 0.01) // Only process if there's significant density
        {
            // Calculate light transmittance
            float transmittance = calculateTransmittance(rayOrigin, rayDir, lightDirection, rayPos);
            
            // Accumulate density
            totalDensity += density * stepSize;
            
            // Calculate lighting
            float3 lighting = lightColor * transmittance * lightIntensity;
            totalColor += density * stepSize * lighting;
        }
        
        dstTravelled += stepSize;
    }
    
    // Calculate final transmittance
    float finalTransmittance = exp(-totalDensity);
    
    // Combine cloud color with lighting
    float3 cloudColor = cloudColor * totalColor;
    
    // Apply alpha based on density
    float alpha = 1.0 - finalTransmittance;
    
    return float4(cloudColor, alpha);
}
