#pragma once
#include "Bindable.h"

/* RASTERIZER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This bindable controls the middle step between the vertex shader and the pixel shader
in the pipeline. Decides which pixels will be drawn from the output of the vertex shader.

The default setting is set to draw both sides of triangles and draw the full triangle. 
This is due to the library being invisioned as a math rendering library. You can disable
double sided view and also enable wireframe drawings via the constructor.

For more information on what the rasterizer is and its role on the pipeline you can check
the microsoft learn sources at the following link:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Rasterizer bindable, tells the pipeline which triangles to draw and how to draw those 
// triangles. From the constructor you can control if both sides are drawn, if you are 
// drawing a wire frame and which side is considered front of a triangle.
class Rasterizer : public Bindable
{
public:
	// Creates the Rasterizer object and GPU pointer as specified in the arguments.
	Rasterizer(bool doubleSided = true, bool wireFrame = false, bool frontCounterClockwise = true);

	// Releases the GPU pointer and deletes the data.
	~Rasterizer() override;

	// Binds the Rasterizer to the global context.
	void Bind() override;

private:
	// Pointer to the internal Rasterizer data.
	void* BindableData = nullptr;
};