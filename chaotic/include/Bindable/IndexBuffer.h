#pragma once
#include "Bindable.h"

/* INDEX BUFFER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
When a vertex buffer is sent to the GPU, it is important for that GPU to know how those
vertices will be organized, for that reason you need to index those vertices into a long 
list so that later the topology knows which vertexs to use and in which order.

The topology of the object drawn (lines, triangles, points) is decided by the Topology 
bindable, you can check it for further reference.

The index buffer tells the GPU which vertices to use and in which order to use them. Every
vertex has its index assigned by its position in the array sent to the Vertex Buffer, and 
the index buffer will point to each one of the one by one to create the desired shape.

For more information on how to structure an index buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Index buffer bindable, takes an array of unsigneds and manages the Index Buffer
// inside the GPU, binding it during draw calls of its drawable.
class IndexBuffer : public Bindable
{
	// Needs access to the index count.
	friend class Drawable;
public:
	// Takes an unsigned array and the count as input, 
	// creates the Index Buffer on GPU and stores the pointer.
	IndexBuffer(const unsigned* indices, unsigned count);

	// Releases the GPU pointer and deletes the data.
	~IndexBuffer() override;

	// Binds the Index Buffer to the global context.
	void Bind() override;

private:
	// To be accessed during the draw call to retriece the index count.
	unsigned getCount() const;

	// Pointer to the internal Index Buffer data.
	void* BindableData = nullptr;
};