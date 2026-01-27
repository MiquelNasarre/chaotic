#pragma once
#include "Drawable.h"

/* CURVE DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Drawable class to draw single parameter 3D functions as curves. Given the math renderer 
nature of this library it is mandatory that we have a drawable meant for drawing curves
in the 3D space, and that is exactly what this class does.

As it is standard on this library it has multiple setting to set the object rotation,
position, linear distortion, and screen shifting and it is displayed in relation to the
perspective of the Graphics currently set as render target.

It allows for transparencies, and function and coloring updates. For information on how 
to handle transparencies you can check the Graphics header.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
	void updateRange(Vector2f range);

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
