#include "Perspective.hlsli"

cbuffer Cbuff1 : register(b1)
{
    float4x4 transform;         // Distortion + Rotation + Translation
    float4x4 norm_transform;    // Distortion + Rotation for normals
    float2 screenDisplacement;  // Screen displacement
};

struct VSOut
{
	float4 coord : TEXCOORD0;
    float4 R3pos : TEXCOORD1;
    float4 norm : NORMAL;
	float4 SCpos : SV_Position;
};

VSOut main(float4 pos : Position, float4 norm : Normal, float4 coord : TexCoor)
{
	VSOut vso;
    
    // Transform the normal vector and position with the transformation matrices
    vso.norm = normalize(mul(norm_transform, norm));
    vso.R3pos = mul(transform, pos);
    
    // Default method to transform from R3 to screen position
    vso.SCpos = R3toScreenPos(vso.R3pos);
    
    // Add screen displacement
    vso.SCpos.rg += screenDisplacement;
    
    // Forward texture coordinate
	vso.coord = coord;
    
	return vso;
}