#include "Drawable/Curve.h"
#include "Bindable/BindableBase.h"

#include "Exception/_exDefault.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Curve Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Curve object.
struct CurveInternals
{
	_float4vector* Vertices = nullptr;

	struct ColVertex 
	{
		_float4vector position;
		_float4color color;
	}*ColVertices = nullptr;

	struct VSconstBuffer
	{
		_float4matrix transform = 
		{
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f,
		};
		_float4vector displacement	= { 0.f, 0.f, 0.f, 0.f };
	}
	vscBuff = {};

	Matrix distortion = Matrix(1.f);
	Quaternion rotation	= 1.f;
	Vector3f position = {};

	VertexBuffer* pUpdateVB = nullptr;

	ConstantBuffer* pVSCB = nullptr;
	ConstantBuffer* pGlobalColorCB = nullptr;

	CURVE_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Curve constructor, if the pointer is valid it will call the initializer.

Curve::Curve(const CURVE_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Curve::~Curve()
{
	if (!isInit)
		return;

	CurveInternals& data = *(CurveInternals*)curveData;

	if (data.Vertices)
		delete[] data.Vertices;

	if (data.ColVertices)
		delete[] data.ColVertices;

	delete& data;
}

// Initializes the Curve object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Curve::initialize(const CURVE_DESC* pDesc)
{
	if (!pDesc)
		throw INFO_EXCEPT("Trying to initialize a Curve with an invalid descriptor pointer.");

	if (isInit)
		throw INFO_EXCEPT("Trying to initialize a Curve that has already been initialized.");
	else
		isInit = true;

	curveData = new CurveInternals;
	CurveInternals& data = *(CurveInternals*)curveData;

	data.desc = *pDesc;

	if (!data.desc.curve_function)
		throw INFO_EXCEPT("Found nullptr when trying to access a curve function to create a Curve.");

	if (data.desc.vertex_count < 2u)
		throw INFO_EXCEPT(
			"Found vertex count smaller than two when trying to create a Curve.\n"
			"You need at least a vertex count of two to initialize a Curve."
		);

	// Define the t_values where the generator function will be evaluated.
	float dt = data.desc.border_points_included ?
		(data.desc.range.y - data.desc.range.x) / (data.desc.vertex_count - 1.f) :
		(data.desc.range.y - data.desc.range.x) / (data.desc.vertex_count + 1.f);

	float t_i = data.desc.border_points_included ? data.desc.range.x : data.desc.range.x + dt;

	switch (data.desc.coloring)
	{
		case CURVE_DESC::GLOBAL_COLORING:
		{
			data.Vertices = new _float4vector[data.desc.vertex_count];

			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
				data.Vertices[n] = data.desc.curve_function(t_i + n * dt).getVector4();

			data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, data.desc.vertex_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

			// If updates disabled delete the vertices
			if (!data.desc.enable_updates)
			{
				delete[] data.Vertices;
				data.Vertices = nullptr;
			}
			// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
			VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/CurveVS.cso"));
#else
			VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_CURVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_CURVE_VS)));
#endif
			// Create the corresponding Pixel Shader and Blender
			if (data.desc.enable_transparency)
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
			}
			else
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OPAQUE));
			}
			// Create the corresponding input layout
			INPUT_ELEMENT_DESC ied[1] =
			{
				{ "Position",	_4_FLOAT },
			};
			AddBind(new InputLayout(ied, 1u, pvs));

			// Create the constant buffer for the global color.
			_float4color col = data.desc.global_color.getColor4();
			data.pGlobalColorCB = AddBind(new ConstantBuffer(&col, PIXEL_CONSTANT_BUFFER, 1u /*Slot*/));
			break;
		}

		case CURVE_DESC::LIST_COLORING:
		{
			if (!data.desc.color_list)
				throw INFO_EXCEPT("Found nullptr when trying to access a color list to create a Curve.");

			data.ColVertices = new CurveInternals::ColVertex[data.desc.vertex_count];

			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
			{
				data.ColVertices[n].position = data.desc.curve_function(t_i + n * dt).getVector4();
				data.ColVertices[n].color = data.desc.color_list[n].getColor4();
			}

			data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.vertex_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

			// If updates disabled delete the vertices
			if (!data.desc.enable_updates)
			{
				delete[] data.ColVertices;
				data.ColVertices = nullptr;
			}
			// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
			VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/ColorCurveVS.cso"));
#else
			VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_COLOR_CURVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_COLOR_CURVE_VS)));
