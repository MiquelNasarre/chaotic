#include "OITheader.hlsli"

struct Lightsource
{
    float2 intensity;
    // pad float2
    float4 color;
    float3 position;
    // pad float
};

cbuffer cBuff : register(b0)
{
    Lightsource lights[8];
};

// Texture and sampler in first register
TextureCube _texture : register(t0);
SamplerState _samp : register(s0);

struct PSOut
{
    float4 accum : SV_Target0; // premultiplied color + alpha
    float reveal : SV_Target1; // alpha for revealage product
};

struct VSOut
{
    float4 coord : TEXCOORD0;
    float4 R3pos : TEXCOORD1;
    float4 norm : NORMAL;
    float4 SCpos : SV_Position;
};

PSOut main(VSOut vso, bool front : SV_IsFrontFace)
{
    float3 pos = vso.R3pos.xyz;
    float3 norm = vso.norm.xyz;
    
    // Sample from texture
    float4 color = _texture.Sample(_samp, vso.coord.xyz);
    
    // Handle backfaces: flip normal if fragment is from back side
    if (!front)
        norm = -norm;
    
    float4 totalLight = float4(0.f, 0.f, 0.f, 0.f);
    float dist = 0.f;
    float dist2 = 0.f;
    float exposure = 0.f;
    
    // Iterate through point lights
    [unroll]
    for (int i = 0; i < 8; i++)
    {
        // Skip unused lights
        if (!lights[i].intensity.r && !lights[i].intensity.g)
            continue;
        
        // Compute distance and direct exposure to light
        dist = distance(lights[i].position, pos);
        exposure = dot(norm, lights[i].position - pos) / dist;
        
        dist2 = dist * dist;
        // Add diffuse illumination
        float light = lights[i].intensity.g / dist2;
        
        // Add direct illumination
        if (exposure > 0)
            light += lights[i].intensity.r * exposure / dist2;
        
        // Add to total light
        totalLight += light * lights[i].color;
    }
    
    // Apply lighting to base color
    float4 lit = color * totalLight;

    // Use incoming color alpha as transparency
    float alpha = saturate(color.a);

    PSOut outp;
    
    // Z dependent weight 
    float w = depth_weight(vso.SCpos);
    
    // Accumulation target: premultiplied color + alpha (C_src * C_dst, A_src + A_dst)
    outp.accum = float4(lit.rgb * alpha * w, alpha * w);
    
    // Reveal target: alpha ((1 - A_src) * (1 - A_dst))
    outp.reveal = alpha;

    return outp;
}