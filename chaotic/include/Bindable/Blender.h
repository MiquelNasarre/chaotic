#pragma once
#include "Bindable.h"

/* BLENDER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
The blend state is the last step when drawing to a render target, and it decides how 
the output color of the pixel is combined with the existent color in the buffer
according to the colors distances and alpha value.

This bindable supports four different modes:

- By default it is mode opaque, where all pixels that pass the depth test will override 
  the previous color and the alpha channel will be ignored.

- Blend mode additive is for a glow like effect, if it passes the depth test it will 
  simply add its color on top of the current pixel, ignoring the alpha channel. This
  combined with a no-write depth stencil can be used for lightsources for example.

- Blend mode alpha is simple additive transparency, if it passed the depth test it will
  combine C_src * A_src + C_dst * (1 - A_src). This is great for simple scenery building
  with a couple transparent objects but it fails when dealing with arbitrarily ordered 
  multiple transparent objects.

- Blend mode Order Independent Transparency. Since this library is meant to be used for
  math rendering intersecting surfaces and functions are very common, therefore it is 
  important to have a reliable way of making them transparent without enforcing ordering.
  This is where this mode comes in handy. 
  
  If the mode is weighted OIT the Blender is just a holder that will tell the graphics object 
  that a transparent object is coming, and the Graphics object will deal with the rest. If 
  your object is transparent it requires a pixel shader that will write in two render targets. 
  The first is a color accumulator, expects a premultiplied (C_src * A_src, A_src). The alpha 
  can be z_value dependent for better depth distinction. The second one is for the final 
  resolve and expects a single float A_src. Transparent objects must always be drawn after 
  opaque objects.

  For more information on how OITransparency works you can check the source code or some
  of the shaders of the default drawables that implement it. The original paper or weighted
  OIT can be found at: https://jcgt.org/published/0002/02/09/

For more information on how blend states work in general in DirectX11 you can check
the microsoft learn resources at their website:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_blend_desc
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Enumerator that defines the blending mode used by the blender.
enum BLEND_MODE
{
	BLEND_MODE_OPAQUE,			// Opaque surfaces
	BLEND_MODE_ALPHA,			// Standard alpha blending
	BLEND_MODE_ADDITIVE,		// Additive blending
	BLEND_MODE_OIT_WEIGHTED,	// Order Independent Transparency
};

// Blender bindable, decides how the colors are blended in the screen by the current drawable.
class Blender : public Bindable
{
	// Needs to acces the blending mode.
	friend class Drawable;
public:
	// Takes the blending mode as an input and creates the blending state accordingly.
	Blender(BLEND_MODE mode);

	// Releases the GPU pointer and deletes the data.
	~Blender() override;

	// Binds the Blender to the global context.
	void Bind() override;

private:
	// To be called by the draw call to check for OIT.
	BLEND_MODE getMode();

	// Pointer to the internal Blender data.
	void* BindableData = nullptr;
};