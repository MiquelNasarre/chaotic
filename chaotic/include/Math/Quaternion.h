#pragma once
#include "Matrix.h"

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

#define _QUATERNION_ADDED // For constants.h to know it is defined.

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
