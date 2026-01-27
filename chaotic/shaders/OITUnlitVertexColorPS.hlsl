#include "OITheader.hlsli"

struct PSOut
{
    float4 accum : SV_Target0; // premultiplied color + alpha
    float reveal : SV_Target1; // alpha for revealage product
};

PSOut main(float4 color : Color, float4 scPos : SV_Position)
{
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