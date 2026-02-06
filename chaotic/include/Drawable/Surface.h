#pragma once
#include "Drawable.h"

/* SURFACE DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
