#include "Bindable/IndexBuffer.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Index Buffer internal data.
struct IndexBufferInternals
{
	ComPtr<ID3D11Buffer> pIndexBuffer;
	unsigned count;
};

/*
--------------------------------------------------------------------------------------------
 Index Buffer Functions
--------------------------------------------------------------------------------------------
*/

// Takes an unsigned array and the count as input, 
// creates the Index Buffer on GPU and stores the pointer.

IndexBuffer::IndexBuffer(const unsigned* indices, unsigned count)
{
	BindableData = new IndexBufferInternals;
	IndexBufferInternals& data = *(IndexBufferInternals*)BindableData;
	data.count = count;

	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = count * sizeof(unsigned);
	ibd.StructureByteStride = sizeof(unsigned);
	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices;
	GFX_THROW_INFO(_device->CreateBuffer(&ibd, &isd, data.pIndexBuffer.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

IndexBuffer::~IndexBuffer()
{
	delete (IndexBufferInternals*)BindableData;
}

// Binds the Index Buffer to the global context.

void IndexBuffer::Bind()
{
	IndexBufferInternals& data = *(IndexBufferInternals*)BindableData;

	GFX_THROW_INFO_ONLY(_context->IASetIndexBuffer(data.pIndexBuffer.Get(),DXGI_FORMAT_R32_UINT,0u));
}

// To be accessed during the draw call to retriece the index count.

unsigned IndexBuffer::getCount() const
{
	IndexBufferInternals& data = *(IndexBufferInternals*)BindableData;

	return data.count;
}
