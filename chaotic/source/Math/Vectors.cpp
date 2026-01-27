#include "Math/Vectors.h"

#include <stdio.h>
#include <math.h>

/*
-----------------------------------------------------------------------------------------------------------
 Global Variables for String Generation
-----------------------------------------------------------------------------------------------------------
*/

// This should ensure that no string will be longer than 512 characters, and that 
// you can simultaneously use up to 16 strings at a time per thread. If you require 
// formatted strings longer than that or more strings at a time just do them yourself.

thread_local char str_buffer[16][512] = {};
thread_local unsigned buffer_pos = 0u;

/*
-----------------------------------------------------------------------------------------------------------
 Double Precision 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

double Vector2d::abs() const
{
	return sqrt(x * x + y * y);
}

// Returns an internal string with the vector coordinates.

const char* Vector2d::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y);

	return str;
}

/*
-----------------------------------------------------------------------------------------------------------
 Single Precision 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

float Vector2f::abs() const
{
	return sqrtf(x * x + y * y);
}

// Returns an internal string with the vector coordinates.

const char* Vector2f::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y);

	return str;
}

/*
-----------------------------------------------------------------------------------------------------------
 Integer 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

float Vector2i::abs() const
{
	return sqrtf(float(x * x + y * y));
}

// Returns an internal string with the vector coordinates.

const char* Vector2i::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y);

	return str;
}

/*
-----------------------------------------------------------------------------------------------------------
 Double Precision 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

double Vector3d::abs() const
{
	return sqrt(x * x + y * y + z * z);
}

// Returns an internal string with the vector coordinates.

const char* Vector3d::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y, z);

	return str;
}

/*
-----------------------------------------------------------------------------------------------------------
 Single Precision 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

float Vector3f::abs() const
{
	return sqrtf(x * x + y * y + z * z);
}

// Returns an internal string with the vector coordinates.

const char* Vector3f::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y, z);

	return str;
}

/*
-----------------------------------------------------------------------------------------------------------
 Integer 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Returns the absolute value of the vector.

float Vector3i::abs() const
{
	return sqrtf(float(x * x + y * y + z * z));
}

// Returns an internal string with the vector coordinates.

const char* Vector3i::str(const char* fmt) const
{
	char* str = str_buffer[buffer_pos];
	buffer_pos = (buffer_pos + 1u) % 16u;

	snprintf(str, 512, fmt, x, y, z);

	return str;
}
