
TextureCube envMap : register(t0);
SamplerState samp : register(s0);

struct VSOut
{
    float3 dir : TEXCOORD0;
    float4 pos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    // Get world-space view ray direction
    float3 d = normalize(vso.dir); 
    // Return sample from cubemap
    return float4(envMap.Sample(samp, d).rgb, 1.f);
}