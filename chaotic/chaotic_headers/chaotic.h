#pragma once

/* LICENSE AND COPYRIGHT
-----------------------------------------------------------------------------------------------------------
 * Chaotic — a 3D renderer for mathematical applications
 * Copyright (c) 2025-2026 Miguel Nasarre Budiño
 * Licensed under the MIT License. See LICENSE file.
-----------------------------------------------------------------------------------------------------------
*/

/* CHAOTIC HEADER FILE
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Welcome to the Chaotic header!! This header contains the tools you need to be able to use 
what this library has to offer. This means the capacity to create Windows with Graphics
renderers, draw all kinds of Drawables and interact with them.

For an example implementation of the code you can check chaotic_defaults.h or the demo window
source file.

This header contains the high level API classes, with no external dependencies.
The classes included are the following:

 Math dependencies:
  * Vectors                     — 2/3D vectors of integer, float and double, defined operators.
  * Matrix						— Fully functional matrix class used for object distortions.
  * Quaternion					— Fully functional quaternion class used for rotations.
  * Constants					— Basic math constants for convenience (optional).

 Image dependencies:
  * Color						— B8G8R8A8 Color class used for all coloring in the library.
  * Image						— Image as array of colors, with convenient operators and file support.
 
 The UI static classes:
  * Keyboard					— Static keyboard class to capture keyboard interaction events.
  * Mouse						— Static mouse class to capture mouse interaction events.
 
 The window creation classes:
  * Graphics					— Renderer class attached to any library window.
  * Window						— Main class of the library for Window creation.

 All the drawable classes:
  * Drawable					— Base class for all drawable objects.
  * Background					— Drawable to create fixed and dynamic backgrounds.
  * Curve						— Drawable to create thin curves in 3D space.
  * Light						— Drawable to represent light sources.
  * Polyhedron					— Drawable to plot any polyhedron or triangle mesh.
  * Scatter						— Drawable to create point and line scatters.
  * Surface						— Drawable to plot all kinds of mathematical defined surfaces.
 
 Other library classes:
  * iGManager					— Support class to incorporate ImGui into the windows (optional).
  * Timer						— Timer class used by the internals added for convenience (optional).
  * ChaoticError				— Error base class used by the library for any error occurred. (optional).
  * UserError default class		— User type to be used by the user (USER_ERROR/USER_CHECK) (optional).

The classes are copied as they are in their original header files. For further reading
each class has its own description and all functions are explained. I suggest reading the 
descriptions of all functions before using them.

Abstraction has a strong hierarchy in this API. This header sits at the surface level
and provides with all kinds of tools you might need to create the apps. Those allows 
for quite a lot of freedom in the way you plot and design your figures, having multiple 
default drawables that allow for all different kinds of plots.

If you decide to create your own Drawables to further customize your plots you need to 
include "chaotic_customs.h", this header contains all the bindable classes, needed for
the creation of your own drawable classes and shaders.

If you decide to expand the functionalities of the Window class or the Graphics class 
or create your own bindables, you need to include "chaotic_internals.h", contains all 
the DirectX11 and Win32 dependencies used to create the internals of the library.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

/*
-----------------------------------------------------------------------------------------------------------
 Optional Classes Disabling
-----------------------------------------------------------------------------------------------------------
*/

// Toggle to enable and disable math constants inside the header.
#define _INCLUDE_CONSTANTS

// Toggle to enable and disable imGui support inside the header.
#define _INCLUDE_IMGUI

// Toggle to enable and disable Timer class inside the header.
#define _INCLUDE_TIMER

// Toggle to enable and disable Error classes inside the header.
#define _INCLUDE_USER_ERROR


/* VECTOR STRUCTURES HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains a group of vector structures for easy 2D and 3D vector manipulation.
Concretely it contains 2D and 3D, integer, single precision and double presicion vector
variants, and a 4D single presicion vector for 16 Byte storage and GPU processing.

All vectors support basic algebra operations like addition, subtraction, scalar division,
scalar multiplication and dot product. They also have other handy functions like str(fmt)
that returns a formatted string of the vector coordinates, abs() that returns the absolute
value of the vector, and other operators like comparissons.

Non integer vectors also support in-place and not in-place normalization and 3D vectors
support regular right handed vectorial product. All vectors are built for performance, so
you can use them for your math computations. For a full description of the functionalities
you can check the different structures functions.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

/*
-----------------------------------------------------------------------------------------------------------
 Float 4 Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a four dimensional vector of single precision, with coordinates x,y,z,w.
// Useful for 16 byte alignment vector storage, does not support operations.
struct alignas(16) _float4vector
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;
};

/*
-----------------------------------------------------------------------------------------------------------
 Integer 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a two dimensional vector of integers, with coordinates x,y.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector2i
{
	int x = 0;
	int y = 0;

	// Basic constructors.
	constexpr Vector2i() = default;
	constexpr Vector2i(int x, int y) { this->x = x; this->y = y; }

	template <typename number_type>
	constexpr Vector2i(number_type x, number_type y) { this->x = x; this->y = y; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector2i(const other_vector& other) { x = (int)other.x, y = (int)other.y; }

	// Addition/Subtraction.
	constexpr Vector2i operator+(const Vector2i& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2i operator-(const Vector2i& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2i operator-() const { return { -x, -y }; }

	// Self Addition/Subtration.
	constexpr Vector2i& operator+=(const Vector2i& other) { return *this = *this + other; }
	constexpr Vector2i& operator-=(const Vector2i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector2i operator*(const int& other) const { return { x * other, y * other }; }
	constexpr Vector2i operator/(const int& other) const { return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2i& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector2i& operator/=(const int& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	float abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+i, %+i)") const;

	// Scalar product operator.
	constexpr int operator^(const Vector2i& other) const { return x * other.x + y * other.y; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0) || (y != 0); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector2i& other) const { return (x != other.x) || (y != other.y); }
	constexpr bool operator==(const Vector2i& other) const { return (x == other.x) && (y == other.y); }
};

// Friend reversed operators.
constexpr Vector2i operator*(const int& lhs, const Vector2i& rhs) { return rhs * lhs; }

/*
-----------------------------------------------------------------------------------------------------------
 Single Precision 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a two dimensional vector of single precision, with coordinates x,y.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector2f
{
	float x = 0.f;
	float y = 0.f;

	// Basic constructors.
	constexpr Vector2f() = default;
	constexpr Vector2f(float x, float y) { this->x = x; this->y = y; }

	template <typename number_type>
	constexpr Vector2f(number_type x, number_type y) { this->x = x; this->y = y; }

	// Explicit _float4 constructors, takes its x,y coordinates.
	constexpr explicit Vector2f(_float4vector vec) { x = vec.x, y = vec.y; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector2f(const other_vector& other) { x = (float)other.x, y = (float)other.y; }

	// Addition/Subtraction.
	constexpr Vector2f operator+(const Vector2f& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2f operator+(const Vector2i& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2f operator-(const Vector2f& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2f operator-(const Vector2i& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2f operator-() const { return { -x, -y }; }

	// Self Addition/Subtration.
	constexpr Vector2f& operator+=(const Vector2f& other) { return *this = *this + other; }
	constexpr Vector2f& operator+=(const Vector2i& other) { return *this = *this + other; }
	constexpr Vector2f& operator-=(const Vector2f& other) { return *this = *this - other; }
	constexpr Vector2f& operator-=(const Vector2i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector2f operator*(const int& other) const { return { x * other, y * other }; }
	constexpr Vector2f operator*(const float& other) const { return { x * other, y * other }; }
	constexpr Vector2f operator/(const int& other) const { return { x / other, y / other }; }
	constexpr Vector2f operator/(const float& other) const { return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2f& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector2f& operator*=(const float& other) { return *this = *this * other; }
	constexpr Vector2f& operator/=(const int& other) { return *this = *this / other; }
	constexpr Vector2f& operator/=(const float& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	float abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+.3f, %+.3f)") const;

	// Returns a normalized vector (not in-place).
	inline Vector2f normal() const { return *this / abs(); }

	// In place normalization.
	inline Vector2f& normalize() { return *this = normal(); }

	// Scalar product operator.
	constexpr float operator^(const Vector2f& other) const { return x * other.x + y * other.y; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0.f) || (y != 0.f); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector2f& other) const { return (x != other.x) || (y != other.y); }
	constexpr bool operator==(const Vector2f& other) const { return (x == other.x) && (y == other.y); }

	// Returns the _float4vector equivalent.
	constexpr _float4vector getVector4() const { return { x, y, 0.f, 1.f }; }
};

// Friend reversed operators.
constexpr Vector2f operator*(const int& lhs, const Vector2f& rhs) { return rhs * lhs; }
constexpr Vector2f operator*(const float& lhs, const Vector2f& rhs) { return rhs * lhs; }

/*
-----------------------------------------------------------------------------------------------------------
 Double Precision 2D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a two dimensional vector of double precision, with coordinates x,y.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector2d
{
	double x = 0.0;
	double y = 0.0;

	// Basic constructors.
	constexpr Vector2d() = default;
	constexpr Vector2d(double x, double y) { this->x = x; this->y = y; }

	template <typename number_type>
	constexpr Vector2d(number_type x, number_type y) { this->x = x; this->y = y; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector2d(const other_vector& other) { x = (double)other.x, y = (double)other.y; }

	// Addition/Subtraction.
	constexpr Vector2d operator+(const Vector2d& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2d operator+(const Vector2f& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2d operator+(const Vector2i& other) const { return { x + other.x, y + other.y }; }
	constexpr Vector2d operator-(const Vector2d& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2d operator-(const Vector2f& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2d operator-(const Vector2i& other) const { return { x - other.x, y - other.y }; }
	constexpr Vector2d operator-() const { return { -x, -y }; }

	// Self Addition/Subtration.
	constexpr Vector2d& operator+=(const Vector2d& other) { return *this = *this + other; }
	constexpr Vector2d& operator+=(const Vector2f& other) { return *this = *this + other; }
	constexpr Vector2d& operator+=(const Vector2i& other) { return *this = *this + other; }
	constexpr Vector2d& operator-=(const Vector2d& other) { return *this = *this - other; }
	constexpr Vector2d& operator-=(const Vector2f& other) { return *this = *this - other; }
	constexpr Vector2d& operator-=(const Vector2i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector2d operator*(const int& other) const { return { x * other, y * other }; }
	constexpr Vector2d operator*(const float& other) const { return { x * other, y * other }; }
	constexpr Vector2d operator*(const double& other) const { return { x * other, y * other }; }
	constexpr Vector2d operator/(const int& other) const { return { x / other, y / other }; }
	constexpr Vector2d operator/(const float& other) const { return { x / other, y / other }; }
	constexpr Vector2d operator/(const double& other) const { return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2d& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector2d& operator*=(const float& other) { return *this = *this * other; }
	constexpr Vector2d& operator*=(const double& other) { return *this = *this * other; }
	constexpr Vector2d& operator/=(const int& other) { return *this = *this / other; }
	constexpr Vector2d& operator/=(const float& other) { return *this = *this / other; }
	constexpr Vector2d& operator/=(const double& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	double abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+.6f, %+.6f)") const;

	// Returns a normalized vector (not in-place).
	inline Vector2d normal() const { return *this / abs(); }

	// In place normalization.
	inline Vector2d& normalize() { return *this = normal(); }

	// Scalar product operator.
	constexpr double operator^(const Vector2d& other) const { return x * other.x + y * other.y; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0.0) || (y != 0.0); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector2d& other) const { return (x != other.x) || (y != other.y); }
	constexpr bool operator==(const Vector2d& other) const { return (x == other.x) && (y == other.y); }


};

// Friend reversed operators.
constexpr Vector2d operator*(const int& lhs, const Vector2d& rhs) { return rhs * lhs; }
constexpr Vector2d operator*(const float& lhs, const Vector2d& rhs) { return rhs * lhs; }
constexpr Vector2d operator*(const double& lhs, const Vector2d& rhs) { return rhs * lhs; }

/*
-----------------------------------------------------------------------------------------------------------
 Integer 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a three dimensional vector of integers, with coordinates x,y,z.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector3i
{
	int x = 0;
	int y = 0;
	int z = 0;

	// Basic constructors.
	constexpr Vector3i() = default;
	constexpr Vector3i(int x, int y, int z) { this->x = x; this->y = y; this->z = z; }

	template <typename number_type>
	constexpr Vector3i(number_type x, number_type y, number_type z) { this->x = x; this->y = y; this->z = z; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector3i(const other_vector& other) { x = (int)other.x, y = (int)other.y, z = (int)other.z; }

	// Addition/Subtraction.
	constexpr Vector3i operator+(const Vector3i& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3i operator-(const Vector3i& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3i operator-() const { return { -x, -y, -z }; }

	// Self Addition/Subtration.
	constexpr Vector3i& operator+=(const Vector3i& other) { return *this = *this + other; }
	constexpr Vector3i& operator-=(const Vector3i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector3i operator*(const int& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3i operator/(const int& other) const { return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3i& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector3i& operator/=(const int& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	float abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+i, %+i, %+i)") const;

	// Scalar product operator.
	constexpr int operator^(const Vector3i& other) const { return x * other.x + y * other.y + z * other.z; }

	// Vectorial product operator.
	constexpr Vector3i operator*(const Vector3i& other) const { return { (other.y * z - y * other.z), (other.z * x - z * other.x), (other.x * y - x * other.y) }; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0) || (y != 0) || (z != 0); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector3i& other) const { return (x != other.x) || (y != other.y) || (z != other.z); }
	constexpr bool operator==(const Vector3i& other) const { return (x == other.x) && (y == other.y) && (z == other.z); }
};

// Friend reversed operators.
constexpr Vector3i operator*(const int& lhs, const Vector3i& rhs) { return rhs * lhs; }

/*
-----------------------------------------------------------------------------------------------------------
 Single Precision 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a three dimensional vector of single precision, with coordinates x,y,z.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector3f
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	// Basic constructors.
	constexpr Vector3f() = default;
	constexpr Vector3f(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	template <typename number_type>
	constexpr Vector3f(number_type x, number_type y, number_type z) { this->x = x; this->y = y; this->z = z; }

	// Explicit _float4 constructors, takes its x,y,z coordinates.
	constexpr explicit Vector3f(_float4vector vec) { x = vec.x, y = vec.y, z = vec.z; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector3f(const other_vector& other) { x = (float)other.x, y = (float)other.y, z = (float)other.z; }

	// Addition/Subtraction.
	constexpr Vector3f operator+(const Vector3f& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3f operator+(const Vector3i& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3f operator-(const Vector3f& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3f operator-(const Vector3i& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3f operator-() const { return { -x, -y, -z }; }

	// Self Addition/Subtration.
	constexpr Vector3f& operator+=(const Vector3f& other) { return *this = *this + other; }
	constexpr Vector3f& operator+=(const Vector3i& other) { return *this = *this + other; }
	constexpr Vector3f& operator-=(const Vector3f& other) { return *this = *this - other; }
	constexpr Vector3f& operator-=(const Vector3i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector3f operator*(const int& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3f operator*(const float& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3f operator/(const int& other) const { return { x / other, y / other, z / other }; }
	constexpr Vector3f operator/(const float& other) const { return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3f& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector3f& operator*=(const float& other) { return *this = *this * other; }
	constexpr Vector3f& operator/=(const int& other) { return *this = *this / other; }
	constexpr Vector3f& operator/=(const float& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	float abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+.3f, %+.3f, %+.3f)") const;

	// Returns a normalized vector (not in-place).
	inline Vector3f normal() const { return *this / abs(); }

	// In place normalization.
	inline Vector3f& normalize() { return *this = normal(); }

	// Scalar product operator.
	constexpr float operator^(const Vector3f& other) const { return x * other.x + y * other.y + z * other.z; }

	// Vectorial product operator.
	constexpr Vector3f operator*(const Vector3f& other) const { return { (other.y * z - y * other.z), (other.z * x - z * other.x), (other.x * y - x * other.y) }; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0.f) || (y != 0.f) || (z != 0.f); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector3f& other) const { return (x != other.x) || (y != other.y) || (z != other.z); }
	constexpr bool operator==(const Vector3f& other) const { return (x == other.x) && (y == other.y) && (z == other.z); }

	// Returns the _float4vector equivalent.
	constexpr _float4vector getVector4() const { return { x, y, z, 1.f }; }
};

// Friend reversed operators.
constexpr Vector3f operator*(const int& lhs, const Vector3f& rhs) { return rhs * lhs; }
constexpr Vector3f operator*(const float& lhs, const Vector3f& rhs) { return rhs * lhs; }

/*
-----------------------------------------------------------------------------------------------------------
 Double Precision 3D Vector
-----------------------------------------------------------------------------------------------------------
*/

