#include "Bindable/Rasterizer.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Rasterizer internal data.
struct RasterizerInternals
{
	ComPtr<ID3D11RasterizerState> pRasterizer;
};

/*
--------------------------------------------------------------------------------------------
 Rasterizer Functions
--------------------------------------------------------------------------------------------
*/

// Creates the Rasterizer object and GPU pointer as specified in the arguments.

Rasterizer::Rasterizer(bool doubleSided, bool wireFrame, bool frontCounterClockwise)
{
	BindableData = new RasterizerInternals;
	RasterizerInternals& data = *(RasterizerInternals*)BindableData;

	// Creates the rasterizer description as specified in the constructor arguments
	D3D11_RASTERIZER_DESC rasterDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
	rasterDesc.CullMode = doubleSided ? D3D11_CULL_NONE : D3D11_CULL_BACK;
	rasterDesc.FillMode = wireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = frontCounterClockwise;

	// Creates the rasterizer object on the GPU and stores the pointer.
	GRAPHICS_HR_CHECK(_device->CreateRasterizerState(&rasterDesc, data.pRasterizer.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

Rasterizer::~Rasterizer()
{
	delete (RasterizerInternals*)BindableData;
}

// Binds the Rasterizer to the global context.

void Rasterizer::Bind()
{
	RasterizerInternals& data = *(RasterizerInternals*)BindableData;

	GRAPHICS_INFO_CHECK(_context->RSSetState(data.pRasterizer.Get()));
}
