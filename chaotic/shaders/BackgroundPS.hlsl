
Texture2D backgrd : register(t0);
SamplerState splr : register(s0);

float4 main(float2 tex : TexCoord) : SV_Target
{
    return backgrd.Sample(splr, tex);
}