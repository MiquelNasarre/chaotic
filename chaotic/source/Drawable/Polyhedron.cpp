#include "Drawable/Polyhedron.h"
#include "Bindable/BindableBase.h"

#include "Error/_erDefault.h"

#include <cstdio> // For mesh support
#include <cstdlib> // For mesh support

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Polyhedron Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Polyhedron object.
struct PolyhedronInternals
{
	struct Vertex
	{
		_float4vector vector = {};
		_float4vector norm = {};
	}*Vertices = nullptr;

	struct ColorVertex
	{
		_float4vector vector = {};
		_float4vector norm = {};
		_float4color color = {};
	}*ColVertices = nullptr;

	struct TextureVertex
	{
		_float4vector vector = {};
		_float4vector norm = {};
		_float4vector coord = {};
	}*TexVertices = nullptr;

	unsigned image_width = 0u;
	unsigned image_height = 0u;

	struct VSconstBuffer
	{
		_float4matrix transform =
		{
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f,
		};
		_float4matrix normal_transform =
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
	Quaternion rotation = 1.f;
	Vector3f position = {};

	struct PSconstBuffer
	{
		struct {
			_float4vector intensity = { 0.f, 0.f, 0.f, 0.f };
			_float4color  color		= { 0.f, 0.f, 0.f, 0.f };
			_float4vector position	= { 0.f, 0.f, 0.f, 0.f };
		}lightsource[8];
	}pscBuff;

	ConstantBuffer* pVSCB = nullptr;
	ConstantBuffer* pPSCB = nullptr;

	ConstantBuffer* pGlobalColorCB = nullptr;

	VertexBuffer* pUpdateVB = nullptr;