// Stores a three dimensional vector of double precision, with coordinates x,y,z.
// Bytes are ordered and aligned and supports multiple simple vector operations.
struct Vector3d
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;

	// Basic constructors.
	constexpr Vector3d() = default;
	constexpr Vector3d(double x, double y, double z) { this->x = x; this->y = y; this->z = z; }

	template <typename number_type>
	constexpr Vector3d(number_type x, number_type y, number_type z) { this->x = x; this->y = y; this->z = z; }

	// Other class constructor.
	template <typename other_vector>
	constexpr explicit Vector3d(const other_vector& other) { x = (double)other.x, y = (double)other.y, z = (double)other.z; }

	// Addition/Subtraction.
	constexpr Vector3d operator+(const Vector3d& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3d operator+(const Vector3f& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3d operator+(const Vector3i& other) const { return { x + other.x, y + other.y, z + other.z }; }
	constexpr Vector3d operator-(const Vector3d& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3d operator-(const Vector3f& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3d operator-(const Vector3i& other) const { return { x - other.x, y - other.y, z - other.z }; }
	constexpr Vector3d operator-() const { return { -x, -y, -z }; }

	// Self Addition/Subtration.
	constexpr Vector3d& operator+=(const Vector3d& other) { return *this = *this + other; }
	constexpr Vector3d& operator+=(const Vector3f& other) { return *this = *this + other; }
	constexpr Vector3d& operator+=(const Vector3i& other) { return *this = *this + other; }
	constexpr Vector3d& operator-=(const Vector3d& other) { return *this = *this - other; }
	constexpr Vector3d& operator-=(const Vector3f& other) { return *this = *this - other; }
	constexpr Vector3d& operator-=(const Vector3i& other) { return *this = *this - other; }

	// Scalar Multiplication/Division.
	constexpr Vector3d operator*(const int& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3d operator*(const float& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3d operator*(const double& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3d operator/(const int& other) const { return { x / other, y / other, z / other }; }
	constexpr Vector3d operator/(const float& other) const { return { x / other, y / other, z / other }; }
	constexpr Vector3d operator/(const double& other) const { return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3d& operator*=(const int& other) { return *this = *this * other; }
	constexpr Vector3d& operator*=(const float& other) { return *this = *this * other; }
	constexpr Vector3d& operator*=(const double& other) { return *this = *this * other; }
	constexpr Vector3d& operator/=(const int& other) { return *this = *this / other; }
	constexpr Vector3d& operator/=(const float& other) { return *this = *this / other; }
	constexpr Vector3d& operator/=(const double& other) { return *this = *this / other; }

	// Returns the absolute value of the vector.
	double abs() const;

	// Returns an internal string with the vector coordinates.
	// The format must be for the entire vector expression.
	const char* str(const char* fmt = "(%+.6f, %+.6f, %+.6f)") const;

	// Returns a normalized vector (not in-place).
	inline Vector3d normal() const { return *this / abs(); }

	// In place normalization.
	inline Vector3d& normalize() { return *this = normal(); }

	// Scalar product operator.
	constexpr double operator^(const Vector3d& other) const { return x * other.x + y * other.y + z * other.z; }

	// Vectorial product operator.
	constexpr Vector3d operator*(const Vector3d& other) const { return { (other.y * z - y * other.z), (other.z * x - z * other.x), (other.x * y - x * other.y) }; }

	// Bool operator.
	constexpr operator bool() const { return (x != 0.0) || (y != 0.0) || (z != 0.0); }

	// Comparisson operators.
	constexpr bool operator!=(const Vector3d& other) const { return (x != other.x) || (y != other.y) || (z != other.z); }
	constexpr bool operator==(const Vector3d& other) const { return (x == other.x) && (y == other.y) && (z == other.z); }
};

// Friend reversed operators.
constexpr Vector3d operator*(const int& lhs, const Vector3d& rhs) { return rhs * lhs; }
constexpr Vector3d operator*(const float& lhs, const Vector3d& rhs) { return rhs * lhs; }
constexpr Vector3d operator*(const double& lhs, const Vector3d& rhs) { return rhs * lhs; }


/* MATRIX STRUCTURE HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the Matrix structure. Built for easy and high performance 3x3 matrix
operations. It supports basic algebra operations like addition, subtraction, scalar division,
scalar multiplication, vector multiplication and matrix multiplication.

It also support basic matrix operations like determinant, in-place and not in-place inversion
and transposition and some handy constructors.

A 4x4 column major matrix is also defined and Matrix can be transformed to it including a
position translation. This is very helpful for the graphics library and it is used to send
object transformations to the GPU.

Matrices are the maximum expression of linear transformations you can have on 3D space, that
makes them very convenient for linear shape distortions and that is why they are used in the
graphics library. Different matrix function constructors are provided to generate all kind
of distortions you can apply to your objects.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Column major 4x4 matrix struct. For 16 Byte alignment and GPU computations.
struct alignas(16) _float4matrix
{
	float indices[16] = {};
};

// Matrix struct that stores an ordered row-major filled matrix and supports basic 
// matrix and vector/matrix operations. Since its most common use is for distortions
// there ar a helper functions to generate this kind of matrices.
struct Matrix
{
	float a00 = 0.f; float a01 = 0.f; float a02 = 0.f;
	float a10 = 0.f; float a11 = 0.f; float a12 = 0.f;
	float a20 = 0.f; float a21 = 0.f; float a22 = 0.f;

	// Zero default initialization.
	constexpr Matrix() = default;

	// For simple identity scaling matrices.
	constexpr explicit Matrix(float k) { a00 = k; a11 = k; a22 = k; }

	// All values initializer.
	constexpr Matrix(
		float _a00, float _a01, float _a02,
		float _a10, float _a11, float _a12,
		float _a20, float _a21, float _a22)
	{
		a00 = _a00; a01 = _a01; a02 = _a02;
		a10 = _a10; a11 = _a11; a12 = _a12;
		a20 = _a20; a21 = _a21; a22 = _a22;
	}

	// Matrix addition operator.
	constexpr Matrix operator+(const Matrix& o) const
	{
		return {
			a00 + o.a00, a01 + o.a01, a02 + o.a02,
			a10 + o.a10, a11 + o.a11, a12 + o.a12,
			a20 + o.a20, a21 + o.a21, a22 + o.a22
		};
	}

	// Matrix subtraction operator.
	constexpr Matrix operator-(const Matrix& o) const
	{
		return {
			a00 - o.a00, a01 - o.a01, a02 - o.a02,
			a10 - o.a10, a11 - o.a11, a12 - o.a12,
			a20 - o.a20, a21 - o.a21, a22 - o.a22
		};
	}

	// Matrix multiplication operator.
	constexpr Matrix operator*(const Matrix& o) const
	{
		// Row-major multiplication: (this) * o
		return {
			a00 * o.a00 + a01 * o.a10 + a02 * o.a20,
			a00 * o.a01 + a01 * o.a11 + a02 * o.a21,
			a00 * o.a02 + a01 * o.a12 + a02 * o.a22,

			a10 * o.a00 + a11 * o.a10 + a12 * o.a20,
			a10 * o.a01 + a11 * o.a11 + a12 * o.a21,
			a10 * o.a02 + a11 * o.a12 + a12 * o.a22,

			a20 * o.a00 + a21 * o.a10 + a22 * o.a20,
			a20 * o.a01 + a21 * o.a11 + a22 * o.a21,
			a20 * o.a02 + a21 * o.a12 + a22 * o.a22
		};
	}

	// Negative operator.
	constexpr Matrix operator-() const
	{
		return { -a00,-a01,-a02,  -a10,-a11,-a12,  -a20,-a21,-a22 };
	}

	// Scalar multiplication operator.
	constexpr Matrix operator*(const float& s) const
	{
		return {
			a00 * s, a01 * s, a02 * s,
			a10 * s, a11 * s, a12 * s,
			a20 * s, a21 * s, a22 * s
		};
	}

	// Column vector multiplication operator.
	constexpr Vector3f operator*(const Vector3f& v) const
	{
		// Column-vector convention: v' = M * v
		return {
			a00 * v.x + a01 * v.y + a02 * v.z,
			a10 * v.x + a11 * v.y + a12 * v.z,
			a20 * v.x + a21 * v.y + a22 * v.z
		};
	}

	// Returns the specified column as a vector.
	constexpr Vector3f column(unsigned n) const
	{
		switch (n)
		{
		default:
		case 0: return { a00, a10, a20 };
		case 1: return { a01, a11, a21 };
		case 2: return { a02, a12, a22 };
		}
	}

	// Returns the specified row as a vector.
	constexpr Vector3f row(unsigned n) const
	{
		switch (n)
		{
		default:
		case 0: return { a00, a01, a02 };
		case 1: return { a10, a11, a12 };
		case 2: return { a20, a21, a22 };
		}
	}

	// Computes the determinant of the matrix.
	constexpr float determinant() const
	{
		return
			a00 * (a11 * a22 - a12 * a21) -
			a01 * (a10 * a22 - a12 * a20) +
			a02 * (a10 * a21 - a11 * a20);
	}

	// Returns the transposed matrix (not in-place).
	constexpr Matrix transposed() const
	{
		return {
			a00, a10, a20,
			a01, a11, a21,
			a02, a12, a22
		};
	}

	// In place matrix transposition.
	constexpr Matrix& transpose() { return *this = transposed(); }

	// Returns the inverse of the matrix (not in-place).
	constexpr Matrix inverse() const
	{
		const float det = determinant();

		// If determinant is very small you cannot invert it.
		const float eps = 1e-8f;
		if (det < eps && -det < eps)
			return Matrix();

		const float invDet = 1.0f / det;

		// Adjugate (cofactor transpose) * 1/det
		Matrix adj(
			(a11 * a22 - a12 * a21), -(a01 * a22 - a02 * a21), (a01 * a12 - a02 * a11),
			-(a10 * a22 - a12 * a20), (a00 * a22 - a02 * a20), -(a00 * a12 - a02 * a10),
			(a10 * a21 - a11 * a20), -(a00 * a21 - a01 * a20), (a00 * a11 - a01 * a10)
		);

		return adj * invDet;
	}

	// In place matrix inversion.
	constexpr Matrix& invert() { return *this = inverse(); }

	// Returns a column major 4x4 matrix with an added translation vector.
	constexpr _float4matrix getMatrix4(const Vector3f t = {}) const
	{
		return {
			a00, a10, a20, 0.f,
			a01, a11, a21, 0.f,
			a02, a12, a22, 0.f,
			t.x, t.y, t.z, 1.f,
		};
	}

	// --- Static Constructors ---

	// Returns identity matrix.
	static constexpr Matrix Identity() { return Matrix(1.f); }

	// stretch in each cardinal direction, returns the diagonal matrix.
	static constexpr Matrix Diagonal(float x, float y, float z)
	{
		return {
			x, 0.f, 0.f,
			0.f, y, 0.f,
			0.f, 0.f, z
		};
	}

	// Stretch along direction 'axis' by 'factor' (factor=1 => no change).
	static inline Matrix Stretch(const Vector3f axis, float factor)
	{
		if (!axis)
			return Matrix::Identity();

		const float a = (factor - 1.0f);
		const Vector3f u = axis.normal();

		// I + a*u*u^T
		return {
			1.f + a * u.x * u.x,       a * u.x * u.y,       a * u.x * u.z,
				  a * u.y * u.x, 1.f + a * u.y * u.y,       a * u.y * u.z,
				  a * u.z * u.x,       a * u.z * u.y, 1.f + a * u.z * u.z
		};
	}

	// Shear that pushes along 'dir' proportionally to projection on 'ref'.
	static constexpr Matrix Shear(const Vector3f dir, const Vector3f ref, float k)
	{
		// A = I + k * dir * ref^T   (dir/ref need not be orthogonal; ref is "measured axis")
		const Vector3f d = dir;
		const Vector3f r = ref;
		return {
			1.f + k * d.x * r.x,       k * d.x * r.y,       k * d.x * r.z,
				  k * d.y * r.x, 1.f + k * d.y * r.y,       k * d.y * r.z,
				  k * d.z * r.x,       k * d.z * r.y, 1.f + k * d.z * r.z
		};
	}
};

// Reversed scalar multiplication operator.
constexpr Matrix operator*(const float& s, const Matrix& M) { return M * s; }

// Reverset row vector multiplication operator.
constexpr Vector3f operator*(const Vector3f& v, const Matrix& M)
{
	// Row-vector convention: v' = v * M
	return {
		v.x * M.a00 + v.y * M.a10 + v.z * M.a20,
		v.x * M.a01 + v.y * M.a11 + v.z * M.a21,
		v.x * M.a02 + v.y * M.a12 + v.z * M.a22
	};
}



/* QUATERNION STRUCTURE HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the Quaternion structure. Built for easy and high performance quaternion
operations. It supports basic algebra operations like addition, subtraction, multiplication,
division and all of the above with scalars.

It also has some quaternion specific operations: absolute value computation, in-place and not
in-place inversion and normalization. And a handy formatted string function str(fmt).

For comatibility with the rest of the library it supports vector conversion in both directions:
(x,y,z) <=> xi+yj+zk. And supports matrix conversion to obtain a rotation matrix from a normalized
Quaternion. To obtain a rotation quaternion it suppports a simple external function.

The value that quaternions have for computation is their capability to easily rotate any 3D figure
in any arbitrary axis, and compose those rotations. If we want to rotate an object around an axis V
defined by a normalized vector, for an angle a, you can easily obtain that rotation by defining the
quaternions:
	q  = cos(a/2) + sin(a/2) * V^(i,j,k)		(^ is the dot product)
	q' = cos(a/2) - sin(a/2) * V^(i,j,k)		(' is the inversion operation)

Then given a point P on the figure you can perform the rotation to that point with the following
formula:
	P_rot = q*P*q'

Where the transformation between vector and quaterion is P = (x,y,z) <=> xi+yj+zk. You can also
compose quaternion rotations: given q0 and q1 rotation quaternions, perform the non-commutative
product, q01=q1*q0. The result applies first the rotation in q0 and then the rotation in q1.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Quaternion struct that stores ordered r,i,j,k single precision values. Typical Quaternion 
// operations are supported and there is a helper function to generate rotation quaternions.
struct alignas(16) Quaternion
{
	float r = 0.f;
	float i = 0.f;
	float j = 0.f;
	float k = 0.f;

	// Default constructor.
	constexpr Quaternion() = default;

	// Float constructor.
	constexpr Quaternion(float _r, float _i = 0.f, float _j = 0.f, float _k = 0.f) { r = _r, i = _i, j = _j, k = _k; }

	// Real value type constructor.
	template <typename number_type>
	constexpr Quaternion(number_type _r, number_type _i = 0.f, number_type _j = 0.f, number_type _k = 0.f) { r = _r, i = _i, j = _j, k = _k; }

	// From vector to pure imaginary Quaternion.
	constexpr Quaternion(Vector3f v) { r = 0.f, i = v.x, j = v.y, k = v.z; }

	// Addition with a real number.
	template <typename number_type>
	constexpr Quaternion operator+(const number_type& other) const { return { r + other, i, j, k }; }

	// Subtraction with a real number.
	template <typename number_type>
	constexpr Quaternion operator-(const number_type& other) const { return { r - other, i, j, k }; }

	// Multiplication with a real number.
	template <typename number_type>
	constexpr Quaternion operator*(const number_type& other) const { return { r * other, i * other, j * other, k * other }; }

	// Division by a real number.
	template <typename number_type>
	constexpr Quaternion operator/(const number_type& other) const { return { r / other, i / other, j / other, k / other }; }

	// Quaternion addition and subtraction.
	constexpr Quaternion operator+(const Quaternion& other) const { return { r + other.r, i + other.i, j + other.j, k + other.k }; }
	constexpr Quaternion operator-(const Quaternion& other) const { return { r - other.r, i - other.i, j - other.j, k - other.k }; }
	constexpr Quaternion operator-() const { return { -r, -i, -j, -k }; }

	// In-place Quaternion addition and subtraction.
	constexpr Quaternion& operator+=(const Quaternion& other) { return *this = *this + other; }
	constexpr Quaternion& operator-=(const Quaternion& other) { return *this = *this - other; }

	// Returns the inverse quaterion (not in-place).
	constexpr Quaternion inv() const { return Quaternion(r, -i, -j, -k) / (r * r + i * i + j * j + k * k); }

	// In place inversion.
	constexpr Quaternion& invert() { return *this = inv(); }

	// Non-commutative Quaternion multiplication operator.
	constexpr Quaternion operator*(const Quaternion& other) const
	{
		return Quaternion(
			r * other.r - i * other.i - j * other.j - k * other.k,
			r * other.i + i * other.r + j * other.k - k * other.j,
			r * other.j + j * other.r + k * other.i - i * other.k,
			r * other.k + k * other.r + i * other.j - j * other.i
		);
	}

	// Quaternion division operator.
	constexpr Quaternion operator/(const Quaternion& other) const { return *this * other.inv(); }

	// In-place Quaternion multiplication and division. 
	// In-place multiplication is reversed for rotation accumulation.
	constexpr Quaternion& operator*=(const Quaternion& other) { return *this = other * *this; }
	constexpr Quaternion& operator/=(const Quaternion& other) { return *this = *this / other; }

	// Returns the absolute value of the Quaternion.
	float abs() const;

	// Returns an internal string with the Quaternion value.
	// The format must be for the four quaternion dimension values.
	const char* str(const char* fmt = "%+.2f %+.2fi %+.2fj %+.2fk") const;

	// Returns a normalized Quaternion (not in-place).
	inline Quaternion normal() const { return *this / abs(); }

	// In place normalization.
	inline Quaternion& normalize() { return *this = normal(); }

	// Returns the pure imaginary 3D vector of from the quaternion.
	constexpr Vector3f getVector() const { return { i, j, k }; }

	// Returns the equivalent rotation matrix given a normalized quaternion.
	constexpr Matrix getMatrix() const
	{
		const float ii = i * i;
		const float jj = j * j;
		const float kk = k * k;

		const float ij = i * j;
		const float ik = i * k;
		const float jk = j * k;

		const float ri = r * i;
		const float rj = r * j;
		const float rk = r * k;

		return 
		{
			1.f - 2.f * (jj + kk),       2.f * (ij - rk),       2.f * (ik + rj),
				  2.f * (ij + rk), 1.f - 2.f * (ii + kk),       2.f * (jk - ri),
				  2.f * (ik - rj),       2.f * (jk + ri), 1.f - 2.f * (ii + jj)
		};
	}

	// Boolean operator.
	constexpr operator bool() const { return (r != 0.f) || (i != 0.f) || (j != 0.f) || (k != 0.f); }

	// Comparisson operators.
	constexpr bool operator!=(const Quaternion& other) const { return (r != other.r) || (i != other.i) || (j != other.j) || (k != other.k); }
	constexpr bool operator==(const Quaternion& other) const { return (r == other.r) && (i == other.i) && (j == other.j) && (k == other.k); }

	// -- STATIC CONSTRUCTOR FOR ROTATION ---

	// Returns the quaternion needed to rotate a the provided axis the specified angle. To rotate 
	// a figure with this quaternion you do "P_rot = q * P * q.inv" for every point P in the figure.
	static Quaternion Rotation(Vector3f axis, float angle);
};

// Reversed order multiplication.
constexpr Quaternion operator*(const float& lhs, const Quaternion& rhs) { return rhs * lhs; }

// Reversed order addition.
constexpr Quaternion operator+(const float& lhs, const Quaternion& rhs) { return rhs + lhs; }

// Reversed order division.
constexpr Quaternion operator/(const float& lhs, const Quaternion& rhs) { return rhs.inv() * lhs; }

// Reversed order subtraction.
constexpr Quaternion operator-(const float& lhs, const Quaternion& rhs) { return -rhs + lhs; }

#ifdef _INCLUDE_CONSTANTS

/* CONSTANTS HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
 This header contains some constant values to be used by the
 user as it pleases, very helpful when creating math functions.

 By default it is included in all the library via the header.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

#define QUAT_I Quaternion(0.f,1.f,0.f,0.f)
#define QUAT_J Quaternion(0.f,0.f,1.f,0.f)
#define QUAT_K Quaternion(0.f,0.f,0.f,1.f)

#define VEC_EI Vector3f(1.f,0.f,0.f)
#define VEC_EJ Vector3f(0.f,1.f,0.f)
#define VEC_EK Vector3f(0.f,0.f,1.f)

#define MATH_PI 3.14159265358979f
#define MATH_E  2.71828182845905f

#undef _INCLUDE_CONSTANTS
#endif // _INCLUDE_CONSTANTS


/* COLOR HEADER CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Inherited class for my 3D renderer solution, useful for dealing
with colored images and GPU printings.

It has a bunch of operators defined tailored to my needs in the 
past with this class, still very simple and robust.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Struct for sending color arrays to the GPU
struct alignas(16) _float4color 
{
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;
};

// Color class with storage BGRA useful for dealing with colored images.
// Operators and some default colors are defined for convenience.
struct Color 
{
	unsigned char B = 0;	// Stores the blue color value
	unsigned char G = 0;	// Stores the green color value
	unsigned char R = 0;	// Stores the red color value
	unsigned char A = 0;	// Stores the transparency value

	// Default constructors
	constexpr Color(const Color&) = default;
	constexpr Color() = default;

	// Initializes from 4 {0,..,255} values
	constexpr Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255u)
		: R{ r }, G{ g }, B{ b }, A{ a } {}

	// Initializes through a float*
	constexpr explicit Color(const _float4color col)
		:	R{ unsigned char(col.r * 255.f) },
			G{ unsigned char(col.g * 255.f) },
			B{ unsigned char(col.b * 255.f) },
			A{ unsigned char(col.a * 255.f) } {}

	// Default colors for convenience
	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Purple;
	static const Color Gray;
	static const Color Orange;
	static const Color Transparent;

	// Basic operations
	friend constexpr Color operator+(const Color& color_0, const Color& color_1)
	{
		int Ri = (int)color_0.R + (int)color_1.R;
		int Gi = (int)color_0.G + (int)color_1.G;
		int Bi = (int)color_0.B + (int)color_1.B;
		int Ai = (int)color_0.A + (int)color_1.A;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color(Ri, Gi, Bi, Ai);
	}
	friend constexpr Color operator-(const Color& color_0, const Color& color_1)
	{
		int Ri = (int)color_0.R - (int)color_1.R;
		int Gi = (int)color_0.G - (int)color_1.G;
		int Bi = (int)color_0.B - (int)color_1.B;
		int Ai = (int)color_0.A - (int)color_1.A;
		if (Ri < 0)Ri = 0;
		if (Gi < 0)Gi = 0;
		if (Bi < 0)Bi = 0;
		if (Ai < 0)Ai = 0;
		return Color(Ri, Gi, Bi, Ai);
	}
	friend constexpr Color operator*(const Color& color_0, const Color& color_1)
	{
		int Ri = ((int)color_0.R * (int)color_1.R) / 255;
		int Gi = ((int)color_0.G * (int)color_1.G) / 255;
		int Bi = ((int)color_0.B * (int)color_1.B) / 255;
		int Ai = ((int)color_0.A * (int)color_1.A) / 255;
		return Color(Ri, Gi, Bi, Ai);
	}
	friend constexpr Color operator/(const Color& color_0, const Color& color_1) 
	{
		auto nz = [](unsigned char v) { return v ? v : 1; };
		return Color(
			(color_0.R * 255) / nz(color_1.R),
			(color_0.G * 255) / nz(color_1.G),
			(color_0.B * 255) / nz(color_1.B),
			(color_0.A * 255) / nz(color_1.A)
		);
	}

	// Basic operations with number
	constexpr Color operator*(const int& rhs) const
	{
		int Ri = (int)R * rhs;
		int Gi = (int)G * rhs;
		int Bi = (int)B * rhs;
		int Ai = (int)A * rhs;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color(Ri, Gi, Bi, Ai);
	}
	constexpr Color operator/(const int& rhs) const
	{
		if (!rhs) return White;
		float Ri = (float)R / rhs;
		float Gi = (float)G / rhs;
		float Bi = (float)B / rhs;
		float Ai = (float)A / rhs;
		return Color((unsigned char)Ri, (unsigned char)Gi, (unsigned char)Bi, (unsigned char)Ai);
	}
	constexpr Color operator*(const float& rhs) const
	{
		float Ri = (int)R * rhs;
		float Gi = (int)G * rhs;
		float Bi = (int)B * rhs;
		float Ai = (int)A * rhs;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color((unsigned char)Ri, (unsigned char)Gi, (unsigned char)Bi, (unsigned char)Ai);
	}
	constexpr Color operator/(const float& rhs) const
	{
		if (!rhs) return White;
		float Ri = (int)R / rhs;
		float Gi = (int)G / rhs;
		float Bi = (int)B / rhs;
		float Ai = (int)A / rhs;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color((unsigned char)Ri, (unsigned char)Gi, (unsigned char)Bi, (unsigned char)Ai);
	}
	constexpr Color operator*(const double& rhs) const
	{
		float Ri = (int)R * (float)rhs;
		float Gi = (int)G * (float)rhs;
		float Bi = (int)B * (float)rhs;
		float Ai = (int)A * (float)rhs;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color((unsigned char)Ri, (unsigned char)Gi, (unsigned char)Bi, (unsigned char)Ai);
	}
	constexpr Color operator/(const double& rhs) const
	{
		if (!rhs) return White;
		float Ri = (int)R / (float)rhs;
		float Gi = (int)G / (float)rhs;
		float Bi = (int)B / (float)rhs;
		float Ai = (int)A / (float)rhs;
		if (Ri > 255)Ri = 255;
		if (Gi > 255)Gi = 255;
		if (Bi > 255)Bi = 255;
		if (Ai > 255)Ai = 255;
		return Color((unsigned char)Ri, (unsigned char)Gi, (unsigned char)Bi, (unsigned char)Ai);
	}

	// Color inversion
	constexpr Color operator-() const { return Color(255 - R, 255 - G, 255 - B, A); }

	// Self operators
	constexpr Color& operator+=(const Color& other)	{ return *this = *this + other; }
	constexpr Color& operator-=(const Color& other)	{ return *this = *this - other; }
	constexpr Color& operator*=(const Color& other)	{ return *this = *this * other; }
	constexpr Color& operator/=(const Color& other)	{ return *this = *this / other; }
	constexpr Color& operator*=(const int& other)		{ return *this = *this * other; }
	constexpr Color& operator/=(const int& other)		{ return *this = *this / other; }
	constexpr Color& operator*=(const float& other)	{ return *this = *this * other; }
	constexpr Color& operator/=(const float& other)	{ return *this = *this / other; }
	constexpr Color& operator*=(const double& other)	{ return *this = *this * other; }
	constexpr Color& operator/=(const double& other)	{ return *this = *this / other; }

	// Comparisons
	constexpr bool operator==(const Color& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
	constexpr bool operator!=(const Color& other) const { return !(*this == other); }

	// For convenience turns into _float4color struct
	constexpr _float4color getColor4() const { return { (float)R / 255.f, (float)G / 255.f, (float)B / 255.f, (float)A / 255.f }; }
};

// Reversed operations
constexpr Color operator*(const int& rhs, const Color& lhs)		{ return lhs * rhs; }
constexpr Color operator*(const float& rhs, const Color& lhs)	{ return lhs * rhs; }
constexpr Color operator*(const double& rhs, const Color& lhs)	{ return lhs * rhs; }

// Default colors definitions
constexpr inline Color Color::Black			= Color(   0,   0,   0, 255);
constexpr inline Color Color::White			= Color( 255, 255, 255, 255);
constexpr inline Color Color::Red			= Color( 255,   0,   0, 255);
constexpr inline Color Color::Green			= Color(   0, 255,   0, 255);
constexpr inline Color Color::Blue			= Color(   0,   0, 255, 255);
constexpr inline Color Color::Yellow		= Color( 255, 255,   0, 255);
constexpr inline Color Color::Cyan			= Color(   0, 255, 255, 255);
constexpr inline Color Color::Purple		= Color( 255,   0, 255, 255);
constexpr inline Color Color::Gray			= Color( 127, 127, 127, 255);
constexpr inline Color Color::Orange		= Color( 255, 127,   0, 255);
constexpr inline Color Color::Transparent	= Color(   0,   0,   0,   0);


/* IMAGE CLASS HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This image class allows for easy image manipulation from your code. To create an image
you can create one yourself by specifying the dimensions and the color of each pixel 
or you can load them from your computer from raw bitmap files using the load() function.

To manipulate the image you can access the pixels directly via tha operator(row,col), or 
via the pixels() function, the Color class is a BGRA color format class that stores the 
channels as unsinged chars (bytes), for more information about the color class please check 
its header file.

Then after some image manipulation, using the save() function you can store them back into 
your computer to your specified path, or in the 3D rendering library you can send it to the 
GPU as textures for your renderings.

To obtain raw bitmap files from your images I strongly suggest the use of ImageMagick, 
a simple console command like: "> magick initial_image.*** -compress none image.bmp"
will give you a raw bitmap of any image.

For information on how to install and use ImageMagick you can check https://imagemagick.org/
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Simple class for handling images, and storing and loading them as BMP files.
class Image
{
private:
	// Private variables

	Color* pixels_ = nullptr;	// Pointer to the image pixels as a color array

	unsigned width_	 = 0u;	// Stores the width of the image
	unsigned height_ = 0u;	// Stores the height of the image

	// Internal function to format strings for the templates
	const char* internal_formatting(const char* fmt_filename, ...);

public:
	// Constructors/Destructors

	// Empty constructor, load() or reset() can be called to reshape it.
	Image() {}

	// Initializes the image as stored in the bitmap file.
	Image(const char* filename);

	// Initializes the image as stored in the bitmap file. Regular string formatting.
	template<class Arg0, class ...Args>
	Image(const char* fmt_filename, Arg0 arg0, Args... args) : Image(internal_formatting(fmt_filename, arg0, args...)) {}

	// Copies the other image.
	Image(const Image& other);

	// Copies the other image.
	Image& operator=(const Image& other);

	// Stores a copy of the color pointer.
	Image(Color* pixels, unsigned width, unsigned height);

	// Creates an image with the specified size and color.
	Image(unsigned width, unsigned height, Color color = Color::Transparent);

	// Frees the pixel pointer.
	~Image();

	// Resets the image to the new dimensions and color.
	void reset(unsigned width, unsigned height, Color color = Color::Transparent);

	// File functions

	// Loads an image from the specified file path. Regular string formatting.
	template<class Arg0, class ...Args>
	bool load(const char* fmt_filename, Arg0 arg0, Args... args) { return load(internal_formatting(fmt_filename, arg0, args...)); }

	// Loads an image from the specified file path.
	bool load(const char* filename);

	// Saves the image to the specified file path. Regular string formatting.
	template<class Arg0, class ...Args>
	bool save(const char* fmt_filename, Arg0 arg0, Args... args) const { return save(internal_formatting(fmt_filename, arg0, args...)); }

	// Saves the image to the specified file path.
	bool save(const char* filename) const;

	// Getters

	// Returns the pointer to the image pixels as a color array.
	inline Color* pixels() { return pixels_; }

	// Returns the constant pointer to the image pixels as a color array.
	inline const Color* pixels() const { return pixels_; }

	// Returns the image width.
	inline unsigned width() const { return width_; }

	// Returns the image height.
	inline unsigned height() const { return height_; }

	// Accessors

	// Returns a color reference to the specified pixel coordinates.
	inline Color& operator()(unsigned row, unsigned col) 
	{ return pixels_[row * width_ + col]; }

	// Returns a constant color reference to the specified pixel coordinates.
	inline const Color& operator()(unsigned row, unsigned col) const 
	{ return pixels_[row * width_ + col]; }
};

// Since the 3D library uses Texture Cubes as its preferred image projection to 
// the sphere, but texture cubes are not so common online resources. This small 
// static struct will allow you to convert the most common spherical projections 
// to texture cubes. 
// They expect a valid images and a cube width for the output resolution, and 
// they will create a cube image and sample its colors from your projection image. 
// The image is allocated with 'new' and is to be destroyed by the user.
struct ToCube
{
	// Equirectangular projections are extremely common, they follow the typical
	// latitude longitude coordinate system and are widely used for all kinds of
	// applications. This function allows you to convert equirectangular projection
	// images to texture cubes.
	static Image* from_equirect(const Image& equirect, unsigned cube_width);

	// Depending on the fisheye image the distance from center to angle projection
	// can be different. IF you are not sure which one your is just try different
	// types until it looks good.
	enum FISHEYE_TYPE
	{
		FISHEYE_EQUIDISTANT,
		FISHEYE_EQUISOLID,
		FISHEYE_STEREOGRAPHIC,
	};

	// If your stereographics look weird just play with this value. Adjusts how wide
	// of an angle your stereographic image is capturing. Beware that stereographic
	// Images cannot fill up the entire sphere unless they are infinately large, 
	// therefore an unknonw region will always be there.
	static inline float STEREOGRAPHIC_DIV = 2.5f;
	static inline Color STEREOGRAPHIC_FILL = Color::Black;

	// Fish-eye or circular panoramic pictures are common as a result of 360º cameras
	// that usually take this format when taking their pictures. This function allows
	// you to convert your fish-eye 360º pictures into texture cubes.
	// This function expects 360º fisheye images, so if your images are only half a 
	// sphere, pass an image double the size with your fisheye image in the middle.
	static Image* from_fisheye(const Image& fisheye, unsigned cube_width, FISHEYE_TYPE type);
};


/* KEYBOARD CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
In order to develop user interaction in your apps it is important to have access
to the message pipeline coming into your window. For that reason this keyboard 
abstraction class stores the keyboard events via the MSGHandlePipeline and gives
simple access to the user to those events.

It has a character buffer, to be easily used for text applications, an event buffer
and a key state array that stores the current state of any of the keys.

The keyCode used by this class is derived from the wParam of the Win32 keyboard messages:
- For key events, keyCodes are the values defined by the Win32 VK_* constants.

- For alphanumeric keys, the virtual-key codes match their uppercase ASCII values:
	'0'–'9' -> codes 0x30–0x39
	'A'–'Z' -> codes 0x41–0x5A
  So you can write Keyboard::isKeyPressed('M') or check for event::keyCode=='3', etc.

- For non-character keys (function keys, arrows, etc.), keyCode corresponds to the 
  appropriate Win32 virtual-key code (VK_F1, VK_LEFT, VK_ESCAPE, …) truncated to char. 
  These values do not correspond to ASCII characters. Check Win32 references if you 
  want to implement non alpha-numeric keys:
  https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

 NOTE: When ImGui requests focus it captures pushChar(), so these are not stored by the
 keyboard, all other keyboard interactions, like setKeyPressed()/setKeyReleased() and
 type events are still recorded regardless of ImGui focus. That is to allow general
 keyboard shortcuts to work.

 If you have definded keyboard interactions but you also use ImGui text tools I suggest
 you turn your keyboard interactions to system keys or "Ctrl + key" to avoid ambiguity.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Keyboard static class, contains a set of user functions to create interactions between
// the keyboard and your app. Stores events, characters and key states and lets the user 
// access them via the public functions.
class Keyboard 
{
	// Needs acces to the keyboard to push events from the message pipeline.
	friend class MSGHandlePipeline;
public:
	// Keyboard event class that stores the event type and key.
	class event 
	{
	public:
		// Keyboard event type enumerator.
		enum Type: unsigned char
		{
			Pressed,	// Key pressed event
			Released,	// Key released event
			Invalid		// Invalid type event
		};

		Type type		= Invalid;	// Stores the event type
		char keyCode	= '\0';		// Stores the event key
	};

private:
	static constexpr unsigned maxBuffer = 64u;	// Maximum amount of events stored on buffer.
	static constexpr unsigned nKeys = 256u;		// Total number of keys stored by Keyboard.

	static inline bool autoRepeat = true;						// Whether the keyboard is on autorepead mode.
	static inline bool keyStates[nKeys] = { false };			// Stores the state of each key, true if pressed.
	static inline char* charBuffer[maxBuffer] = { nullptr };	// Buffer of characters pushed to the keyboard.
	static inline event* keyBuffer[maxBuffer] = { nullptr };	// Buffer of the events pushed to the keyboard.

	// Internal function triggered by the MSG Handle to set a key as pressed.
	static void setKeyPressed(unsigned char keycode);

	// Internal function triggered by the MSG Handle to set a key as released.
	static void setKeyReleased(unsigned char keycode);

	// Internal function triggered by the MSG Handle to clear all key states.
	static void clearKeyStates();

	// Internal function triggered by the MSG Handle to push a character to the buffer.
	static void pushChar(char character);

	// Internal function triggered by the MSG Handle to push an event to the buffer.
	static void pushEvent(event::Type type, unsigned char keycode);

public:
	// Toggles the autorepeat behavior on or off as specified.
	static void setAutorepeat(bool state);

	// Returns the current autorepeat behavior. (Default On)
	static bool getAutorepeat();

	// Clears the character and event buffers.
	static void clearBuffers();

	// Checks whether a key is being pressed.
	static bool isKeyPressed(unsigned char keycode);

	// Checks whether the char buffer is empty.
	static bool charIsEmpty();

	// Checks whether the event buffer is empty.
	static bool eventIsEmpty();

	// Pops last character of the buffer. To be used by an application event manager.
	static char popChar();

	// Pops last event of the buffer. To be used by an application event manager.
	static event popEvent();
};


/* MOUSE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
In order to develop user interaction in your apps it is important to have access
to the message pipeline coming into your window. For that reason this mouse
abstraction class stores the mouse events via the MSGHandlePipeline and gives
simple access to the user to those events.

It has a button state that keeps track of which mouse buttons are being pressed, 
and an event buffer to store the events coming from the message pipeline and be 
processed by your application.

It also stores a the mouse position relative to the window and to the screen.
And has a buffer that stores the mouse wheel movement, it can be accessed and 
reset by the user.

NOTE: When ImGui requests focus, all Mouse interactions are exclusively captured
by ImGui, this is to avoid unintentionally moving the plot while interacting with
the widgets. Only mouse position changes are still recorded.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Mouse static class, contains a set of user functions to create interactions between
// the mouse and your app. Stores events, button states and positions and lets the user 
// access them via the public functions.
class Mouse 
{
	// Needs acces to the Mouse to push events from the message pipeline.
	friend class MSGHandlePipeline;
public:
	// Mouse buttons enumerator.
	enum Button: unsigned
	{
		Left	= 0u,	// Left mouse button
		Right	= 1u,	// Right mouse buttos
		Middle	= 2u,	// Middle mouse button
		None	= 3u,	// No mouse button (used in move events)
	};

	// Mouse event class that stores the event type, button, 
	// and mouse position when the event was stored.
	class event 
	{
	public:
		// Mouse event type enumerator.
		enum Type: unsigned
		{
			Pressed,	// Button pressed event
			Released,	// Button released event
			Moved,		// Mouse moved event
			Wheel,		// Wheel moved event
			Invalid		// Invalid event
		};

		Vector2i position	= {};		// Stores the mouse position during event
		Type type			= Invalid;	// Stores the event type
		Button button		= None;		// Stores the event button
	};

private:
	static constexpr unsigned int maxBuffer = 64u;	// Maximum amount of events stored on buffer.
	static constexpr unsigned int nButtons = 4u;	// Total number of buttons stored by Mouse.

	static inline bool buttonStates[nButtons] = { false };		// Stores the state of each button, true if pressed.
	static inline event* buttonBuffer[maxBuffer] = { nullptr };	// Buffer of the events pushed to the mouse.
	static inline Vector2i Position = {};						// Current mouse position relative to window.
	static inline Vector2i ScPosition = {};						// Current mouse position relative to screen.
	static inline int deltaWheel = 0;							// Stores the movement of the mouse wheel.

	// Internal function triggered by the MSG Handle to set a button as pressed.
	static void setButtonPressed(Button button);

	// Internal function triggered by the MSG Handle to set a button as released.
	static void setButtonReleased(Button button);

	// Internal function triggered by the MSG Handle to set the current position relative to the window.
	static void setPosition(Vector2i Position);

	// Internal function triggered by the MSG Handle to set the current position relative to the screen.
	static void setScPosition(Vector2i Position);

	// Internal function triggered by the MSG Handle to add to the mouse wheel movement.
	static void increaseWheel(int delta);

	// Internal function triggered by the MSG Handle to push an event to the buffer.
	static void pushEvent(event::Type type, Button button);

public:
	// Sets the wheel movement back to 0. To be called after reading the wheel value.
	static void resetWheel();

	// Returns the current mouse wheel movement value. Does reset it.
	static int getWheel();

	// Returns the current mouse position in pixels relative to the window.
	static Vector2i getPosition();

	// Returns the current mouse position in pixels relative to the screen.
	static Vector2i getScPosition();

	// Checks whether a button is being pressed.
	static bool isButtonPressed(Button button);

	// Clears the mouse event buffer.
	static void clearBuffer();

	// Checks whether the event buffer is empty.
	static bool eventIsEmpty();

	// Pops last event of the buffer. To be used by an application event manager.
	static event popEvent();
};


/* GRAPHICS OBJECT CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the Graphics object and its functions. All windows contain their own 
graphics object which controls the GPU pointers regarding that window.

Also due to the nature of the API the class also contains the POV of the window, including 
a direction of view and a center. All drawables have acces to this buffer and the shaders 
are built accordingly. The direction is given by a quaternion and the center is given by a 
3D vector. They can be updated via class functions.

The graphics also defines two default bindables, these being the depth stencil state to be 
always DEPTH_STENCIL_MODE_DEFAULT, and the blender to be always BLEND_MODE_OPAQUE. Therefore 
if you design a drawable that does not require different settings you don't have to include 
those bindables. Every other bindable must be in every single drawable. Check bindable base
to see all the different bindables contained in this library.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Class to manage the global device shared by all windows and 
// graphics instances along the process.
class GlobalDevice
{
	friend class Bindable;
	friend class Graphics;
	friend class iGManager;
private:
	// Helper to delete the global device at the end of the process.
	static GlobalDevice helper;
	GlobalDevice() = default;
	~GlobalDevice();

	static inline void* globalDeviceData = nullptr; // Stores the device data masked as void*
	static inline bool skip_error = false;			// Whether to skip debug tools error
public:
	// Different GPU preferences following the IDXGIFactory6::EnumAdapterByGpuPreference
	// GPU distinction layout. 
	enum GPU_PREFERENCE
	{
		GPU_HIGH_PERFORMANCE,
		GPU_MINIMUM_POWER,
		GPU_UNSPECIFIED,
	};

	// Sets the global devica according to the GPU preference, must be set before
	// creating any window instance, otherwise it will be ignored.
	// If none is set it will automatically be created by the first window creation.
	static void set_global_device(GPU_PREFERENCE preference = GPU_HIGH_PERFORMANCE);

	// To be called by the user if it is using debug binaries to avoid D3D11 device 
	// no debug tools error message, must be called at the beggining of the code.
	static void skip_debug_tools_error() { skip_error = true; }

private:
	// Internal function that returns the ID3D11Device* masked as a void*.
	static void* get_device_ptr();

	// Internal function that returns the ID3D11DeviceContext* masked as void*
	static void* get_context_ptr();
};

// Contains the graphics information of the window, storing the GPU access pointers
// as well as the POV of the window, draw calls are funneled through this object.
class Graphics
{
	friend class Drawable;
	friend class Window;
	friend class MSGHandlePipeline;
public:
	// Before issuing any draw calls to the window, for multiple window settings 
	// this function has to be called to bind the window as the render target.
	void setRenderTarget();

	// Swaps the current frame and shows the new frame to the window.
	void pushFrame();

	// Clears the buffer with the specified color. If all buffers is false it will only clear
	// the screen color. the depth buffer and the transparency buffers will stay the same.
	void clearBuffer(Color color = Color::Transparent, bool all_buffers = true);

	// Clears the depth buffer so that all objects painted are moved to the back.
	// The last frame pixels are still on the render target.
	void clearDepthBuffer();

	// If OITransparency is enabled clears the two buffers related to OIT plotting.
	// IF it is not enabled it does nothing.
	void clearTransparencyBuffers();

	// Sets the perspective on the window, by changing the observer direction, 
	// the center of the POV and the scale of the object looked at.
	void setPerspective(Quaternion obs, Vector3f center, float scale);

	// Sets the observer quaternion that defines the POV on the window.
	void setObserver(Quaternion obs);

	// Sets the center of the window perspective.
	void setCenter(Vector3f center);

	// Sets the scale of the objects, defined as pixels per unit distance.
	void setScale(float scale);

	// Schedules a frame capture to be done during the next pushFrame() call. It expects 
	// a valid pointer to an Image where the capture will be stored. The image dimensions 
	// will be adjusted automatically. Pointer must be valid during next push call.
	// If ui_visible is set to false the capture will be taken before rendering imGui.
	void scheduleFrameCapture(Image* image, bool ui_visible = true);

	// To draw transparent objects this setting needs to be toggled on, it causes extra 
	// conputation due to other buffers being used for rendering, so only turn on if needed.
	// It uses the McGuire/Bavoli OIT approach. For more information you can check the 
	// original paper at: https://jcgt.org/published/0002/02/09/
	void enableTransparency();

	// Deletes the extra buffers and disables the extra steps when pushing frames.
	void disableTransparency();

	// Returns whether OITransparency is enabled on this Graphics object.
	bool isTransparencyEnabled() const;

	// Returns the current observer quaternion.
	inline Quaternion getObserver() const { return cbuff.observer; }

	// Returns the current Center POV.
	inline Vector3f getCenter() const { return Vector3f(cbuff.center); }

	// Returns the current scals.
	inline float getScale() const { return Scale; }

private:
	// Initializes the class data and calls the creation of the graphics instance.
	// Initializes all the necessary GPU data to be able to render the graphics objects.
	Graphics(void* hWnd);

	// Destroys the class data and frees the pointers to the graphics instance.
	~Graphics();

	// To be called by drawable objects during their draw calls, issues an indexed 
	// draw call drawing the object to the render target., If the object is transparent 
	// redirects it to the accumulation targets for later composing. At the end returns 
	// to default Blender and DepthStencil states.
	static void drawIndexed(unsigned IndexCount, bool isOIT);

	// To be called by its window. When window dimensions are updated it reshapes its
	// buffers to match the new window dimensions, as specified by the vector.
	void setWindowDimensions(const Vector2i Dim);

	// No copies of a graphics instance are allowed.
	Graphics(Graphics&&) = delete;
	Graphics& operator=(Graphics&&) = delete;
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;

private:
	void* GraphicsData = nullptr; // Stores the graphics object internal data.

	static inline Graphics* currentRenderTarget = nullptr; // Stores the pointer to the current graphics target.

	struct // Constant buffer of the current graphics perspective, accessable to all vertex shaders.
	{
		Quaternion observer = 1.f;	// Stores the current observer direction.
		_float4vector center = {};	// Stores the current center of the POV.
		_float4vector scaling = {};	// Scaling values for the shader
	}cbuff;

	Vector2i WindowDim;			// Stores the current window dimensions.
	float Scale = 250.f;		// Stores the current view scale.
};


/* WINDOW OBJECT CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
To create a window in your desktop just create a Window object and you have it! 
To close the window just delete the object and it is gone. No headaches, and no 
non-sense.

The window class deals with all the Win32 API in the background, and allows for a 
flexible window design, with multiple functions to customize the window to your 
liking, check out the class to learn about all its functionalities.

To know if a window close button has been pressed or close() has been called the 
processEvents() will return the ID of the window that wants to close.

This API also supports multiple window settings. For proper opening and closing I 
suggest holding pointers to windows and a counter, and deleting the windows when 
the global process events returns their ID.

For App creation this API comes with a set of default Drawables and a Graphics 
object contained inside the window, for an example on how to use the library you 
can check the demo apps or the default helpers header.

For further information check the github page: 
https://github.com/MiquelNasarre/Chaotic.git
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Window descriptor struct, to be created and passed as a pointer to initialize
// a Window. Contains some basic initial setting for the window as well as the 
// mode toggle that allows you to create a desktop background window.
struct WINDOW_DESC
{
	// Initial window title.
	char window_title[512] = "Chaotic Window";

	// Window mode, If set to wallpaper it will default to the back of 
	// the desktop at full screen.
	// NOTE: A wallpaper window does not take focus, so no messages will 
	// be processed, other methoes must be used for interaction. For example,
	// an active console, another window or a limited lifespan.
	// NOTE: You can swap the monitor display by using setWallpaperMonitor, 
	// other reshaping functions will error. It does not adjust automatically 
	// to settings changes, so setWallpaperMonitor must be called to re-adjust.
	enum WINDOW_MODE
	{
		WINDOW_MODE_NORMAL,
		WINDOW_MODE_WALLPAPER
	}
	window_mode = WINDOW_MODE_NORMAL;

	// Initial window dimensions.
	Vector2i window_dim = { 720, 480 };

	// Initial window icon file path. If none provided defaults.
	char icon_filename[512] = "";

	// Initial window theme.
	bool dark_theme = true;

	// Whether the wallpaper persists past the window lifetime. 
	// NOTE: It will persist until the desktop flushes itself, 
	// due to reshaping, restart, etc.
	bool wallpaper_persist = false;

	// Selects the monitor where the wallpaper will be shown. If -1
	// then the wallpaper window will expand to all monitors in use.
	int monitor_idx = 0;
};

// The window class contains all the necessary functions to create and use a window.
// Since the library is built for 3D apps it also contains a graphics object and 
// default drawables to choose from to create any graphics App from scratch.
class Window
{
	// Class that handles the user updates to the window.
	friend class MSGHandlePipeline;
	friend class Graphics;
	friend class iGManager;
private:
	static inline unsigned next_id = 1u;	// Stores the next ID assigned to a window
	const unsigned w_id;					// Stores the window ID.
public:
	// Returns a reference to the window internal Graphics object.
	Graphics& graphics();
	// Returns a constant reference to the window internal Graphics object.
	const Graphics& graphics() const;

	// Function that may be called every frame during the App runtime. It manages the
	// message pipeline, pushing events to the Mouse and Keboard for user interaction,
	// sending them to ImGui if required, and doing important window internal updates.
	// It also manages the framerate, waiting the correct amount of time to keep it 
	// stable. 
	// The function returns 0 during normal runtime, but if a window close button has
	// been pressed or its close function has been called it will return the ID of the
	// corresponding window, so that the App can safely delete the object if it pleases.
	static unsigned processEvents();

	// Equivalent to pressing the close button on the window. Sends a message to the
	// handler and process events will catch it and return this window ID so that the
	// user can close it if he pleases. The only way of closing a window properly is 
	// through the desctructor.
	void close() const;

public:
	// --- CONSTRUCTOR / DESTRUCTOR ---

	// Creates the window and its associated Graphics, expects a valid descriptor 
	// pointer, if not provided it chooses the default descriptor settings.
	Window(const WINDOW_DESC* pDesc = nullptr);

	// Handles the proper deletion of the window data after its closing.
	~Window();

	// --- GETTERS / SETTERS ---

	// Returs the window ID. 
	// The one posted by process events when close button pressed.
	unsigned getID() const;

	// Checks whether the window has focus.
	bool hasFocus() const;

	// Sets the focus to the current window.
	void requestFocus();

	// Enumerator for all the defalut type of cursors. To see the cursor each one
	// produces, they reproduce the original Win32 macros found at the website 
	// URL: https://learn.microsoft.com/en-us/windows/win32/menurc/about-cursors
	enum class CURSOR
	{
		ARROW      , IBEAM   , WAIT    , CROSS , 
		UPARROW    , SIZENWSE, SIZENESW, SIZEWE,
		SIZENS     , SIZEALL , NO      , HAND  , 
		APPSTARTING, HELP    , PIN     , PERSON,
	};

	// Sets the window cursor to the one specified.
	void setCursor(CURSOR cursor);

	// Set the title of the window, allows for formatted strings.
	void setTitle(const char* name, ...);

	// Sets the icon of the window via its filename (Has to be a .ico file).
	void setIcon(const char* filename);

	// Sets the dimensions of the window to the ones specified.
	void setDimensions(Vector2i Dim);

	// Sets the position of the window to the one specified.
	void setPosition(Vector2i Pos);

	// Selects the monitor where the wallpaper will be shown. If -1
	// then the wallpaper window will expand to all monitors in use.
	void setWallpaperMonitor(int monitor_idx);

	// In the event of a failed Wallpaper creation the window fallsback to 
	// regular. This function returns whether the window is in wallpaper mode.
	bool isWallpaperWindow() const;

	// For the wallpaper mode, it tells you if a specific monitor 
	// exists or not. Expand option, -1, is not considered a monitor.
	static bool hasMonitor(int monitor_idx);

	// Toggles the dark theme of the window on or off as specified.
	void setDarkTheme(bool DARK_THEME);

	// Toggles the window on or off of full screen as specified.
	void setFullScreen(bool FULL_SCREEN);

	// Returns a string pointer to the title of the window.
	const char* getTitle() const;

	// Returns the dimensions vector of the window.
	Vector2i getDimensions() const;

	// Returns the position vector of the window.
	Vector2i getPosition() const;

	// Sets the maximum framerate of the process to the one specified.
	// This framerate is controlled by the process events, after every
	// processing run if the time since last frame is too short it will 
	// wait to keep the framerate stable.
	static void setFramerateLimit(int fps);

	// Returs the current framerate of the process.
	static float getFramerate();

public:
	// --- GRAPHICS OVERLOADED CALLS FOR SIMPLICITY ---

	// Original method on the graphics class. Sets this window as render target.
	inline void setRenderTarget()														{ graphics().setRenderTarget(); }
	// Original method on the graphics class. Shows the new frame to the window.
	inline void pushFrame()																{ graphics().pushFrame(); }
	// Original method on the graphics class. Clears the buffer with the specified color.
	inline void clearBuffer(Color color = Color::Transparent, bool all_buffers = true)	{ graphics().clearBuffer(color, all_buffers); }
	// Original method on the graphics class. Clears the depth buffer.
	inline void clearDepthBuffer()														{ graphics().clearDepthBuffer(); }
	// Original method on the graphics class. Clears the buffers used for transparencies.
	inline void clearTransparencyBuffers()												{ graphics().clearTransparencyBuffers(); }
	// Original method on the graphics class. Updates the perspective on the window.
	inline void setPerspective(Quaternion obs, Vector3f center, float scale)			{ graphics().setPerspective(obs, center, scale); }
	// Original method on the graphics class. Sets the observer quaternion.
	inline void setObserver(Quaternion obs)												{ graphics().setObserver(obs); }
	// Original method on the graphics class. Sets the center of the window perspective.
	inline void setCenter(Vector3f center)												{ graphics().setCenter(center); }
	// Original method on the graphics class. Sets the scale of the objects.
	inline void setScale(float scale)													{ graphics().setScale(scale); }
	// Original method on the graphics class. Schedules a frame capture to be done.
	inline void scheduleFrameCapture(Image* image, bool ui_visible = true)				{ graphics().scheduleFrameCapture(image, ui_visible); }
	// Original method on the graphics class. Allows transparent drawables.
	inline void enableTransparency()													{ graphics().enableTransparency(); }
	// Original method on the graphics class. Disallows transparent drawables.
	inline void disableTransparency()													{ graphics().disableTransparency(); }
	// Original method on the graphics class. Tells you whether transparenzy is allowed.
	inline bool isTransparencyEnabled() const											{ return graphics().isTransparencyEnabled(); }
	// Original method on the graphics class. Returns the current observer quaternion.
	inline Quaternion getObserver() const												{ return graphics().getObserver(); }
	// Original method on the graphics class. Returns the current Center POV.
	inline Vector3f getCenter() const													{ return graphics().getCenter(); }
	// Original method on the graphics class. Returns the current scals.
	inline float getScale() const														{ return graphics().getScale(); }

private:
	// --- INTERNALS ---

	// Returns the HWND of the window.
	void* getWindowHandle();

	// Waits for a certain amount of time to keep the window
	// running stable at the desired framerate.
	static void handleFramerate();
#ifdef _INCLUDE_IMGUI
	// Returns adress to the pointer to the iGManager bound to the window.
	// This is to be accessed by the iGManager and set the data accordingly.
	void** imGuiPtrAdress();
#endif
	// Contains the internal data used by the window.
	void* WindowData = nullptr;

	// No Window copies are allowed
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
};

#ifdef _CHAOTIC_CUSTOMS // If included from chaotic_customs.h the Bindable class nees to be defined here

/* BINDABLE OBJECT BASE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the base class for all bindable objects.
Minimal class includes a virtual binding function to be defined
by each bindable and a default virtual deleter.

See bindable object source files for reference on how to implement 
different bindable classes.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Bindable base class, contains the minimal functions expeted by 
// any bindable object to have.
class Bindable
{
public:
	// Bind function to be defined by each bindable object.
	virtual void Bind() = 0;

	// Deleter function optionally overwritten by inherited class.
	virtual ~Bindable() = default;

protected:
	// Pure virtual class.
	Bindable() = default;

	// Helper function that returns the pointer to the global device.
	static void* device() { return GlobalDevice::get_device_ptr(); }

	// Helper function that returns the pointer to the global context.
	static void* context() { return GlobalDevice::get_context_ptr(); }

private:
	// Bindable copies or move operations are not allowed.
	Bindable(const Bindable&) = delete;
	Bindable operator=(const Bindable&) = delete;
	Bindable(Bindable&&) = delete;
	Bindable operator=(Bindable&&) = delete;
};

#endif // _CHAOTIC_CUSTOMS

/* DRAWABLE OBJECT BASE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the base class for all drawable objects.
Minimal class includes a virtual draw function optionally to be
overwritten. Some internal functions and internal data to store
the bindables.

See drawable object source files for reference on how to implement
different drawable classes.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Drawable base class, contains minimal functions to be 
// used by any drawable object.
class Drawable
{
protected:
	// Constructor, allocates space to store the bindables.
	Drawable();

	// Helper for inheritance to access current render target.
	Graphics* currentTarget() { return Graphics::currentRenderTarget; }
public:
	// Destructor, deletes allovated storage space.
	virtual ~Drawable();

	// Drawable copies or move operations are not allowed.
	Drawable(const Drawable&) = delete;
	Drawable operator=(const Drawable&) = delete;
	Drawable(Drawable&&) = delete;
	Drawable& operator=(Drawable&&) = delete;

	// Virtual drawing function, checks that the object is initialized
	// and calls the internal draw function.
	virtual void Draw();

protected:
	// Boolean to make sure the drawable has been initialized.
	bool isInit = false;

	// Internal draw function, to be called by any overwriting of the
	// main Draw() function, iterates through the bindable objects
	// list and once all are bind, issues a draw call to the window.
	void _draw() const;

#ifdef _CHAOTIC_CUSTOMS
	// Templated AddBind, helper to return the original class pointer.
	// Adds a new bindable to the bindable list of the object. For proper
	// memory management the bindable sent to this function must be allocated
	// using new(), and the deletion must be left to the drawable management.
	template <class BindableType>
	inline BindableType* AddBind(BindableType* bind)
	{
		return (BindableType*)AddBind((Bindable*)bind);
	}

	// Adds a new bindable to the bindable list of the object. For proper
	// memory management the bindable sent to this function must be allocated
	// using new(), and the deletion must be left to the drawable management.
	Bindable* AddBind(Bindable* bind);

	// Changes an existing bindable from the bindable list of the object. For proper
	// memory management the bindable sent to this function must be allocated
	// using new(), and the deletion must be left to the drawable management.
	Bindable* changeBind(Bindable* bind, unsigned N, bool delete_replaced = true);
#endif
private:
	void* DrawableData; // Stores the internal data of the drawable.
};


/* BACKGROUND DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Drawable class to draw backgrounds. If you want to see an image or a texture in the back
this drawable allows for easy creation of both static and 3D backgrounds that can be drawn
back the Graphics of any window.

For static backgrounds the image you upload will render directly in the back of your window
when drawed. You can select a sub-section of the image to be rendered by updateRectagle.

For dynamic backgrounds the image is expected to be a cube-map, and will render as a 3D scene
in the background when drawn. This scene will follow the default perspective of the Window, but
you can also update the rotation of the background scene using updateRotation. For information
on how to create a cube-map image check the Texture bindable header.

The Image header contains a small library of functions called ToCube, for simple conversion
from common spherical projections to cube-maps. I strongly recommend checking them out if you
have other type of spherical projected images.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Background descriptor struct, to be created and passed as a pointer to initialize
// a Background. it allows for different usage settings including the static/dynamic 
// enum. The image pointer memory is to be managed by the user, the creation function 
// will not modify or store it.
struct BACKGROUND_DESC
{
	// Expects a valid Image pointer that will be used to initialize the background
	// texture. If the background is dynamic it expects valid cube-map dimensions.
	Image* image = nullptr;

	// Wether the background is static or dynamic.
	enum BACKGROUND_TYPE
	{
		STATIC_BACKGROUND,
		DYNAMIC_BACKGROUND,
	}
	type = STATIC_BACKGROUND; // Defaults to static

	// Whether the Background allows for texture updates, leave as false if 
	// you do not intend to use the updateTexture() function.
	bool texture_updates = false;

	// Whether the Background texture is pixelated or linearly interpolates.
	bool pixelated_texture = false;

	// If true, when drawn it will override whatever is on the depth buffer and render
	// target, basically clearing the screen with the background. If drawn first, set 
	// to true and clearBuffer() is not necessary. Better for performance.
	// If using OIT clearTransparencyBuffers() is still necessary.
	bool override_buffers = false;
};

// Background drawable class, used for drawing and interaction with textured backgrounds
// on a Graphics instance. Allows for static image or dynamic scenery backgrounds, check
// the descriptor and the user functions for further detail.
class Background : public Drawable
{
public:
	// Background constructor, if the pointer is valid it will call the initializer.
	Background(const BACKGROUND_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Background();

	// Initializes the Background object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const BACKGROUND_DESC* pDesc);

	// If updates are enabled, this function allows to update the background texture.
	// Expects a valid image pointer to the new Background image, with the same dimensions
	// as the image used in the constructor.
	void updateTexture(const Image* image);

	// If the Background is dynamic, it updates the rotation quaternion of the scene. If 
	// multiplicative it will apply the rotation on top of the current rotation. For more 
	// information on how to rotate with quaternions check the Quaternion header file.
	void updateRotation(Quaternion rotation, bool multiplicative = false);

	// If the Background is dynamic, it updates the field of view of the Background.
	// By defalut the Vector is (1.0,1.0) and 1000 window pixels correspond to one cube 
	// arista, in both x and y coordinates. FOV does not get affected by graphics scale.
	void updateFieldOfView(Vector2f FOV);

	// If the background is static it updates the rectangle of Image pixels drawn. By 
	// default it draws the entire image: min->(0,0) max->(width,height).
	// The dimensions are in image pixels, intermediate positions are allowed.
	void updateRectangle(Vector2f min_coords, Vector2f max_coords);

	// Returns the Texture image dimensions.
	Vector2i getImageDim() const;

	// Returns the current rotation quaternion.
	Quaternion getRotation() const;

	// Returns the current field of view.
	Vector2f getFieldOfView() const;

private:
	// Pointer to the internal class storage.
	void* backgroundData = nullptr;
};


/* CURVE DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Drawable class to draw single parameter 3D functions as curves. Given the math renderer 
nature of this library it is mandatory that we have a drawable meant for drawing curves
in the 3D space, and that is exactly what this class does.

As it is standard on this library it has multiple setting to set the object rotation,
position, linear distortion, and screen shifting and it is displayed in relation to the
perspective of the Graphics currently set as render target.

It allows for transparencies, and function and coloring updates. For information on how 
to handle transparencies you can check the Graphics header.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Curve descriptor struct, to be created and passed as a pointer to initialize
// a Curve object, it allows for different coloring and rendering settings. The 
// pointers memory is to be managed by the user. If updates are intended, function
// pointers are expected to still be callable during update calls.
struct CURVE_DESC
{
	// Expects a pointer to a valid function to generate the vertices.
	Vector3f(*curve_function)(float) = nullptr;

	// Range where the curve function will be called to generate vertices.
	Vector2f range = { -1.f,1.f };

	// Number of total vertices calculated by the function.
	unsigned vertex_count = 200u;

	// Specifies how the coloring will be done for the Curve.
	enum CURVE_COLORING
	{
		FUNCTION_COLORING,
		LIST_COLORING,
		GLOBAL_COLORING
	}
	coloring = GLOBAL_COLORING; // Defaults to global color

	// If coloring is global, sets the color of the curve.
	Color global_color = Color::White;

	// If coloring is functional, expects a valid function to generate the 
	// colors of each vertex given the same float values as the vertexs.
	Color(*color_function)(float) = nullptr;

	// If coloring is with a list, expects a valid pointer to a list of 
	// colors as long as the vertex count.
	Color* color_list = nullptr;

	// Sets Order Indepentdent Transparency for the Curve. Check 
	// Graphics.h or Blender.h for more information on how to use it.
	bool enable_transparency = false;

	// Whether the Curve allows for shape updates, leave at false if you 
	// don't intend to update the shape of the Curve. Only the functions 
	// updateVertices() and updateColors() require it.
	bool enable_updates = false;

	// Whether the edge values of the range are included in the set.
	bool border_points_included = true;
};

// Curve drawable class, used for drawing and interacting with user defined single parameter
// functions on a Graphics instance. Allows for updates to be able to represent moving curves
// and supports different coloring settings. Check the descriptor and class for more information.
class Curve : public Drawable
{
public:
	// Curve constructor, if the pointer is valid it will call the initializer.
	Curve(const CURVE_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Curve();

	// Initializes the Curve object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const CURVE_DESC* pDesc);

	// If updates are enabled this function allows to change the range of the curve function. It 
	// expects the initial function pointer to still be callable, it will evaluate it on the new 
	// range and send the vertices to the GPU. If coloring is functional it also expects the color 
	// function to still be callable, if coloring is not functional it will reuse the old colors.
	void updateRange(Vector2f range = {});

	// If updates are enabled, and coloring is with a list, this function allows to change 
	// the current vertex colors for the new ones specified. It expects a valid pointer 
	// with a list of colors as long as the vertex count.
	void updateColors(Color* color_list);

	// If the coloring is set to global, updates the global Curve color.
	void updateGlobalColor(Color color);

	// Updates the rotation quaternion of the Curve. If multiplicative it will apply
	// the rotation on top of the current rotation. For more information on how to rotate
	// with quaternions check the Quaternion header file.
	void updateRotation(Quaternion rotation, bool multiplicative = false);

	// Updates the scene position of the Curve. If additive it will add the vector
	// to the current position vector of the Curve.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the matrix multiplied to the Curve, adding any arbitrary linear distortion to 
	// it. If you want to simply modify the size of the object just call this function on a 
	// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
	// If multiplicative the distortion will be added to the current distortion.
	void updateDistortion(Matrix distortion, bool multiplicative = false);

	// Updates the screen displacement of the figure. To be used if you intend to render 
	// multiple scenes/plots on the same render target.
	void updateScreenPosition(Vector2f screenDisplacement);

	// Returns the current rotation quaternion.
	Quaternion getRotation() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current distortion matrix.
	Matrix getDistortion() const;

	// Returns the current screen position.
	Vector2f getScreenPosition() const;

private:
	// Pointer to the internal class storage.
	void* curveData = nullptr;
};


/* LIGHT DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Drawable class to draw point light sources in the scene. Given this library supports 
illumination it is only natural to have a drawable to represent such light sources.

This object allows for drawing of light sources anywhere in you scene with any color or
intensity. Note that the are not actual light sources for other drawables and you will 
have to call their own updateLight() function to add any additional light.

Lights should be drawn after all other drawables have been drawn and thay are single 
point fully transparent light sources, so they only add light to the pixels.

Unfortunately lights have poor interaction with transparent objects. Due to the nature
of OIT the light glow state can not be reconciled, so lights will always appear behind 
transparent objects even if they are acutally in front.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Light descriptor struct, to be created and passed as a pointer to initialize
// a Light object. Sets the ligth color, position, intensity and misc.
struct LIGHT_DESC
{
	// Defines the color of the light source. Alpha value is 
	// ignored because lighting is always additive.
	Color color = Color::White;

	// Defines the R3 position of the light source.
	Vector3f position = {};
	
	// Defines the intensity of the light source.
	float intensity = 1.f;
	
	// Defines the radius in 3D coordinates of the circle the
	// drawable will call the pixel shader on to add the light.
	float radius = 1.f;

	// Defines the number of sides the poilhedron used to create 
	// the circle will have. Minimum value is three.
	unsigned poligon_sides = 40u;
};

// Light drawable class, used for drawing and interacting with single point transparent light 
// sources on a Graphics instance. Allows for color, position and intensity updates.
class Light : public Drawable
{
public:
	// Light constructor, if the pointer is valid it will call the initializer.
	Light(const LIGHT_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Light();

	// Initializes the Light object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const LIGHT_DESC* pDesc);

	// Updates the intensity value of the light.
	void updateIntensity(float intensity);

	// Updates the scene position of the Light. If additive it will add the vector
	// to the current position vector of the Light.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the color of the Light.
	void updateColor(Color color);

	// Updates the maximum radius of the Light.
	void updateRadius(float radius);

	// Returns the current light intensity.
	float getIntensity() const;

	// Returns the current light color.
	Color getColor() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current maximum radius.
	float getRadius() const;

private:
	// Pointer to the internal class storage.
	void* lightData = nullptr;
};


/* POLYHEDRON DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Drawable class to draw triangle meshes, it is a must for any rendering library and in
this case here we have it. To initialize it, it expects a valid pointer to a descriptor
and will create the Shape with the specified data.

As it is standard on this library it has multiple setting to set the object rotation,
position, linear distortion, and screen shifting and it is displayed in relation to the 
perspective of the Graphics currently set as render target.

It allows for illumination, texturing, transparencies and figure updates. For information 
on how to handle transparencies you can check the Graphics header. For information on how 
to create images for the texture you can check the Image class header.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Polyhedron descriptor struct, to be created and passed as a pointer to initialize
// a polyhedron, it allows for different coloring and rendering settings. The 
// pointers memory is to be managed by the user, the creation function will not 
// modify or store any of the original pointers in the descriptor.
struct POLYHEDRON_DESC
{
	// Expects a valid pointer to a list of vertices. The list must be 
	// as long as the highest index found in the triangle list.
	Vector3f* vertex_list = nullptr;

	// Expects a valid pointer to a list of oriented triangles in integer
	// format that will be interpreted as a Vector3i pointer. Must be as
	// long as three intergers times the triangle count.
	Vector3i* triangle_list = nullptr;

	// Number of triangles that form the Polyhedron.
	unsigned triangle_count = 0u;

	// Specifies how the coloring will be done for the Polyhedron.
	enum POLYHEDRON_COLORING
	{
		TEXTURED_COLORING,
		PER_VERTEX_COLORING,
		GLOBAL_COLORING
	} 
	coloring = GLOBAL_COLORING; // Defaults to global color

	// If coloring is set to global the shape will have this color.
	Color global_color = Color::White;

	// If coloring is set to per vertex it expects a valid pointer 
	// to a list of colors containing one color per every vertex
	// of every triangle. Three times the triangle count.
	Color* color_list = nullptr;

	// If coloring is set to textured it expects a valid pointer to 
	// an image containing the texture to be used by the Polyhedron.
	Image* texture_image = nullptr;

	// If coloring is set to textured it expects a valid pointer to 
	// a list of pixel coordinates containing one coordinate per every
	// vertex of every triangle. Three times the traiangle count.
	Vector2i* texture_coordinates_list = nullptr;

	// If the Polyhedron is illuminated it specifies how the normal vectors will be obtained.
	enum POLYHEDRON_NORMALS
	{
		// The normal vectors will be computed by triagle and assigned to each vertex 
		// accordingly, grid pattern is clearly visible unless the grid is very thin.
		COMPUTED_TRIANGLE_NORMALS,
		// Each vertex has its own normal vector attached, and that one will be used
		// across all its triangle connections. The list must be provided.
		PER_VERTEX_LIST_NORMALS,
		// Each vertex appearence on each triangle will have a different normal from
		// the normal vector list. So the list must be as long as three times the number
		// of triangles.
		PER_TRIANGLE_LIST_NORMALS,
	}
	normal_computation = COMPUTED_TRIANGLE_NORMALS; // Defaults to computation.

	// IF normals are not computed this variable is expected to contain a valid list
	// of normal vectors as long as the vertex count in case of per vertex normals, 
	// or as long a three times the triangle count in case of per triangle normals.
	Vector3f* normal_vectors_list = nullptr;

	// Whether both sides of each triangle are rendered or not.
	bool double_sided_rendering = true;

	// Whether the polyhedron uses illumination or not.
	bool enable_illuminated = true;

	// Sets Order Indepentdent Transparency for the Polihedrom. Check 
	// Graphics.h or Blender.h for more information on how to use it.
	bool enable_transparency = false;

	// Whether the polyhedron allows for shape updates, leave at false if 
	// you don't intend to update the shape of the Polyhedron. Only the functions 
	// updateVertices(), updateColors(), updateTextureCoordinates() require it.
	bool enable_updates	= false;

	// IF true renders only the aristas of the Polyhedron.
	bool wire_frame_topology = false;

	// Uses a nearest point sampler instead of a linear one.
	bool pixelated_texture = false;

	// By default polyhedrons and surfaces are lit by four different color lights
	// around the center of coordinates, allows for a nice default that illuminates
	// everything and distiguishes different areas, disable to set all to black.
	bool default_initial_lights = true;
};

// Polyhedron drawable class, used for drawing and interacting with user defined triangle 
// meshes on a Graphics instance. Allows for different rendering settings including but not 
// limited to textures, illumination, transparencies. Check the descriptor to see all options.
class Polyhedron : public Drawable
{
public:
	// To facilitate the loading of triangle meshes, this function is a parser for 
	// *.obj files, that reads the files and outputs a valid descriptor. If the file 
	// supports texturing, optionally accepts an image to be used as texture_image.
	// NOTE: All data is allocated by (new) and its deletion must be handled by the 
	// user. The image pointer used is the same as provided..
	static POLYHEDRON_DESC getDescFromObj(const char* obj_file_path, Image* texture = nullptr);

public:
	// Polyhedron constructor, if the pointer is valid it will call the initializer.
	Polyhedron(const POLYHEDRON_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Polyhedron();

	// Initializes the Polyhedron object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const POLYHEDRON_DESC* pDesc);

	// If updates are enabled this function allows to change the current vertex positions
	// for the new ones specified. It expects a valid pointer with a list as long as the 
	// highest index found in the triangle list used for initialization.
	void updateVertices(const Vector3f* vertex_list);

	// If updates are enabled, and coloring is per vertex, this function allows to change 
	// the current vertex colors for the new ones specified. It expects a valid pointer 
	// with a list of colors containing one color per every vertex of every triangle. 
	// Three times the triangle count.
	void updateColors(const Color* color_list);

	// If normals are provided this function allows to update the normal vectors in the 
	// same satting that the Polyhedron was initialized on. It expects a valid list of
	// normal vectors of the correspoding lenght.
	void updateNormals(const Vector3f* normal_vectors_list);

	// If updates are enabled, and coloring is texured, this functions allows o change the 
	// current vertex texture coordinates for the new ones specified. It expects a valid 
	// pointer with a list of pixels containing one coordinates per every vertex of every
	// triangle. Three times the triangle count.
	void updateTextureCoordinates(const Vector2i* texture_coordinates_list);

	// If the coloring is set to global, updates the global Polyhedron color.
	void updateGlobalColor(Color color);

	// Updates the rotation quaternion of the Polyhedron. If multiplicative it will apply
	// the rotation on top of the current rotation. For more information on how to rotate
	// with quaternions check the Quaternion header file.
	void updateRotation(Quaternion rotation, bool multiplicative = false);

	// Updates the scene position of the Polihedrom. If additive it will add the vector
	// to the current position vector of the Polyhedron.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the matrix multiplied to the Polihedrom, adding any arbitrary linear distortion 
	// to it. If you want to simply modify the size of the object just call this function on a 
	// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
	// If multiplicative the distortion will be added to the current distortion.
	void updateDistortion(Matrix distortion, bool multiplicative = false);

	// Updates the screen displacement of the figure. To be used if you intend to render 
	// multiple scenes/plots on the same render target.
	void updateScreenPosition(Vector2f screenDisplacement);

	// If illumination is enabled it sets the specified light to the specified parameters.
	// Eight lights are allowed. And the intensities are directional and diffused.
	void updateLight(unsigned id, Vector2f intensities, Color color, Vector3f position);

	// If illumination is enabled clears all lights for the Polyhedron.
	void clearLights();

	// If illumination is enabled, to the valid pointers it writes the specified lights data.
	void getLight(unsigned id, Vector2f* intensities, Color* color, Vector3f* position);

	// Returns the current rotation quaternion.
	Quaternion getRotation() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current distortion matrix.
	Matrix getDistortion() const;

	// Returns the current screen position.
	Vector2f getScreenPosition() const;
	
private:
	// Pointer to the internal class storage.
	void* polyhedronData = nullptr;
};


/* SCATTER DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Drawable class to draw scatter plots on 3D space. Who said that point simulations are 
not fun? In my opinion having a math 3D library demands the possibility of running all
kinds of simulations on it including particle simulations. This class allows for that.
Just give it a list of points, a list of colors and you will have them on your screen.
You can also connect them by pairs and make a line-mesh!!

As it is standard on this library it has multiple settings to set the object rotation,
position, linear distortion, and screen shifting. And it is displayed in relation to the
perspective of the Graphics currently set as render target.

It allows for a glow effect, similar to how the light functions, for long list of points
simulations that you might want to stack color on top of each other. And allows transparency. 
If you want proper interaction with other objects, opaque objects must be drawn first.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Scatter descriptor struct, to be created and passed as a pointer to initialize
// a Scatter object, it allows for different coloring and blending settings. The 
// pointers memory is to be managed by the user, Scatter will not modify them or
// use them past the initializer function.
struct SCATTER_DESC
{
	// Expects a valid pointer to a list of points. 
	// The list must be as long as the point count.
	Vector3f* point_list = nullptr;

	// Number of points in the point list.
	unsigned point_count = 0u;

	// Specifies how the coloring will be done for the Scatter.
	enum SCATTER_COLORING
	{
		POINT_COLORING,
		GLOBAL_COLORING
	}
	coloring = GLOBAL_COLORING; // Defaults to global color

	// If coloring is set to global points will have this color.
	Color global_color = Color::White;

	// If coloring is set to per point it expects a valid pointer 
	// to a list of colors containing one color per every point.
	Color* color_list = nullptr;

	// Specifies how the color of the points will blend with the render target.
	enum BLENDING_MODE
	{
		TRANSPARENT_POINTS,
		OPAQUE_POINTS,
		GLOWING_POINTS,
	}
	blending = GLOWING_POINTS;	// Defaults to glow effect

	// Whether the scatter allows for list updates, leave at false if you 
	// don't intend to update the position of the points. Only the functions 
	// updatePoints() and updateColors() require it.
	bool enable_updates = false;

	// If true it will assume the points are a list of lines by pairs and 
	// create a line-mesh out of them. Point count must be divisible by two.
	bool line_mesh = false;
};

// Scatter drawable class, used for drawing and interaction with 3D user created 
// scatter plots in a Graphics instance. Perfect for particle simulations! Allows
// for position and color updates. Check the descriptor and class functions for
// more information about the class capabilities.
class Scatter : public Drawable
{
public:
	// Scatter constructor, if the pointer is valid it will call the initializer.
	Scatter(const SCATTER_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Scatter();

	// Initializes the Scatter object, it expects a valid pointer to a descriptor
	// and will initialize everything as specified, can only be called once per object.
	void initialize(const SCATTER_DESC* pDesc);

	// If updates are enabled this function allows to change the position of the points.
	// It expects a valid pointer to a 3D vector list as long as the point count, it will 
	// copy the position data and send it to the GPU for drawing.
	void updatePoints(Vector3f* point_list);

	// If updates are enabled, and coloring is with a list, this function allows to change 
	// the current point colors for the new ones specified. It expects a valid pointer 
	// with a list of colors as long as the point count.
	void updateColors(Color* color_list);

	// If the coloring is set to global, updates the global Scatter color.
	void updateGlobalColor(Color color);

	// Updates the rotation quaternion of the Scatter. If multiplicative it will apply
	// the rotation on top of the current rotation. For more information on how to rotate
	// with quaternions check the Quaternion header file.
	void updateRotation(Quaternion rotation, bool multiplicative = false);

	// Updates the scene position of the Scatter. If additive it will add the vector
	// to the current position vector of the Scatter.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the matrix multiplied to the Scatter, adding any arbitrary linear distortion to 
	// the points position. If you want to simply modify the size of the scatter just call this 
	// function on a diagonal matrix. Check the Matrix header for helpers to create any arbitrary 
	// distortion. If multiplicative the distortion will be added to the current distortion.
	void updateDistortion(Matrix distortion, bool multiplicative = false);

	// Updates the screen displacement of the points. To be used if you intend to render 
	// multiple scenes/plots on the same render target.
	void updateScreenPosition(Vector2f screenDisplacement);

	// Returns the current rotation quaternion.
	Quaternion getRotation() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current distortion matrix.
	Matrix getDistortion() const;

	// Returns the current screen position.
	Vector2f getScreenPosition() const;

private:
	// Pointer to the internal class storage.
	void* scatterData = nullptr;
};


/* SURFACE DRAWABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Given the nature of the library, this being a math renderer, this is the most important 
and diverse drawable of all. It is meant for anybody to use and create the most ridiculous
and exciting ideas they want to see in 3D. It provides as much flexibility as I can offer
to create mathematical surfaces in 3D and visualize them in meny different ways.

Has multiple generation modes you can find in the descriptor and also allows for updates 
if enabled, making it very easy to generate real-time moving surfaces in your window.
It also has different coloring and normal vector setting to fully customize your plot.

As it is standard on this library it has multiple setting to set the object rotation,
position, linear distortion, and screen shifting and it is displayed in relation to the
perspective of the Graphics currently set as render target.

It allows for illumination, texturing, transparencies and many more settings. For information
on how to handle transparencies you can check the Graphics header. For information on how
to create images for the texture you can check the Image class header.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Surface descriptor struct, to be created and passed as a pointer to initialize
// a surface, it allows for different generation functions, coloring and rendering 
// settings. The pointers memory is to be managed by the user, the creation function 
// will not modify or store any of the original pointers in the descriptor.
struct SURFACE_DESC
{
	// Specifies the type of function that will be used to generate the surface.
	enum SURFACE_TYPE
	{
		// Uses f(x,y) functions to obtain the z value.
		EXPLICIT_SURFACE,
		// Uses r(x.y.z) where x,y,z in S2 funtions to obtain the radius of that
		// point. It covers the entire sphere with an icosphere representation.
		// This type is special from my bachelor's thesis where I used star-shaped
		// surfaces calculated with Fourier series.
		// But it is also the easiest way of drawing uniform spheres with this 
		// library, just send a constant function in.
		// If you want to light or color one of this surfaces you will have to use
		// output functions, that take the output vector and produce a normal/color.
		// For spheres, to obtain the normal you can just feed forward the output.
		SPHERICAL_SURFACE,
		// Uses P(u,v) functions to obtain an R3 position given two initial values.
		PARAMETRIC_SURFACE,
		// Uses F(x,y,z)=0 functions to derive the points that satisfy equality.
		IMPLICIT_SURFACE,
	} 
	type = EXPLICIT_SURFACE; // Defaults to explicit

	// If type is explicit expects a valid pointer to a function that will be called 
	// on the x,y ranges to evaluate the z value of the plot.
	float (*explicit_func)(float, float) = nullptr;

	// If type is spehrical expects a valid pointer to a function that will be called on
	// every point of the sphere to evaluate the radius of that point.
	float (*spherical_func)(float, float, float) = nullptr;

	// If type is parametric expects a valid pointer to a function tha will be called
	// on the u,v ranges to evaluate the R3 position of each vertex.
	Vector3f(*parametric_func)(float, float) = nullptr;

	// If type is explicit expects a valid pointer to a function that will be called
	// inside the cube coordinates and will find the surface where the function is 0.
	float (*implicit_func)(float, float, float) = nullptr;

	// Specifies how the surface will be colored.
	enum SURFACE_COLORING
	{
		// The input coordinates are used to obtain a color.
		INPUT_FUNCTION_COLORING,
		// The output position is used to obtain a color.
		OUTPUT_FUNCTION_COLORING,
		// A texture is used to color the functions given the input coordinates.
		TEXTURED_COLORING,
		// Colors for each vertex are sampled from an array.
		ARRAY_COLORING,
		// The entire function has the same color.
		GLOBAL_COLORING
	}
	coloring = GLOBAL_COLORING; // Defaults to global coloring.

	// If coloring is set to global the surface will have this color.
	Color global_color = Color::White;

	// If coloring is input functional, expects a valid function to generate the 
	// colors of each vertex given the same coordinate values as the generation func.
	Color(*input_color_func)(float, float) = nullptr;

	// If coloring is output functional, expects a valid function to generate the 
	// colors of each vertex given the output coordinate values from the generation func.
	Color(*output_color_func)(float, float, float) = nullptr;

	// Is coloring is from an array, expects a valid pointer to an array of colors 
	// of size num_u x num_v from which the color of the surface will be sampled.
	Color** color_array = nullptr;

	// If coloring is textured it expects a valid image from which the colors will be
	// sampled given the input coordinates. If type is explicit spherical the image is
	// expected to be a cube-box. Check the texture header for more information.
	Image* texture_image = nullptr;

	// If the surface is illuminated it specifies how the normal vectors will be computed.
	enum SURFACE_NORMALS
	{
		// The normal vectors will be computed by calculating the function in a small
		// neighborhood around the vertex and computing the normal to that neighborhood.
		DERIVATE_NORMALS,
		// The normal vectors will be calculated by a user provided function, that takes
		// the input variables as input and outputs the normal vector for those inputs.
		INPUT_FUNCTION_NORMALS,
		// The normal vectors will be calculated by a user provided function, that takes
		// the output position as input and outputs the normal vector for that position.
		OUTPUT_FUNCTION_NORMALS,
		// The closest vertexs around each other will be used as neigborhood.
		CLOSEST_NEIGHBORS,
	}
	normal_computation = DERIVATE_NORMALS; // Defaults to derivation.

	// If the surface is illumaneted and the normal vectors are input function provided it
	// expects a valid function that will take the same coordinates as the generation function
	// and output the normal vector of the surface. The vector will not be normailzed.
	Vector3f(*input_normal_func)(float, float) = nullptr;

	// If the surface is illumaneted and the normal vectors are output function provided it
	// expects a valid function that will take the output position of the generation function
	// and output the normal vector of that position. The vector will not be normailzed.
	Vector3f(*output_normal_func)(float, float, float) = nullptr;

	// Small delta value used to derivate the normal vectors.
	float delta_value = 1e-5f;

	// Range of the coordinates at which the generation function will be calculated.
	// For explicit functions (x,y)=(u,v). The w coordinate is for implicit functions.
	Vector2f range_u = { -1.f, 1.f }, range_v = { -1.f, 1.f }, range_w = { -1.f, 1.f };

	// Number of point for each coordinate at which the generation function will be calculated.
	// For explicit functions (x,y)=(u,v).
	unsigned num_u = 200u, num_v = 200u;

	// If surface type is explicit icosphere here you can specify the partition depths
	// of the triangles of the icosphere, number of triangles is T = 20 * 4^d.
	unsigned icosphere_depth = 5u;

	// If surface type is implicit, these are the refinements used to generate the cubes.
	unsigned refinements[10] = { 20u, 4u, 0u/* Guard */};

	// If surface type is implicit, specifies the number of cube refinements that 
	// will be done before presenting the final vertices. If you increase this number you 
	// must specify the new refinements.
	unsigned max_refinements = 2u;

	// Maximum amount of triangles an implicit surface can have, this space will be allocated
	// in RAM to generate the surface, so only tune if you think you will need more than this
	// amount of triangles or much less than this amount of triangles.
	unsigned max_implicit_triangles = 0x20000u;

	// Whether both sides of each triangle are rendered or not.
	bool double_sided_rendering = true;

	// Whether the surface uses illumination or not.
	bool enable_illuminated = true;

	// Sets Order Indepentdent Transparency for the Surface. Check 
	// Graphics.h or Blender.h for more information on how to use it.
	bool enable_transparency = false;

	// Whether the Surface allows for shape updates, leave at false if you 
	// don't intend to update the shape of the Surface. Only the functions 
	// updateShape(), updateColors(), updateTexture() require it.
	bool enable_updates = false;

	// IF true renders only the aristas of the triangle mesh of the Surface.
	bool wire_frame_topology = false;

	// Uses a nearest point sampler instead of a linear one.
	bool pixelated_texture = false;

	// Whether the edge values of the range are included in the set.
	bool border_points_included = true;

	// By default polyhedrons and surfaces are lit by four different color lights
	// around the center of coordinates, allows for a nice default that illuminates
	// everything and distiguishes different areas, disable to set all to black.
	bool default_initial_lights = true;
};

// Surface drawable class, used for drawing, interaction and visualization of user defined 
// functionson a Graphics instance. Allows for different generation and rendering settings 
// including but not limited to generation, textures, illumination, transparencies. Check 
// the descriptor to see all options.
class Surface : public Drawable
{
public:
	// Surface constructor, if the pointer is valid it will call the initializer.
	Surface(const SURFACE_DESC* pDesc = nullptr);

	// Frees the GPU pointers and all the stored data.
	~Surface();
	
	// Initializes the Surface object, it expects a valid pointer to a descriptor and 
	// will initialize everything as specified, can only be called once per object.
	void initialize(const SURFACE_DESC* pDesc);

	// If updates are enabled it expects the original generation function, coloring function and 
	// normal function (if used) to still be callable and will use the new ranges and the original 
	// settings to update the surface. For in place dynamic surfaces you can internally change 
	// static parameters in the called functions to output different values with same inputs.
	// The ranges will only be updated if they are different than (0.f,0.f).
	void updateShape(Vector2f range_u = {}, Vector2f range_v = {}, Vector2f range_w = {});

	// If updates are enabled and coloring is from an array, expects a valid array of 
	// size num_u x num_v and updates every vertex of the surface with the new colors.
	void updateColors(const Color** color_array);

	// If updates are enabled and coloring is textured, it expects a valid image pointer and 
	// uses it to update the texture. Image dimensions must be equal to the initial image.
	void updateTexture(const Image* texture_image);

	// If the coloring is set to global, updates the global Surface color.
	void updateGlobalColor(Color color);

	// Updates the rotation quaternion of the Surface. If multiplicative it will apply
	// the rotation on top of the current rotation. For more information on how to rotate
	// with quaternions check the Quaternion header file.
	void updateRotation(Quaternion rotation, bool multiplicative = false);

	// Updates the scene position of the Surface. If additive it will add the vector
	// to the current position vector of the Surface.
	void updatePosition(Vector3f position, bool additive = false);

	// Updates the matrix multiplied to the Surface, adding any arbitrary linear distortion 
	// to it. If you want to simply modify the size of the object just call this function on a 
	// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
	// If multiplicative the distortion will be added to the current distortion.
	void updateDistortion(Matrix distortion, bool multiplicative = false);

	// Updates the screen displacement of the figure. To be used if you intend to render 
	// multiple scenes/plots on the same render target.
	void updateScreenPosition(Vector2f screenDisplacement);

	// If illumination is enabled it sets the specified light to the specified parameters.
	// Eight lights are allowed. And the intensities are directional and diffused.
	void updateLight(unsigned id, Vector2f intensities, Color color, Vector3f position);

	// If illumination is enabled clears all lights for the Surface.
	void clearLights();

	// If illumination is enabled, to the valid pointers it writes the specified lights data.
	void getLight(unsigned id, Vector2f* intensities, Color* color, Vector3f* position);

	// Returns the current rotation quaternion.
	Quaternion getRotation() const;

	// Returns the current scene position.
	Vector3f getPosition() const;

	// Returns the current distortion matrix.
	Matrix getDistortion() const;

	// Returns the current screen position.
	Vector2f getScreenPosition() const;

private:
	// Pointer to the internal class storage.
	void* surfaceData = nullptr;
};

#ifdef _INCLUDE_IMGUI

/* IMGUI BASE CLASS MANAGER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Given the difficulty of developing User Interface classes this library encourages 
the use of ImGui for the user interface of the developed applications.

To use ImGui inside your applications you just need to create an inherited iGManager
class and a constructor that feeds the widow to the base constructor.

Then you can create a render function that starts with newFrame and end with drawFrame
using the imGui provided functions and your ImGui interface will be drawn to the window.

For more information on how to develop ImGui interfaces please visit:
ImGui GitHub Repositiory: https://github.com/ocornut/imgui.git
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// ImGui Interface base class, contains the basic building blocks to connect the ImGui
// API to this library, any app that wishes to use ImGui needs to inherit this class and 
// call the constructor on the desired window.
class iGManager
{
	// Needs access to the internals to feed messages to ImGui if used.
	friend class MSGHandlePipeline;
	// To get access to newFrame, drawFrame and render.
	friend class Graphics;

protected:
	// Constructor, initializes ImGui WIN32/DX11 for the specific instance and
	// binds Win32 to the specified window, for user interface.
	explicit iGManager(Window& _w, bool ini_file = true);

	// Constructor, initializes ImGui DX11 for the specific instance. Does not 
	// bind user interface to any window. Bind must be called for interaction.
	explicit iGManager(bool ini_file = true);

	// If it is the last class instance shuts down ImGui WIN32/DX11.
	virtual ~iGManager();

	// Calls new frame on Win32 and DX11 on the imGui API. Called automatically during
	// pushFrame() before render(). If you define custom imGui rendering you must call 
	// this function before drawing any imgui objects.
	void newFrame();

	// Calls the rendering method of DX11 on the imGui API. Called automatically during
	// pushFrame() after render(). If you define custom imGui rendering you must call 
	// this function after drawing your imgui objects.
	void drawFrame();

	// InGui rendering function, needs to be implemented by inheritance and will be 
	// called by the bounded window just before pushing a frame. For custom behavior 
	// just leave it blank and create your onw rendering functions.
	virtual void render() = 0;

public:
	// Binds the objects user interaction to the specified window.
	void bind(Window& _w);

	// If it's bound to a window it unbinds it.
	void unbind();

	// Whether ImGui is currently capturing mouse events.
	bool wantCaptureMouse();

	// Whether ImGui is currently capturing keyboard events.
	bool wantCaptureKeyboard();

private:
	// Stores the pointer to the ImGui context of the specific window of the instance.
	void* Context = nullptr;

	// Stores the pointer to its corresponding window.
	Window* pWindow = nullptr;

	// Called by the MSGHandlePipeline lets ImGui handle the message flow
	// if imGui is active and currently has focus on the window.
	static bool WndProcHandler(void* hWnd, unsigned int msg, unsigned int wParam, unsigned int lParam);

	// No copies are allowed. For proper context management.
	iGManager(iGManager&&) = delete;
	iGManager operator=(iGManager&&) = delete;
	iGManager(const iGManager&) = delete;
	iGManager operator=(const iGManager&) = delete;
};

#endif // _INCLUDE_IMGUI

#ifdef _INCLUDE_TIMER

/* TIMER CLASS HEADER FILE
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This class is a handy multiplatform timer to be used by applications 
that require some type of timing. It produces precise timing using QPC 
for Windows and the system clock for other operating systems.

It has an array of times stored so that averages can be computed and 
it circles through it with a user set max length.

It also includes some sleeping functions. In Windows it sets the sleeping timer
resolution to 1ms by default, modifiable with set_sleep_timer_resolution_1ms().
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Definition of the class, everything in this header is contained inside the class Timer.

// A timer object can be generated anywhere in your code, and will run independently of 
// the other timers. Upon construction the timer is reset and a first push is made.
class Timer {
private:
	// Default max number of markers for Timer.
	static constexpr unsigned DEFAULT_TIMER_CAP_ = 60u;

	// Time stamps history ring

	unsigned int cap_ = DEFAULT_TIMER_CAP_;		// MaxMarkers
	unsigned int size_ = 0u;					// Number of valid entries
	unsigned int head_ = 0u;					// Index of most-recent entry (when size_>0)
	long long* stamps_ = nullptr;				// Pointer to the markers in ticks

	// Tick measuring variables

	long long    last_ = 0LL;					// Last timestamp
	static long long freq_;						// high-res frequency (ticks per second)

	// Pushes current 'last_' into ring (most recent).
	inline void push_last();

	// Converts tick delta to seconds by dividing by _freq.
	static inline float to_sec(long long dt);

#ifdef _WIN32
	// Reads QPC counter to obtain time.
	static inline long long qpc_now();

	// Reads frequency to be stored in _freq.
	static inline long long qpc_freq();
#endif

public:
	// Copy functions are deleted. Copies not allowed

	Timer(const Timer&)				= delete;
	Timer& operator=(const Timer&)	= delete;

	// Constructor / Destructor.

	Timer();
	~Timer();

	// Resets history and start new timing window.
	void reset();

	// Pushes current time, returns seconds since previous mark.
	float mark();

	// Returns seconds since previous mark without modifying history.
	float check();

	// Advances 'last' by elapsed time (shifts existing markers forward), 
	// practically skipping the time since last mark, returns elapsed seconds.
	float skip();

	// Returns the seconds since the oldest stored marker.
	// If size_ < cap_ this will be the time since last reset.
	float checkTotal();

	// Returns average seconds per interval over the stored markers. 
	// (0 if insufficient data, size_ < 2).
	float average();

	// Returns number of stored markers (size_).
	unsigned int getSize();

	// Sets the maximum size for the history ring.
	void setMax(unsigned int max);


#ifdef _WIN32
private:
	// Static helper class calls TimeBeginPeriod(1) and TimeEndPeriod(1).
	class precisionSleeper
	{
	public:
		static bool _precise_period;
	private:
		precisionSleeper();
		~precisionSleeper();
		static precisionSleeper helper;
	};
public:

	// Static Helper Functions

	// It is set to true by default by precisionSleeper but can be modified.
	static void set_sleep_timer_resolution_1ms(bool enable);
#endif

	// Waits for a precise amount of microseconds.
	static void sleep_for_us(unsigned long long us);

	// Waits for a certain amount of milliseconds, precise in Windows if
	// timer resolution is set to 1ms (default).
	static void sleep_for(unsigned long ms);

	// Returns system time in nanoseconds as a 64-bit tick (monotonic).
	// Perfect for precise time tracking and as seed for random variables.
	static unsigned long long get_system_time_ns();

};

#undef _INCLUDE_TIMER
#endif // _INCLUDE_TIMER

#ifdef _INCLUDE_USER_ERROR

/* ERROR BASE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the base class for all error types defined across the library.
It also contains the main macro where all fatal checks are funneled through, to be
modified by the user if so desires.

Failed check in this library are to be considered always fatal, when a check is
failed, a ChaoticError object is created, and is always funneled through
the CHAOTIC_FATAL macro, which by default calls PopMessageBoxAbort.

We distinguish errors between two types, user errors, which are given from improper
use of the API, this will appear as "Info Errors" upon crash. And system errors,
which are failed checks on Win32 or DX11 functions, all of them do diagnostics if
available and print it to the message box.

Checks are performed in both debug and release mode, due to their almost negligible
overhead and their really valuable diagnostics. However this can be disabled by the
user, all expressions that make it into CHAOTIC_CHECK are intended to be skipped,
they do not modify any state, so CHAOTIC_CHECK and CHAOTIC_FATAL can be nullyfied
with no additional modifications needed.

The modifications must be done in the original header for compilation, in this case
the header is "ChaoticError.h".
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// General macro where all failed checks are funneled through
#ifdef _DEBUG
#define CHAOTIC_FATAL(_ERROR)  (_ERROR).PopMessageBoxAbort()
#else
#define CHAOTIC_FATAL(_ERROR)  (_ERROR).PopMessageBoxAbort()
#endif

// Macro used for regular checks with no state modifination expressions.
#ifdef _DEBUG
#define CHAOTIC_CHECK(_EXPR, _ERROR)  do { if (!(_EXPR)) { CHAOTIC_FATAL(_ERROR); } } while(0)
#else
#define CHAOTIC_CHECK(_EXPR, _ERROR)  do { if (!(_EXPR)) { CHAOTIC_FATAL(_ERROR); } } while(0)
#endif

// Error Base class layout, all error classes must follow it, when 
// an error is found, PopMessageBoxAbort() is called.
class ChaoticError
{
protected:
	// Default constructor, Only accessable to inheritance.
	// Stores the code line and file where the error was found.
	ChaoticError(int line, const char* file) noexcept;

public:
	// Returns the error type, to be overwritten by inheritance.
	virtual const char* GetType() const noexcept = 0;

	// Getters for custom error behavior.
	constexpr int		  GetLine()   const noexcept { return line;   }
	constexpr const char* GetFile()   const noexcept { return file;   }
	constexpr const char* GetOrigin() const noexcept { return origin; }
	constexpr const char* GetInfo()   const noexcept { return info;   }

	// Creates a default message box using Win32 with the error data.
	[[noreturn]] void PopMessageBoxAbort() const noexcept;
protected:
	int line;				// Stores the line where the error was found.
	char file[512] = {};	// Stores the file where the error was found.

	char origin[512] = {};	// Stores the origin string.

protected:
	// Pointer to the what buffer to be used by inheritance.
	char info[2048] = {};
};

/* USER ERROR CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the default error class and user check, used by the
library when no internal error has ocurred. Mostly used for user driven
errors due to failed API conditions.

Contains the line, file and a description of the error, as every other
error funnels through the defalut error pipeline.

It can also be used by the user own developed apps, but given the conditions
outlined in the ChaoticError header, any expression inside a USER_CHEK must
not modify internal variable because it might be nullyfied by the end user.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Default user error macro used for user error calls across the library. 
// This is used if no specific condition has been checked on the path or 
// if the condition check is necessary for normal library runtime.
#define USER_ERROR(_MSG)			CHAOTIC_FATAL(UserError(__LINE__, __FILE__, _MSG))

// Default user check macro used along the library for non system errors.
// Can also be used by the user when designing their own apps.
#define USER_CHECK(_EXPR, _MSG)		CHAOTIC_CHECK(_EXPR, UserError(__LINE__, __FILE__, _MSG))

// Basic Error class, stores the given information and adds it
// to the whatBuffer when the what() function is called.
class UserError : public ChaoticError
{
public:
	// Single message constructor, the message is stored in the info.
	UserError(int line, const char* file, const char* msg) noexcept;

	// Info Error type override.
	const char* GetType() const noexcept override { return "Default User Error"; }
};

#undef _INCLUDE_USER_ERROR
#endif // _INCLUDE_DEFAULT_ERROR