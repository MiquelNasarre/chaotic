#include "Bindable/InputLayout.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Input Layout internal data.
struct InputLayoutInternals
{
	ComPtr<ID3D11InputLayout> pInputLayout;	// Pointer to a GPU Input Layout
};

/*
--------------------------------------------------------------------------------------------
 Input Layout Functions
--------------------------------------------------------------------------------------------
*/

// Constructor, expects a valid pointer to a list of 'size' element descriptors
// and a valid pointer to a VertexShader to extract the bytecode.

InputLayout::InputLayout(const INPUT_ELEMENT_DESC* layout, unsigned size, const VertexShader* pVS)
{
	BindableData = new InputLayoutInternals;
	InputLayoutInternals& data = *(InputLayoutInternals*)BindableData;

	// Create the actual D3D11 descriptor based on the simplified API descriptor
	D3D11_INPUT_ELEMENT_DESC* ied = new D3D11_INPUT_ELEMENT_DESC[size];
	for (unsigned i = 0; i < size; i++)
		ied[i] = { layout[i].name, 0, (DXGI_FORMAT)layout[i].fmt, 0, (i > 0) ? D3D11_APPEND_ALIGNED_ELEMENT : 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	// Reinterpret the pointer as a vertex shader and extract the bytecode
	ID3DBlob* pBytecode = (ID3DBlob*)pVS->GetBytecode();

	// Create the input layout on the device
	GFX_THROW_INFO(_device->CreateInputLayout(
		ied, size,
		pBytecode->GetBufferPointer(),
		pBytecode->GetBufferSize(),
		data.pInputLayout.GetAddressOf()
	));

	// Free the D3D11 descriptor
	delete[] ied;
}

// Alternative constructor, expects a valid D3D11_INPUT_ELEMENT_DESC* masked as a void*
// of 'size' elements and a valid pointer to a VertexShader to extract the bytecode.

InputLayout::InputLayout(const void* layout, unsigned size, const VertexShader* pVS)
{
	BindableData = new InputLayoutInternals;
	InputLayoutInternals& data = *(InputLayoutInternals*)BindableData;

	// Reinterpret the pointer as a vertex shader and extract the bytecode
	ID3DBlob* pBytecode = (ID3DBlob*)pVS->GetBytecode();

	// Create the input layout on the device (Assuming the input data is a valid D3D11_INPUT_ELEMENT_DESC*)
	GFX_THROW_INFO(_device->CreateInputLayout(
		(D3D11_INPUT_ELEMENT_DESC*)layout, size,
		pBytecode->GetBufferPointer(),
		pBytecode->GetBufferSize(),
		data.pInputLayout.GetAddressOf()
	));
}

// Releases the GPU pointer and deletes the data.

InputLayout::~InputLayout()
{
	delete (InputLayoutInternals*)BindableData;
}

// Binds the Input Layout to the global context.

void InputLayout::Bind()
{
	InputLayoutInternals& data = *(InputLayoutInternals*)BindableData;

	GFX_THROW_INFO_ONLY(_context->IASetInputLayout(data.pInputLayout.Get()));
}
