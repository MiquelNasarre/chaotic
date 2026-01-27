#include "Perspective.hlsli"

cbuffer Cbuff1 : register(b1)
{
    float4x4 transform;         // Distortion + Rotation + Translation
    float2 screenDisplacement;  // Screen displacement
};

struct VSOut
{
    float4 R3pos : PointPos;
    float4 SCpos : SV_Position;
};

VSOut main(float4 pos : Position)
{
    VSOut vso;
    
    // Transform the position with the objects distortion/rotation/translation.
    vso.R3pos = mul(transform, pos);
    
    // Default method to transform from R3 to screen position
    vso.SCpos = R3toScreenPos(vso.R3pos);
    
    // Add screen displacement
    vso.SCpos.rg += screenDisplacement;
    
    return vso;
}