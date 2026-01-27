
// Texture and sampler in first register
Texture2D _texture : register(t0);
SamplerState _samp : register(s0);

float4 main(float2 coord : Coord) : SV_Target
{
    // Sample from texture
    return _texture.Sample(_samp, coord);
}