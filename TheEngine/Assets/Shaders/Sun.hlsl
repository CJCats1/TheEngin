// Sun Shader - Creates a glowing sun effect with animated surface
// Based on procedural sun rendering techniques

cbuffer SunBuffer : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldMatrix;
    float3 cameraPosition;
    float time;
    float3 sunColor;
    float sunIntensity;
    float sunRadius;
    float sunPulseSpeed;
    float sunPulseAmplitude;
    float sunSurfaceNoise;
    float sunBrightness;
    float sunGlow;
    float sunCorona;
}

Texture2D<float4> sunTexture : register(t0);
SamplerState sunSampler : register(s0);

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
    float3 viewDir : TEXCOORD3;
};

// 3D Noise function for sun surface
float3 mod289(float3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 mod289(float4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 permute(float4 x) {
    return mod289(((x*34.0)+1.0)*x);
}

float4 taylorInvSqrt(float4 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(float3 v) {
    const float2 C = float2(1.0/6.0, 1.0/3.0);
    const float4 D = float4(0.0, 0.5, 1.0, 2.0);

    // First corner
    float3 i  = floor(v + dot(v, C.yyy));
    float3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min(g.xyz, l.zxy);
    float3 i2 = max(g.xyz, l.zxy);

    // x0 = x0 - 0.0 + 0.0 * C.xxx;
    // x1 = x0 - i1  + 1.0 * C.xxx;
    // x2 = x0 - i2  + 2.0 * C.xxx;
    // x3 = x0 - 1.0 + 3.0 * C.xxx;
    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    float3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

    // Permutations
    i = mod289(i);
    float4 p = permute(permute(permute(
                i.z + float4(0.0, i1.z, i2.z, 1.0))
              + i.y + float4(0.0, i1.y, i2.y, 1.0))
              + i.x + float4(0.0, i1.x, i2.x, 1.0));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    float3 ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z);  // mod(p,7*7)

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_);    // mod(j,N)

    float4 x = x_ *ns.x + ns.yyyy;
    float4 y = y_ *ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4(x.xy, y.xy);
    float4 b1 = float4(x.zw, y.zw);

    //float4 s0 = float4(lessThan(b0,0.0))*2.0 - 1.0;
    //float4 s1 = float4(lessThan(b1,0.0))*2.0 - 1.0;
    float4 s0 = floor(b0)*2.0 + 1.0;
    float4 s1 = floor(b1)*2.0 + 1.0;
    float4 sh = -step(h, 0.0);

    float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
    float4 a1 = b1.xzyw + s1.xzyw*sh.zzww;

    float3 p0 = float3(a0.xy, h.x);
    float3 p1 = float3(a0.zw, h.y);
    float3 p2 = float3(a1.xy, h.z);
    float3 p3 = float3(a1.zw, h.w);

    //Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    float4 m = max(0.6 - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m*m, float4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

// Sun surface noise
float sunSurfaceNoise(float3 pos, float time) {
    float noise1 = snoise(pos * 2.0 + time * 0.1);
    float noise2 = snoise(pos * 4.0 + time * 0.05) * 0.5;
    float noise3 = snoise(pos * 8.0 + time * 0.02) * 0.25;
    
    return noise1 + noise2 + noise3;
}

// Sun corona effect
float sunCoronaEffect(float3 worldPos, float3 sunCenter, float radius) {
    float dist = distance(worldPos, sunCenter);
    float corona = 1.0 - saturate(dist / (radius * 3.0));
    return pow(corona, 2.0);
}

// Sun glow effect
float sunGlowEffect(float3 viewDir, float3 sunDir) {
    float dotProduct = dot(normalize(viewDir), normalize(sunDir));
    return pow(saturate(dotProduct), 8.0);
}

VSInput VSMain(VSInput input) {
    return input;
}

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
    output.position = mul(mul(worldPos, viewMatrix), projectionMatrix);
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.viewDir = normalize(cameraPosition - worldPos.xyz);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Calculate sun center (assuming sun is at origin)
    float3 sunCenter = float3(0.0, 0.0, 0.0);
    
    // Surface noise for sun texture
    float noise = sunSurfaceNoise(input.worldPos, time);
    float surfaceVariation = noise * sunSurfaceNoise;
    
    // Pulsing effect
    float pulse = 1.0 + sin(time * sunPulseSpeed) * sunPulseAmplitude;
    
    // Base sun color with surface variation
    float3 baseColor = sunColor * (1.0 + surfaceVariation * 0.3);
    
    // Brightness based on distance from center
    float distFromCenter = distance(input.worldPos, sunCenter);
    float brightness = 1.0 - saturate(distFromCenter / sunRadius);
    brightness = pow(brightness, 0.5);
    
    // Corona effect
    float corona = sunCoronaEffect(input.worldPos, sunCenter, sunRadius);
    
    // Glow effect
    float glow = sunGlowEffect(input.viewDir, normalize(sunCenter - input.worldPos));
    
    // Combine effects
    float3 finalColor = baseColor * brightness * pulse;
    finalColor += corona * sunColor * sunCorona;
    finalColor += glow * sunColor * sunGlow;
    
    // Apply intensity
    finalColor *= sunIntensity;
    
    // Add some atmospheric scattering effect
    float3 atmosphericColor = float3(1.0, 0.8, 0.6) * 0.1;
    finalColor += atmosphericColor;
    
    return float4(finalColor, 1.0);
}