	POLYHEDRON_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Triangle mesh formatting support
-----------------------------------------------------------------------------------------------------------
*/

// To facilitate the loading of triangle meshes, this function is a parser for 
// *.obj files, that reads the files and outputs a valid descriptor. If the file 
// supports texturing, optionally accepts an image to be used as texture_image.
// NOTE: All data is allocated by (new) and its deletion must be handled by the 
// user. The image pointer used is the same as provided.

POLYHEDRON_DESC Polyhedron::getDescFromObj(const char* obj_file_path, Image* texture)
{
	// First let's open the file.
	FILE* file = nullptr;
	fopen_s(&file, obj_file_path, "r");
	USER_CHECK(file,
		"OBJ parse error: Unable to open OBJ file."
	);

	POLYHEDRON_DESC desc = {};
	desc.texture_image = texture;

	// Define convenient helpers

	auto startsWith = [](const char* s, const char* prefix) -> bool
		{
			while (*prefix)
			{
				if (*s++ != *prefix++) return false;
			}
			return true;
		};

	auto objToZeroBased = [](long idx, int countSoFar)
		{
			if (idx > 0) return (int)(idx - 1);
			if (idx < 0) return (int)(countSoFar + idx); // idx is negative
			return -1; // invalid in OBJ
		};

	auto parseFaceCornerToken = [objToZeroBased](
		const char* token,
		int vpCountSoFar, int vtCountSoFar, int vnCountSoFar,
		int& out_vp, int& out_vt, int& out_vn)
		{
			out_vp = out_vt = out_vn = -1;

			const char* p = token;

			// v
			char* end = nullptr;
			long v = std::strtol(p, &end, 10);
			USER_CHECK(end != p, "OBJ parse error: face token missing vertex index.");
			out_vp = objToZeroBased(v, vpCountSoFar);
			p = end;

			if (*p == '\0') return;
			USER_CHECK(*p == '/', "OBJ parse error: invalid face token format.");

			++p; // skip '/'

			// Could be v//vn or v/vt[/vn]
			if (*p != '/')
			{
				long vt = std::strtol(p, &end, 10);
				if (end != p) out_vt = objToZeroBased(vt, vtCountSoFar);
				p = end;
			}

			if (*p == '\0') return;
			USER_CHECK(*p == '/', "OBJ parse error: invalid face token format.");

			++p; // skip '/'

			// vn (may be empty in malformed files; treat as missing)
			if (*p != '\0')
			{
				long vn = std::strtol(p, &end, 10);
				if (end != p) out_vn = objToZeroBased(vn, vnCountSoFar);
			}
		};

	auto countFaceCorners = [](char* lineAfterF)
		{
			// Counts tokens separated by whitespace
			int count = 0;
			char* p = lineAfterF;
			while (*p)
			{
				while (*p == ' ' || *p == '\t') ++p;
				if (*p == '\0' || *p == '\n' || *p == '\r') break;

				// token start
				++count;
				while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') ++p;
			}
			return count;
		};

	// Pass 1: count

	unsigned vertexCount = 0;
	unsigned uvCount = 0;
	unsigned normalCount = 0;
	unsigned triangleCount = 0;

	int W = 0, H = 0;
	if (texture)
		W = texture->width(), H = texture->height();

	{
		char line[2048];
		while (fgets(line, sizeof(line), file))
		{
			char* p = line;
			while (*p == ' ' || *p == '\t') ++p;

			if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') continue;

			if (startsWith(p, "v "))
				++vertexCount;

			else if (startsWith(p, "vt "))
				++uvCount;

			else if (startsWith(p, "vn "))
				++normalCount;

			else if (startsWith(p, "f "))
			{
				p += 2; // skip "f "
				int corners = countFaceCorners(p);
				USER_CHECK(corners >= 3, "OBJ parse error: face has fewer than 3 vertices.");
				triangleCount += (unsigned)(corners - 2);
			}
		}
	}

	if (vertexCount == 0 || triangleCount == 0)
	{
		fclose(file);
		USER_ERROR("OBJ parse error: file contains no vertices or no faces.");
	}

	// Allocate arrays owned by the user
	desc.vertex_list = new Vector3f[vertexCount];
	desc.triangle_list = new Vector3i[triangleCount];
	desc.triangle_count = triangleCount;

	// Temporary raw arrays for vt/vn
	Vector2f* rawUV = nullptr;
	Vector3f* rawNrm = nullptr;
	if (uvCount) rawUV = new Vector2f[uvCount];
	if (normalCount) rawNrm = new Vector3f[normalCount];

	// Decide whether to allocate per-triangle UVs and normals
	bool wantTextured = (texture != nullptr) && (uvCount > 0);
	bool wantPerTriNormals = (normalCount > 0);

	// Set parameters
	if (wantTextured)
	{
		desc.coloring = POLYHEDRON_DESC::TEXTURED_COLORING;
		desc.texture_coordinates_list = new Vector2i[triangleCount * 3u];
	}
	else
		desc.coloring = POLYHEDRON_DESC::GLOBAL_COLORING;

	if (wantPerTriNormals)
	{
		desc.normal_computation = POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS;
		desc.normal_vectors_list = new Vector3f[triangleCount * 3u];
	}
	else
		desc.normal_computation = POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS;


	// Cleanup helper
	auto cleanupAndError = [&](const char* msg) -> void
		{
			delete[] desc.vertex_list;
			delete[] desc.triangle_list;
			delete[] desc.texture_coordinates_list;
			delete[] desc.normal_vectors_list;
			delete[] rawUV;
			delete[] rawNrm;

			desc.vertex_list = nullptr;
			desc.triangle_list = nullptr;
			desc.texture_coordinates_list = nullptr;
			desc.normal_vectors_list = nullptr;

			fclose(file);
			USER_ERROR(msg);
		};

	// Pass 2: fill

	rewind(file);

	unsigned vpSoFar = 0;
	unsigned vtSoFar = 0;
	unsigned vnSoFar = 0;
	unsigned triSoFar = 0;

	// We keep texturing enabled when possible, and default missing VT to (0,0).
	// We keep normals enabled only if faces actually provide them.

	const int MAX_FACE_CORNERS = 256;
	int faceVP[MAX_FACE_CORNERS];
	int faceVT[MAX_FACE_CORNERS];
	int faceVN[MAX_FACE_CORNERS];

	char line[2048];
	while (fgets(line, (int)sizeof(line), file))
	{
		char* p = line;
		while (*p == ' ' || *p == '\t') ++p;

		if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') continue;

		if (startsWith(p, "v "))
		{
			p += 2;
			char* end = nullptr;
			float x = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid v line.");
			p = end; float y = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid v line.");
			p = end; float z = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid v line.");

			desc.vertex_list[vpSoFar] = { x, y, z };
			++vpSoFar;
		}
		else if (startsWith(p, "vt "))
		{
			p += 3;
			char* end = nullptr;
			float u = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid vt line.");
			p = end; float v = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid vt line.");

			if (rawUV)
				rawUV[vtSoFar] = { u, v };

			++vtSoFar;
		}
		else if (startsWith(p, "vn "))
		{
			p += 3;
			char* end = nullptr;
			float x = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid vn line.");
			p = end; float y = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid vn line.");
			p = end; float z = strtof(p, &end); if (end == p) cleanupAndError("OBJ parse error: invalid vn line.");

			if (rawNrm)
				rawNrm[vnSoFar] = { x, y, z };

			++vnSoFar;
		}
		else if (startsWith(p, "f "))
		{
			p += 2;

			// Extract tokens
			int cornerCount = 0;
			while (*p)
			{
				while (*p == ' ' || *p == '\t') ++p;
				if (*p == '\0' || *p == '\n' || *p == '\r') break;

				if (cornerCount >= MAX_FACE_CORNERS)
					cleanupAndError("OBJ parse error: face has too many vertices (increase MAX_FACE_CORNERS).");

				char* tokenStart = p;
				while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') ++p;

				char saved = *p;
				*p = '\0';

				int vp = -1, vt = -1, vn = -1;
				parseFaceCornerToken(tokenStart, (int)vpSoFar, (int)vtSoFar, (int)vnSoFar, vp, vt, vn);

				if (vp < 0 || vp >= (int)vpSoFar) cleanupAndError("OBJ parse error: face vertex index out of range.");

				faceVP[cornerCount] = vp;
				faceVT[cornerCount] = vt;
				faceVN[cornerCount] = vn;

				*p = saved;
				++cornerCount;
			}

			if (cornerCount < 3) cleanupAndError("OBJ parse error: face has fewer than 3 vertices.");

			// Fan triangulation: (0, i, i+1)
			for (int i = 1; i < cornerCount - 1; ++i)
			{
				if (triSoFar >= triangleCount)
					cleanupAndError("OBJ parse error: internal triangle count mismatch.");

				const int a = 0;
				const int b = i;
				const int c = i + 1;

				desc.triangle_list[triSoFar] = Vector3i{ faceVP[a], faceVP[b], faceVP[c] };

				// Per-triangle UVs -> pixel coords (3 per triangle)
				if (wantTextured)
				{
					auto uvToPixel = [&](const Vector2f& uv) -> Vector2i
						{
							float u = uv.x;
							float v = uv.y;

							// Common convention: flip V. If your textures appear upside down, remove (1 - v).
							int px = (int)(u * (float)(W - 1) + 0.5f);
							int py = (int)((1.0f - v) * (float)(H - 1) + 0.5f);

							if (px < 0) px = 0; else if (px >= W) px = W - 1;
							if (py < 0) py = 0; else if (py >= H) py = H - 1;
							return Vector2i{ px, py };
						};

					auto cornerUV = [&](int vtIndex) -> Vector2i
						{
							if (vtIndex < 0 || vtIndex >= (int)vtSoFar) // missing or not yet defined
								return Vector2i{ 0, 0 };
							return uvToPixel(rawUV[vtIndex]);
						};

					desc.texture_coordinates_list[triSoFar * 3u + 0u] = cornerUV(faceVT[a]);
					desc.texture_coordinates_list[triSoFar * 3u + 1u] = cornerUV(faceVT[b]);
					desc.texture_coordinates_list[triSoFar * 3u + 2u] = cornerUV(faceVT[c]);
				}

				// Per-triangle-corner normals (3 per triangle)
				// (unchanged: if a face corner lacks VN, you will still have vn=-1 and likely want computed normals)
				if (wantPerTriNormals)
				{
					// If VN is missing for any corner, we cannot safely index rawNrm.
					// Keep the old "global downgrade" behavior for normals by detecting missing VN per triangle.
					if (faceVN[a] < 0 || faceVN[a] >= (int)vnSoFar ||
						faceVN[b] < 0 || faceVN[b] >= (int)vnSoFar ||
						faceVN[c] < 0 || faceVN[c] >= (int)vnSoFar)
					{
						// Drop normals and compute later (same end-result as your previous logic)
						delete[] desc.normal_vectors_list;
						desc.normal_vectors_list = nullptr;
						desc.normal_computation = POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS;
						wantPerTriNormals = false;
					}
					else
					{
						desc.normal_vectors_list[triSoFar * 3u + 0u] = rawNrm[faceVN[a]];
						desc.normal_vectors_list[triSoFar * 3u + 1u] = rawNrm[faceVN[b]];
						desc.normal_vectors_list[triSoFar * 3u + 2u] = rawNrm[faceVN[c]];
					}
				}

				++triSoFar;
			}
		}
		// else ignore: usemtl, mtllib, o, g, s, etc.
	}

	// Temp arrays no longer needed
	delete[] rawUV;
	delete[] rawNrm;

	fclose(file);
	return desc;
}

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Polyhedron constructor, if the pointer is valid it will call the initializer.

Polyhedron::Polyhedron(const POLYHEDRON_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Polyhedron::~Polyhedron()
{
	if (!isInit)
		return;

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	if (data.Vertices)
		delete[] data.Vertices;

	if (data.ColVertices)
		delete[] data.ColVertices;

	if (data.TexVertices)
		delete[] data.TexVertices;

	if (data.desc.triangle_list)
		delete[] data.desc.triangle_list;

	delete &data;
}

// Initializes the Polyhedron object, it expects a valid pointer to a descriptor
// and will initialize everything as specified, can only be called once per object.

void Polyhedron::initialize(const POLYHEDRON_DESC* pDesc)
{
	USER_CHECK(pDesc,
		"Trying to initialize a Polyhedron with an invalid descriptor pointer."
	);

	USER_CHECK(isInit == false,
		"Trying to initialize a Polyhedron that has already been initialized."
	);

	isInit = true;

	polyhedronData = new PolyhedronInternals;
	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	data.desc = *pDesc;

	USER_CHECK(data.desc.vertex_list,
		"Found nullptr when trying to access a vertex list to create a Polyhedron."
	);

	USER_CHECK(data.desc.triangle_list,
		"Found nullptr when trying to access a triangle list to create a Polyhedron."
	);

	USER_CHECK(data.desc.normal_computation == POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS || data.desc.normal_vectors_list,
		"Found nullptr when trying to access a normal vector list to create a Polyhedron."
	);

	switch (data.desc.coloring)
	{
	case POLYHEDRON_DESC::GLOBAL_COLORING:
	{
		data.Vertices = new PolyhedronInternals::Vertex[3 * data.desc.triangle_count];

		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = data.desc.vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = data.desc.vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = data.desc.vertex_list[data.desc.triangle_list[i].z];

			data.Vertices[3 * i + 0].vector = v0.getVector4();
			data.Vertices[3 * i + 1].vector = v1.getVector4();
			data.Vertices[3 * i + 2].vector = v2.getVector4();

			if (data.desc.enable_illuminated)
			{
				switch (data.desc.normal_computation)
				{
				case POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS:
				{
					Vector3f norm = ((v1 - v0) * (v2 - v0)).normalize();

					data.Vertices[3 * i + 0].norm = norm.getVector4();
					data.Vertices[3 * i + 1].norm = norm.getVector4();
					data.Vertices[3 * i + 2].norm = norm.getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
				{
					data.Vertices[3 * i + 0].norm = data.desc.normal_vectors_list[3 * i + 0].getVector4();
					data.Vertices[3 * i + 1].norm = data.desc.normal_vectors_list[3 * i + 1].getVector4();
					data.Vertices[3 * i + 2].norm = data.desc.normal_vectors_list[3 * i + 2].getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
				{
					data.Vertices[3 * i + 0].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
					data.Vertices[3 * i + 1].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
					data.Vertices[3 * i + 2].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
					break;
				}

				default:
					USER_ERROR("Unknown normal computation mode found when trying to initialize a Polyhedron.");
				}
			}
		}
		data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, 3u * data.desc.triangle_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

		// If updates disabled delete the vertexs
		if (!data.desc.enable_updates)
		{
			delete[] data.Vertices;
			data.Vertices = nullptr;
		}
		// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
		VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/GlobalColorVS.cso"));
#else
		VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_GLOBAL_COLOR_VS), getBlobSizeFromId(BLOB_ID::BLOB_GLOBAL_COLOR_VS)));
#endif
		// Create the corresponding Pixel Shader and Blender
		if (data.desc.enable_transparency)
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITGlobalColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
		}
		else
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/GlobalColorPS.cso" : PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OPAQUE));
		}
		// Create the corresponding input layout
		INPUT_ELEMENT_DESC ied[2] =
		{
			{ "Position",	_4_FLOAT },
			{ "Normal",		_4_FLOAT },
		};
		AddBind(new InputLayout(ied, 2u, pvs));

