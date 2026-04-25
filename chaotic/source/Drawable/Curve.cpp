#include "Drawable/Curve.h"
#include "Bindable/BindableBase.h"

#include "Error/_erDefault.h"
#include "chaotic/render/CurveGeometry.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

namespace
{
	chaotic::render::CurveColoring makeRenderCurveColoring(CURVE_DESC::CURVE_COLORING coloring)
	{
		switch (coloring)
		{
		case CURVE_DESC::FUNCTION_COLORING: return chaotic::render::CurveColoring::Function;
		case CURVE_DESC::LIST_COLORING: return chaotic::render::CurveColoring::List;
		case CURVE_DESC::GLOBAL_COLORING: return chaotic::render::CurveColoring::Global;
		default: return chaotic::render::CurveColoring::Global;
		}
	}

	chaotic::render::CurveDesc makeRenderCurveDesc(const CURVE_DESC& desc)
	{
		return {
			desc.curve_function,
			desc.range,
			desc.vertex_count,
			makeRenderCurveColoring(desc.coloring),
			desc.global_color,
			desc.color_function,
			desc.color_list,
			desc.enable_transparency,
			desc.enable_updates,
			desc.border_points_included
		};
	}
}

/*
-----------------------------------------------------------------------------------------------------------
 Curve Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Curve object.
struct CurveInternals
{
	chaotic::render::CurveGeometry geometry;

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

	delete& data;
}

// Initializes the Curve object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Curve::initialize(const CURVE_DESC* pDesc)
{
	USER_CHECK(pDesc,
		"Trying to initialize a Curve with an invalid descriptor pointer."
	);

	USER_CHECK(isInit == false,
		"Trying to initialize a Curve that has already been initialized."
	);

	isInit = true;

	curveData = new CurveInternals;
	CurveInternals& data = *(CurveInternals*)curveData;

	data.desc = *pDesc;

	USER_CHECK(data.desc.curve_function,
		"Found nullptr when trying to access a curve function to create a Curve."
	);

	USER_CHECK(data.desc.vertex_count >= 2u,
		"Found vertex count smaller than two when trying to create a Curve.\n"
		"You need at least a vertex count of two to initialize a Curve."
	);

	data.geometry.reset(makeRenderCurveDesc(data.desc));

	switch (data.desc.coloring)
	{
		case CURVE_DESC::GLOBAL_COLORING:
		{
			data.pUpdateVB = AddBind(new VertexBuffer(
				data.geometry.vertexData(),
				data.geometry.vertexStride(),
				data.geometry.vertexCount(),
				data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT
			));
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
			_float4color col = data.geometry.globalColor();
			data.pGlobalColorCB = AddBind(new ConstantBuffer(&col, PIXEL_CONSTANT_BUFFER, 1u /*Slot*/));
			break;
		}

		case CURVE_DESC::LIST_COLORING:
		{
			USER_CHECK(data.desc.color_list,
				"Found nullptr when trying to access a color list to create a Curve."
			);

			data.pUpdateVB = AddBind(new VertexBuffer(
				data.geometry.vertexData(),
				data.geometry.vertexStride(),
				data.geometry.vertexCount(),
				data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT
			));
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
			USER_CHECK(data.desc.color_function,
				"Found nullptr when trying to access a color function to create a Curve."
			);

			data.pUpdateVB = AddBind(new VertexBuffer(
				data.geometry.vertexData(),
				data.geometry.vertexStride(),
				data.geometry.vertexCount(),
				data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT
			));
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
			USER_ERROR("Found an unrecognized coloring mode when trying to create a Curve.");
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
	USER_CHECK(isInit,
		"Trying to update the vertices on an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the vertices on a Curve with updates disabled."
	);

	if (range)
		data.desc.range = range;

	data.geometry.updateRange(range);
	data.pUpdateVB->updateVertices(
		data.geometry.vertexData(),
		data.geometry.vertexStride(),
		data.geometry.vertexCount()
	);
}

// If updates are enabled, and coloring is with a list, this function allows to change 
// the current vertex colors for the new ones specified. It expects a valid pointer 
// with a list of colors as long as the vertex count.

void Curve::updateColors(Color* color_list)
{
	USER_CHECK(isInit,
		"Trying to update the colors on an uninitialized Curve."
	);

	USER_CHECK(color_list,
		"Trying to update the colors on a Curve with an invalid color list."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	USER_CHECK(data.desc.coloring == CURVE_DESC::LIST_COLORING,
		"Trying to update the colors on a Curve with a different coloring."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the colors on a Curve with updates disabled."
	);

	data.desc.color_list = color_list;
	data.geometry.updateColors(color_list);
	data.pUpdateVB->updateVertices(
		data.geometry.vertexData(),
		data.geometry.vertexStride(),
		data.geometry.vertexCount()
	);
}

// If the coloring is set to global, updates the global Curve color.

void Curve::updateGlobalColor(Color color)
{
	USER_CHECK(isInit,
		"Trying to update the global color on an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	USER_CHECK(data.desc.coloring == CURVE_DESC::GLOBAL_COLORING,
		"Trying to update the global color on a Curve with a different coloring."
	);

	data.desc.global_color = color;
	data.geometry.updateGlobalColor(color);

	_float4color col = data.geometry.globalColor();
	data.pGlobalColorCB->update(&col);
}

// Updates the rotation quaternion of the Curve. If multiplicative it will apply
// the rotation on top of the current rotation. For more information on how to rotate
// with quaternions check the Quaternion header file.

void Curve::updateRotation(Quaternion rotation, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the rotation on an uninitialized Curve."
	);

	USER_CHECK(rotation,
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
	USER_CHECK(isInit,
		"Trying to update the position on an uninitialized Curve."
	);

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
	USER_CHECK(isInit,
		"Trying to update the distortion on an uninitialized Curve."
	);

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
	USER_CHECK(isInit,
		"Trying to update the screen position on an uninitialized Curve."
	);

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
	USER_CHECK(isInit,
		"Trying to get the rotation of an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.rotation;
}

// Returns the current scene position.

Vector3f Curve::getPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the position of an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.position;
}

// Returns the current distortion matrix.

Matrix Curve::getDistortion() const
{
	USER_CHECK(isInit,
		"Trying to get the distortion matrix of an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	return data.distortion;
}

// Returns the current screen position.

Vector2f Curve::getScreenPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the screen position of an uninitialized Curve."
	);

	CurveInternals& data = *(CurveInternals*)curveData;

	return { data.vscBuff.displacement.x, data.vscBuff.displacement.y };
}
