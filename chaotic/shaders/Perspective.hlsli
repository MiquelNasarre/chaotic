#include "Quaternion.hlsli"

// Default Constant buffer bound to slot 0 by every Graphics instance.
cbuffer Perspective : register(b0)
{
    float4 observer;    // Quaternion defining observer
    float4 center;      // Center of the scene
    float4 scaling;     // Scaling data (scale / WindowDim, scale, 0)
};

// Sigmoid transformation to clip z between 0 and 1.
inline float z_trans(float z)
{
    return 1.f / (1.f + exp(-z));
}

// Takes an R3 position as input and outputs the default 
// SV_Position given the perspective constant buffer.
inline float4 R3toScreenPos(float4 R3pos)
{
    // Move to the center of the scene
    float4 pos = R3pos - center;
    // Apply the quaternion rotation
    float4 rotpos = Q2V(qRot(observer, V2Q(pos)));
    // Apply scaling and depth clipping.
    return float4(rotpos.rg * scaling.rg, z_trans(rotpos.b), 1.f);
}
