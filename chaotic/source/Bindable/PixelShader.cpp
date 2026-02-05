#include "Bindable/PixelShader.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

#include <d3dcompiler.h> // For D3DReadFileToBlob()

// Structure to manage the Pixel Shader internal data.
struct PixelShaderInternals
{
	ComPtr<ID3D11PixelShader> pPixelShader;
};

/*
--------------------------------------------------------------------------------------------
 Pixel Shader Functions
--------------------------------------------------------------------------------------------
*/

// Given a (*.cso) file path it creates the bytecode and the pixel shader object.

PixelShader::PixelShader(const wchar_t* path)
{
	BindableData = new PixelShaderInternals;
	PixelShaderInternals& data = *(PixelShaderInternals*)BindableData;

	ComPtr<ID3DBlob> pBlob;
    GRAPHICS_HR_CHECK(D3DReadFileToBlob(path, &pBlob));
    GRAPHICS_HR_CHECK(_device->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, data.pPixelShader.GetAddressOf()));
}

// Raw constructor for deployment, uses embedded resources. Expects valid blobs.

PixelShader::PixelShader(const void* bytecode, unsigned long long size)
{
    BindableData = new PixelShaderInternals;
    auto& ps = *static_cast<PixelShaderInternals*>(BindableData);

    GRAPHICS_HR_CHECK(
        _device->CreatePixelShader(
            bytecode,
            size,
            nullptr,
            ps.pPixelShader.GetAddressOf()
        )
    );
}

// Releases the pointers and deletes the internal data.

PixelShader::~PixelShader()
{
	delete (PixelShaderInternals*)BindableData;
}

// Binds the Pixel Shader to the global context.

void PixelShader::Bind()
{
	PixelShaderInternals& data = *(PixelShaderInternals*)BindableData;

    GRAPHICS_INFO_CHECK(_context->PSSetShader(data.pPixelShader.Get(), NULL, 0u));
}
