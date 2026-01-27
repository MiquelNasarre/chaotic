#include "Drawable/Background.h"
#include "Bindable/BindableBase.h"

#include "Exception/_exDefault.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Background Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Background object.
struct BackgroundInternals
{
	struct alignas(16) VSconstBuffer 
	{
		Quaternion rot = 1.f;
		Vector2f fov = { 1.f,1.f };
	}projection = {};

	Vector2i imageDim = {};

	Texture* textureUpdates = nullptr;
	ConstantBuffer* pCBuff = nullptr;

	BACKGROUND_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Background constructor, if the pointer is valid it will call the initializer.

Background::Background(const BACKGROUND_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Background::~Background()
{
	if (!isInit)
		return;
	
	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	delete &data;
}

// Initializes the Background object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Background::initialize(const BACKGROUND_DESC* pDesc)
{
	if (!pDesc)
		throw INFO_EXCEPT("Trying to initialize a Background with an invalid descriptor pointer.");

	if (isInit)
		throw INFO_EXCEPT("Trying to initialize a Background that has already been initialized.");
	else
		isInit = true;

	backgroundData = new BackgroundInternals;
	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	data.desc = *pDesc;

	if (!data.desc.image)
		throw INFO_EXCEPT("Found nullptr when trying to access an Image to create a Background.");

	data.imageDim = { data.desc.image->width(), data.desc.image->height() };

	VertexShader* pvs;
	switch (data.desc.type)
	{
	case BACKGROUND_DESC::STATIC_BACKGROUND:
	{
#ifndef _DEPLOYMENT
		pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/BackgroundVS.cso"));
		AddBind(new PixelShader(PROJECT_DIR L"shaders/BackgroundPS.cso"));
#else
		pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_BACKGROUND_VS), getBlobSizeFromId(BLOB_ID::BLOB_BACKGROUND_VS)));
		AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_BACKGROUND_PS), getBlobSizeFromId(BLOB_ID::BLOB_BACKGROUND_PS)));
#endif

		_float4vector rectangle = { 0.f, 0.f, 1.f, 1.f };
		data.pCBuff = AddBind(new ConstantBuffer(&rectangle, VERTEX_CONSTANT_BUFFER));

		data.textureUpdates = AddBind(new Texture(data.desc.image, data.desc.texture_updates ? TEXTURE_USAGE_DYNAMIC : TEXTURE_USAGE_DEFAULT));
		break;
	}

	case BACKGROUND_DESC::DYNAMIC_BACKGROUND:
	{
#ifndef _DEPLOYMENT
		pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/DynamicBgVS.cso"));
		AddBind(new PixelShader(PROJECT_DIR L"shaders/DynamicBgPS.cso"));
#else
		pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_DYNAMIC_BG_VS), getBlobSizeFromId(BLOB_ID::BLOB_DYNAMIC_BG_VS)));
		AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_DYNAMIC_BG_PS), getBlobSizeFromId(BLOB_ID::BLOB_DYNAMIC_BG_PS)));
#endif

		data.pCBuff = AddBind(new ConstantBuffer(&data.projection, VERTEX_CONSTANT_BUFFER));

		data.textureUpdates = AddBind(new Texture(data.desc.image, data.desc.texture_updates ? TEXTURE_USAGE_DYNAMIC : TEXTURE_USAGE_DEFAULT, TEXTURE_TYPE_CUBEMAP));
		break;
	}

	default:
		throw INFO_EXCEPT("Unknown background type found when trying to initialize a Background.");
	}


	AddBind(new Sampler(data.desc.pixelated_texture ? SAMPLE_FILTER_POINT : SAMPLE_FILTER_LINEAR, SAMPLE_ADDRESS_CLAMP));

	// Quad full screen at the back.
	_float4vector vertexs[4] = 
	{ 
		{ -1.f, -1.f,  0.999999f, 1.f }, 
		{ +1.f, -1.f,  0.999999f, 1.f },
		{ -1.f, +1.f,  0.999999f, 1.f },
		{ +1.f, +1.f,  0.999999f, 1.f }
	};
	AddBind(new VertexBuffer(vertexs, 4u));

	INPUT_ELEMENT_DESC ied[1] = { { "Position", _4_FLOAT } };
	AddBind(new InputLayout(ied, 1u, pvs));

	unsigned indexs[4] = { 0u,1u,2u,3u };
	AddBind(new IndexBuffer(indexs, 4u));

	AddBind(new Rasterizer(false));
	AddBind(new Topology(TRIANGLE_STRIP));
	AddBind(new Blender(BLEND_MODE_OPAQUE));

	if (data.desc.override_buffers)
		AddBind(new DepthStencil(DEPTH_STENCIL_MODE_OVERRIDE));
}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// If updates are enabled, this function allows to update the background texture.
// Expects a valid image pointer to the new Background image, with the same dimensions
// as the image used in the constructor.

