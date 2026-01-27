#include "Bindable/Blender.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Blender internal data.
struct BlenderInternals
{
	ComPtr<ID3D11BlendState> pBlender;
	BLEND_MODE mode;
};

/*
--------------------------------------------------------------------------------------------
 Blender Functions
--------------------------------------------------------------------------------------------
*/

// Takes the blending mode as an input and creates the blending state accordingly.

Blender::Blender(BLEND_MODE mode)
{
	BindableData = new BlenderInternals;
	BlenderInternals& data = *(BlenderInternals*)BindableData;
	data.mode = mode;

	D3D11_BLEND_DESC blendDesc = {};
	auto& brt = blendDesc.RenderTarget[0];

	switch (mode)
	{
	case BLEND_MODE_OPAQUE:
		brt.BlendEnable				= false;
		brt.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;
		break;

	case BLEND_MODE_ALPHA:
		brt.BlendEnable				= true;
		brt.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;

		// Color: C_out = C_src * A_src + C_dst * (1 - A_src)
		brt.SrcBlend				= D3D11_BLEND_SRC_ALPHA;
		brt.DestBlend				= D3D11_BLEND_INV_SRC_ALPHA;
		brt.BlendOp					= D3D11_BLEND_OP_ADD;

		// Alpha: Source alpha
		brt.SrcBlendAlpha			= D3D11_BLEND_ONE;
		brt.DestBlendAlpha			= D3D11_BLEND_ZERO;
		brt.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		break;

	case BLEND_MODE_ADDITIVE:
		brt.BlendEnable				= true;
		brt.RenderTargetWriteMask	= D3D11_COLOR_WRITE_ENABLE_ALL;

		// Color: C_out = C_src + C_dst
		brt.SrcBlend				= D3D11_BLEND_ONE;
		brt.DestBlend				= D3D11_BLEND_ONE;
		brt.BlendOp					= D3D11_BLEND_OP_ADD;

		// Alpha: Keep destination alpha.
		brt.SrcBlendAlpha			= D3D11_BLEND_ZERO;
		brt.DestBlendAlpha			= D3D11_BLEND_ONE;
		brt.BlendOpAlpha			= D3D11_BLEND_OP_ADD;
		break;

	case BLEND_MODE_OIT_WEIGHTED:
		return; // Graphics takes care of all the OIT pipeline.
	};

	GFX_THROW_INFO(_device->CreateBlendState(&blendDesc, data.pBlender.GetAddressOf()));
}

// Releases the GPU pointer and deletes the data.

Blender::~Blender()
{
	delete (BlenderInternals*)BindableData;
}

// Binds the Blender to the global context.

void Blender::Bind()
{
	BlenderInternals& data = *(BlenderInternals*)BindableData;

	if (data.mode == BLEND_MODE_OIT_WEIGHTED)
		return;	// Graphics will take care of the rest.

	GFX_THROW_INFO_ONLY(_context->OMSetBlendState(data.pBlender.Get(), nullptr, 0xFFFFFFFFu));
}

// To be called by the draw call to check for OIT.

BLEND_MODE Blender::getMode()
{
	BlenderInternals& data = *(BlenderInternals*)BindableData;

	return data.mode;
}
