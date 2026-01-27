
// Returns a weight depending on the z position of the object.
inline float depth_weight(float4 scPos)
{
    float w = saturate(1.f - scPos.z);
    
    return max(w, 0.05f);
}
