#include "Drawable/Scatter.h"
#include "Bindable/BindableBase.h"

#include "Error/_erDefault.h"
#include "chaotic/render/ScatterGeometry.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

namespace
{
	chaotic::render::ScatterColoring makeRenderScatterColoring(SCATTER_DESC::SCATTER_COLORING coloring)
	{
		switch (coloring)
		{
		case SCATTER_DESC::POINT_COLORING: return chaotic::render::ScatterColoring::Point;
		case SCATTER_DESC::GLOBAL_COLORING: return chaotic::render::ScatterColoring::Global;
		default: return chaotic::render::ScatterColoring::Global;
		}
	}

	chaotic::render::ScatterBlending makeRenderScatterBlending(SCATTER_DESC::BLENDING_MODE blending)
	{
		switch (blending)
		{
		case SCATTER_DESC::TRANSPARENT_POINTS: return chaotic::render::ScatterBlending::Transparent;
		case SCATTER_DESC::OPAQUE_POINTS: return chaotic::render::ScatterBlending::Opaque;
		case SCATTER_DESC::GLOWING_POINTS: return chaotic::render::ScatterBlending::Glow;
		default: return chaotic::render::ScatterBlending::Glow;
		}
	}

	chaotic::render::ScatterDesc makeRenderScatterDesc(const SCATTER_DESC& desc)
	{
		return {
			desc.point_list,
			desc.point_count,
			makeRenderScatterColoring(desc.coloring),
			desc.global_color,
			desc.color_list,
			makeRenderScatterBlending(desc.blending),
			desc.enable_updates,
			desc.line_mesh
		};
	}
}