void Background::updateTexture(const Image* image)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the texture on an uninitialized Background.");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (!image)
		throw INFO_EXCEPT("Found nullptr when trying to access an Image to update a Background.");

	if (!data.desc.texture_updates)
		throw INFO_EXCEPT(
			"Trying to update the texture on a Background without updates enabled.\n"
			"To update the texture on a background set texture_updates on the descriptor to true."
		);

	data.textureUpdates->update(image);
}

// If the Background is dynamic, it updates the rotation quaternion of the scene. If 
// multiplicative it will apply the rotation on top of the current rotation. For more 
// information on how to rotate with quaternions check the Quaternion header file.

void Background::updateRotation(Quaternion rotation, bool multiplicative)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the rotation on an uninitialized Background.");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (data.desc.type != BACKGROUND_DESC::DYNAMIC_BACKGROUND)
		throw INFO_EXCEPT(
			"Trying to update the rotation on a non-dynamic Background. \n"
			"To update the Texture visualization on a non-dynamic Backgroud you can use updateRectangle."
		);

	if (!rotation)
		throw INFO_EXCEPT(
			"Invalid quaternion found when trying to update the rotation on a Background.\n"
			"Quaternion 0 can not be normalized and therefore can not describe a rotation."
		);

	if (multiplicative)
		data.projection.rot *= rotation.normalize();
	else
		data.projection.rot = rotation.normalize();

	data.pCBuff->update(&data.projection);
}

// If the Background is dynamic, it updates the field of view of the Background.
// By defalut the Vector is (1.0,1.0) and 1000 window pixels correspond to one cube 
// arista, in both x and y coordinates. FOV does not get affected by graphics scale.

void Background::updateFieldOfView(Vector2f FOV)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the wideness on an uninitialized Background.");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (data.desc.type != BACKGROUND_DESC::DYNAMIC_BACKGROUND)
		throw INFO_EXCEPT(
			"Trying to update the field of view on a non-dynamic Background. \n"
			"To update the Texture visualization on a non-dynamic Backgroud you can use updateRectangle."
		);

	data.projection.fov = FOV;
	data.pCBuff->update(&data.projection);
}

// If the background is static it updates the rectangle of Image pixels drawn. By 
// default it draws the entire image: min->(0,0) max->(width,height).

void Background::updateRectangle(Vector2i min_coords, Vector2i max_coords)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the rectangle view on an uninitialized Background.");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (data.desc.type != BACKGROUND_DESC::STATIC_BACKGROUND)
		throw INFO_EXCEPT(
			"Trying to update the rectangle view on a dynamic Background. \n"
			"To update the view on a dynamic Background the functions to call are updateRotation and updateFieldOfView."
		);

	_float4vector rectangle =
	{
		float(min_coords.x) / data.imageDim.x,
		float(min_coords.y) / data.imageDim.y,
		float(max_coords.x) / data.imageDim.x,
		float(max_coords.y) / data.imageDim.y
	};
	data.pCBuff->update(&rectangle);
}

/*
-----------------------------------------------------------------------------------------------------------
 Getters
-----------------------------------------------------------------------------------------------------------
*/

// Returns the Texture image dimensions.

Vector2i Background::getImageDim() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the image dimensions of an uninitialized Background.");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	return data.imageDim;
}

// Returns the current rotation quaternion.

Quaternion Background::getRotation() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the rotation of an uninitialized Background");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (data.desc.type != BACKGROUND_DESC::DYNAMIC_BACKGROUND)
		throw INFO_EXCEPT("Trying to get the rotation of a non-dynamic Background.");

	return data.projection.rot;
}

// Returns the current field of view.

Vector2f Background::getFieldOfView() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the field of view of an uninitialized Background");

	BackgroundInternals& data = *(BackgroundInternals*)backgroundData;

	if (data.desc.type != BACKGROUND_DESC::DYNAMIC_BACKGROUND)
		throw INFO_EXCEPT("Trying to get the field of view of a non-dynamic Background.");

	return data.projection.fov;
}