		// Create the constant buffer for the global color.
		_float4color col = data.desc.global_color.getColor4();
		data.pGlobalColorCB = AddBind(new ConstantBuffer(&col, PIXEL_CONSTANT_BUFFER, 1u /*Slot*/));
		break;
	}

	case POLYHEDRON_DESC::PER_VERTEX_COLORING:
	{
		USER_CHECK(data.desc.color_list,
			"Found nullptr when trying to access a color list to create a vertex colored Polyhedron."
		);

		data.ColVertices = new PolyhedronInternals::ColorVertex[3 * data.desc.triangle_count];

		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = data.desc.vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = data.desc.vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = data.desc.vertex_list[data.desc.triangle_list[i].z];

			data.ColVertices[3 * i + 0].vector = v0.getVector4();
			data.ColVertices[3 * i + 1].vector = v1.getVector4();
			data.ColVertices[3 * i + 2].vector = v2.getVector4();

			data.ColVertices[3 * i + 0].color = data.desc.color_list[3 * i + 0].getColor4();
			data.ColVertices[3 * i + 1].color = data.desc.color_list[3 * i + 1].getColor4();
			data.ColVertices[3 * i + 2].color = data.desc.color_list[3 * i + 2].getColor4();

			if (data.desc.enable_illuminated)
			{
				switch (data.desc.normal_computation)
				{
				case POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS:
				{
					Vector3f norm = ((v1 - v0) * (v2 - v0)).normalize();

					data.ColVertices[3 * i + 0].norm = norm.getVector4();
					data.ColVertices[3 * i + 1].norm = norm.getVector4();
					data.ColVertices[3 * i + 2].norm = norm.getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
				{
					data.ColVertices[3 * i + 0].norm = data.desc.normal_vectors_list[3 * i + 0].getVector4();
					data.ColVertices[3 * i + 1].norm = data.desc.normal_vectors_list[3 * i + 1].getVector4();
					data.ColVertices[3 * i + 2].norm = data.desc.normal_vectors_list[3 * i + 2].getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
				{
					data.ColVertices[3 * i + 0].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
					data.ColVertices[3 * i + 1].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
					data.ColVertices[3 * i + 2].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
					break;
				}

				default:
					USER_ERROR("Unknown normal computation mode found when trying to initialize a Polyhedron.");
				}
			}
		}
		data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, 3u * data.desc.triangle_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

		// If updates disabled delete the vertexs
		if (!data.desc.enable_updates)
		{
			delete[] data.ColVertices;
			data.ColVertices = nullptr;
		}
		// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
		VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/VertexColorVS.cso"));
#else
		VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_VERTEX_COLOR_VS), getBlobSizeFromId(BLOB_ID::BLOB_VERTEX_COLOR_VS)));
