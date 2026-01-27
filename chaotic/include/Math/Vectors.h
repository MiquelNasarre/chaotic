#pragma once

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

#define _VECTORS_ADDED // For constants.h to know it is defined.

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
	constexpr Vector2i operator*(const int& other) const	{ return { x * other, y * other }; }
	constexpr Vector2i operator/(const int& other) const	{ return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2i& operator*=(const int& other)		{ return *this = *this * other; }
	constexpr Vector2i& operator/=(const int& other)		{ return *this = *this / other; }

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
constexpr Vector2i operator*(const int& lhs, const Vector2i& rhs)		{ return rhs * lhs; }

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
	constexpr Vector2f(float x, float y)	{ this->x = x; this->y = y; }

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
	constexpr Vector2f operator*(const int& other) const	{ return { x * other, y * other }; }
	constexpr Vector2f operator*(const float& other) const	{ return { x * other, y * other }; }
	constexpr Vector2f operator/(const int& other) const	{ return { x / other, y / other }; }
	constexpr Vector2f operator/(const float& other) const	{ return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2f& operator*=(const int& other)		{ return *this = *this * other; }
	constexpr Vector2f& operator*=(const float& other)		{ return *this = *this * other; }
	constexpr Vector2f& operator/=(const int& other)		{ return *this = *this / other; }
	constexpr Vector2f& operator/=(const float& other)		{ return *this = *this / other; }

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
constexpr Vector2f operator*(const int& lhs, const Vector2f& rhs)		{ return rhs * lhs; }
constexpr Vector2f operator*(const float& lhs, const Vector2f& rhs)		{ return rhs * lhs; }

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
	constexpr Vector2d(double x, double y)	{ this->x = x; this->y = y; }

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
	constexpr Vector2d operator*(const int& other) const	{ return { x * other, y * other }; }
	constexpr Vector2d operator*(const float& other) const	{ return { x * other, y * other }; }
	constexpr Vector2d operator*(const double& other) const { return { x * other, y * other }; }
	constexpr Vector2d operator/(const int& other) const	{ return { x / other, y / other }; }
	constexpr Vector2d operator/(const float& other) const	{ return { x / other, y / other }; }
	constexpr Vector2d operator/(const double& other) const { return { x / other, y / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector2d& operator*=(const int& other)		{ return *this = *this * other; }
	constexpr Vector2d& operator*=(const float& other)		{ return *this = *this * other; }
	constexpr Vector2d& operator*=(const double& other)		{ return *this = *this * other; }
	constexpr Vector2d& operator/=(const int& other)		{ return *this = *this / other; }
	constexpr Vector2d& operator/=(const float& other)		{ return *this = *this / other; }
	constexpr Vector2d& operator/=(const double& other)		{ return *this = *this / other; }

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
constexpr Vector2d operator*(const int& lhs, const Vector2d& rhs)		{ return rhs * lhs; }
constexpr Vector2d operator*(const float& lhs, const Vector2d& rhs)		{ return rhs * lhs; }
constexpr Vector2d operator*(const double& lhs, const Vector2d& rhs)	{ return rhs * lhs; }

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
	constexpr Vector3i(int x, int y, int z)	{ this->x = x; this->y = y; this->z = z; }

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
	constexpr Vector3i operator*(const int& other) const	{ return { x * other, y * other, z * other }; }
	constexpr Vector3i operator/(const int& other) const	{ return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3i& operator*=(const int& other)	{ return *this = *this * other; }
	constexpr Vector3i& operator/=(const int& other)	{ return *this = *this / other; }

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
constexpr Vector3i operator*(const int& lhs, const Vector3i& rhs)		{ return rhs * lhs; }

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
	constexpr Vector3f operator*(const int& other) const	{ return { x * other, y * other, z * other }; }
	constexpr Vector3f operator*(const float& other) const	{ return { x * other, y * other, z * other }; }
	constexpr Vector3f operator/(const int& other) const	{ return { x / other, y / other, z / other }; }
	constexpr Vector3f operator/(const float& other) const	{ return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3f& operator*=(const int& other)	{ return *this = *this * other; }
	constexpr Vector3f& operator*=(const float& other)	{ return *this = *this * other; }
	constexpr Vector3f& operator/=(const int& other)	{ return *this = *this / other; }
	constexpr Vector3f& operator/=(const float& other)	{ return *this = *this / other; }

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
constexpr Vector3f operator*(const int& lhs, const Vector3f& rhs)		{ return rhs * lhs; }
constexpr Vector3f operator*(const float& lhs, const Vector3f& rhs)		{ return rhs * lhs; }

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
	constexpr Vector3d operator*(const int& other) const	{ return { x * other, y * other, z * other }; }
	constexpr Vector3d operator*(const float& other) const	{ return { x * other, y * other, z * other }; }
	constexpr Vector3d operator*(const double& other) const { return { x * other, y * other, z * other }; }
	constexpr Vector3d operator/(const int& other) const	{ return { x / other, y / other, z / other }; }
	constexpr Vector3d operator/(const float& other) const	{ return { x / other, y / other, z / other }; }
	constexpr Vector3d operator/(const double& other) const { return { x / other, y / other, z / other }; }

	// Self scalar Multiplication/Division.
	constexpr Vector3d& operator*=(const int& other)	{ return *this = *this * other; }
	constexpr Vector3d& operator*=(const float& other)	{ return *this = *this * other; }
	constexpr Vector3d& operator*=(const double& other) { return *this = *this * other; }
	constexpr Vector3d& operator/=(const int& other)	{ return *this = *this / other; }
	constexpr Vector3d& operator/=(const float& other)	{ return *this = *this / other; }
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
constexpr Vector3d operator*(const int& lhs, const Vector3d& rhs)		{ return rhs * lhs; }
constexpr Vector3d operator*(const float& lhs, const Vector3d& rhs)		{ return rhs * lhs; }
constexpr Vector3d operator*(const double& lhs, const Vector3d& rhs)	{ return rhs * lhs; }
