#pragma once
#include "Bindable.h"

/* VERTEX SHADER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This bindable is used to create the bytecode for a (*.cso) vertex shader file and bind it to 
the current context. To create such vertex shader you can write an HLSL file and compile it 
to a CSO. For examples on how to write an HLSL file you can look at the shaders used by the 
drawables in this library.

This class is connected to the input layout because it need to be able to match the data sent 
from the CPU to the data expected by the GPU, and that is doe via an input layout. Therefore 
you need to store the pointer and later use it to create the input layout.

Check the input layout header for more information on how to properly create both bindables.
For further information on how to create shaders using HLSL check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Vertex Shader bindable class, expects a CSO file path created from an HLSL vertex shader.
// It binds the vertex shader to the GPU during draw calls when added to a drawable.
class VertexShader : public Bindable
{
	// Needs access to the bytecode.
	friend class InputLayout;
public:
	// Given a (*.cso) file path it creates the bytecode and the vertex shader object.
	VertexShader(const wchar_t* path);

	// Raw constructor for deployment, uses embedded resources. Expects valid blobs.
	VertexShader(const void* bytecode, unsigned long long size);

	// Releases the pointers and deletes the internal data.
	~VertexShader();

	// Binds the Vertex Shader to the global context.
	void Bind() override;

private:
	// To be used by InputLayout. Returns the shader ID3DBlob* masked as a void*.
	void* GetBytecode() const noexcept;

	// Pointer to the internal Vertex Shader data.
	void* BindableData = nullptr;
};