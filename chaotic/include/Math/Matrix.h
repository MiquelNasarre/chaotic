#pragma once
#include "Vectors.h"

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

    // Returns identity matrix.
    static constexpr Matrix Identity() { return Matrix(1.f); }

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
             (a11 * a22 - a12 * a21), -(a01 * a22 - a02 * a21),  (a01 * a12 - a02 * a11),
            -(a10 * a22 - a12 * a20),  (a00 * a22 - a02 * a20), -(a00 * a12 - a02 * a10),
             (a10 * a21 - a11 * a20), -(a00 * a21 - a01 * a20),  (a00 * a11 - a01 * a10)
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

// Ctretch in each cardinal direction, returns the diagonal matrix.
constexpr Matrix ScalingMatrix(float x, float y, float z)
{
    return {
        x, 0.f, 0.f,
        0.f, y, 0.f,
        0.f, 0.f, z
    };
}

// Stretch along direction 'axis' by 'factor' (factor=1 => no change).
inline Matrix StretchMatrix(const Vector3f axis, float factor)
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
constexpr Matrix ShearMatrix(const Vector3f dir, const Vector3f ref, float k)
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