#endif
		// Create the corresponding Pixel Shader and Blender
		if (data.desc.enable_transparency)
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
		}
		else
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OPAQUE));
		}
		// Create the corresponding input layout
		INPUT_ELEMENT_DESC ied[3] =
		{
			{ "Position",	_4_FLOAT },
			{ "Normal",		_4_FLOAT },
			{ "Color",		_4_FLOAT },
		};
		AddBind(new InputLayout(ied, 3u, pvs));
		break;
	}

	case POLYHEDRON_DESC::TEXTURED_COLORING:
	{
		USER_CHECK(data.desc.texture_image,
			"Found nullptr when trying to access an Image to create a textured Polyhedron."
		);

		USER_CHECK(data.desc.texture_coordinates_list,
			"Found nullptr when trying to access a texture coordinate list to create a textured Polyhedron."
		);

		data.TexVertices = new PolyhedronInternals::TextureVertex[3 * data.desc.triangle_count];

		data.image_height = data.desc.texture_image->height();
		data.image_width = data.desc.texture_image->width();

		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = data.desc.vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = data.desc.vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = data.desc.vertex_list[data.desc.triangle_list[i].z];

			data.TexVertices[3 * i + 0].vector = v0.getVector4();
			data.TexVertices[3 * i + 1].vector = v1.getVector4();
			data.TexVertices[3 * i + 2].vector = v2.getVector4();

			data.TexVertices[3 * i + 0].coord = {
				float(data.desc.texture_coordinates_list[3 * i + 0].x) / data.image_width,
				float(data.desc.texture_coordinates_list[3 * i + 0].y) / data.image_height,
			0.f,0.f };

			data.TexVertices[3 * i + 1].coord = {
				float(data.desc.texture_coordinates_list[3 * i + 1].x) / data.image_width,
				float(data.desc.texture_coordinates_list[3 * i + 1].y) / data.image_height,
			0.f,0.f };

			data.TexVertices[3 * i + 2].coord = {
				float(data.desc.texture_coordinates_list[3 * i + 2].x) / data.image_width,
				float(data.desc.texture_coordinates_list[3 * i + 2].y) / data.image_height,
			0.f,0.f };

			if (data.desc.enable_illuminated)
			{
				switch (data.desc.normal_computation)
				{
				case POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS:
				{
					Vector3f norm = ((v1 - v0) * (v2 - v0)).normalize();

					data.TexVertices[3 * i + 0].norm = norm.getVector4();
					data.TexVertices[3 * i + 1].norm = norm.getVector4();
					data.TexVertices[3 * i + 2].norm = norm.getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
				{
					data.TexVertices[3 * i + 0].norm = data.desc.normal_vectors_list[3 * i + 0].getVector4();
					data.TexVertices[3 * i + 1].norm = data.desc.normal_vectors_list[3 * i + 1].getVector4();
					data.TexVertices[3 * i + 2].norm = data.desc.normal_vectors_list[3 * i + 2].getVector4();
					break;
				}

				case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
				{
					data.TexVertices[3 * i + 0].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
					data.TexVertices[3 * i + 1].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
					data.TexVertices[3 * i + 2].norm = data.desc.normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
					break;
				}

				default:
					USER_ERROR("Unknown normal computation mode found when trying to initialize a Polyhedron.");
				}
			}

		}
		data.pUpdateVB = AddBind(new VertexBuffer(data.TexVertices, 3u * data.desc.triangle_count, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

		// If updates disabled delete the vertexs
		if (!data.desc.enable_updates)
		{
			delete[] data.TexVertices;
			data.TexVertices = nullptr;
		}
		// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
		VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/VertexTextureVS.cso"));
#else
		VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_VERTEX_TEXTURE_VS), getBlobSizeFromId(BLOB_ID::BLOB_VERTEX_TEXTURE_VS)));
