
cbuffer cBuff : register(b0)
{
    float4 color;
}

float4 main(float dist : Distance) : SV_Target
{
    if (dist <1e-5)
        dist = 1e-5;
    // Light dims with the square to the dist
    return color / (dist * dist);
}