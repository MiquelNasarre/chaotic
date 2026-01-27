
// Operation to multiply two quaternions (non-commutative).
inline float4 qMul(float4 q0, float4 q1)
{
    return float4(
		q0.r * q1.r - q0.g * q1.g - q0.b * q1.b - q0.a * q1.a,
		q0.r * q1.g + q0.g * q1.r + q0.b * q1.a - q0.a * q1.b,
		q0.r * q1.b + q0.b * q1.r + q0.a * q1.g - q0.g * q1.a,
		q0.r * q1.a + q0.a * q1.r + q0.g * q1.b - q0.b * q1.g
	);
}

// Operation to multiply three quaternions (non-commutative).
inline float4 qMul(float4 q0, float4 pos, float4 q1)
{
    return qMul(qMul(q0, pos), q1);
}

// Returns the conjugate of a quaternion.
inline float4 qConj(float4 q)
{
    return float4(q.r, -q.gba);
}

// Operation to take a rotation quaternion and apply it to a position.
inline float4 qRot(float4 q, float4 pos)
{
    return qMul(q, pos, qConj(q));
}

// Convert from Quaternion to R3,
inline float4 Q2V(float4 q)
{
    return float4(q.gba, 1.f);
}

// Convert from R3 to Quaternion.
inline float4 V2Q(float4 v)
{
    return float4(0.f, v.rgb);
}
inline float4 V2Q(float3 v)
{
    return float4(0.f, v);
}
