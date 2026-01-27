
struct Lightsource
{
    float2 intensity;
    // pad float2
    float4 color;
    float3 position;
    // pad float
};

cbuffer cBuff : register(b0)
{
    Lightsource lights[8];
};

float4 main(float4 color : Color, float3 pos : PointPos, float3 norm : Norm, bool front : SV_IsFrontFace) : SV_Target
{
    // Handle backfaces: flip normal if fragment is from back side
    if (!front)
        norm = -norm;
    
    float4 totalLight = float4(0.f, 0.f, 0.f, 0.f);
    float dist = 0.f;
    float dist2 = 0.f;
    float exposure = 0.f;
    
    // Iterate through point lights
    [unroll]
    for (int i = 0; i < 8; i++)
    {
        // Skip unused lights
        if (!lights[i].intensity.r && !lights[i].intensity.g)
            continue;
        
        // Compute distance and direct exposure to light
        dist = distance(lights[i].position, pos);
        exposure = dot(norm, lights[i].position -  pos) / dist;
        
        dist2 = dist * dist;
        // Add diffuse ilumination
        float light = lights[i].intensity.g / dist2;
        
        // Add direct ilumination
        if (exposure > 0)
            light += lights[i].intensity.r * exposure / dist2;
        
        // Add to total light
        totalLight += light * lights[i].color;
    }
    
    // Return light composition with your color
    return color * totalLight;
}