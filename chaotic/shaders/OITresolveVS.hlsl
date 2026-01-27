
struct VSOut
{
    float4 TXpos : TexPos;
    float4 SCpos : SV_Position;
};

VSOut main(float2 pos : Position)
{
    VSOut vso;

    vso.TXpos = float4((pos.x + 1.f) / 2.f, (-pos.y + 1.f) / 2.f, 0.f, 0.5f);
    vso.SCpos = float4(pos.xy, 0.f, 1.f);
    
    return vso;
}