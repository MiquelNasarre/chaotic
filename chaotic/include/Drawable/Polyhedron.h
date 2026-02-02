#pragma once
#include "Drawable.h"

/* POLYHEDRON DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Drawable class to draw triangle meshes, it is a must for any rendering library and in
this case here we have it. To initialize it, it expects a valid pointer to a descriptor
and will create the Shape with the specified data.

As it is standard on this library it has multiple setting to set the object rotation,
position, linear distortion, and screen shifting and it is displayed in relation to the 
perspective of the Graphics currently set as render target.

It allows for ilumination, texturing, transparencies and figure updates. For information 
on how to handle transparencies you can check the Graphics header. For information on how 
to create images for the texture you can check the Image class header.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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

	// If the Polyhedron is iluminated it specifies how the normal vectors will be obtained.
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

	// Whether the polyhedron uses ilumination or not.
	bool enable_iluminated = true;

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
	// around the center of coordinates, allows for a nice default that iluminates
	// everything and distiguishes different areas, disable to set all to black.
	bool default_initial_lights = true;
};

// Polyhedron drawable class, used for drawing and interacting with user defined triangle 
// meshes on a Graphics instance. Allows for different rendering settings including but not 
// limited to textures, ilumination, transparencies. Check the descriptor to see all options.
class Polyhedron : public Drawable
{
public:
	// To facilitate the loading of triangle meshes, this function is a parser for 
	// *.obj files, that reads the files and outputs a valid descriptor. If the file 
	// supports texturing, optionally accepts an image to be used as texture_image.
	// NOTE: All data is allocated by (new) and its deletion must be handled by the 
	// user. The image pointer used is the same as provided.
	// If any error occurs, including missing file, it will throw.
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

	// If ilumination is enabled it sets the specified light to the specified parameters.
	// Eight lights are allowed. And the intensities are directional and diffused.
	void updateLight(unsigned id, Vector2f intensities, Color color, Vector3f position);

	// If ilumination is enabled clears all lights for the Polyhedron.
	void clearLights();

	// If ilumination is enabled, to the valid pointers it writes the specified lights data.
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