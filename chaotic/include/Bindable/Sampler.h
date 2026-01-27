#pragma once
#include "Bindable.h"

/* SAMPLER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
The sampler state controls the function inside pixel shaders of Texture::Sample(). 
It determines how to assign a color to a given texture coordinate from the texture.

Two main variables are controlled in this class: 

- The filter, that tells you how the point maps to the texture, it can be mapping to 
  the nearest pixel, getting therefore a pixelated view in low resolution. Linearly 
  combining the nearest pixels getting a blurred effect, or non-linearly combining them
  with the anisotropic filter, giving better texture quality in some cases.

- The address mode, controls what happens when the texture coordinate gets out of bounds,
  since the textures are defined between 0 and 1 in all directions.

For more information on how to choose a sampler for texture rendering you can check
the microsoft learn sources at the following link:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_sampler_desc
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Sample filter enumerator mimquing D3D11_FILTER. Decides how the texture 
// color will be interpolated from the neighboring pixels in the shader.
enum SAMPLE_FILTER 
{
    SAMPLE_FILTER_POINT = 0,            // Nearest pixel
    SAMPLE_FILTER_LINEAR = 0x15,        // interpolate pixels
    SAMPLE_FILTER_ANISOTROPIC = 0x55,   // Non linear for improved perspective (default x8)
};

// Sample address mode filter enumerator mimiquing D3D11_TEXTURE_ADDRESS_MODE.
// Decides how the texture coordinate will be assigned when it gets out of bounds.
enum SAMPLE_ADDRESS_MODE 
{
    SAMPLE_ADDRESS_WRAP = 1,            // Repeating pattern
    SAMPLE_ADDRESS_MIRROR = 2,          // Repeating pattern mirrored
    SAMPLE_ADDRESS_CLAMP = 3,           // Clamped to edge of texture
    SAMPLE_ADDRESS_BORDER = 4,          // Border color assigned
    SAMPLE_ADDRESS_MIRROR_ONCE = 5      // Mirror negative axis then clamp
};

// Sampler bindable, decides how given texture coordinates inside pixel shaders will be 
// mapped to a color. From the constructor the filter and the address mode are specified.
class Sampler : public Bindable
{
public:
    // Creates the Sampler object and GPU pointer as specified in the arguments.
	Sampler(SAMPLE_FILTER filter, SAMPLE_ADDRESS_MODE address_mode = SAMPLE_ADDRESS_WRAP, unsigned slot = 0u);

    // Releases the GPU pointer and deletes the data.
	~Sampler() override;

    // Binds the Sampler to the global context.
	void Bind() override;

    // Sets the slot at which the Sampler will be bound.
    void setSlot(unsigned slot);

private:
    // Pointer to the internal Sampler data.
	void* BindableData = nullptr;
};
