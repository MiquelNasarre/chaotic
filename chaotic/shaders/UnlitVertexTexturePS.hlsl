
// Texture and sampler in first register
Texture2D _texture : register(t0);
SamplerState _samp : register(s0);

struct VSOut
{
    float4 coord : TEXCOORD0;
    float4 R3pos : TEXCOORD1;
    float4 norm : NORMAL;
    float4 SCpos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    // Sample from texture
    return _texture.Sample(_samp, vso.coord.xy);
}