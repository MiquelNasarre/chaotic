#include "Bindable/VertexBuffer.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Vertex Buffer internal data.
struct VertexBufferInternals
{
	ComPtr<ID3D11Buffer> pVertexBuffer;
	VERTEX_BUFFER_USAGE usage;
	UINT byteWidth;
	UINT stride;
};

/*
--------------------------------------------------------------------------------------------
 Vertex Buffer Functions
--------------------------------------------------------------------------------------------
*/

// Raw constructor, expects an ordered array of Vertices with the specified stride 
// per vertex, creates the GPU Vertex Buffer and stores the pointer for binding calls.

VertexBuffer::VertexBuffer(const void* vertices, unsigned stride, unsigned count, VERTEX_BUFFER_USAGE usage)
{
	BindableData = new VertexBufferInternals;
	VertexBufferInternals& data = *(VertexBufferInternals*)BindableData;
	data.stride = stride;
	data.byteWidth = stride * count;
	data.usage = usage;

	// Create the Vertex buffer descriptor
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
	bd.Usage				= (usage == VB_USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC		: D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags		= (usage == VB_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE	: 0u;
	bd.MiscFlags			= 0u;
	bd.ByteWidth			= stride * count;
	bd.StructureByteStride	= stride;

	// Create the specific data ptr
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;

	// Create the vertex buffer
	GRAPHICS_HR_CHECK(_device->CreateBuffer(&bd, &sd, data.pVertexBuffer.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

VertexBuffer::~VertexBuffer()
{
	delete (VertexBufferInternals*)BindableData;
}

// If the Vertex Buffer has dynamic usage it updates the data with the new Vertices
// information. If byteWidth is bigger or usage is not dynamic it will cause assertion.

void VertexBuffer::updateVertices(const void* vertices, unsigned stride, unsigned count)
{
	VertexBufferInternals& data = *(VertexBufferInternals*)BindableData;

	USER_CHECK(data.usage == VB_USAGE_DYNAMIC,
		"Trying to update vertices on a non-dynamic Vertex Buffer is not allowed. \n"
		"Set the VERTEX_BUFFER_USAGE in the constructor to VB_USAGE_DYNAMIC if you intend to use this function.\n"
		"Or alternatively replace the Vertex Buffer entirely by calling Drawable::changeBind()."
	);

	USER_CHECK(stride * count <= data.byteWidth,
		"Trying to update vertices with a higher byteWidth than the one created in the constructor is not allowed."
	);

	// Create the mapping
	D3D11_MAPPED_SUBRESOURCE msr;
	GRAPHICS_HR_CHECK(_context->Map(data.pVertexBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));

	// Copy the data
	memcpy(msr.pData, vertices, count * stride);

	// Unmap the data
	GRAPHICS_INFO_CHECK(_context->Unmap(data.pVertexBuffer.Get(), 0u));

	// Store the new stride for binding calls
	data.stride = stride;
}

// Binds the Vertex Buffer to the global context.

void VertexBuffer::Bind()
{
	VertexBufferInternals& data = *(VertexBufferInternals*)BindableData;

	const UINT offset = 0u;
	GRAPHICS_INFO_CHECK(_context->IASetVertexBuffers(0u, 1u, data.pVertexBuffer.GetAddressOf(), &data.stride, &offset));
}
