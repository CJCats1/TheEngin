Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 worldMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

// Simple directional light
// Multi-light (directional) setup
static const uint MAX_LIGHTS = 4;
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

// Camera parameters
cbuffer CameraBuffer : register(b4)
{
    float3 cameraPosition; float padC;
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
    float4 baseColor = tex.Sample(samp, input.uv);

    float3 N = normalize(input.normal);
    float3 V = normalize(cameraPosition - input.worldPos);

    float3 accum = float3(0,0,0);
    [unroll]
    for (uint i = 0; i < lightCount && i < MAX_LIGHTS; ++i) {
        float3 L = normalize(-lights[i].dir);
        float NdotL = saturate(dot(N, L));

        // Blinn-Phong specular
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float spec = pow(NdotH, max(1.0f, shininess));

        float3 diffuse = (lights[i].intensity * NdotL) * (lights[i].color * baseColor.rgb);
        float3 specular = (lights[i].intensity) * (specularColor * spec);
        accum += diffuse + specular;
    }

    accum += ambient * baseColor.rgb;
    return float4(accum, baseColor.a);
}


