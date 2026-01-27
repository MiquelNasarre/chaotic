#include "Perspective.hlsli"

struct VSOut
{
    float dist : Distance;
    float4 SCpos : SV_Position;
};

cbuffer cBuff1 : register(b1)
{
    float3 position;
    float radius;
}

VSOut main(float2 pos : CirclePos)
{
    VSOut vso;
    // Distance is radius for all points but the origin.
    vso.dist = (pos.x == 0.f && pos.y == 0.f) ? 0.f : radius;
    // Get center point screen position.
    vso.SCpos = R3toScreenPos(float4(position, 0.f));
    // Add your circle position scaled.
    vso.SCpos.rg += radius * pos * scaling.rg;
    
    return vso;
}