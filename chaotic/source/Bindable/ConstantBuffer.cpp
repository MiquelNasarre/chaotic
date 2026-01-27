#include "Bindable/ConstantBuffer.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Constant Buffer internal data.
struct ConstantBufferInternals
{
	ComPtr<ID3D11Buffer> pConstantBuffer;
	CONSTANT_BUFFER_TYPE Type;
	unsigned Slot;
	unsigned size;
};

/*
--------------------------------------------------------------------------------------------
 Constant Buffer Functions
--------------------------------------------------------------------------------------------
*/

// Raw constructor, expects a valid pointer to the data with the specified byte size, 
// creates the GPU Constant Buffer and stores the pointer for binding and update calls.

ConstantBuffer::ConstantBuffer(const void* _data, unsigned size, CONSTANT_BUFFER_TYPE type, const int slot)
{
	if (size % 16u)
		throw INFO_EXCEPT("The constant buffer size must be divisible by 16, please use alignas(16) to avoid invalid sizes.");

	BindableData = new ConstantBufferInternals;
	ConstantBufferInternals& data = *(ConstantBufferInternals*)BindableData;

	data.size = size;
	data.Type = type;

	if (slot == CONSTANT_BUFFER_DEFAULT_SLOT) 
	{
		if (type == VERTEX_CONSTANT_BUFFER)
			data.Slot = 1u;
		if (type == PIXEL_CONSTANT_BUFFER)
			data.Slot = 0u;
	}
	else
		data.Slot = (UINT)slot;

	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = _data;

	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage				= D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
	cbd.MiscFlags			= 0u;
	cbd.ByteWidth			= size;
	cbd.StructureByteStride = 0u;

	GFX_THROW_INFO(_device->CreateBuffer(&cbd, &csd, data.pConstantBuffer.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

ConstantBuffer::~ConstantBuffer()
{
	delete (ConstantBufferInternals*)BindableData;
}

// Raw update function, expects a valid pointer and the byte size to match the one used in 
// the construtor, otherwise will throw. Updates the data in the GPU with the new data.

void ConstantBuffer::update(const void* _data, unsigned size_check)
{
	ConstantBufferInternals& data = *(ConstantBufferInternals*)BindableData;

	if (data.size != size_check)
		throw INFO_EXCEPT("Mismatch in the Constant Buffer stored data size and the updated data size.");

	// Create the mapping
	D3D11_MAPPED_SUBRESOURCE msr;
	GFX_THROW_INFO(_context->Map(data.pConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));

	// Copy the data
	memcpy(msr.pData, _data, data.size);

	// Unmap the data
	GFX_THROW_INFO_ONLY(_context->Unmap(data.pConstantBuffer.Get(), 0u));
}

// Binds the Constant Buffer to the global context according to its type.

void ConstantBuffer::Bind()
{
	ConstantBufferInternals& data = *(ConstantBufferInternals*)BindableData;

	if (data.Type == VERTEX_CONSTANT_BUFFER)
		GFX_THROW_INFO_ONLY(_context->VSSetConstantBuffers(data.Slot, 1u, data.pConstantBuffer.GetAddressOf()));

	if (data.Type == PIXEL_CONSTANT_BUFFER)
		GFX_THROW_INFO_ONLY(_context->PSSetConstantBuffers(data.Slot, 1u, data.pConstantBuffer.GetAddressOf()));
}

