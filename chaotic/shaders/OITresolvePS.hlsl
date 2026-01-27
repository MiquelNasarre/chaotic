
Texture2D OITAccum : register(t0);
Texture2D OITReveal : register(t1);
SamplerState Nearest : register(s0);

float4 main(float2 pos : TexPos) : SV_Target
{
    float4 accum = OITAccum.Sample(Nearest, pos); // (rgbAccum, alphaAccum)
    float reveal = OITReveal.Sample(Nearest, pos).r;
    
    // Derive final transparent coverage
    float alpha = 1.0f - reveal;

    // Reconstruct color from premultiplied accum
    float3 color = accum.rgb / accum.a;

    return float4(color, alpha);
}