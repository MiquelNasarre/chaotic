#include "Math/Quaternion.h"

#include <math.h>
#include <stdio.h>

// Returns the absolute value of the Quaternion.

float Quaternion::abs() const
{
	return sqrtf(r * r + i * i + j * j + k * k);
}

// Returns an internal string with the Quaternion value.
// The format must be for the four quaternion dimension values.

const char* Quaternion::str(const char* fmt) const
{
	// This should ensure that no string will be longer than 512 characters, and that 
	// you can simultaneously use up to 16 strings at a time per thread. If you require 
	// formatted strings longer than that or more strings at a time just do them yourself.

	thread_local char str_buffer[16][512] = {};
	thread_local unsigned buffer_pos = 0u;

	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, r, i, j, k);

	return str;
}

// Returns the quaternion needed to rotate a the provided axis the specified angle. To rotate 
// a figure with this quaternion you do "P_rot = q * P * q.inv" for every point P in the figure.

Quaternion Quaternion::Rotation(Vector3f axis, float angle)
{
	if (!axis) return 1.f;

	return cosf(angle / 2.f) + Quaternion(sinf(angle / 2.f) * axis.normal());
}
