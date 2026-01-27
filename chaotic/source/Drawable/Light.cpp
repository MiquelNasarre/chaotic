#include "Drawable/Light.h"
#include "Bindable/BindableBase.h"

#include "Exception/_exDefault.h"
#include <math.h>

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Light Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Light object.
struct LightInternals
{
	struct VSconstBuffer
	{
		Vector3f position = {};
		float radius = 1.f;
	}vscBuff;

	ConstantBuffer* pVSCB = nullptr;
	ConstantBuffer* pPSCB = nullptr;

	LIGHT_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Light constructor, if the pointer is valid it will call the initializer.

Light::Light(const LIGHT_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Light::~Light()
{
	if (!isInit)
		return;

	LightInternals& data = *(LightInternals*)lightData;

	delete &data;
}

// Initializes the Light object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Light::initialize(const LIGHT_DESC* pDesc)
{
	if (!pDesc)
		throw INFO_EXCEPT("Trying to initialize a Light with an invalid descriptor pointer.");

	if (isInit)
		throw INFO_EXCEPT("Trying to initialize a Light that has already been initialized.");
	else
		isInit = true;

	lightData = new LightInternals;
	LightInternals& data = *(LightInternals*)lightData;

	data.desc = *pDesc;

	if (data.desc.poligon_sides < 3u)
		throw INFO_EXCEPT(
			"Found poligon sides smaller than three when trying to create a Light.\n"
			"You need at least three poligon sides to initialize a Light."
		);

	// Make a circle of points and connect them.

	Vector2f* vertices = new Vector2f[data.desc.poligon_sides + 1];

	vertices[0u] = { 0.f,0.f };

	for (unsigned i = 1u; i < data.desc.poligon_sides + 1u; i++)
		vertices[i] = { cosf((2.f * MATH_PI * i) / data.desc.poligon_sides), sinf((2.f * MATH_PI * i) / data.desc.poligon_sides) };

	AddBind(new VertexBuffer(vertices, data.desc.poligon_sides + 1));

	unsigned int* indices = new unsigned[3u * data.desc.poligon_sides];

	for (unsigned i = 0u; i < data.desc.poligon_sides; i++)
	{
		indices[3 * i] = 0u;
		indices[3 * i + 1] = i + 1u;
		indices[3 * i + 2] = (i + 1u) % data.desc.poligon_sides + 1u;
	}

	AddBind(new IndexBuffer(indices, 3u * data.desc.poligon_sides));

	delete[] vertices;
	delete[] indices;

	// Add the constant buffers.

	data.vscBuff.position = data.desc.position;
	data.vscBuff.radius = data.desc.radius;
	data.pVSCB = AddBind(new ConstantBuffer(&data.vscBuff, VERTEX_CONSTANT_BUFFER));

	_float4color col = data.desc.color.getColor4();
	col = { col.r * data.desc.intensity / 100.f, col.g * data.desc.intensity / 100.f, col.b * data.desc.intensity / 100.f, 1.f };
	data.pPSCB = AddBind(new ConstantBuffer(&col, PIXEL_CONSTANT_BUFFER));

	// Then add all other binds.

#ifndef _DEPLOYMENT
	VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/LightVS.cso"));
#else
	VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_LIGHT_VS), getBlobSizeFromId(BLOB_ID::BLOB_LIGHT_VS)));
#endif

	INPUT_ELEMENT_DESC ied[1] =
	{
		{ "CirclePos", _2_FLOAT },
	};

	AddBind(new DepthStencil(DEPTH_STENCIL_MODE_NOWRITE));
	AddBind(new InputLayout(ied, 1u, pvs));
	AddBind(new Rasterizer(false));
	AddBind(new Topology(TRIANGLE_LIST));
	AddBind(new Blender(BLEND_MODE_ADDITIVE));
#ifndef _DEPLOYMENT
	AddBind(new PixelShader(PROJECT_DIR L"shaders/LightPS.cso"));
#else
	AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_LIGHT_PS), getBlobSizeFromId(BLOB_ID::BLOB_LIGHT_PS)));
#endif
}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// Updates the intensity value of the light.

void Light::updateIntensity(float intensity)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the intensity on an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	data.desc.intensity = intensity;

	_float4color col = data.desc.color.getColor4();
	col = { col.r * data.desc.intensity / 100.f, col.g * data.desc.intensity / 100.f, col.b * data.desc.intensity / 100.f, 1.f };

	data.pPSCB->update(&col);
}

// Updates the scene position of the Light. If additive it will add the vector
// to the current position vector of the Light.

void Light::updatePosition(Vector3f position, bool additive)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the position on an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	if (additive) position += data.desc.position;

	data.desc.position = position;
	data.vscBuff.position = position;

	data.pVSCB->update(&data.vscBuff);
}

// Updates the color of the Light.

void Light::updateColor(Color color)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the color on an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	data.desc.color = color;

	_float4color col = data.desc.color.getColor4();
	col = { col.r * data.desc.intensity / 100.f, col.g * data.desc.intensity / 100.f, col.b * data.desc.intensity / 100.f, 1.f };

	data.pPSCB->update(&col);
}

// Updates the maximum radius of the Light.

void Light::updateRadius(float radius)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the radius on an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	data.desc.radius = radius;
	data.vscBuff.radius = radius;

	data.pVSCB->update(&data.vscBuff);
}

/*
-----------------------------------------------------------------------------------------------------------
 Getters
-----------------------------------------------------------------------------------------------------------
*/

// Returns the current light intensity.

float Light::getIntensity() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the intensity of an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	return data.desc.intensity;
}

// Returns the current light color.

Color Light::getColor() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the color of an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	return data.desc.color;
}

// Returns the current scene position.

Vector3f Light::getPosition() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the position of an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	return data.desc.position;
}

// Returns the current maximum radius.

float Light::getRadius() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the radius of an uninitialized Light.");

	LightInternals& data = *(LightInternals*)lightData;

	return data.desc.radius;
}
