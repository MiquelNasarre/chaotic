#pragma once
#include "Bindable.h"

/* DEPTH STENCIL BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
The depth stencil state is an important part of the last step when drawing to a 
render target, and it decides how the drawn object depth will interact with the 
bound depth buffer.

By default the depth stencil overrides the depth if the object is closer, but for
some cases like glow effects, light sources, etc, we might want different settings.

By default the depth stencil is alway set to override the objects depth if closer
in this library. But if one of your drawables requires a different interaction
you can use this bindable to set it.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Enumerator that defines the depth stencil mode.
enum DEPTH_STENCIL_MODE
{
	DEPTH_STENCIL_MODE_DEFAULT,			// Only writes if the object is closer.
	DEPTH_STENCIL_MODE_NOWRITE,			// Never writes to the depth buffer but still tests.
	DEPTH_STENCIL_MODE_NOWRITE_NOTEST,	// Never writes to the depth buffer, always draws.
	DEPTH_STENCIL_MODE_OVERRIDE,		// Always writes to the depth buffer and draws.
};

// Depth Stencil bindable, decides the interaction between the object drawn and the depth buffer.
class DepthStencil : public Bindable
{
public:
	// Takes the depth stencil mode mode as an input and creates the depth stencil state accordingly.
	DepthStencil(DEPTH_STENCIL_MODE mode);

	// Releases the GPU pointer and deletes the data.
	~DepthStencil() override;

	// Binds the Depth Stencil State to the global context.
	void Bind() override;

private:
	// Pointer to the internal Depth Stencil data.
	void* BindableData = nullptr;
};