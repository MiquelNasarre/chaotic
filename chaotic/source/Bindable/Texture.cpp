#include "Bindable/Texture.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

// Structure to manage the Texture internal data.
struct TextureInternals
{
	ComPtr<ID3D11Texture2D> pTexture;
	ComPtr<ID3D11ShaderResourceView> pTextureView;
	Vector2i dimensions;
	unsigned slot;
	TEXTURE_USAGE usage;
	TEXTURE_TYPE type;
};

/*
--------------------------------------------------------------------------------------------
 Texture Functions
--------------------------------------------------------------------------------------------
*/

// Takes the Images reference and creates the texture in the GPU.

Texture::Texture(const Image* image, TEXTURE_USAGE usage, TEXTURE_TYPE type, unsigned slot)
{
	USER_CHECK(image,
		"Found nullptr when expecting an Image to create a Texture."
	);

	BindableData = new TextureInternals;
	TextureInternals& data = *(TextureInternals*)BindableData;
	data.slot = slot;
	data.usage = usage;
	data.type = type;
	data.dimensions = { image->width(), image->height() };

	switch (type)
	{
	case TEXTURE_TYPE_IMAGE:
	{
		// Create texture resource 

		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width				= image->width();
		textureDesc.Height				= image->height();
		textureDesc.Usage				= (usage == TEXTURE_USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		textureDesc.CPUAccessFlags		= (usage == TEXTURE_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0u;
		textureDesc.MipLevels			= 1u;
		textureDesc.ArraySize			= 1u;
		textureDesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count	= 1u;
		textureDesc.SampleDesc.Quality	= 0u;
		textureDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags			= 0u;
		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = image->pixels();
		sd.SysMemPitch = image->width() * sizeof(Color);

		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&textureDesc, &sd, data.pTexture.GetAddressOf()));

		// Create the resource view on the texture

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0u;
		srvDesc.Texture2D.MipLevels = 1u;

		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(data.pTexture.Get(), &srvDesc, data.pTextureView.GetAddressOf()));
		break;
	}

	case TEXTURE_TYPE_CUBEMAP:
	{
		USER_CHECK(image->width() * 6u == image->height(),
			"Invalid image dimensions found when trying to create a cubemap Texture.\n"
			"To create a cubemap Texture the 6 sides must be stacked on top of each other.\n"
			"Image dimensions must be (width, height = 6 * width)."
		);

		// Create cubemap texture resource

		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width				= image->width();
		textureDesc.Height				= image->width();
		textureDesc.Usage				= (usage == TEXTURE_USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		textureDesc.CPUAccessFlags		= (usage == TEXTURE_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0u;
		textureDesc.MipLevels			= 1u;
		textureDesc.ArraySize			= 6u;
		textureDesc.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count	= 1u;
		textureDesc.SampleDesc.Quality	= 0u;
		textureDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags			= D3D11_RESOURCE_MISC_TEXTURECUBE;

		D3D11_SUBRESOURCE_DATA psd[6] = {};
		const unsigned faceBytes = image->width() * image->width() * sizeof(Color);
		for (unsigned face = 0u; face < 6; face++)
		{
			psd[face].pSysMem = (byte*)image->pixels() + face * faceBytes;
			psd[face].SysMemPitch = image->width() * sizeof(Color);
		}
		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&textureDesc, psd, data.pTexture.GetAddressOf()));

		// Create the resource view on the texture

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0u;
		srvDesc.TextureCube.MipLevels = 1u;

		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(data.pTexture.Get(), &srvDesc, data.pTextureView.GetAddressOf()));
		break;
	}

	default:
		USER_ERROR("Unknown texture type found when trying to create a Texture.");
	}

}

// Releases the GPU pointer and deletes the data.

Texture::~Texture()
{
	delete (TextureInternals*)BindableData;
}

// Binds the Texture to the global context at the specified slot.

void Texture::Bind()
{
	TextureInternals& data = *(TextureInternals*)BindableData;

	GRAPHICS_INFO_CHECK(_context->PSSetShaderResources(data.slot, 1u, data.pTextureView.GetAddressOf()));
}

// Sets the slot at which the Texture will be bound.

void Texture::setSlot(unsigned slot)
{
	TextureInternals& data = *(TextureInternals*)BindableData;

	data.slot = slot;
}

// If usage is dynamic updates the texture with the new image.
// Dimensions must match the initial image dimensions.

void Texture::update(const Image* image)
{
	USER_CHECK(image,
		"Found nullptr when expecting an Image to update a Texture."
	);

	TextureInternals& data = *(TextureInternals*)BindableData;

	USER_CHECK(data.usage == TEXTURE_USAGE_DYNAMIC,
		"Trying to update a texture without dynamic usage.\n"
		"To use the update function on a Texture you should set TEXTURE_USAGE_DYNAMIC on the constructor."
	);

	USER_CHECK(data.dimensions == Vector2i(image->width(), image->height()),
		"Trying to update a texture with an image of different dimensions to the one used in the constructor."
	);

	switch (data.type)
	{
		case TEXTURE_TYPE_IMAGE:
		{
			// Create the mapping to the image data
			D3D11_MAPPED_SUBRESOURCE msr;
			GRAPHICS_HR_CHECK(_context->Map(data.pTexture.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &msr));

			// Copy the new image pixels
			const unsigned rowBytes = image->width() * sizeof(Color);
			for (unsigned y = 0; y < image->height(); y++)
				memcpy((byte*)msr.pData + y * msr.RowPitch, (byte*)image->pixels() + y * rowBytes, rowBytes);

			// Unmap the data
			GRAPHICS_INFO_CHECK(_context->Unmap(data.pTexture.Get(), 0u));
			break;
		}

		case TEXTURE_TYPE_CUBEMAP:
		{
			const unsigned faceBytes = image->width() * image->width() * sizeof(Color);
			const unsigned rowBytes = image->width() * sizeof(Color);
			for (unsigned face = 0u; face < 6u; face++)
			{
				// Create the mapping to the face data
				D3D11_MAPPED_SUBRESOURCE msr;
				GRAPHICS_HR_CHECK(_context->Map(data.pTexture.Get(), face, D3D11_MAP_WRITE_DISCARD, 0u, &msr));

				// Copy the new cube face pixels
				for (unsigned y = 0; y < image->height() / 6u; y++)
					memcpy((byte*)msr.pData + y * msr.RowPitch, (byte*)image->pixels() + face * faceBytes + y * rowBytes, rowBytes);

				// Unmap the data
				GRAPHICS_INFO_CHECK(_context->Unmap(data.pTexture.Get(), face));
			}
			break;
		}
	}
}
