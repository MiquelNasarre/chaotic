
cbuffer cBuff : register(b0)
{
    float4 color;
}

struct VSOut
{
    float dist : TEXCOORD0;
    float4 SCpos : SV_Position;
};

float4 main(VSOut vso) : SV_Target
{
    if (vso.dist <1e-5)
        vso.dist = 1e-5;
    // Light dims with the square to the dist
    return color / (vso.dist * vso.dist);
}