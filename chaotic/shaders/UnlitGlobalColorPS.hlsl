
cbuffer GlobalColor : register(b1)
{
    float4 color;
}

float4 main() : SV_Target
{
	return float4(color.rgb, 1.f);
}