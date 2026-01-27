#pragma once

/* COLOR HEADER CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Inherited class for my 3D renderer solution, useful for dealing
with colored images and GPU printings.

It has a bunch of operators defined tailored to my needs in the 
past with this class, still very simple and robust.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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