#endif
		// Create the corresponding Pixel Shader and Blender
		if (data.desc.enable_transparency)
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexTexturePS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexTexturePS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OIT_WEIGHTED));
		}
		else
		{
#ifndef _DEPLOYMENT
			AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexTexturePS.cso" : PROJECT_DIR L"shaders/UnlitVertexTexturePS.cso"));
#else
			AddBind(new PixelShader(
				getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS),
				getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS)
			));
#endif
			AddBind(new Blender(BLEND_MODE_OPAQUE));
		}
		// Create the corresponding input layout
		INPUT_ELEMENT_DESC ied[3] =
		{
			{ "Position",	_4_FLOAT },
			{ "Normal",		_4_FLOAT },
			{ "TexCoor",	_4_FLOAT },
		};
		AddBind(new InputLayout(ied, 3u, pvs));

		// Create the texture from the image
		AddBind(new Texture(data.desc.texture_image));

		// Set the sampler as linear for the texture
		AddBind(new Sampler(data.desc.pixelated_texture ? SAMPLE_FILTER_POINT : SAMPLE_FILTER_LINEAR, SAMPLE_ADDRESS_WRAP));
		break;
	}

	default:
		USER_ERROR("Found an unrecognized coloring mode when trying to create a Polyhedron.");
	}

	// If update enabled save a copy to update vertices
	if (data.desc.enable_updates)
	{
		Vector3i* new_list = new Vector3i[data.desc.triangle_count];
		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
			new_list[i] = data.desc.triangle_list[i];

		data.desc.triangle_list = new_list;
	}
	// Else forget about it.
	else
		data.desc.triangle_list = nullptr;

	unsigned* indexs = new unsigned[3u * data.desc.triangle_count];
	for (unsigned i = 0; i < 3u * data.desc.triangle_count; i++)
		indexs[i] = i;

	AddBind(new IndexBuffer(indexs, 3u * data.desc.triangle_count));
	delete[] indexs;

	AddBind(new Topology(TRIANGLE_LIST));
	AddBind(new Rasterizer(data.desc.double_sided_rendering, data.desc.wire_frame_topology));

	data.pVSCB = AddBind(new ConstantBuffer(&data.vscBuff, VERTEX_CONSTANT_BUFFER));

	// If illuminated is enabled bind the default lights
	if (data.desc.enable_illuminated)
	{
		if (data.desc.default_initial_lights)
		{
			data.pscBuff.lightsource[0].intensity = { 60.f, 10.f };
			data.pscBuff.lightsource[1].intensity = { 60.f, 10.f };
			data.pscBuff.lightsource[2].intensity = { 60.f, 10.f };
			data.pscBuff.lightsource[3].intensity = { 60.f, 10.f };

			data.pscBuff.lightsource[0].color = { 1.0f, 0.2f, 0.2f, 1.f };
			data.pscBuff.lightsource[1].color = { 0.0f, 1.0f, 0.0f, 1.f };
			data.pscBuff.lightsource[2].color = { 0.5f, 0.0f, 1.0f, 1.f };
			data.pscBuff.lightsource[3].color = { 1.0f, 1.0f, 0.0f, 1.f };

			data.pscBuff.lightsource[0].position = { 0.f, 8.f, 8.f };
			data.pscBuff.lightsource[1].position = { 0.f,-8.f, 8.f };
			data.pscBuff.lightsource[2].position = { -8.f, 0.f,-8.f };
			data.pscBuff.lightsource[3].position = { 8.f, 0.f, 8.f };
		}

		data.pPSCB = AddBind(new ConstantBuffer(&data.pscBuff, PIXEL_CONSTANT_BUFFER));
	}

}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// If updates are enabled this function allows to change the current vertex positions
// for the new ones specified. It expects a valid pointer with a list as long as the 
// highest index found in the triangle list used for initialization.

