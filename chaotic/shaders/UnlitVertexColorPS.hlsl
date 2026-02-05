
struct VSOut
{
    float4 color : COLOR0;
    float4 R3pos : TEXCOORD0;
    float4 norm : NORMAL;
    float4 SCpos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    return vso.color;
}