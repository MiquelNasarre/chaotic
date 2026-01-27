#pragma once
#include "Color.h"

/* IMAGE CLASS HEADER
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
	bool save(const char* fmt_filename) const;

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