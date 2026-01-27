#pragma once
#include "Drawable.h"

/* SCATTER DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
