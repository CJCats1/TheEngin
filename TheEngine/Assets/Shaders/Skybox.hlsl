// Sample from cubemap SRV produced by LoadSkyboxCubemap
TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

cbuffer TransformBuffer : register(b0)
{
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

struct VSInput { float3 position : POSITION0; };

struct VSOutput
{
    float4 position : SV_Position;
    float3 texCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    
    // Remove translation so the skybox stays centered on the camera
    float4x4 viewNoTranslation = viewMatrix;
    viewNoTranslation._41 = 0.0f;
    viewNoTranslation._42 = 0.0f;
    viewNoTranslation._43 = 0.0f;
    
    // Project cube to far plane; skybox should pass with LESS_EQUAL depth test
    float4 worldPos = float4(input.position, 1.0f);
    float4 viewPos = mul(worldPos, viewNoTranslation);
    o.position = mul(viewPos, projectionMatrix);
    o.position.z = o.position.w;
    
    // Direction for cubemap sampling in view space (rotation only)
    float3 dirVS = mul(input.position, (float3x3)viewNoTranslation);
    o.texCoord = normalize(dirVS);
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 d = normalize(input.texCoord);
    // Sample cubemap with standard orientation (no ad-hoc axis flips)
    return skyboxTexture.Sample(skyboxSampler, d);
}
