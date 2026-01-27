#pragma once
#include "Bindable.h"

/* PIXEL SHADER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This bindable is used to create the bytecode for a (*.cso) pixel shader file and bind it to
the current context. To create such pixel shader you can write an HLSL file and compile it
to a CSO. For examples on how to write an HLSL file you can look at the shaders used by the
drawables in this library.

Your pixel shader will be called on every pixel of your drawable you want to draw in the screen 
and will evaluate the color corresponding to that pixel given the input it receives from the 
vertex shader.

For further information on how to create shaders using HLSL check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Pixel Shader bindable class, expects a CSO file path created from an HLSL pixel shader.
// It binds the pixel shader to the GPU during draw calls when added to a drawable.
class PixelShader : public Bindable
{
public:
	// Given a (*.cso) file path it creates the bytecode and the pixel shader object.
	PixelShader(const wchar_t* path);

	// Raw constructor for deployment, uses embedded resources. Expects valid blobs.
	PixelShader(const void* bytecode, unsigned long long size);

	// Releases the pointers and deletes the internal data.
	~PixelShader() override;

	// Binds the Pixel Shader to the global context.
	void Bind() override;

private:
	// Pointer to the internal Pixel Shader data.
	void* BindableData = nullptr;
};