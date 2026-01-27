#pragma once
#include "Bindable.h"

/* VERTEX BUFFER BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
The first information accessed by the GPU on the graphics pipeline is the Vertex Buffer, this
data will feed into every Vertex Shader call for every vertex as specified by the Input Layout.

To create a Vertex Buffer you need to define a structure with all the information you would 
like every vertex to have, for example 3D position, normal vector, color, etc. Then create an
array of that struct and send the pointer to the constructor specifying the amount of vertices.

For more information on possible padding issues in your structure and how to access the struct
information inside the vertex shader I recommend checking the Input Layout header.

For more information on how to structure a Vertex Buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Usage specifier, if the Drawable intends to update the vertices, this can be done from 
// the Vertex Buffer itself without needing to replace it, set to dynamic if intended.
enum VERTEX_BUFFER_USAGE
{
	VB_USAGE_DEFAULT,
	VB_USAGE_DYNAMIC,
};

// Vertex buffer bindable, takes an array of custom Vertices and manages the
// Vertex Buffer inside the GPU, binding it during draw calls of its drawable.
class VertexBuffer : public Bindable
{
public:
	// Templated Vertex Buffer constructor, expects an array of custom Vertices and 
	// calls the raw constructor to create the GPU object and store the pointer.
	template<typename V>
	VertexBuffer(const V* vertices, unsigned count, VERTEX_BUFFER_USAGE usage = VB_USAGE_DEFAULT)
		: VertexBuffer((const void*)vertices, sizeof(V), count, usage) {}

	// Raw constructor, expects an ordered array of Vertices with the specified stride 
	// per vertex, creates the GPU Vertex Buffer and stores the pointer for binding calls.
	VertexBuffer(const void* vertices, unsigned stride, unsigned count, VERTEX_BUFFER_USAGE usage = VB_USAGE_DEFAULT);

	// Releases the GPU pointer and deletes the data.
	~VertexBuffer() override;

	// If the Vertex Buffer has dynamic usage it updates the data with the new Vertices
	// information. If byteWidth is bigger or usage is not dynamic it will cause assertion.
	template<typename V>
	void updateVertices(const V* vertices, unsigned count)
	{
		updateVertices((const void*)vertices, sizeof(V), count);
	}

	// If the Vertex Buffer has dynamic usage it updates the data with the new Vertices
	// information. If byteWidth is bigger or usage is not dynamic it will cause assertion.
	void updateVertices(const void* vertices, unsigned stride, unsigned count);

	// Binds the Vertex Buffer to the global context.
	void Bind() override;

private:
	// Pointer to the internal Vertex Buffer data.
	void* BindableData = nullptr;
};
