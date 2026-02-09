
struct VSOut
{
    float4 color : COLOR0;
    float4 R3pos : TEXCOORD0;
    float4 norm : NORMAL;
    float4 SCpos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    return float4(vso.color.rgb, 1.f);
}