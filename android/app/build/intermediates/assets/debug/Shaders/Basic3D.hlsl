Texture2D tex : register(t0);
SamplerState samp : register(s0);

// Shadow mapping (support up to 10 shadowed lights)
static const uint MAX_SHADOW_LIGHTS = 10;
Texture2D shadowMaps[MAX_SHADOW_LIGHTS] : register(t1);
SamplerComparisonState shadowSampler : register(s1);

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 worldMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

// Simple directional light
// Multi-light (directional) setup
static const uint MAX_LIGHTS = 10;
struct DirectionalLight { float3 dir; float intensity; float3 color; float pad0; };

cbuffer LightBuffer : register(b2)
{
    uint lightCount; float3 padA;
    DirectionalLight lights[MAX_LIGHTS];
};

// Material parameters
cbuffer MaterialBuffer : register(b3)
{
    float3 specularColor; float shininess; // shininess ~ 8..128
    float ambient; float3 padB;
};

// Optional tint (matches 2D shader usage). a = strength (0..1)
cbuffer TintBuffer : register(b1)
{
    float4 tintColor; // rgb = tint color, a = strength
};

// Camera parameters
cbuffer CameraBuffer : register(b4)
{
    float3 cameraPosition; float padC;
};

// PBR parameters / toggle
cbuffer PBRBuffer : register(b5)
{
    uint usePBR; float3 padP0;
    float3 albedo; float metallic;
    float roughness; float3 padP1;
};

// Flashlight/spotlight parameters
cbuffer SpotlightBuffer : register(b6)
{
    uint spotEnabled; float3 spotPad0;
    float3 spotPosition; float spotRange;
    float3 spotDirection; float spotInnerCos; // cos(innerAngle)
    float spotOuterCos; float3 spotColor;     // rgb color
    float spotIntensity; float3 spotPad1;
};

// Shadow mapping parameters
cbuffer ShadowBuffer : register(b7)
{
    row_major float4x4 lightViewProj[MAX_SHADOW_LIGHTS];
    uint shadowLightCount; float3 _shadowPad;
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 wp = mul(float4(input.position, 1.0f), worldMatrix);
    float4 vp = mul(wp, viewMatrix);
    o.position = mul(vp, projectionMatrix);
    // Transform normal by world (assume uniform scale for now)
    o.normal = mul(float4(input.normal, 0.0f), worldMatrix).xyz;
    o.uv = input.uv;
    o.worldPos = wp.xyz;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Sample texture
    float4 baseColor = tex.Sample(samp, input.uv);
    
    // Apply tint if any
    float3 tintedColor = lerp(baseColor.rgb, baseColor.rgb * tintColor.rgb, tintColor.a);
    
    // Normalize normal
    float3 normal = normalize(input.normal);
    
    // Calculate lighting
    float3 finalColor = float3(0, 0, 0);
    
    // Ambient lighting
    finalColor += tintedColor * ambient;
    
    // Directional lights
    for (uint i = 0; i < lightCount; i++) {
        float3 lightDir = normalize(-lights[i].dir);
        float NdotL = max(0, dot(normal, lightDir));
        
        // Shadow mapping (simplified for performance)
        float shadowFactor = 1.0f;
        // Sample shadow only if we have a corresponding shadow map/matrix
        if (i < shadowLightCount) {
            // Transform world position to light space
            float4 lightPos = mul(float4(input.worldPos, 1.0f), lightViewProj[i]);
            lightPos.xyz /= lightPos.w;
            
            // Quick bounds check
            if (abs(lightPos.x) <= 1.0f && abs(lightPos.y) <= 1.0f && lightPos.z >= 0.0f && lightPos.z <= 1.0f) {
                // Convert to shadow map UV coordinates
                float2 shadowUV = lightPos.xy * 0.5f + 0.5f;
                shadowUV.y = 1.0f - shadowUV.y; // Flip Y for DirectX
                
                // Sample shadow map with bias (increase slightly for plane to receive clean shadow)
                float shadowBias = 0.0045f;
                float shadowDepth = lightPos.z - shadowBias;
                shadowFactor = shadowMaps[i].SampleCmpLevelZero(shadowSampler, shadowUV, shadowDepth);
            }
        }
        
        // Apply shadow to lighting
        float3 lightColor = lights[i].color * lights[i].intensity * NdotL * shadowFactor;
        finalColor += tintedColor * lightColor;
        
        // Specular lighting
        float3 viewDir = normalize(cameraPosition - input.worldPos);
        float3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(0, dot(viewDir, reflectDir)), shininess);
        finalColor += specularColor * spec * lights[i].intensity * shadowFactor;
    }
    
    // PBR lighting (simplified for performance)
    if (usePBR && lightCount > 0) {
        float3 viewDir = normalize(cameraPosition - input.worldPos);
        float3 halfDir = normalize(viewDir + normalize(-lights[0].dir));
        float NdotH = max(0, dot(normal, halfDir));
        float spec = pow(NdotH, (1.0f - roughness) * 64.0f); // Reduced from 128
        finalColor += albedo * spec * metallic * 0.5f; // Reduced intensity
    }
    
    // Spotlight (simplified for performance)
    if (spotEnabled) {
        float3 spotDir = normalize(spotPosition - input.worldPos);
        float distance = length(spotPosition - input.worldPos);
        float attenuation = 1.0f / (1.0f + 0.2f * distance); // Simplified attenuation
        
        float spotFactor = dot(normalize(-spotDirection), spotDir);
        if (spotFactor > spotOuterCos) {
            float spotIntensity = smoothstep(spotOuterCos, spotInnerCos, spotFactor);
            float NdotL = max(0, dot(normal, spotDir));
            finalColor += tintedColor * spotColor * spotIntensity * NdotL * attenuation * 0.5f; // Reduced intensity
        }
    }
    
    return float4(finalColor, baseColor.a);
}