/*
-----------------------------------------------------------------------------------------------------------
 Scatter Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Scatter object.
struct ScatterInternals
{
	chaotic::render::ScatterGeometry geometry;

	struct VSconstBuffer
	{
		_float4matrix transform =
		{
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f,
		};
		_float4vector displacement = { 0.f, 0.f, 0.f, 0.f };
	}
	vscBuff = {};

	Matrix distortion = Matrix(1.f);
	Quaternion rotation = 1.f;
	Vector3f position = {};

	VertexBuffer* pUpdateVB = nullptr;

	ConstantBuffer* pVSCB = nullptr;
	ConstantBuffer* pGlobalColorCB = nullptr;

	SCATTER_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Scatter constructor, if the pointer is valid it will call the initializer.

Scatter::Scatter(const SCATTER_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Scatter::~Scatter()
{
	if (!isInit)
		return;

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	delete& data;
}

// Initializes the Scatter object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Scatter::initialize(const SCATTER_DESC* pDesc)
{
	USER_CHECK(pDesc,
		"Trying to initialize a Scatter with an invalid descriptor pointer."
	);

	USER_CHECK(isInit == false,
		"Trying to initialize a Scatter that has already been initialized."
	);

	isInit = true;

	scatterData = new ScatterInternals;
	ScatterInternals& data = *(ScatterInternals*)scatterData;

	data.desc = *pDesc;

	USER_CHECK(data.desc.point_list,
		"Found nullptr when trying to access a point list to create a Scatter."
	);

	USER_CHECK(data.desc.point_count,
		"Found zero point count when trying to create a Scatter.\n"
		"You need at least a point count of one to initialize a Scatter."
	);

	USER_CHECK(!data.desc.line_mesh || data.desc.point_count % 2u == 0u,
		"Found an odd number of points when trying to initialize a line-mesh Scatter.\n"
		"If you want to set the Scatter to line-mesh the number of points must be divisible by two."
	);

	data.geometry.reset(makeRenderScatterDesc(data.desc));

	switch (data.desc.coloring)
	{
	case SCATTER_DESC::GLOBAL_COLORING:
	{
		data.pUpdateVB = AddBind(new VertexBuffer(
			data.geometry.vertexData(),
			data.geometry.vertexStride(),
			data.geometry.pointCount(),
			data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT
		));
		// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
		VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/CurveVS.cso"));
#else
		VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_CURVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_CURVE_VS)));
#endif
		// Create the corresponding Pixel Shader and Blender
		switch (data.desc.blending)
		{
		case SCATTER_DESC::OPAQUE_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_OPAQUE));
			break;

		case SCATTER_DESC::GLOWING_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_ADDITIVE));
			AddBind(new DepthStencil(DEPTH_STENCIL_MODE_NOWRITE));
			break;

		case SCATTER_DESC::TRANSPARENT_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
			break;
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

	case SCATTER_DESC::POINT_COLORING:
	{
		USER_CHECK(data.desc.color_list,
			"Found nullptr when trying to access a color list to create a Scatter."
		);

		data.pUpdateVB = AddBind(new VertexBuffer(
			data.geometry.vertexData(),
			data.geometry.vertexStride(),
			data.geometry.pointCount(),
			data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT
		));
		// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
		VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/ColorCurveVS.cso"));
#else
		VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_COLOR_CURVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_COLOR_CURVE_VS)));
#endif
		// Create the corresponding Pixel Shader and Blender
// Create the corresponding Pixel Shader and Blender
		switch (data.desc.blending)
		{
		case SCATTER_DESC::OPAQUE_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_OPAQUE));
			break;

		case SCATTER_DESC::GLOWING_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_ADDITIVE));
			AddBind(new DepthStencil(DEPTH_STENCIL_MODE_NOWRITE));
			break;

		case SCATTER_DESC::TRANSPARENT_POINTS:
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
			AddBind(new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)));
#endif
			AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
			break;
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
		USER_ERROR("Found an unrecognized coloring mode when trying to create a Scatter.");
	}

	unsigned* indexs = new unsigned[data.desc.point_count];
	for (unsigned i = 0; i < data.desc.point_count; i++)
		indexs[i] = i;

	AddBind(new IndexBuffer(indexs, data.desc.point_count));
	delete[] indexs;

	AddBind(new Topology(data.desc.line_mesh ? LINE_LIST : POINT_LIST));
	AddBind(new Rasterizer());

	data.pVSCB = AddBind(new ConstantBuffer(&data.vscBuff, VERTEX_CONSTANT_BUFFER));
}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// If updates are enabled this function allows to change the position of the points.
// It expects a valid pointer to a 3D vector list as long as the point count, it will 
// copy the position data and send it to the GPU for drawing.

void Scatter::updatePoints(Vector3f* point_list)
{
	USER_CHECK(isInit,
		"Trying to update the points on an uninitialized Scatter."
	);

	USER_CHECK(point_list,
		"Trying to update the points on a Scatter with an invalid point list."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the points on a Scatter with updates disabled."
	);

	data.desc.point_list = point_list;
	data.geometry.updatePoints(point_list);
	data.pUpdateVB->updateVertices(
		data.geometry.vertexData(),
		data.geometry.vertexStride(),
		data.geometry.pointCount()
	);
}

// If updates are enabled, and coloring is with a list, this function allows to change 
// the current point colors for the new ones specified. It expects a valid pointer 
// with a list of colors as long as the point count.

void Scatter::updateColors(Color* color_list)
{
	USER_CHECK(isInit,
		"Trying to update the colors on an uninitialized Scatter."
	);

	USER_CHECK(color_list,
		"Trying to update the colors on a Scatter with an invalid color list."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	USER_CHECK(data.desc.coloring == SCATTER_DESC::POINT_COLORING,
		"Trying to update the colors on a Scatter with a different coloring."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the colors on a Scatter with updates disabled."
	);

	data.desc.color_list = color_list;
	data.geometry.updateColors(color_list);
	data.pUpdateVB->updateVertices(
		data.geometry.vertexData(),
		data.geometry.vertexStride(),
		data.geometry.pointCount()
	);
}

// If the coloring is set to global, updates the global Scatter color.

void Scatter::updateGlobalColor(Color color)
{
	USER_CHECK(isInit,
		"Trying to update the global color on an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	USER_CHECK(data.desc.coloring == SCATTER_DESC::GLOBAL_COLORING,
		"Trying to update the global color on a Scatter with a different coloring."
	);

	data.desc.global_color = color;
	data.geometry.updateGlobalColor(color);

	_float4color col = data.geometry.globalColor();
	data.pGlobalColorCB->update(&col);
}

// Updates the rotation quaternion of the Scatter. If multiplicative it will apply
// the rotation on top of the current rotation. For more information on how to rotate
// with quaternions check the Quaternion header file.

void Scatter::updateRotation(Quaternion rotation, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the rotation on an uninitialized Scatter."
	);

	USER_CHECK(rotation,
		"Invalid quaternion found when trying to update rotation on a Scatter.\n"
		"Quaternion 0 can not be normalized and therefore can not describe an objects rotation."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	if (multiplicative)
		data.rotation *= rotation.normal();
	else
		data.rotation = rotation;

	data.rotation.normalize();

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the scene position of the Scatter. If additive it will add the vector
// to the current position vector of the Scatter.

void Scatter::updatePosition(Vector3f position, bool additive)
{
	USER_CHECK(isInit,
		"Trying to update the position on an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	if (additive)
		data.position += position;
	else
		data.position = position;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the matrix multiplied to the Scatter, adding any arbitrary linear distortion to 
// the points position. If you want to simply modify the size of the scatter just call this 
// function on a diagonal matrix. Check the Matrix header for helpers to create any arbitrary 
// distortion. If multiplicative the distortion will be added to the current distortion.

void Scatter::updateDistortion(Matrix distortion, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the distortion on an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	if (multiplicative)
		data.distortion = distortion * data.distortion;
	else
		data.distortion = distortion;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the screen displacement of the points. To be used if you intend to render 
// multiple scenes/plots on the same render target.

void Scatter::updateScreenPosition(Vector2f screenDisplacement)
{
	USER_CHECK(isInit,
		"Trying to update the screen position on an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	data.vscBuff.displacement = screenDisplacement.getVector4();
	data.pVSCB->update(&data.vscBuff);
}

/*
-----------------------------------------------------------------------------------------------------------
 Getters
-----------------------------------------------------------------------------------------------------------
*/

// Returns the current rotation quaternion.

Quaternion Scatter::getRotation() const
{
	USER_CHECK(isInit,
		"Trying to get the rotation of an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	return data.rotation;
}

// Returns the current scene position.

Vector3f Scatter::getPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the position of an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	return data.position;
}

// Returns the current distortion matrix.

Matrix Scatter::getDistortion() const
{
	USER_CHECK(isInit,
		"Trying to get the distortion matrix of an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	return data.distortion;
}

// Returns the current screen position.

Vector2f Scatter::getScreenPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the screen position of an uninitialized Scatter."
	);

	ScatterInternals& data = *(ScatterInternals*)scatterData;

	return { data.vscBuff.displacement.x, data.vscBuff.displacement.y };
}