void Polyhedron::updateVertices(const Vector3f* vertex_list)
{
	USER_CHECK(isInit,
		"Trying to update the vertices on an uninitialized Polyhedron."
	);

	USER_CHECK(vertex_list,
		"Trying to update the vertices with an invalid vertex list."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the vertices on a Polyhedron with updates disabled."
	);

	switch (data.desc.coloring)
	{
	case POLYHEDRON_DESC::GLOBAL_COLORING:
	{
		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = vertex_list[data.desc.triangle_list[i].z];

			data.Vertices[3 * i + 0].vector = v0.getVector4();
			data.Vertices[3 * i + 1].vector = v1.getVector4();
			data.Vertices[3 * i + 2].vector = v2.getVector4();

			if (data.desc.normal_computation == POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS)
			{
				Vector3f norm = data.desc.enable_illuminated ? ((v1 - v0) * (v2 - v0)).normalize() : Vector3f();

				data.Vertices[3 * i + 0].norm = norm.getVector4();
				data.Vertices[3 * i + 1].norm = norm.getVector4();
				data.Vertices[3 * i + 2].norm = norm.getVector4();
			}
		}
		data.pUpdateVB->updateVertices(data.Vertices, 3u * data.desc.triangle_count);
		break;
	}

	case POLYHEDRON_DESC::PER_VERTEX_COLORING:
	{
		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = vertex_list[data.desc.triangle_list[i].z];

			data.ColVertices[3 * i + 0].vector = v0.getVector4();
			data.ColVertices[3 * i + 1].vector = v1.getVector4();
			data.ColVertices[3 * i + 2].vector = v2.getVector4();

			if (data.desc.normal_computation == POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS)
			{
				Vector3f norm = data.desc.enable_illuminated ? ((v1 - v0) * (v2 - v0)).normalize() : Vector3f();

				data.ColVertices[3 * i + 0].norm = norm.getVector4();
				data.ColVertices[3 * i + 1].norm = norm.getVector4();
				data.ColVertices[3 * i + 2].norm = norm.getVector4();
			}
		}
		data.pUpdateVB->updateVertices(data.ColVertices, 3u * data.desc.triangle_count);
		break;
	}

	case POLYHEDRON_DESC::TEXTURED_COLORING:
	{
		for (unsigned i = 0u; i < data.desc.triangle_count; i++)
		{
			const Vector3f& v0 = vertex_list[data.desc.triangle_list[i].x];
			const Vector3f& v1 = vertex_list[data.desc.triangle_list[i].y];
			const Vector3f& v2 = vertex_list[data.desc.triangle_list[i].z];

			data.TexVertices[3 * i + 0].vector = v0.getVector4();
			data.TexVertices[3 * i + 1].vector = v1.getVector4();
			data.TexVertices[3 * i + 2].vector = v2.getVector4();

			if (data.desc.normal_computation == POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS)
			{
				Vector3f norm = data.desc.enable_illuminated ? ((v1 - v0) * (v2 - v0)).normalize() : Vector3f();

				data.TexVertices[3 * i + 0].norm = norm.getVector4();
				data.TexVertices[3 * i + 1].norm = norm.getVector4();
				data.TexVertices[3 * i + 2].norm = norm.getVector4();
			}
		}
		data.pUpdateVB->updateVertices(data.TexVertices, 3u * data.desc.triangle_count);
		break;
	}
	}
}

// If updates are enabled, and coloring is per vertex, this function allows to change 
// the current vertex colors for the new ones specified. It expects a valid pointer 
// with a list of colors containing one color per every vertex of every triangle. 
// Three times the triangle count.