#endif
			// Create the corresponding Pixel Shader and Blender
			if (data.desc.enable_transparency)
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
			}
			else
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OPAQUE));
			}
			// Create the corresponding input layout
			INPUT_ELEMENT_DESC ied[2] =
			{
				{ "Position",	_4_FLOAT },
				{ "Color",		_4_FLOAT },
			};
			AddBind(new InputLayout(ied, 2u, pvs));
			break;
		}

		case CURVE_DESC::FUNCTION_COLORING:
		{
			if (!data.desc.color_function)
				throw INFO_EXCEPT("Found nullptr when trying to access a color function to create a Curve.");

			data.ColVertices = new CurveInternals::ColVertex[data.desc.vertex_count];

			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
			{
				data.ColVertices[n].position = data.desc.curve_function(t_i + n * dt).getVector4();
				data.ColVertices[n].color = data.desc.color_function(t_i + n * dt).getColor4();
			}

			data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.vertex_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

			// If updates disabled delete the vertices
			if (!data.desc.enable_updates)
			{
				delete[] data.ColVertices;
				data.ColVertices = nullptr;
			}
			// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
			VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/ColorCurveVS.cso"));
#else
			VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_COLOR_CURVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_COLOR_CURVE_VS)));
#endif
			// Create the corresponding Pixel Shader and Blender
			if (data.desc.enable_transparency)
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
			}
			else
			{
#ifndef _DEPLOYMENT
				AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
				AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)));
#endif
				AddBind(new Blender(BLEND_MODE_OPAQUE));
			}
			// Create the corresponding input layout
			INPUT_ELEMENT_DESC ied[2] =
			{
				{ "Position",	_4_FLOAT },
				{ "Color",		_4_FLOAT },
			};
			AddBind(new InputLayout(ied, 2u, pvs));
			break;
		}

		default:
			throw INFO_EXCEPT("Found an unrecognized coloring mode when trying to create a Curve.");
	}

	unsigned* indexs = new unsigned[data.desc.vertex_count];
	for (unsigned i = 0; i < data.desc.vertex_count; i++)
		indexs[i] = i;

	AddBind(new IndexBuffer(indexs, data.desc.vertex_count));
	delete[] indexs;

	AddBind(new Topology(LINE_STRIP));
	AddBind(new Rasterizer());

	data.pVSCB = AddBind(new ConstantBuffer(&data.vscBuff, VERTEX_CONSTANT_BUFFER));
}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// If updates are enabled this function allows to change the range of the curve function. It 
// expects the initial function pointer to still be callable, it will evaluate it on the new 
// range and send the vertices to the GPU. If coloring is functional it also expects the color 
// function to still be callable, if coloring is not functional it will reuse the old colors.

void Curve::updateRange(Vector2f range)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the vertices on an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	if (!data.desc.enable_updates)
		throw INFO_EXCEPT("Trying to update the vertices on a Curve with updates disabled.");

	// Define the t_values where the generator function will be evaluated.
	float dt = data.desc.border_points_included ?
		(range.y - range.x) / (data.desc.vertex_count - 1.f) :
		(range.y - range.x) / (data.desc.vertex_count + 1.f);

	float t_i = data.desc.border_points_included ? range.x : range.x + dt;

	switch (data.desc.coloring)
	{
		case CURVE_DESC::GLOBAL_COLORING:
		{
			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
				data.Vertices[n] = data.desc.curve_function(t_i + n * dt).getVector4();

			data.pUpdateVB->updateVertices(data.Vertices, data.desc.vertex_count);
			break;
		}

		case CURVE_DESC::LIST_COLORING:
		{
			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
				data.ColVertices[n].position = data.desc.curve_function(t_i + n * dt).getVector4();

			data.pUpdateVB->updateVertices(data.ColVertices, data.desc.vertex_count);
			break;
		}

		case CURVE_DESC::FUNCTION_COLORING:
		{
			for (unsigned n = 0u; n < data.desc.vertex_count; n++)
			{
				data.ColVertices[n].position = data.desc.curve_function(t_i + n * dt).getVector4();
				data.ColVertices[n].color = data.desc.color_function(t_i + n * dt).getColor4();
			}

			data.pUpdateVB->updateVertices(data.ColVertices, data.desc.vertex_count);
			break;
		}
	}
}

