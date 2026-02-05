#include "perspective.hlsli"

cbuffer cBuff : register(b1)
{
    // Quaternion storing the scene rotation
    float4 rotation;
    // Vector storing the wideness of the FOV in each direction.
    float2 FOV;
}

struct VSOut
{
    float3 dir : TEXCOORD0;
    float4 pos : SV_Position;
};

VSOut main(float4 pos : Position)
{
    VSOut vso;
    vso.pos = pos;
    
    // Compose scene observer with scene rotation to obtain total rotation.
    float4 total_rotation = qConj(qMul(observer, rotation));
    
    // To get the initial vector we consider we have a rectangular screen one distance 
    // away from origin in the z axis where the spherical points are projected.
    float4 init_dir = float4(pos.xy * FOV * (scaling.z / scaling.xy) / 1000.f, 1.f, 0.f);
    
    // Apply the rotation to the vector to obtain the final sphere vector.
    vso.dir = Q2V(qRot(total_rotation, V2Q(init_dir))).xyz;
    
    return vso;
}