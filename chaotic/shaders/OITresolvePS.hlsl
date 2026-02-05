
Texture2D OITAccum : register(t0);
Texture2D OITReveal : register(t1);
SamplerState Nearest : register(s0);

struct VSOut
{
    float4 TXpos : TEXCOORD0;
    float4 SCpos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    float2 pos = vso.TXpos.xy;
    
    float4 accum = OITAccum.Sample(Nearest, pos); // (rgbAccum, alphaAccum)
    float reveal = OITReveal.Sample(Nearest, pos).r;
    
    // Derive final transparent coverage
    float alpha = 1.0f - reveal;

    // Reconstruct color from premultiplied accum
    float3 color = accum.rgb / accum.a;

    return float4(color, alpha);
}