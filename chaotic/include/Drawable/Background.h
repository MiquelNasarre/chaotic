#pragma once
#include "Drawable.h"

/* BACKGROUND DRAWABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
	void updateRectangle(Vector2i min_coords, Vector2i max_coords);

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