#pragma once
#include "Bindable.h"

/* TEXTURE BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
A texture from a graphics engine point of view it is simply a grid of texels sent to
the GPU that the Pixel Shader can sample from and use it to output its pixel color.

For the Pixel Shader to sample from a texture a sampler state needs to be defined, refer
to the Sampler bindable header for more information on how to set a sampler state.

To be able to send your Images to the GPU this class handles the creation of textures and 
their GPU binding. To create a texture the Image class is to be used. Internally contains
a pointer to an array of colors and the image dimensions. It supports transparencies and 
image loading and saving from memory.

This bindable also supports cube-maps for background creation. The image uploaded must contain
the six faces of the cube stacked on top of each other in the order [+X,-X,+Y,-Y,+Z,-Z].
And the orientation must correspond to what a camera at the origin would see when looking 
along that axis direction, with +Y as “up” in world.

The Image header contains a small library of functions called ToCube, for simple conversion
from common spherical projections to cube-maps. I strongly recommend checking them out if you
have other type of spherical projected images.

For more information on how to use the Image class I strongly suggest checking its header.
For more information on how textures are used in Pixel Shaders and DX11 in general you can 
check the microsoft learn documentation at:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Usage specifier, if the Drawable intends to update the Texture, this can be done from 
// the Texture class itself without needing to replace it, set to dynamic if intended.
enum TEXTURE_USAGE
{
	TEXTURE_USAGE_DEFAULT,
	TEXTURE_USAGE_DYNAMIC,
};

// For background images that support fields of view cubemaps are a very common choice to
// minimize distortion while making the math simple on the GPU side. If you want to generate 
// a cube-map the uploaded image dimensions must be (width, 6 * width).
enum TEXTURE_TYPE
{
	TEXTURE_TYPE_IMAGE,
	TEXTURE_TYPE_CUBEMAP,
};

// Texture bindable class, from an Image it creates a texture and sends it to the GPU to 
// be read by the Pixel Shader at the specified slot.
class Texture : public Bindable
{
public:
	// Expects a valid image pointer and creates the texture in the GPU.
	Texture(const Image* image, TEXTURE_USAGE usage = TEXTURE_USAGE_DEFAULT, TEXTURE_TYPE type = TEXTURE_TYPE_IMAGE, unsigned slot = 0u);

	// Releases the GPU pointer and deletes the data.
	~Texture() override;

	// Binds the Texture to the global context at the specified slot.
	void Bind() override;

	// Sets the slot at which the Texture will be bound.
	void setSlot(unsigned slot);

	// If usage is dynamic updates the texture with the new image.
	// Dimensions must match the initial image dimensions.
	void update(const Image* image);

private:
	// Pointer to the internal Texture data.
	void* BindableData = nullptr;
};