void Polyhedron::updateColors(const Color* color_list)
{
	USER_CHECK(isInit,
		"Trying to update the colors on an uninitialized Polyhedron."
	);

	USER_CHECK(color_list,
		"Trying to update the colors on a Polyhedron with an invalid color list."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.coloring == POLYHEDRON_DESC::PER_VERTEX_COLORING,
		"Trying to update the colors on a Polyhedron with a different coloring."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the colors on a Polyhedron with updates disabled."
	);

	for (unsigned i = 0u; i < 3u * data.desc.triangle_count; i++)
		data.ColVertices[i].color = color_list[i].getColor4();

	data.pUpdateVB->updateVertices(data.ColVertices, 3u * data.desc.triangle_count);
}

// If normals are provided this function allows to update the normal vectors in the 
// same satting that the Polyhedron was initialized on. It expects a valid list of
// normal vectors of the correspoding lenght.

void Polyhedron::updateNormals(const Vector3f* normal_vectors_list)
{
	USER_CHECK(isInit,
		"Trying to update the normal vectors on an uninitialized Polyhedron."
	);

	USER_CHECK(normal_vectors_list,
		"Trying to update the normal vectors on a Polyhedron with an invalid normal vectors list."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.normal_computation != POLYHEDRON_DESC::COMPUTED_TRIANGLE_NORMALS,
		"Trying to update the normal vectors on a Polyhedron with a different normal vectors setting.\n"
		"When normals are on computed mode, they are recomputed automatically when vertices are updated."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the normal vectors on a Polyhedron with updates disabled."
	);

	switch (data.desc.coloring)
	{
	case POLYHEDRON_DESC::GLOBAL_COLORING:

		switch (data.desc.normal_computation)
		{
		case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
			for (unsigned i = 0u; i < 3u * data.desc.triangle_count; i++)
				data.Vertices[i].norm = normal_vectors_list[i].getVector4();
			break;

		case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
			for (unsigned i = 0u; i < data.desc.triangle_count; i++)
			{
				data.Vertices[3 * i + 0].norm = normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
				data.Vertices[3 * i + 1].norm = normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
				data.Vertices[3 * i + 2].norm = normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
			}
			break;
		}

		data.pUpdateVB->updateVertices(data.Vertices, 3u * data.desc.triangle_count);
		break;

	case POLYHEDRON_DESC::PER_VERTEX_COLORING:

		switch (data.desc.normal_computation)
		{
		case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
			for (unsigned i = 0u; i < 3u * data.desc.triangle_count; i++)
				data.ColVertices[i].norm = normal_vectors_list[i].getVector4();
			break;

		case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
			for (unsigned i = 0u; i < data.desc.triangle_count; i++)
			{
				data.ColVertices[3 * i + 0].norm = normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
				data.ColVertices[3 * i + 1].norm = normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
				data.ColVertices[3 * i + 2].norm = normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
			}
			break;
		}

		data.pUpdateVB->updateVertices(data.ColVertices, 3u * data.desc.triangle_count);
		break;

	case POLYHEDRON_DESC::TEXTURED_COLORING:

		switch (data.desc.normal_computation)
		{
		case POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS:
			for (unsigned i = 0u; i < 3u * data.desc.triangle_count; i++)
				data.TexVertices[i].norm = normal_vectors_list[i].getVector4();
			break;

		case POLYHEDRON_DESC::PER_VERTEX_LIST_NORMALS:
			for (unsigned i = 0u; i < data.desc.triangle_count; i++)
			{
				data.TexVertices[3 * i + 0].norm = normal_vectors_list[data.desc.triangle_list[i].x].getVector4();
				data.TexVertices[3 * i + 1].norm = normal_vectors_list[data.desc.triangle_list[i].y].getVector4();
				data.TexVertices[3 * i + 2].norm = normal_vectors_list[data.desc.triangle_list[i].z].getVector4();
			}
			break;
		}

		data.pUpdateVB->updateVertices(data.TexVertices, 3u * data.desc.triangle_count);
		break;
	}

}

// If updates are enabled, and coloring is texured, this functions allows o change the 
// current vertex texture coordinates for the new ones specified. It expects a valid 
// pointer with a list of pixels containing one coordinates per every vertex of every
// triangle. Three times the triangle count.

void Polyhedron::updateTextureCoordinates(const Vector2i* texture_coordinates_list)
{
	USER_CHECK(isInit,
		"Trying to update the texture coordinates on an uninitialized Polyhedron."
	);

	USER_CHECK(texture_coordinates_list,
		"Trying to update the texture coordinates with an invalid texture coordinate list."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.coloring == POLYHEDRON_DESC::TEXTURED_COLORING,
		"Trying to update the texture coordinates on a Polyhedron with a different coloring."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the texture coordinates on a Polyhedron with updates disabled."
	);

	for (unsigned i = 0u; i < 3u * data.desc.triangle_count; i++)
		data.TexVertices[i].coord = {
			float(texture_coordinates_list[i].x) / data.image_width,
			float(texture_coordinates_list[i].y) / data.image_height,
			0.f, 0.f };

	data.pUpdateVB->updateVertices(data.TexVertices, 3u * data.desc.triangle_count);
}

