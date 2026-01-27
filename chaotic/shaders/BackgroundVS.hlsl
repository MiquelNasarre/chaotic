
cbuffer cBuff : register(b1)
{
    float4 rect;
}

struct VSOut
{
    float2 tex : TexCoord;
    float4 pos : SV_Position;
};

VSOut main(float4 pos : Position)
{
    VSOut vso;
    
    vso.tex.x = (pos.x == -1.f) ? rect.r : rect.b;
    vso.tex.y = (pos.y == +1.f) ? rect.g : rect.a;
    
    vso.pos = pos;
    return vso;
}