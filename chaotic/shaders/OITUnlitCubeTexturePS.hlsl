#include "OITheader.hlsli"

// Texture and sampler in first register
TextureCube _texture : register(t0);
SamplerState _samp : register(s0);

struct PSOut
{
    float4 accum : SV_Target0; // premultiplied color + alpha
    float reveal : SV_Target1; // alpha for revealage product
};

PSOut main(float3 coord : Coord, float4 scPos : SV_Position)
{
    // Sample from texture
    float4 color = _texture.Sample(_samp, coord);
    
    // Use incoming color alpha as transparency
    float alpha = saturate(color.a);

    PSOut outp;
    
    // Z dependent weight 
    float w = depth_weight(scPos);
    
    // Accumulation target: premultiplied color + alpha (C_src * C_dst, A_src + A_dst)
    outp.accum = float4(color.rgb * alpha * w, alpha * w);
    
    // Reveal target: alpha ((1 - A_src) * (1 - A_dst))
    outp.reveal = alpha;

    return outp;
}