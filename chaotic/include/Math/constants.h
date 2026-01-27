#pragma once

/* CONSTANTS HEADER
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
 This header contains some constant values to be used by the 
 user as it pleases, very helpful when creating math functions.

 By default it is included in all the library via the header.
--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------
*/

#ifdef _QUATERNION_ADDED
#define QUAT_I Quaternion(0.f,1.f,0.f,0.f)
#define QUAT_J Quaternion(0.f,0.f,1.f,0.f)
#define QUAT_K Quaternion(0.f,0.f,0.f,1.f)
#endif // _QUATERNION_ADDED

#ifdef _VECTORS_ADDED
#define VEC_EI Vector3f(1.f,0.f,0.f)
#define VEC_EJ Vector3f(0.f,1.f,0.f)
#define VEC_EK Vector3f(0.f,0.f,1.f)
#endif // _VECTORS_ADDED

#define MATH_PI 3.14159265358979f
#define MATH_E  2.71828182845905f