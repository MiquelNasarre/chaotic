#include "Bindable/DepthStencil.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Depth Stencil internal data.
struct DepthStencilInternals
{
	ComPtr<ID3D11DepthStencilState> pDepthStencil;
};

/*
--------------------------------------------------------------------------------------------
 Depth Stencil Functions
--------------------------------------------------------------------------------------------
*/

// Takes the depth stencil mode mode as an input and creates the depth stencil state accordingly.

DepthStencil::DepthStencil(DEPTH_STENCIL_MODE mode)
{
	BindableData = new DepthStencilInternals;
	DepthStencilInternals& data = *(DepthStencilInternals*)BindableData;

	D3D11_DEPTH_STENCIL_DESC dsd = {};
	switch (mode)
	{
	case DEPTH_STENCIL_MODE_DEFAULT:
		dsd.DepthEnable = TRUE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		dsd.StencilEnable = FALSE;
		break;

	case DEPTH_STENCIL_MODE_NOWRITE:
		dsd.DepthEnable = TRUE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No writes
		dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // Still tests
		dsd.StencilEnable = FALSE;
		break;

	case DEPTH_STENCIL_MODE_NOWRITE_NOTEST:
		dsd.DepthEnable = FALSE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No writes
		dsd.DepthFunc = D3D11_COMPARISON_ALWAYS; // Always draws
		dsd.StencilEnable = FALSE;
		break;

	case DEPTH_STENCIL_MODE_OVERRIDE:
		dsd.DepthEnable = TRUE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsd.DepthFunc = D3D11_COMPARISON_ALWAYS; // Always write
		dsd.StencilEnable = FALSE;
		break;

	default:
		USER_ERROR("Unknown DEPTH_STENCIL_MODE found in the constructor.");
	}

	GRAPHICS_HR_CHECK(_device->CreateDepthStencilState(&dsd, data.pDepthStencil.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

DepthStencil::~DepthStencil()
{
	delete (DepthStencilInternals*)BindableData;
}

// Binds the Depth Stencil State to the global context.

void DepthStencil::Bind()
{
	DepthStencilInternals& data = *(DepthStencilInternals*)BindableData;

	GRAPHICS_INFO_CHECK(_context->OMSetDepthStencilState(data.pDepthStencil.Get(), 0u));
}
