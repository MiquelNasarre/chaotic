
Texture2D backgrd : register(t0);
SamplerState splr : register(s0);

struct VSOut
{
    float2 tex : TEXCOORD0;
    float4 pos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    return backgrd.Sample(splr, vso.tex);
}