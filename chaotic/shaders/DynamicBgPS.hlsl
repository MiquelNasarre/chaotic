
TextureCube envMap : register(t0);
SamplerState samp : register(s0);

float4 main(float3 dir : SphereVector) : SV_Target
{
    // Get world-space view ray direction
    float3 d = normalize(dir); 
    // Return sample from cubemap
    return envMap.Sample(samp, d);
}