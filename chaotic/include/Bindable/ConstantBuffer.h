#pragma once
#include "Bindable.h"

/* CONSTANT BUFFER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Our drawables might sometimes have general information that we would like all the shader calls to
have access to, like rotations, global color, lights, etc. For this reason the constant buffer
exists, it stores information available and constant to all shader during a draw call that can 
be modified by the CPU across different calls.

To create a Constant Buffer you need to define a structure with the information you would like 
the PS or VS to have access to and send the pointer to the constructor. The constructor only 
expects 1 object, if you want to structure it different you can also use the raw constructor.

DirectX11 enforces 16 Byte alignment, so if the size is not divisible by 16 the constructor will
throw an exception, to avoid that I strongly recommend using alignas(16) in your struct definition.

In the specific case of this library given its 3D nature the Graphics object associated to a Window
reserves the right to the first VS Constant Buffer slot to store the current scene perspective.

For more information on how to structure a Constant Buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Enumerator to determine whether the constant buffer will 
// be accessed by the Pixel or the Vertex Shader.
enum CONSTANT_BUFFER_TYPE 
{
	VERTEX_CONSTANT_BUFFER,
	PIXEL_CONSTANT_BUFFER
};

// Due to the first VS slot being reserved by the graphics, the default 
// constant buffer slot will be 1 for the VS and 0 for the PS.
#define CONSTANT_BUFFER_DEFAULT_SLOT -1

// Constant buffer bindable, takes a pointer to the structured data and manages the
// Constant Buffer inside the GPU, binding it during draw calls of its drawable.
class ConstantBuffer : public Bindable
{
public:
	// Expects a valid pointer to a single structure object and a type specifier, sends the
	// data to the raw constructor that creates the GPU object and stores the pointer.
	template<typename C>
	ConstantBuffer(const C* consts, CONSTANT_BUFFER_TYPE type, const int slot = CONSTANT_BUFFER_DEFAULT_SLOT)
		: ConstantBuffer((void*)consts, sizeof(C), type, slot) {}

	// Raw constructor, expects a valid pointer to the data with the specified byte size, 
	// creates the GPU Constant Buffer and stores the pointer for binding and update calls.
	ConstantBuffer(const void* data, unsigned size, CONSTANT_BUFFER_TYPE type, const int slot);

	// Releases the GPU pointer and deletes the data.
	~ConstantBuffer() override;

	// Expects a valid pointer to a single structure object of the same size as the one used 
	// in the constructor, feeds it to the raw update which updates the data on the GPU.
	template<typename C>
	void update(const C* consts)
	{
		update((void*)consts, sizeof(C));
	}

	// Raw update function, expects a valid pointer and the byte size to match the one used in 
	// the constructor, otherwise will throw. Updates the data in the GPU with the new data.
	void update(const void* data, unsigned size);

	// Binds the Constant Buffer to the global context according to its type.
	void Bind() override;

private:
	// Pointer to the internal Constant Buffer data.
	void* BindableData = nullptr;
};