// If the coloring is set to global, updates the global Polyhedron color.

void Polyhedron::updateGlobalColor(Color color)
{
	USER_CHECK(isInit,
		"Trying to update the global color on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.coloring == POLYHEDRON_DESC::GLOBAL_COLORING,
		"Trying to update the global color on a Polyhedron with a different coloring."
	);

	_float4color col = color.getColor4();
	data.pGlobalColorCB->update(&col);
}

// Updates the rotation quaternion of the Polyhedron. If multiplicative it will apply
// the rotation on top of the current rotation. For more information on how to rotate
// with quaternions check the Quaternion header file.

void Polyhedron::updateRotation(Quaternion rotation, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the rotation on an uninitialized Polyhedron."
	);

	USER_CHECK(rotation,
		"Invalid quaternion found when trying to update rotation on a Polyhedron.\n"
		"Quaternion 0 can not be normalized and therefore can not describe an objects rotation."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	if (multiplicative)
		data.rotation *= rotation.normal();
	else
		data.rotation = rotation;

	data.rotation.normalize();

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);
	data.vscBuff.normal_transform = L.inverse().transposed().getMatrix4();

	data.pVSCB->update(&data.vscBuff);
}

// Updates the scene position of the Polihedrom. I additive it will add the vector
// to the current position vector of the Polyhedron.

void Polyhedron::updatePosition(Vector3f position, bool additive)
{
	USER_CHECK(isInit,
		"Trying to update the position on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	if (additive)
		data.position += position;
	else
		data.position = position;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the matrix multiplied to the Polihedrom, adding any arbitrary linear distortion 
// to it. If you want to simply modify the size of the object just call this function on a 
// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
// If multiplicative the distortion will be added to the current distortion.

void Polyhedron::updateDistortion(Matrix distortion, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the distortion on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	if (multiplicative)
		data.distortion = distortion * data.distortion;
	else
		data.distortion = distortion;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);
	data.vscBuff.normal_transform = L.inverse().transposed().getMatrix4();

	data.pVSCB->update(&data.vscBuff);
}

// Updates the screen displacement of the figure. To be used if you intend to render 
// multiple scenes/plots on the same render target.

void Polyhedron::updateScreenPosition(Vector2f screenDisplacement)
{
	USER_CHECK(isInit,
		"Trying to update the screen position on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	data.vscBuff.displacement = screenDisplacement.getVector4();
	data.pVSCB->update(&data.vscBuff);
}

// If illumination is enabled it sets the specified light to the specified parameters.
// Eight lights are allowed. And the intensities are directional and diffused.

void Polyhedron::updateLight(unsigned id, Vector2f intensities, Color color, Vector3f position)
{
	USER_CHECK(isInit,
		"Trying to update a light on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to update a light on a Polyhedron with illumination disabled."
	);

	USER_CHECK(id < 8,
		"Trying to update a light with an invalid id (must be 0-7)."
	);

	data.pscBuff.lightsource[id] = { intensities.getVector4(), color.getColor4(), position.getVector4() };
	data.pPSCB->update(&data.pscBuff);
}

// If illumination is enabled clears all lights for the Polyhedron.

void Polyhedron::clearLights()
{
	USER_CHECK(isInit,
		"Trying to clear the lights on an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to clear the lights on a Polyhedron with illumination disabled."
	);

	for (auto& light : data.pscBuff.lightsource)
		light = {};

	data.pPSCB->update(&data.pscBuff);
}

/*
-----------------------------------------------------------------------------------------------------------
 Getters
-----------------------------------------------------------------------------------------------------------
*/


// If illumination is enabled, to the valid pointers it writes the specified lights data.

void Polyhedron::getLight(unsigned id, Vector2f* intensities, Color* color, Vector3f* position)
{
	USER_CHECK(isInit,
		"Trying to get a light of an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to get a light of a Polyhedron with illumination disabled."
	);

	USER_CHECK(id < 8,
		"Trying to get a light with an invalid id (must be 0-7)."
	);

	if (intensities)
		*intensities = { data.pscBuff.lightsource[id].intensity.x,data.pscBuff.lightsource[id].intensity.y };

	if (color)
		*color = Color(data.pscBuff.lightsource[id].color);

	if (position)
		*position = {
			data.pscBuff.lightsource[id].position.x,
			data.pscBuff.lightsource[id].position.y,
			data.pscBuff.lightsource[id].position.z
	};
}

// Returns the current rotation quaternion.

Quaternion Polyhedron::getRotation() const
{
	USER_CHECK(isInit,
		"Trying to get the rotation of an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	return data.rotation;
}

// Returns the current scene position.

Vector3f Polyhedron::getPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the position of an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	return data.position;
}

// Returns the current distortion matrix.

Matrix Polyhedron::getDistortion() const
{
	USER_CHECK(isInit,
		"Trying to get the distortion matrix of an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	return data.distortion;
}

// Returns the current screen position.

Vector2f Polyhedron::getScreenPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the screen position of an uninitialized Polyhedron."
	);

	PolyhedronInternals& data = *(PolyhedronInternals*)polyhedronData;

	return { data.vscBuff.displacement.x, data.vscBuff.displacement.y };
}