// If updates are enabled, and coloring is with a list, this function allows to change 
// the current vertex colors for the new ones specified. It expects a valid pointer 
// with a list of colors as long as the vertex count.

void Curve::updateColors(Color* color_list)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the colors on an uninitialized Curve.");

	if (!color_list)
		throw INFO_EXCEPT("Trying to update the colors on a Curve with an invalid color list.");

	CurveInternals& data = *(CurveInternals*)curveData;

	if (data.desc.coloring != CURVE_DESC::LIST_COLORING)
		throw INFO_EXCEPT("Trying to update the colors on a Curve with a different coloring.");

	if (!data.desc.enable_updates)
		throw INFO_EXCEPT("Trying to update the colors on a Curve with updates disabled.");

	for (unsigned i = 0u; i < data.desc.vertex_count; i++)
		data.ColVertices[i].color = color_list[i].getColor4();

	data.pUpdateVB->updateVertices(data.ColVertices, data.desc.vertex_count);
}

// If the coloring is set to global, updates the global Curve color.

void Curve::updateGlobalColor(Color color)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the global color on an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	if (data.desc.coloring != CURVE_DESC::GLOBAL_COLORING)
		throw INFO_EXCEPT("Trying to update the global color on a Curve with a different coloring.");

	_float4color col = color.getColor4();
	data.pGlobalColorCB->update(&col);
}

// Updates the rotation quaternion of the Curve. If multiplicative it will apply
// the rotation on top of the current rotation. For more information on how to rotate
// with quaternions check the Quaternion header file.

void Curve::updateRotation(Quaternion rotation, bool multiplicative)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the rotation on an uninitialized Curve.");

	if (!rotation)
		throw INFO_EXCEPT(
			"Invalid quaternion found when trying to update rotation on a Curve.\n"
			"Quaternion 0 can not be normalized and therefore can not describe an objects rotation."
		);

	CurveInternals& data = *(CurveInternals*)curveData;

	if (multiplicative)
		data.rotation *= rotation.normal();
	else
		data.rotation = rotation;

	data.rotation.normalize();

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the scene position of the Curve. If additive it will add the vector
// to the current position vector of the Curve.

void Curve::updatePosition(Vector3f position, bool additive)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the position on an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	if (additive)
		data.position += position;
	else
		data.position = position;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the matrix multiplied to the Curve, adding any arbitrary linear distortion to 
// it. If you want to simply modify the size of the object just call this function on a 
// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
// If multiplicative the distortion will be added to the current distortion.

void Curve::updateDistortion(Matrix distortion, bool multiplicative)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the distortion on an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	if (multiplicative)
		data.distortion = distortion * data.distortion;
	else
		data.distortion = distortion;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the screen displacement of the figure. To be used if you intend to render 
// multiple scenes/plots on the same render target.

void Curve::updateScreenPosition(Vector2f screenDisplacement)
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to update the screen position on an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	data.vscBuff.displacement = screenDisplacement.getVector4();
	data.pVSCB->update(&data.vscBuff);
}

/*
-----------------------------------------------------------------------------------------------------------
 Getters
-----------------------------------------------------------------------------------------------------------
*/

// Returns the current rotation quaternion.

Quaternion Curve::getRotation() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the rotation of an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.rotation;
}

// Returns the current scene position.

Vector3f Curve::getPosition() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the position of an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.position;
}

// Returns the current distortion matrix.

Matrix Curve::getDistortion() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the distortion matrix of an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.distortion;
}

// Returns the current screen position.

Vector2f Curve::getScreenPosition() const
{
	if (!isInit)
		throw INFO_EXCEPT("Trying to get the screen position of an uninitialized Curve.");

	CurveInternals& data = *(CurveInternals*)curveData;

	return { data.vscBuff.displacement.x, data.vscBuff.displacement.y };
}
