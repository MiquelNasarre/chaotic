#include "Drawable/Surface.h"
#include "Bindable/BindableBase.h"

#include "Error/_erDefault.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Surface Internals
-----------------------------------------------------------------------------------------------------------
*/

// Struct that stores the internal data for a given Surface object.
struct SurfaceInternals
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
		_float4vector displacement = { 0.f, 0.f, 0.f, 0.f };
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

	Vector3f* implicit_vertices = nullptr;
	Vector3i* implicit_triangles = nullptr;

	Vector3f* spherical_vertices = nullptr;

	ConstantBuffer* pVSCB = nullptr;
	ConstantBuffer* pPSCB = nullptr;

	ConstantBuffer* pGlobalColorCB = nullptr;

	VertexBuffer* pUpdateVB = nullptr;
	Texture* pUpdateTexture = nullptr;

	SURFACE_DESC desc = {};
};

/*
-----------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-----------------------------------------------------------------------------------------------------------
*/

// Surface constructor, if the pointer is valid it will call the initializer.

Surface::Surface(const SURFACE_DESC* pDesc)
{
	if (pDesc)
		initialize(pDesc);
}

// Frees the GPU pointers and all the stored data.

Surface::~Surface()
{
	if (!isInit)
		return;

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	if (data.Vertices)
		delete[] data.Vertices;

	if (data.ColVertices)
		delete[] data.ColVertices;

	if (data.TexVertices)
		delete[] data.TexVertices;

	if (data.implicit_vertices)
		delete[] data.implicit_vertices;

	if (data.implicit_triangles)
		delete[] data.implicit_triangles;

	if (data.spherical_vertices)
		delete[] data.spherical_vertices;

	delete& data;
}

// Initializes the Surface object, it expects a valid pointer to a descriptor and 
// will initialize everything as specified, can only be called once per object.

void Surface::initialize(const SURFACE_DESC* pDesc)
{
	USER_CHECK(pDesc,
		"Trying to initialize a Surface with an invalid descriptor pointer."
	);

	USER_CHECK(isInit == false,
		"Trying to initialize a Surface that has already been initialized."
	);

		isInit = true;

	surfaceData = new SurfaceInternals;
	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	data.desc = *pDesc;

	USER_CHECK(data.desc.normal_computation != SURFACE_DESC::INPUT_FUNCTION_NORMALS || data.desc.input_normal_func,
		"Found nullptr when trying to access a normal vector function to generate the normal vectors on a Surface."
	);

	USER_CHECK(data.desc.normal_computation != SURFACE_DESC::OUTPUT_FUNCTION_NORMALS || data.desc.output_normal_func,
		"Found nullptr when trying to access a normal vector function to generate the normal vectors on a Surface."
	);

	USER_CHECK(data.desc.normal_computation != SURFACE_DESC::DERIVATE_NORMALS || data.desc.delta_value,
		"Invalid delta value found when trying to derivate the normal vectors on a Surface.\n"
		"Zero is not a valid delta since the function will be evaluated on the same point, therefore not giving any spatial information."
	);

	USER_CHECK(data.desc.num_u >= 2u && data.desc.num_v >= 2u,
		"Invalid number of vertex found when trying to initialize a Surface.\n"
		"At least two vertices in each dimension are needed to generate a grid."
	);

	// Calculate initial values and deltas of both coordinates.

	float du = data.desc.border_points_included ?
		(data.desc.range_u.y - data.desc.range_u.x) / (data.desc.num_u - 1.f) :
		(data.desc.range_u.y - data.desc.range_u.x) / (data.desc.num_u + 1.f);

	float u_i = data.desc.border_points_included ? data.desc.range_u.x : data.desc.range_u.x + du;

	float dv = data.desc.border_points_included ?
		(data.desc.range_v.y - data.desc.range_v.x) / (data.desc.num_v - 1.f) :
		(data.desc.range_v.y - data.desc.range_v.x) / (data.desc.num_v + 1.f);

	float v_i = data.desc.border_points_included ? data.desc.range_v.x : data.desc.range_v.x + dv;

	// Different generation methods for different surface types.
	switch (data.desc.type)
	{
		case SURFACE_DESC::EXPLICIT_SURFACE:
		{
			USER_CHECK(data.desc.explicit_func,
				"Found nullptr when trying to access an explicit function to generate a Surface."
			);

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					data.Vertices = new SurfaceInternals::Vertex[data.desc.num_u * data.desc.num_v];

					// First assign a position to each vertex given the explicit function.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									SurfaceInternals::Vertex* prev_col = (n > 0u) ? &data.Vertices[(n - 1u) * data.desc.num_v] : data.Vertices;

									SurfaceInternals::Vertex* next_col = (n < data.desc.num_u - 1u) ? &data.Vertices[(n + 1u) * data.desc.num_v] : &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITGlobalColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/GlobalColorPS.cso" : PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)
						));
#endif

					// Create the input layout
					INPUT_ELEMENT_DESC ied[2] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 2u, pvs));
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					USER_CHECK(data.desc.texture_image,
						"Found nullptr when trying to acces an image to create a texture for a textured Surface."
					);

					// Create the texture from the input image.
					data.pUpdateTexture = AddBind(new Texture(data.desc.texture_image, data.desc.enable_updates ? TEXTURE_USAGE_DYNAMIC : TEXTURE_USAGE_DEFAULT));

					// Create the sampler for the texture.
					AddBind(new Sampler(data.desc.pixelated_texture ? SAMPLE_FILTER_POINT : SAMPLE_FILTER_LINEAR, SAMPLE_ADDRESS_CLAMP));

					data.TexVertices = new SurfaceInternals::TextureVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a texture coordinate.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a texture coordinate to the vertex given u and v.
							col[m].coord = _float4vector{ float(n) / (data.desc.num_u - 1u), float(m) / (data.desc.num_v - 1u) };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									SurfaceInternals::TextureVertex* prev_col = (n > 0u) ? &data.TexVertices[(n - 1u) * data.desc.num_v] : data.TexVertices;

									SurfaceInternals::TextureVertex* next_col = (n < data.desc.num_u - 1u) ? &data.TexVertices[(n + 1u) * data.desc.num_v] : &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.TexVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexTexturePS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexTexturePS.cso" : PROJECT_DIR L"shaders/UnlitVertexTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS)
						));
#endif

					// Create the corresponding input layout
					INPUT_ELEMENT_DESC ied[3] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
						{ "TexCoor",	_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 3u, pvs));
					break;
				}

				case SURFACE_DESC::ARRAY_COLORING:
				{
					USER_CHECK(data.desc.color_array,
						"Found nullptr when trying to acces a color array to color an array colored Surface."
					);
					
					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a color to the vertex given its indexs.
							col[m].color = data.desc.color_array[n][m].getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.input_color_func,
						"Found nullptr when trying to acces a color function to color an input function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a color to the vertex given the input coordinates.
							col[m].color = data.desc.input_color_func(x, y).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.output_color_func,
						"Found nullptr when trying to acces a color function to color an output function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a color to the vertex given the output coordinates.
							col[m].color = data.desc.output_color_func(x, y, z).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				default:
					USER_ERROR("Unknonw surface coloring type found when trying to initialize a Surface.");
			}

			// Create the index buffer for the surface.
			unsigned* indices = new unsigned[6u * (data.desc.num_u - 1u) * (data.desc.num_v - 1u)];

			for (unsigned n = 0u; n < data.desc.num_u - 1u; n++)
			{
				unsigned* col = &indices[6u * n * (data.desc.num_v - 1u)];

				for (unsigned m = 0u; m < data.desc.num_v - 1u; m++)
				{
					col[6u * m + 0u] = n * data.desc.num_v + m;
					col[6u * m + 1u] = (n + 1u) * data.desc.num_v + m;
					col[6u * m + 2u] = n * data.desc.num_v + m + 1u;

					col[6u * m + 3u] = n * data.desc.num_v + m + 1u;
					col[6u * m + 4u] = (n + 1u) * data.desc.num_v + m;
					col[6u * m + 5u] = (n + 1u) * data.desc.num_v + m + 1u;
				}
			}

			AddBind(new IndexBuffer(indices, 6u * (data.desc.num_u - 1u) * (data.desc.num_v - 1u)));

			delete[] indices;
			break;
		}

		case SURFACE_DESC::SPHERICAL_SURFACE:
		{
			USER_CHECK(data.desc.spherical_func,
				"Found nullptr when trying to access an spherical function to generate a Surface."
			);

			unsigned V = 12u;
			unsigned C = 20u;
			unsigned A = 30u;

			// First create an icosahedron.
			Vector3f* vertices	= new Vector3f[V];
			Vector2i* aristas	= new Vector2i[A];
			Vector3i* triangles = new Vector3i[C];

			// We will use the golden ratio for the initial coordinates.
			constexpr float gold = 1.618033988749894f; // (1+sqrt5) / 2

			vertices[0] = { 0.f, 1.f, gold };
			vertices[1] = { 0.f, 1.f,-gold };
			vertices[2] = { 0.f,-1.f, gold };
			vertices[3] = { 0.f,-1.f,-gold };
			vertices[4] = { 1.f, gold, 0.f };
			vertices[5] = { 1.f,-gold, 0.f };
			vertices[6] = { -1.f, gold, 0.f };
			vertices[7] = { -1.f,-gold, 0.f };
			vertices[8] = { gold, 0.f, 1.f };
			vertices[9] = { -gold, 0.f, 1.f };
			vertices[10] = { gold, 0.f,-1.f };
			vertices[11] = { -gold, 0.f,-1.f };

			aristas[0] = { 0, 2 };
			aristas[1] = { 0, 4 };
			aristas[2] = { 0, 6 };
			aristas[3] = { 0, 8 };
			aristas[4] = { 0, 9 };
			aristas[5] = { 1, 3 };
			aristas[6] = { 1, 4 };
			aristas[7] = { 1, 6 };
			aristas[8] = { 1,10 };
			aristas[9] = { 1,11 };
			aristas[10] = { 2, 5 };
			aristas[11] = { 2, 7 };
			aristas[12] = { 2, 8 };
			aristas[13] = { 2, 9 };
			aristas[14] = { 3, 5 };
			aristas[15] = { 3, 7 };
			aristas[16] = { 3,10 };
			aristas[17] = { 3,11 };
			aristas[18] = { 4, 6 };
			aristas[19] = { 4, 8 };
			aristas[20] = { 4,10 };
			aristas[21] = { 5, 7 };
			aristas[22] = { 5, 8 };
			aristas[23] = { 5,10 };
			aristas[24] = { 6, 9 };
			aristas[25] = { 6,11 };
			aristas[26] = { 7, 9 };
			aristas[27] = { 7,11 };
			aristas[28] = { 8,10 };
			aristas[29] = { 9,11 };

			triangles[0] =  {  3,-19, -2 };
			triangles[1] =  {  7, 19, -8 };
			triangles[2] =  { 11, 22,-12 };
			triangles[3] =  { 16,-22,-15 };
			triangles[4] =  { 21,-29,-20 };
			triangles[5] =  { 23, 29,-24 };
			triangles[6] =  { 25, 30,-26 };
			triangles[7] =  { 28,-30,-27 };
			triangles[8] =  {-13, -1,  4 };
			triangles[9] =  { -5,  1, 14 };
			triangles[10] = { -9,  6, 17 };
			triangles[11] = {-18, -6, 10 };
			triangles[12] = {  2, 20, -4 };
			triangles[13] = {  5,-25, -3 };
			triangles[14] = { -7,  9,-21 };
			triangles[15] = { 26,-10,  8 };
			triangles[16] = { 18,-28,-16 };
			triangles[17] = { 15, 24,-17 };
			triangles[18] = { 27,-14, 12 };
			triangles[19] = {-11, 13,-23 };

			// Subdivide the triangles as many times as the depth specified.
			for (unsigned d = 0u; d < data.desc.icosphere_depth; d++)
			{
				unsigned next_V = V + A;
				unsigned next_A = A * 4;
				unsigned next_C = C * 4;

				Vector3i* next_triangles	= new Vector3i[next_C];
				Vector2i* next_aristas		= new Vector2i[next_A];
				Vector3f* next_vertices		= new Vector3f[next_V];

				// Old vertices stay in place
				for (unsigned i = 0; i < V; i++)
					next_vertices[i] = vertices[i];

				// New vertices appear between each arista, aristas split in half.
				for (unsigned i = 0; i < A; i++)
				{
					next_vertices[V + i] = (vertices[aristas[i].x] + vertices[aristas[i].y]) / 2.f;
					next_aristas[2 * i + 0] = { aristas[i].x, int(V + i) };
					next_aristas[2 * i + 1] = { int(V + i), aristas[i].y };
				}

				// For each triangle four are created, the orientation of the aristas of the
				// triangle is codified in the sign of the arista.
				int used0, used1, used2;
				for (unsigned i = 0; i < C; i++)
				{
					unsigned aris0 = triangles[i].x > 0.f ? unsigned(triangles[i].x - 1) : unsigned(-triangles[i].x - 1);
					unsigned aris1 = triangles[i].y > 0.f ? unsigned(triangles[i].y - 1) : unsigned(-triangles[i].y - 1);
					unsigned aris2 = triangles[i].z > 0.f ? unsigned(triangles[i].z - 1) : unsigned(-triangles[i].z - 1);

					next_aristas[2 * A + 3 * i + 0] = { next_aristas[2 * aris0].y, next_aristas[2 * aris1].y };
					next_aristas[2 * A + 3 * i + 1] = { next_aristas[2 * aris1].y, next_aristas[2 * aris2].y };
					next_aristas[2 * A + 3 * i + 2] = { next_aristas[2 * aris2].y, next_aristas[2 * aris0].y };

					next_triangles[4 * i] = { int(2 * A + 3 * i + 1) , int(2 * A + 3 * i + 2) , int(2 * A + 3 * i + 3) };

					used0 = (triangles[i].x > 0) ? int(2 * aris0 + 2) : -int(2 * aris0 + 1);
					used1 = (triangles[i].y > 0) ? int(2 * aris1 + 1) : -int(2 * aris1 + 2);

					next_triangles[4 * i + 1] = { used0, used1, -int(2 * A + 3 * i + 1) };

					used1 = (triangles[i].y > 0) ? int(2 * aris1 + 2) : -int(2 * aris1 + 1);
					used2 = (triangles[i].z > 0) ? int(2 * aris2 + 1) : -int(2 * aris2 + 2);

					next_triangles[4 * i + 2] = { used1, used2, -int(2 * A + 3 * i + 2) };

					used2 = (triangles[i].z > 0) ? int(2 * aris2 + 2) : -int(2 * aris2 + 1);
					used0 = (triangles[i].x > 0) ? int(2 * aris0 + 1) : -int(2 * aris0 + 2);

					next_triangles[4 * i + 3] = { used2, used0, -int(2 * A + 3 * i + 3) };
				}

				V = next_V;
				A = next_A;
				C = next_C;

				delete[] vertices;
				delete[] aristas;
				delete[] triangles;

				vertices = next_vertices;
				aristas = next_aristas;
				triangles = next_triangles;
			}

			unsigned* indices = new unsigned[3 * C];

			for (unsigned int i = 0; i < C; i++)
			{
				unsigned aris0 = triangles[i].x > 0.f ? unsigned(triangles[i].x - 1) : unsigned(-triangles[i].x - 1);
				unsigned aris1 = triangles[i].y > 0.f ? unsigned(triangles[i].y - 1) : unsigned(-triangles[i].y - 1);
				unsigned aris2 = triangles[i].z > 0.f ? unsigned(triangles[i].z - 1) : unsigned(-triangles[i].z - 1);

				indices[3 * i + 0] = (triangles[i].x > 0) ? aristas[aris0].x : aristas[aris0].y;
				indices[3 * i + 1] = (triangles[i].y > 0) ? aristas[aris1].x : aristas[aris1].y;
				indices[3 * i + 2] = (triangles[i].z > 0) ? aristas[aris2].x : aristas[aris2].y;

			}

			AddBind(new IndexBuffer(indices, 3u * C));

			delete[] indices;
			delete[] aristas;
			delete[] triangles;

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					data.Vertices = new SurfaceInternals::Vertex[V];

					// First assign a position to each vertex given the parametric function.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = vertices[n].normalize();

						// Assign a radius for each point of the sphere.
						data.Vertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
								USER_ERROR(
									"Input function normals is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
									"If you want to implement the normal vectors of a spherical function you have to use output function normals."
								);

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
									// Use the normal function to assign normals to each point.
									data.Vertices[n].norm = data.desc.output_normal_func(data.Vertices[n].vector.x, data.Vertices[n].vector.y, data.Vertices[n].vector.z).getVector4();

								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
								{
									Vector3f a = (vertices[n].z < 0.999f && vertices[n].z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

									Vector3f ei = vertices[n] * a;
									Vector3f ej = vertices[n] * ei;

									Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
									Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

									Vector3f plus_i = (rot_i * Quaternion(vertices[n]) * rot_i.inv()).getVector();
									Vector3f minus_i = (rot_i.inv() * Quaternion(vertices[n]) * rot_i).getVector();

									Vector3f plus_j = (rot_j * Quaternion(vertices[n]) * rot_j.inv()).getVector();
									Vector3f minus_j = (rot_j.inv() * Quaternion(vertices[n]) * rot_j).getVector();

									Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
									Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

									data.Vertices[n].norm = (dsdi * dsdj).normalize().getVector4();
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
								USER_ERROR(
									"Closest neighbor normal derivation is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
									"If you want to implement the normal vectors of a spherical function you have to use output function normals."
								);

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, V, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITGlobalColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/GlobalColorPS.cso" : PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)
						));
#endif

					// Create the input layout
					INPUT_ELEMENT_DESC ied[2] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 2u, pvs));
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					USER_CHECK(data.desc.texture_image,
						"Found nullptr when trying to acces an image to create a texture for a textured Surface."
					);

					// Create the texture from the input image. Since it is an icosphere the texture must be a cube-map.
					data.pUpdateTexture = AddBind(new Texture(data.desc.texture_image, data.desc.enable_updates ? TEXTURE_USAGE_DYNAMIC : TEXTURE_USAGE_DEFAULT, TEXTURE_TYPE_CUBEMAP));

					// Create the sampler for the texture.
					AddBind(new Sampler(data.desc.pixelated_texture ? SAMPLE_FILTER_POINT : SAMPLE_FILTER_LINEAR, SAMPLE_ADDRESS_CLAMP));

					data.TexVertices = new SurfaceInternals::TextureVertex[V];

					// Assign a position to each vertex given the explicit function and a cube texture coordinate equal to the S2 position.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = vertices[n].normalize();

						// Assign a radius for each point of the sphere.
						data.TexVertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();

						// Assign a texture coordinate to the vertex given u and v.
						data.TexVertices[n].coord = vertex.getVector4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
						case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							USER_ERROR(
								"Input function normals is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
								"If you want to implement the normal vectors of a spherical function you have to use output function normals."
							);

						case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
						{
							for (unsigned n = 0u; n < V; n++)
								// Use the normal function to assign normals to each point.
								data.TexVertices[n].norm = data.desc.output_normal_func(data.TexVertices[n].vector.x, data.TexVertices[n].vector.y, data.TexVertices[n].vector.z).getVector4();

							break;
						}

						case SURFACE_DESC::DERIVATE_NORMALS:
						{
							for (unsigned n = 0u; n < V; n++)
							{
								Vector3f a = (vertices[n].z < 0.999f && vertices[n].z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

								Vector3f ei = vertices[n] * a;
								Vector3f ej = vertices[n] * ei;

								Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
								Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

								Vector3f plus_i = (rot_i * Quaternion(vertices[n]) * rot_i.inv()).getVector();
								Vector3f minus_i = (rot_i.inv() * Quaternion(vertices[n]) * rot_i).getVector();

								Vector3f plus_j = (rot_j * Quaternion(vertices[n]) * rot_j.inv()).getVector();
								Vector3f minus_j = (rot_j.inv() * Quaternion(vertices[n]) * rot_j).getVector();

								Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
								Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

								data.TexVertices[n].norm = (dsdi * dsdj).normalize().getVector4();
							}
							break;
						}

						case SURFACE_DESC::CLOSEST_NEIGHBORS:
							USER_ERROR(
								"Closest neighbor normal derivation is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
								"If you want to implement the normal vectors of a spherical function you have to use output function normals."
							);

						default:
							USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.TexVertices, V, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITCubeTexturePS.cso" : PROJECT_DIR L"shaders/OITUnlitCubeTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_CUBE_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_CUBE_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_CUBE_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_CUBE_TEXTURE_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/CubeTexturePS.cso" : PROJECT_DIR L"shaders/UnlitCubeTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_CUBE_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_CUBE_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_CUBE_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_CUBE_TEXTURE_PS)
						));
#endif

					// Create the corresponding input layout
					INPUT_ELEMENT_DESC ied[3] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
						{ "TexCoor",	_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 3u, pvs));
					break;
				}

				case SURFACE_DESC::ARRAY_COLORING:
					USER_ERROR(
						"Array coloring is not supported for a spherical function Surface.\n"
						"Since the function input is an unordered spherical vector the only colorings allowed are global, output function and cube-map textured."
					);

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
					USER_ERROR(
						"Input function coloring is not supported for a spherical function Surface.\n"
						"Since the function input is an unordered spherical vector the only colorings allowed are global, output function and cube-map textured."
					);

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.output_color_func,
						"Found nullptr when trying to acces a color function to color an output function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[V];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = vertices[n].normalize();

						// Assign a radius for each point of the sphere.
						data.ColVertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();

						// Assign a color to the vertex given the output coordinates.
						data.ColVertices[n].color = data.desc.output_color_func(data.ColVertices[n].vector.x, data.ColVertices[n].vector.y, data.ColVertices[n].vector.z).getColor4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
						case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							USER_ERROR(
								"Input function normals is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
								"If you want to implement the normal vectors of a spherical function you have to use output function normals."
							);

						case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
						{
							for (unsigned n = 0u; n < V; n++)
								// Use the normal function to assign normals to each point.
								data.ColVertices[n].norm = data.desc.output_normal_func(data.ColVertices[n].vector.x, data.ColVertices[n].vector.y, data.ColVertices[n].vector.z).getVector4();

							break;
						}

						case SURFACE_DESC::DERIVATE_NORMALS:
						{
							for (unsigned n = 0u; n < V; n++)
							{
								Vector3f a = (vertices[n].z < 0.999f && vertices[n].z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

								Vector3f ei = vertices[n] * a;
								Vector3f ej = vertices[n] * ei;

								Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
								Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

								Vector3f plus_i = (rot_i * Quaternion(vertices[n]) * rot_i.inv()).getVector();
								Vector3f minus_i = (rot_i.inv() * Quaternion(vertices[n]) * rot_i).getVector();

								Vector3f plus_j = (rot_j * Quaternion(vertices[n]) * rot_j.inv()).getVector();
								Vector3f minus_j = (rot_j.inv() * Quaternion(vertices[n]) * rot_j).getVector();

								Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
								Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

								data.ColVertices[n].norm = (dsdi * dsdj).normalize().getVector4();
							}
							break;
						}

						case SURFACE_DESC::CLOSEST_NEIGHBORS:
							USER_ERROR(
								"Closest neighbor normal derivation is not allowed for a spherical surface beacause the input is a 3D normalized vector.\n"
								"If you want to implement the normal vectors of a spherical function you have to use output function normals."
							);

						default:
							USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, V, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				default:
					USER_ERROR("Unknonw surface coloring type found when trying to initialize a Surface.");
			}

			if (data.desc.enable_updates)
				data.spherical_vertices = vertices;
			else
				delete[] vertices;
			break;
		}

		case SURFACE_DESC::PARAMETRIC_SURFACE:
		{
			USER_CHECK(data.desc.parametric_func,
				"Found nullptr when trying to access a parametric function to generate a Surface."
			);

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					data.Vertices = new SurfaceInternals::Vertex[data.desc.num_u * data.desc.num_v];

					// First assign a position to each vertex given the parametric function.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									SurfaceInternals::Vertex* prev_col = (n > 0u) ? &data.Vertices[(n - 1u) * data.desc.num_v] : data.Vertices;

									SurfaceInternals::Vertex* next_col = (n < data.desc.num_u - 1u) ? &data.Vertices[(n + 1u) * data.desc.num_v] : &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITGlobalColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/GlobalColorPS.cso" : PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)
						));
#endif

					// Create the input layout
					INPUT_ELEMENT_DESC ied[2] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 2u, pvs));
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					USER_CHECK(data.desc.texture_image,
						"Found nullptr when trying to acces an image to create a texture for a textured Surface."
					);

					// Create the texture from the input image.
					data.pUpdateTexture = AddBind(new Texture(data.desc.texture_image, data.desc.enable_updates ? TEXTURE_USAGE_DYNAMIC : TEXTURE_USAGE_DEFAULT));

					// Create the sampler for the texture.
					AddBind(new Sampler(data.desc.pixelated_texture ? SAMPLE_FILTER_POINT : SAMPLE_FILTER_LINEAR, SAMPLE_ADDRESS_CLAMP));

					data.TexVertices = new SurfaceInternals::TextureVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a texture coordinate.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a texture coordinate to the vertex given u and v.
							col[m].coord = _float4vector{ float(n) / (data.desc.num_u - 1u), float(m) / (data.desc.num_v - 1u) };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);
										
										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									SurfaceInternals::TextureVertex* prev_col = (n > 0u) ? &data.TexVertices[(n - 1u) * data.desc.num_v] : data.TexVertices;

									SurfaceInternals::TextureVertex* next_col = (n < data.desc.num_u - 1u) ? &data.TexVertices[(n + 1u) * data.desc.num_v] : &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.TexVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexTexturePS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexTexturePS.cso" : PROJECT_DIR L"shaders/UnlitVertexTexturePS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_TEXTURE_PS : BLOB_ID::BLOB_UNLIT_VERTEX_TEXTURE_PS)
						));
#endif

					// Create the corresponding input layout
					INPUT_ELEMENT_DESC ied[3] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
						{ "TexCoor",	_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 3u, pvs));
					break;
				}

				case SURFACE_DESC::ARRAY_COLORING:
				{
					USER_CHECK(data.desc.color_array,
						"Found nullptr when trying to acces a color array to color an array colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a color to the vertex given its indexs.
							col[m].color = data.desc.color_array[n][m].getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.input_color_func,
						"Found nullptr when trying to acces a color function to color an input function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a color to the vertex given the input coordinates.
							col[m].color = data.desc.input_color_func(u, v).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.output_color_func,
						"Found nullptr when trying to acces a color function to color an output function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.num_u * data.desc.num_v];

					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a color to the vertex given the output coordinates.
							col[m].color = data.desc.output_color_func(pos.x, pos.y, pos.z).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.num_u * data.desc.num_v, data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
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
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				default:
					USER_ERROR("Unknonw surface coloring type found when trying to initialize a Surface.");
			}

			// Create the index buffer for the surface.
			unsigned* indices = new unsigned[6u * (data.desc.num_u - 1u) * (data.desc.num_v - 1u)];

			for (unsigned n = 0u; n < data.desc.num_u - 1u; n++)
			{
				unsigned* col = &indices[6u * n * (data.desc.num_v - 1u)];

				for (unsigned m = 0u; m < data.desc.num_v - 1u; m++)
				{
					col[6u * m + 0u] = n * data.desc.num_v + m;
					col[6u * m + 1u] = (n + 1u) * data.desc.num_v + m;
					col[6u * m + 2u] = n * data.desc.num_v + m + 1u;

					col[6u * m + 3u] = n * data.desc.num_v + m + 1u;
					col[6u * m + 4u] = (n + 1u) * data.desc.num_v + m;
					col[6u * m + 5u] = (n + 1u) * data.desc.num_v + m + 1u;
				}
			}

			AddBind(new IndexBuffer(indices, 6u * (data.desc.num_u - 1u) * (data.desc.num_v - 1u)));

			delete[] indices;
			break;
		}

		case SURFACE_DESC::IMPLICIT_SURFACE:
		{
			USER_CHECK(data.desc.implicit_func,
				"Found nullptr when trying to access an implicit function to generate a Surface."
			);

			USER_CHECK(data.desc.max_refinements,
				"Found no refinements when trying to initialize and implicit Surface.\n"
				"The initial range cube needs to be subdivided at least once to generate an implicit Surface"
			);

			for (unsigned i = 0u; i < data.desc.max_refinements; i++)
				USER_CHECK(data.desc.refinements[i],
					"Found zero when trying to get a refinement for an implicit Surface.\n"
					"You cannot subdivide the cube in zero pieces, refinement values must be at least one.\n"
					"If you increased the maximum refinements you also have to specify what those new refinements will be."
				);

			struct cube_search
			{
				static inline Vector3f lerp_iso(const Vector3f& a, const Vector3f& b, float fa, float fb)
				{
					// Solve fa + t(fb-fa) = 0  =>  t = fa/(fa-fb)
					float denom = (fa - fb);
					float t = (denom != 0.0f) ? (fa / denom) : 0.5f; // fallback
					return a + (b - a) * t;
				}

				// Function to add the vertices and triangles to the list from an intesection cube.
				static inline void vertices_from_cube(Vector3f* vertices, Vector3i* triangles, unsigned& num_vertices, unsigned& num_triangles,
					const Vector3f& p0, const Vector3f& dp, const float val[8])
				{
					static const int aiCubeEdgeFlags[256] =
					{
						0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
						0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
						0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
						0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
						0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
						0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
						0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
						0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
						0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
						0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
						0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
						0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
						0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
						0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
						0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
						0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
					};

					static const int a2iTriangleConnectionTable[256][16] =
					{
							{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
							{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
							{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
							{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
							{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
							{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
							{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
							{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
							{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
							{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
							{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
							{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
							{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
							{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
							{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
							{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
							{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
							{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
							{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
							{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
							{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
							{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
							{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
							{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
							{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
							{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
							{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
							{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
							{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
							{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
							{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
							{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
							{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
							{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
							{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
							{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
							{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
							{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
							{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
							{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
							{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
							{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
							{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
							{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
							{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
							{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
							{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
							{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
							{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
							{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
							{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
							{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
							{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
							{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
							{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
							{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
							{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
							{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
							{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
							{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
							{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
							{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
							{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
							{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
							{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
							{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
							{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
							{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
							{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
							{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
							{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
							{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
							{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
							{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
							{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
							{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
							{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
							{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
							{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
							{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
							{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
							{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
							{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
							{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
							{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
							{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
							{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
							{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
							{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
							{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
							{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
							{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
							{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
							{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
							{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
							{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
							{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
							{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
							{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
							{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
							{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
							{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
							{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
							{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
							{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
							{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
							{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
							{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
							{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
							{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
							{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
							{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
							{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
							{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
							{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
							{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
							{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
							{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
							{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
							{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
							{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
							{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
							{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
							{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
							{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
							{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
							{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
							{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
							{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
							{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
							{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
							{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
							{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
							{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
							{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
							{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
							{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
							{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
							{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
							{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
							{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
							{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
							{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
							{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
							{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
							{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
							{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
							{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
							{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
							{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
							{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
							{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
							{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
							{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
							{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
							{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
					};

					// Corner positions (standard MC corner order)
					Vector3f p[8] = {
						{p0.x,        p0.y,        p0.z       },
						{p0.x + dp.x, p0.y,        p0.z       },
						{p0.x + dp.x, p0.y + dp.y, p0.z       },
						{p0.x,        p0.y + dp.y, p0.z       },
						{p0.x,        p0.y,        p0.z + dp.z},
						{p0.x + dp.x, p0.y,        p0.z + dp.z},
						{p0.x + dp.x, p0.y + dp.y, p0.z + dp.z},
						{p0.x,        p0.y + dp.y, p0.z + dp.z}
					};

					// Build cubeIndex bitmask: bit i = 1 if corner i is "inside" (val < 0)
					int cubeIndex = 0;
					for (int i = 0; i < 8; ++i)
						if (val[i] < 0.0f) cubeIndex |= (1 << i);

					// No intersections
					if (aiCubeEdgeFlags[cubeIndex] == 0) return;

					Vector3f vertList[12];

					// Edge endpoints in standard MC ordering
					static const int edgeCorners[12][2] = {
						{0,1},{1,2},{2,3},{3,0},
						{4,5},{5,6},{6,7},{7,4},
						{0,4},{1,5},{2,6},{3,7}
					};

					// For each intersected edge, compute intersection vertex
					int mask = aiCubeEdgeFlags[cubeIndex];
					for (int e = 0; e < 12; ++e)
					{
						if (mask & (1 << e))
						{
							int a = edgeCorners[e][0];
							int b = edgeCorners[e][1];
							vertList[e] = lerp_iso(p[a], p[b], val[a], val[b]);
						}
					}

					// Emit triangles using triTable (indices into vertList)
					// triTable[cubeIndex] is a list like: e0,e1,e2, e3,e4,e5, ..., -1
					for (int i = 0; a2iTriangleConnectionTable[cubeIndex][i] != -1; i += 3)
					{
						// Add 3 vertices (simple version: no dedup)
						unsigned i0 = num_vertices++;
						unsigned i1 = num_vertices++;
						unsigned i2 = num_vertices++;

						vertices[i0] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 0]];
						vertices[i1] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 1]];
						vertices[i2] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 2]];

						triangles[num_triangles++] = Vector3i{ (int)i0, (int)i1, (int)i2 };
					}
				}

				// Function to recursively fill up the vertex and triangle lists.
				static void recursive_search(SurfaceInternals& data, Vector2f range_u, Vector2f range_v, Vector2f range_w, unsigned depth,
					Vector3f* vertices, Vector3i* triangles, unsigned& num_vertices, unsigned& num_triangles)
				{
					unsigned refinement = data.desc.refinements[depth];

					unsigned R = refinement + 1u;

					auto idx = [R](unsigned n, unsigned m, unsigned o) { return (n * R + m) * R + o; };

					float u_i = range_u.x;
					float du = (range_u.y - range_u.x) / refinement;
					float v_i = range_v.x;
					float dv = (range_v.y - range_v.x) / refinement;
					float w_i = range_w.x;
					float dw = (range_w.y - range_w.x) / refinement;

					float* cube_grid = new float[R * R * R];

					for (unsigned n = 0u; n < R; n++)
						for (unsigned m = 0u; m < R; m++)
							for (unsigned o = 0u; o < R; o++)
								cube_grid[idx(n, m, o)] = data.desc.implicit_func(u_i + n * du, v_i + m * dv, w_i + o * dw);

					for (unsigned n = 0u; n < refinement; n++)
						for (unsigned m = 0u; m < refinement; m++)
							for (unsigned o = 0u; o < refinement; o++)
								if (
									cube_grid[idx(n, m, o)] * cube_grid[idx(n+1, m  , o  )] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n+1, m+1, o  )] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n  , m+1, o  )] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n  , m  , o+1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n+1, m  , o+1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n+1, m+1, o+1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n  , m+1, o+1)] <= 0.f
									)
								{
									if (depth + 1u == data.desc.max_refinements)
									{
										USER_CHECK(num_triangles + 5u < data.desc.max_implicit_triangles,
											"Maximum amount of triangles reached when generating an implicit surface.\n"
											"If you want to generate this implicit surface you will have to increase the number of triangles.\n"
											"Icrease with caution because the entire length will be stored on CPU and on GPU if updates are enabled.\n"
											"Function constant zero is invalid and will quickly crash the implicit generation."
										);

										float values[8] = 
										{
											cube_grid[idx(n  , m  , o  )],
											cube_grid[idx(n+1, m  , o  )],
											cube_grid[idx(n+1, m+1, o  )],
											cube_grid[idx(n  , m+1, o  )],
											cube_grid[idx(n  , m  , o+1)],
											cube_grid[idx(n+1, m  , o+1)],
											cube_grid[idx(n+1, m+1, o+1)],
											cube_grid[idx(n  , m+1, o+1)]
										};
										vertices_from_cube(vertices, triangles, num_vertices, num_triangles, 
											Vector3f(u_i + n * du, v_i + m * dv, w_i + o * dw), Vector3f(du,dv,dw), values);
									}
									else
										recursive_search(data, { u_i + n * du,u_i + (n + 1) * du }, { v_i + m * dv,v_i + (m + 1) * dv }, { w_i + o * dw,w_i + (o + 1) * dw }, 
											depth + 1u, vertices, triangles, num_vertices, num_triangles);
								}
					
					delete[] cube_grid;
				}
			};

			unsigned n_vertices = 0u;
			unsigned n_triangles = 0u;

			data.implicit_vertices = new Vector3f[data.desc.max_implicit_triangles * 3u];
			data.implicit_triangles = new Vector3i[data.desc.max_implicit_triangles];

			cube_search::recursive_search(data, data.desc.range_u, data.desc.range_v, data.desc.range_w, 0u, data.implicit_vertices, data.implicit_triangles, n_vertices, n_triangles);

			// First add the index buffer.
			AddBind(new IndexBuffer((unsigned*)data.implicit_triangles, 3u * n_triangles));

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					data.Vertices = new SurfaceInternals::Vertex[data.desc.enable_updates ? 3u * data.desc.max_implicit_triangles : n_vertices];

					// First assign a position to each vertex.
					for (unsigned n = 0u; n < n_vertices; n++)
						data.Vertices[n].vector = data.implicit_vertices[n].getVector4();

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
								USER_ERROR(
									"Input function normal computation is not allowed for an implicit surface.\n"
									"Given the nature of the surface only output function and derivation are allowed for normal computation."
								);

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
									data.Vertices[n].norm = data.desc.output_normal_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getVector4();

								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
								{
									float dFdx = data.desc.implicit_func(data.implicit_vertices[n].x + data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x - data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z);
									float dFdy = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y + data.desc.delta_value, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y - data.desc.delta_value, data.implicit_vertices[n].z);
									float dFdz = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z + data.desc.delta_value) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z - data.desc.delta_value);

									data.Vertices[n].norm = Vector3f(dFdx, dFdy, dFdz).normalize().getVector4();
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
								USER_ERROR(
									"Closest neighbors normal computation is not allowed for an implicit surface.\n"
									"Given the nature of the surface only output function and derivation are allowed for normal computation."
								);

							default:
								USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.Vertices, data.desc.enable_updates ? 3u * data.desc.max_implicit_triangles : n_vertices, 
						data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
					if (!data.desc.enable_updates)
					{
						delete[] data.Vertices;
						data.Vertices = nullptr;

						delete[] data.implicit_vertices;
						data.implicit_vertices = nullptr;

						delete[] data.implicit_triangles;
						data.implicit_triangles = nullptr;
					}

					// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
					VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/GlobalColorVS.cso"));
#else
					VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_GLOBAL_COLOR_VS), getBlobSizeFromId(BLOB_ID::BLOB_GLOBAL_COLOR_VS)));
#endif

					// Create the corresponding Pixel Shader
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITGlobalColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_GLOBAL_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_GLOBAL_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/GlobalColorPS.cso" : PROJECT_DIR L"shaders/UnlitGlobalColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_GLOBAL_COLOR_PS : BLOB_ID::BLOB_UNLIT_GLOBAL_COLOR_PS)
						));
#endif

					// Create the input layout
					INPUT_ELEMENT_DESC ied[2] =
					{
						{ "Position",	_4_FLOAT },
						{ "Normal",		_4_FLOAT },
					};
					AddBind(new InputLayout(ied, 2u, pvs));
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
					USER_ERROR(
						"Textured coloring is not supported for an implicit Surface.\n"
						"Given the nature of the function the only colorings allowed are global and output function."
					);

				case SURFACE_DESC::ARRAY_COLORING:
					USER_ERROR(
						"Array coloring is not supported for an implicit Surface.\n"
						"Given the nature of the function the only colorings allowed are global and output function."
					);

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
					USER_ERROR(
						"Input function coloring is not supported for an implicit Surface.\n"
						"Given the nature of the function the only colorings allowed are global and output function."
					);

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					USER_CHECK(data.desc.output_color_func,
						"Found nullptr when trying to acces a color function to color an output function colored Surface."
					);

					data.ColVertices = new SurfaceInternals::ColorVertex[data.desc.enable_updates ? 3u * data.desc.max_implicit_triangles : n_vertices];

					// Assign a position and color to each vertex.
					for (unsigned n = 0u; n < n_vertices; n++)
					{
						data.ColVertices[n].vector = data.implicit_vertices[n].getVector4();

						data.ColVertices[n].color = data.desc.output_color_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getColor4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
						case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							USER_ERROR(
								"Input function normal computation is not allowed for an implicit surface.\n"
								"Given the nature of the surface only output function and derivation are allowed for normal computation."
							);

						case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
						{
							for (unsigned n = 0u; n < n_vertices; n++)
								data.ColVertices[n].norm = data.desc.output_normal_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getVector4();

							break;
						}

						case SURFACE_DESC::DERIVATE_NORMALS:
						{
							for (unsigned n = 0u; n < n_vertices; n++)
							{
								float dFdx = data.desc.implicit_func(data.implicit_vertices[n].x + data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z) -
									data.desc.implicit_func(data.implicit_vertices[n].x - data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z);
								float dFdy = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y + data.desc.delta_value, data.implicit_vertices[n].z) -
									data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y - data.desc.delta_value, data.implicit_vertices[n].z);
								float dFdz = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z + data.desc.delta_value) -
									data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z - data.desc.delta_value);

								data.ColVertices[n].norm = Vector3f(dFdx, dFdy, dFdz).normalize().getVector4();
							}
							break;
						}

						case SURFACE_DESC::CLOSEST_NEIGHBORS:
							USER_ERROR(
								"Closest neighbors normal computation is not allowed for an implicit surface.\n"
								"Given the nature of the surface only output function and derivation are allowed for normal computation."
							);

						default:
							USER_ERROR("Unknonw surface normal computation type found when trying to initialize a Surface.");
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB = AddBind(new VertexBuffer(data.ColVertices, data.desc.enable_updates ? 3u * data.desc.max_implicit_triangles : n_vertices, 
						data.desc.enable_updates ? VB_USAGE_DYNAMIC : VB_USAGE_DEFAULT));

					// If updates are disabled free the memory on the CPU.
					if (!data.desc.enable_updates)
					{
						delete[] data.ColVertices;
						data.ColVertices = nullptr;

						delete[] data.implicit_vertices;
						data.implicit_vertices = nullptr;

						delete[] data.implicit_triangles;
						data.implicit_triangles = nullptr;
					}

					// Create the corresponding Vertex Shader
#ifndef _DEPLOYMENT
					VertexShader* pvs = AddBind(new VertexShader(PROJECT_DIR L"shaders/VertexColorVS.cso"));
#else
					VertexShader* pvs = AddBind(new VertexShader(getBlobFromId(BLOB_ID::BLOB_VERTEX_COLOR_VS), getBlobSizeFromId(BLOB_ID::BLOB_VERTEX_COLOR_VS)));
#endif

					// Create the corresponding Pixel Shader and Blender
					if (data.desc.enable_transparency)
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/OITVertexColorPS.cso" : PROJECT_DIR L"shaders/OITUnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_OIT_VERTEX_COLOR_PS : BLOB_ID::BLOB_OIT_UNLIT_VERTEX_COLOR_PS)
						));
#endif
					else
#ifndef _DEPLOYMENT
						AddBind(new PixelShader(data.desc.enable_illuminated ? PROJECT_DIR L"shaders/VertexColorPS.cso" : PROJECT_DIR L"shaders/UnlitVertexColorPS.cso"));
#else
						AddBind(new PixelShader(
							getBlobFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS),
							getBlobSizeFromId(data.desc.enable_illuminated ? BLOB_ID::BLOB_VERTEX_COLOR_PS : BLOB_ID::BLOB_UNLIT_VERTEX_COLOR_PS)
						));
#endif

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

				default:
					USER_ERROR("Unknonw surface coloring type found when trying to initialize a Surface.");
			}
			break;
		}

		default:
			USER_ERROR("Unknonw surface type found when trying to initialize a Surface.");
	}

	AddBind(new Topology(TRIANGLE_LIST));
	AddBind(new Rasterizer(data.desc.double_sided_rendering, data.desc.wire_frame_topology));
	AddBind(new Blender(data.desc.enable_transparency ? BLEND_MODE_OIT_WEIGHTED : BLEND_MODE_OPAQUE));

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
			data.pscBuff.lightsource[2].position = {-8.f, 0.f,-8.f };
			data.pscBuff.lightsource[3].position = { 8.f, 0.f, 8.f };
		}

		data.pPSCB = AddBind(new ConstantBuffer(&data.pscBuff, PIXEL_CONSTANT_BUFFER));
	}

	// If coloring is global create and bind the global color.
	if (data.desc.coloring == SURFACE_DESC::GLOBAL_COLORING)
	{
		_float4color col = data.desc.global_color.getColor4();
		data.pGlobalColorCB = AddBind(new ConstantBuffer(&col, PIXEL_CONSTANT_BUFFER, 1u /*Slot*/));
	}
}

/*
-----------------------------------------------------------------------------------------------------------
 User Functions
-----------------------------------------------------------------------------------------------------------
*/

// If updates are enabled it expects the original generation function, coloring function and 
// normal function (if used) to still be callable and will use the new ranges and the original 
// settings to update the surface. For in place dynamic surfaces you can internally change 
// static parameters in the called functions to output different values with same inputs.
// The ranges will only be updated if they are different than (0.f,0.f).

void Surface::updateShape(Vector2f range_u, Vector2f range_v, Vector2f range_w)
{
	USER_CHECK(isInit,
		"Trying to update the shape on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the vertices on a Surface with updates disabled."
	);

	if (range_u)
		data.desc.range_u = range_u;

	if (range_v)
		data.desc.range_v = range_v;

	if (range_w)
		data.desc.range_w = range_w;

	// Calculate initial values and deltas of both coordinates.
	float du = data.desc.border_points_included ?
		(data.desc.range_u.y - data.desc.range_u.x) / (data.desc.num_u - 1.f) :
		(data.desc.range_u.y - data.desc.range_u.x) / (data.desc.num_u + 1.f);

	float u_i = data.desc.border_points_included ? data.desc.range_u.x : data.desc.range_u.x + du;

	float dv = data.desc.border_points_included ?
		(data.desc.range_v.y - data.desc.range_v.x) / (data.desc.num_v - 1.f) :
		(data.desc.range_v.y - data.desc.range_v.x) / (data.desc.num_v + 1.f);

	float v_i = data.desc.border_points_included ? data.desc.range_v.x : data.desc.range_v.x + dv;

	// Different generation methods for different surface types.
	switch (data.desc.type)
	{
		case SURFACE_DESC::EXPLICIT_SURFACE:
		{
			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					// First assign a position to each vertex given the explicit function.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									SurfaceInternals::Vertex* prev_col = (n > 0u) ? &data.Vertices[(n - 1u) * data.desc.num_v] : data.Vertices;

									SurfaceInternals::Vertex* next_col = (n < data.desc.num_u - 1u) ? &data.Vertices[(n + 1u) * data.desc.num_v] : &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Update the Vertex Buffer
					data.pUpdateVB->updateVertices(data.Vertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					// Assign a position to each vertex given the explicit function.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									SurfaceInternals::TextureVertex* prev_col = (n > 0u) ? &data.TexVertices[(n - 1u) * data.desc.num_v] : data.TexVertices;

									SurfaceInternals::TextureVertex* next_col = (n < data.desc.num_u - 1u) ? &data.TexVertices[(n + 1u) * data.desc.num_v] : &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.TexVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::ARRAY_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Update the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a color to the vertex given the input coordinates.
							col[m].color = data.desc.input_color_func(x, y).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Update the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float x = u_i + n * du;
							float y = v_i + m * dv;
							float z = data.desc.explicit_func(x, y);

							// Assign a value for each point of the function.
							col[m].vector = _float4vector{ x, y, z, 0.f };

							// Assign a color to the vertex given the output coordinates.
							col[m].color = data.desc.output_color_func(x, y, z).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(x, y).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(x, y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float x = u_i + n * du;
										float y = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = { 2 * data.desc.delta_value, 0.f, data.desc.explicit_func(x + data.desc.delta_value, y) - data.desc.explicit_func(x - data.desc.delta_value, y) };
										Vector3f dsdv = { 0.f, 2 * data.desc.delta_value, data.desc.explicit_func(x, y + data.desc.delta_value) - data.desc.explicit_func(x, y - data.desc.delta_value) };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Update the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
					break;
				}
			}
			break;
		}

		case SURFACE_DESC::SPHERICAL_SURFACE:
		{
			unsigned V = 12u;
			unsigned A = 30u;

			for (unsigned d = 0u; d < data.desc.icosphere_depth; d++)
			{
				V = V + A;
				A = A * 4;
			}

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					// First assign a position to each vertex given the parametric function.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = data.spherical_vertices[n];

						// Assign a radius for each point of the sphere.
						data.Vertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
								{
									Vector3f vertex = data.spherical_vertices[n];

									Vector3f a = (vertex.z < 0.999f && vertex.z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

									Vector3f ei = vertex * a;
									Vector3f ej = vertex * ei;

									Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
									Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

									Vector3f plus_i = (rot_i * Quaternion(vertex) * rot_i.inv()).getVector();
									Vector3f minus_i = (rot_i.inv() * Quaternion(vertex) * rot_i).getVector();

									Vector3f plus_j = (rot_j * Quaternion(vertex) * rot_j.inv()).getVector();
									Vector3f minus_j = (rot_j.inv() * Quaternion(vertex) * rot_j).getVector();

									Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
									Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

									data.Vertices[n].norm = (dsdi * dsdj).normalize().getVector4();
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
									// Use the normal function to assign normals to each point.
									data.Vertices[n].norm = data.desc.output_normal_func(data.Vertices[n].vector.x, data.Vertices[n].vector.y, data.Vertices[n].vector.z).getVector4();
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.Vertices, V);
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a cube texture coordinate equal to the S2 position.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = data.spherical_vertices[n];

						// Assign a radius for each point of the sphere.
						data.TexVertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
								{
									Vector3f vertex = data.spherical_vertices[n];

									Vector3f a = (vertex.z < 0.999f && vertex.z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

									Vector3f ei = vertex * a;
									Vector3f ej = vertex * ei;

									Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
									Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

									Vector3f plus_i = (rot_i * Quaternion(vertex) * rot_i.inv()).getVector();
									Vector3f minus_i = (rot_i.inv() * Quaternion(vertex) * rot_i).getVector();

									Vector3f plus_j = (rot_j * Quaternion(vertex) * rot_j.inv()).getVector();
									Vector3f minus_j = (rot_j.inv() * Quaternion(vertex) * rot_j).getVector();

									Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
									Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

									data.TexVertices[n].norm = (dsdi * dsdj).normalize().getVector4();
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
									// Use the normal function to assign normals to each point.
									data.TexVertices[n].norm = data.desc.output_normal_func(data.TexVertices[n].vector.x, data.TexVertices[n].vector.y, data.TexVertices[n].vector.z).getVector4();
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.TexVertices, V);
					break;
				}

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < V; n++)
					{
						Vector3f vertex = data.spherical_vertices[n];

						// Assign a radius for each point of the sphere.
						data.ColVertices[n].vector = (vertex * data.desc.spherical_func(vertex.x, vertex.y, vertex.z)).getVector4();

						// Assign a color to the vertex given the output coordinates.
						data.ColVertices[n].color = data.desc.output_color_func(data.ColVertices[n].vector.x, data.ColVertices[n].vector.y, data.ColVertices[n].vector.z).getColor4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
								{
									Vector3f vertex = data.spherical_vertices[n];

									Vector3f a = (vertex.z < 0.999f && vertex.z > -0.999f) ? Vector3f(0.f, 0.f, 1.f) : Vector3f(0.f, 1.f, 0.f);

									Vector3f ei = vertex * a;
									Vector3f ej = vertex * ei;

									Quaternion rot_i = Quaternion::Rotation(ei, data.desc.delta_value);
									Quaternion rot_j = Quaternion::Rotation(ej, data.desc.delta_value);

									Vector3f plus_i = (rot_i * Quaternion(vertex) * rot_i.inv()).getVector();
									Vector3f minus_i = (rot_i.inv() * Quaternion(vertex) * rot_i).getVector();

									Vector3f plus_j = (rot_j * Quaternion(vertex) * rot_j.inv()).getVector();
									Vector3f minus_j = (rot_j.inv() * Quaternion(vertex) * rot_j).getVector();

									Vector3f dsdi = plus_i * data.desc.spherical_func(plus_i.x, plus_i.y, plus_i.z) - minus_i * data.desc.spherical_func(minus_i.x, minus_i.y, minus_i.z);
									Vector3f dsdj = plus_j * data.desc.spherical_func(plus_j.x, plus_j.y, plus_j.z) - minus_j * data.desc.spherical_func(minus_j.x, minus_j.y, minus_j.z);

									data.ColVertices[n].norm = (dsdi * dsdj).normalize().getVector4();
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < V; n++)
									// Use the normal function to assign normals to each point.
									data.ColVertices[n].norm = data.desc.output_normal_func(data.ColVertices[n].vector.x, data.ColVertices[n].vector.y, data.ColVertices[n].vector.z).getVector4();
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, V);
					break;
				}
			}
			break;
		}

		case SURFACE_DESC::PARAMETRIC_SURFACE:
		{
			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					// First assign a position to each vertex given the parametric function.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::Vertex* col = &data.Vertices[n * data.desc.num_v];

									SurfaceInternals::Vertex* prev_col = (n > 0u) ? &data.Vertices[(n - 1u) * data.desc.num_v] : data.Vertices;

									SurfaceInternals::Vertex* next_col = (n < data.desc.num_u - 1u) ? &data.Vertices[(n + 1u) * data.desc.num_v] : &data.Vertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.Vertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::TEXTURED_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a texture coordinate.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);
										
										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::TextureVertex* col = &data.TexVertices[n * data.desc.num_v];

									SurfaceInternals::TextureVertex* prev_col = (n > 0u) ? &data.TexVertices[(n - 1u) * data.desc.num_v] : data.TexVertices;

									SurfaceInternals::TextureVertex* next_col = (n < data.desc.num_u - 1u) ? &data.TexVertices[(n + 1u) * data.desc.num_v] : &data.TexVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.TexVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::ARRAY_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::INPUT_FUNCTION_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a color to the vertex given the input coordinates.
							col[m].color = data.desc.input_color_func(u, v).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
					break;
				}

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					// Assign a position to each vertex given the explicit function and a color.
					for (unsigned n = 0u; n < data.desc.num_u; n++)
					{
						// Create a pointer to the current column for convenience.
						SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

						for (unsigned m = 0u; m < data.desc.num_v; m++)
						{
							float u = u_i + n * du;
							float v = v_i + m * dv;
							Vector3f pos = data.desc.parametric_func(u, v);

							// Assign a value for each point of the function.
							col[m].vector = pos.getVector4();

							// Assign a color to the vertex given the output coordinates.
							col[m].color = data.desc.output_color_func(pos.x, pos.y, pos.z).getColor4();
						}
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::INPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.input_normal_func(u, v).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Use the normal function to assign normals to each point.
										col[m].norm = data.desc.output_normal_func(col[m].vector.x, col[m].vector.y, col[m].vector.z).getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the current column for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										float u = u_i + n * du;
										float v = v_i + m * dv;

										// Derivate around the point to find the normal vector of the point.
										Vector3f dsdu = data.desc.parametric_func(u + data.desc.delta_value, v) - data.desc.parametric_func(u - data.desc.delta_value, v);
										Vector3f dsdv = data.desc.parametric_func(u, v + data.desc.delta_value) - data.desc.parametric_func(u, v - data.desc.delta_value);

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}

							case SURFACE_DESC::CLOSEST_NEIGHBORS:
							{
								for (unsigned n = 0u; n < data.desc.num_u; n++)
								{
									// Create a pointer to the surrounding columns for convenience.
									SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

									SurfaceInternals::ColorVertex* prev_col = (n > 0u) ? &data.ColVertices[(n - 1u) * data.desc.num_v] : data.ColVertices;

									SurfaceInternals::ColorVertex* next_col = (n < data.desc.num_u - 1u) ? &data.ColVertices[(n + 1u) * data.desc.num_v] : &data.ColVertices[n * data.desc.num_v];

									for (unsigned m = 0u; m < data.desc.num_v; m++)
									{
										unsigned prev_m = (m > 0u) ? m - 1u : 0u;
										unsigned next_m = (m < data.desc.num_v - 1u) ? m + 1u : m;

										// Find the points around to use for derivation.
										Vector3f dsdu = { next_col[m].vector.x - prev_col[m].vector.x, 0.f, next_col[m].vector.z - prev_col[m].vector.z };
										Vector3f dsdv = { 0.f, col[next_m].vector.y - col[prev_m].vector.y, col[next_m].vector.z - col[prev_m].vector.z };

										col[m].norm = (dsdu * dsdv).normalize().getVector4();
									}
								}
								break;
							}
						}
					}

					// Update the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u* data.desc.num_v);
					break;
				}
			}
			break;
		}

		case SURFACE_DESC::IMPLICIT_SURFACE:
		{
			struct cube_search
			{
				static inline Vector3f lerp_iso(const Vector3f& a, const Vector3f& b, float fa, float fb)
				{
					// Solve fa + t(fb-fa) = 0  =>  t = fa/(fa-fb)
					float denom = (fa - fb);
					float t = (denom != 0.0f) ? (fa / denom) : 0.5f; // fallback
					return a + (b - a) * t;
				}

				// Function to add the vertices and triangles to the list from an intesection cube.
				static inline void vertices_from_cube(Vector3f* vertices, Vector3i* triangles, unsigned& num_vertices, unsigned& num_triangles,
					const Vector3f& p0, const Vector3f& dp, const float val[8])
				{
					static const int aiCubeEdgeFlags[256] =
					{
						0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
						0x190, 0x099, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
						0x230, 0x339, 0x033, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
						0x3a0, 0x2a9, 0x1a3, 0x0aa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
						0x460, 0x569, 0x663, 0x76a, 0x066, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
						0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0x0ff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
						0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x055, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
						0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0x0cc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
						0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0x0cc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
						0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x055, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
						0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0x0ff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
						0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x066, 0x76a, 0x663, 0x569, 0x460,
						0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0x0aa, 0x1a3, 0x2a9, 0x3a0,
						0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x033, 0x339, 0x230,
						0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x099, 0x190,
						0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x000
					};

					static const int a2iTriangleConnectionTable[256][16] =
					{
							{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
							{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
							{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
							{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
							{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
							{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
							{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
							{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
							{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
							{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
							{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
							{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
							{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
							{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
							{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
							{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
							{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
							{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
							{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
							{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
							{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
							{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
							{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
							{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
							{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
							{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
							{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
							{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
							{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
							{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
							{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
							{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
							{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
							{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
							{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
							{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
							{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
							{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
							{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
							{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
							{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
							{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
							{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
							{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
							{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
							{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
							{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
							{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
							{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
							{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
							{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
							{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
							{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
							{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
							{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
							{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
							{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
							{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
							{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
							{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
							{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
							{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
							{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
							{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
							{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
							{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
							{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
							{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
							{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
							{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
							{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
							{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
							{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
							{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
							{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
							{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
							{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
							{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
							{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
							{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
							{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
							{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
							{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
							{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
							{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
							{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
							{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
							{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
							{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
							{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
							{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
							{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
							{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
							{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
							{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
							{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
							{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
							{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
							{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
							{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
							{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
							{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
							{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
							{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
							{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
							{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
							{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
							{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
							{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
							{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
							{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
							{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
							{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
							{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
							{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
							{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
							{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
							{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
							{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
							{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
							{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
							{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
							{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
							{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
							{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
							{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
							{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
							{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
							{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
							{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
							{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
							{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
							{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
							{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
							{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
							{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
							{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
							{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
							{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
							{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
							{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
							{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
							{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
							{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
							{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
							{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
							{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
							{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
							{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
							{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
							{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
							{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
							{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
							{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
							{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
							{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
							{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
							{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
							{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
							{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
							{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
							{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
							{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
							{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
							{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
							{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
					};

					// Corner positions (standard MC corner order)
					Vector3f p[8] = {
						{p0.x,        p0.y,        p0.z       },
						{p0.x + dp.x, p0.y,        p0.z       },
						{p0.x + dp.x, p0.y + dp.y, p0.z       },
						{p0.x,        p0.y + dp.y, p0.z       },
						{p0.x,        p0.y,        p0.z + dp.z},
						{p0.x + dp.x, p0.y,        p0.z + dp.z},
						{p0.x + dp.x, p0.y + dp.y, p0.z + dp.z},
						{p0.x,        p0.y + dp.y, p0.z + dp.z}
					};

					// Build cubeIndex bitmask: bit i = 1 if corner i is "inside" (val < 0)
					int cubeIndex = 0;
					for (int i = 0; i < 8; ++i)
						if (val[i] < 0.0f) cubeIndex |= (1 << i);

					// No intersections
					if (aiCubeEdgeFlags[cubeIndex] == 0) return;

					Vector3f vertList[12];

					// Edge endpoints in standard MC ordering
					static const int edgeCorners[12][2] = {
						{0,1},{1,2},{2,3},{3,0},
						{4,5},{5,6},{6,7},{7,4},
						{0,4},{1,5},{2,6},{3,7}
					};

					// For each intersected edge, compute intersection vertex
					int mask = aiCubeEdgeFlags[cubeIndex];
					for (int e = 0; e < 12; ++e)
					{
						if (mask & (1 << e))
						{
							int a = edgeCorners[e][0];
							int b = edgeCorners[e][1];
							vertList[e] = lerp_iso(p[a], p[b], val[a], val[b]);
						}
					}

					// Emit triangles using triTable (indices into vertList)
					// triTable[cubeIndex] is a list like: e0,e1,e2, e3,e4,e5, ..., -1
					for (int i = 0; a2iTriangleConnectionTable[cubeIndex][i] != -1; i += 3)
					{
						// Add 3 vertices (simple version: no dedup)
						unsigned i0 = num_vertices++;
						unsigned i1 = num_vertices++;
						unsigned i2 = num_vertices++;

						vertices[i0] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 0]];
						vertices[i1] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 1]];
						vertices[i2] = vertList[a2iTriangleConnectionTable[cubeIndex][i + 2]];

						triangles[num_triangles++] = Vector3i{ (int)i0, (int)i1, (int)i2 };
					}
				}

				// Function to recursively fill up the vertex and triangle lists.
				static void recursive_search(SurfaceInternals& data, Vector2f range_u, Vector2f range_v, Vector2f range_w, unsigned depth,
					Vector3f* vertices, Vector3i* triangles, unsigned& num_vertices, unsigned& num_triangles)
				{
					unsigned refinement = data.desc.refinements[depth];

					unsigned R = refinement + 1u;

					auto idx = [R](unsigned n, unsigned m, unsigned o) { return (n * R + m) * R + o; };

					float u_i = range_u.x;
					float du = (range_u.y - range_u.x) / refinement;
					float v_i = range_v.x;
					float dv = (range_v.y - range_v.x) / refinement;
					float w_i = range_w.x;
					float dw = (range_w.y - range_w.x) / refinement;

					float* cube_grid = new float[R * R * R];

					for (unsigned n = 0u; n < R; n++)
						for (unsigned m = 0u; m < R; m++)
							for (unsigned o = 0u; o < R; o++)
								cube_grid[idx(n, m, o)] = data.desc.implicit_func(u_i + n * du, v_i + m * dv, w_i + o * dw);

					for (unsigned n = 0u; n < refinement; n++)
						for (unsigned m = 0u; m < refinement; m++)
							for (unsigned o = 0u; o < refinement; o++)
								if (
									cube_grid[idx(n, m, o)] * cube_grid[idx(n + 1, m, o)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n + 1, m + 1, o)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n, m + 1, o)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n, m, o + 1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n + 1, m, o + 1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n + 1, m + 1, o + 1)] <= 0.f ||
									cube_grid[idx(n, m, o)] * cube_grid[idx(n, m + 1, o + 1)] <= 0.f
									)
								{
									if (depth + 1u == data.desc.max_refinements)
									{
										USER_CHECK(num_triangles + 5u < data.desc.max_implicit_triangles,
											"Maximum amount of triangles reached when generating an implicit surface.\n"
											"If you want to generate this implicit surface you will have to increase the number of triangles.\n"
											"Icrease with caution because the entire length will be stored on CPU and on GPU if updates are enabled.\n"
											"Function constant zero is invalid and will quickly crash the implicit generation."
										);

										float values[8] =
										{
											cube_grid[idx(n    , m  , o   )],
											cube_grid[idx(n + 1, m  , o   )],
											cube_grid[idx(n + 1, m + 1, o)],
											cube_grid[idx(n  , m + 1, o)],
											cube_grid[idx(n  , m  , o + 1)],
											cube_grid[idx(n + 1, m  , o + 1)],
											cube_grid[idx(n + 1, m + 1, o + 1)],
											cube_grid[idx(n  , m + 1, o + 1)]
										};
										vertices_from_cube(vertices, triangles, num_vertices, num_triangles,
											Vector3f(u_i + n * du, v_i + m * dv, w_i + o * dw), Vector3f(du, dv, dw), values);
									}
									else
										recursive_search(data, { u_i + n * du,u_i + (n + 1) * du }, { v_i + m * dv,v_i + (m + 1) * dv }, { w_i + o * dw,w_i + (o + 1) * dw },
											depth + 1u, vertices, triangles, num_vertices, num_triangles);
								}

					delete[] cube_grid;
				}
			};

			unsigned n_vertices = 0u;
			unsigned n_triangles = 0u;

			cube_search::recursive_search(data, data.desc.range_u, data.desc.range_v, data.desc.range_w, 0u, data.implicit_vertices, data.implicit_triangles, n_vertices, n_triangles);

			// First replace the index buffer.
			changeBind(new IndexBuffer((unsigned*)data.implicit_triangles, 3u * n_triangles), 0u);

			switch (data.desc.coloring)
			{
				case SURFACE_DESC::GLOBAL_COLORING:
				{
					// First assign a position to each vertex.
					for (unsigned n = 0u; n < n_vertices; n++)
						data.Vertices[n].vector = data.implicit_vertices[n].getVector4();

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
									data.Vertices[n].norm = data.desc.output_normal_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getVector4();

								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
								{
									float dFdx = data.desc.implicit_func(data.implicit_vertices[n].x + data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x - data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z);
									float dFdy = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y + data.desc.delta_value, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y - data.desc.delta_value, data.implicit_vertices[n].z);
									float dFdz = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z + data.desc.delta_value) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z - data.desc.delta_value);

									data.Vertices[n].norm = Vector3f(dFdx, dFdy, dFdz).normalize().getVector4();
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.Vertices, 3u * data.desc.max_implicit_triangles);
					break;
				}

				case SURFACE_DESC::OUTPUT_FUNCTION_COLORING:
				{
					// Assign a position and color to each vertex.
					for (unsigned n = 0u; n < n_vertices; n++)
					{
						data.ColVertices[n].vector = data.implicit_vertices[n].getVector4();

						data.ColVertices[n].color = data.desc.output_color_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getColor4();
					}

					// If illuminated find normal vector.
					if (data.desc.enable_illuminated)
					{
						switch (data.desc.normal_computation)
						{
							case SURFACE_DESC::OUTPUT_FUNCTION_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
									data.ColVertices[n].norm = data.desc.output_normal_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z).getVector4();

								break;
							}

							case SURFACE_DESC::DERIVATE_NORMALS:
							{
								for (unsigned n = 0u; n < n_vertices; n++)
								{
									float dFdx = data.desc.implicit_func(data.implicit_vertices[n].x + data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x - data.desc.delta_value, data.implicit_vertices[n].y, data.implicit_vertices[n].z);
									float dFdy = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y + data.desc.delta_value, data.implicit_vertices[n].z) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y - data.desc.delta_value, data.implicit_vertices[n].z);
									float dFdz = data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z + data.desc.delta_value) -
										data.desc.implicit_func(data.implicit_vertices[n].x, data.implicit_vertices[n].y, data.implicit_vertices[n].z - data.desc.delta_value);

									data.ColVertices[n].norm = Vector3f(dFdx, dFdy, dFdz).normalize().getVector4();
								}
								break;
							}
						}
					}

					// Create the Vertex Buffer
					data.pUpdateVB->updateVertices(data.ColVertices, 3u * data.desc.max_implicit_triangles);
					break;
				}
			}
			break;
		}
	}
}

// If updates are enabled and coloring is from an array, expects a valid array of 
// size num_u x num_v and updates every vertex of the surface with the new colors.

void Surface::updateColors(const Color** color_array)
{
	USER_CHECK(isInit,
		"Trying to update the colors on an uninitialized Surface."
	);

	USER_CHECK(color_array,
		"Trying to update the colors on a Surface with an invalid color array."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.coloring == SURFACE_DESC::ARRAY_COLORING,
		"Trying to update the colors on a Surface with a different coloring."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the colors on a Surface with updates disabled."
	);

	// Store the pointer for use during this function.
	data.desc.color_array = (Color**)color_array;

	// Assign a color to each vertex given its indices.
	for (unsigned n = 0u; n < data.desc.num_u; n++)
	{
		// Create a pointer to the current column for convenience.
		SurfaceInternals::ColorVertex* col = &data.ColVertices[n * data.desc.num_v];

		for (unsigned m = 0u; m < data.desc.num_v; m++)
			col[m].color = data.desc.color_array[n][m].getColor4();
	}

	// Update the vertex buffer.
	data.pUpdateVB->updateVertices(data.ColVertices, data.desc.num_u * data.desc.num_v);
}

// If updates are enabled and coloring is textured, it expects a valid image pointer and 
// uses it to update the texture. Image dimensions must be equal to the initial image.

void Surface::updateTexture(const Image* texture_image)
{
	USER_CHECK(isInit,
		"Trying to update the texture on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.coloring == SURFACE_DESC::TEXTURED_COLORING,
		"Trying to update the texture on a Surface with a different coloring."
	);

	USER_CHECK(texture_image,
		"Found nullptr when trying to access an Image to update a textured Surface."
	);

	USER_CHECK(data.desc.enable_updates,
		"Trying to update the texture on a Surface with updates disabled."
	);

	data.pUpdateTexture->update(texture_image);
}

// If the coloring is set to global, updates the global Surface color.

void Surface::updateGlobalColor(Color color)
{
	USER_CHECK(isInit,
		"Trying to update the global color on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.coloring == SURFACE_DESC::GLOBAL_COLORING,
		"Trying to update the global color on a Surface with a different coloring."
	);

	_float4color col = color.getColor4();
	data.pGlobalColorCB->update(&col);
}

// Updates the rotation quaternion of the Surface. If multiplicative it will apply
// the rotation on top of the current rotation. For more information on how to rotate
// with quaternions check the Quaternion header file.

void Surface::updateRotation(Quaternion rotation, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the rotation on an uninitialized Surface."
	);

	USER_CHECK(rotation,
		"Invalid quaternion found when trying to update rotation on a Surface.\n"
		"Quaternion 0 can not be normalized and therefore can not describe an objects rotation."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

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

// Updates the scene position of the Surface. If additive it will add the vector
// to the current position vector of the Surface.

void Surface::updatePosition(Vector3f position, bool additive)
{
	USER_CHECK(isInit,
		"Trying to update the position on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	if (additive)
		data.position += position;
	else
		data.position = position;

	Matrix L = data.rotation.getMatrix() * data.distortion;

	data.vscBuff.transform = L.getMatrix4(data.position);

	data.pVSCB->update(&data.vscBuff);
}

// Updates the matrix multiplied to the Surface, adding any arbitrary linear distortion 
// to it. If you want to simply modify the size of the object just call this function on a 
// diagonal matrix. Check the Matrix header for helpers to create any arbitrary distortion. 
// If multiplicative the distortion will be added to the current distortion.

void Surface::updateDistortion(Matrix distortion, bool multiplicative)
{
	USER_CHECK(isInit,
		"Trying to update the distortion on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

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

void Surface::updateScreenPosition(Vector2f screenDisplacement)
{
	USER_CHECK(isInit,
		"Trying to update the screen position on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	data.vscBuff.displacement = screenDisplacement.getVector4();
	data.pVSCB->update(&data.vscBuff);
}

// If illumination is enabled it sets the specified light to the specified parameters.
// Eight lights are allowed. And the intensities are directional and diffused.

void Surface::updateLight(unsigned id, Vector2f intensities, Color color, Vector3f position)
{
	USER_CHECK(isInit,
		"Trying to update a light on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to update a light on a Surface with illumination disabled."
	);

	USER_CHECK(id < 8,
		"Trying to update a light with an invalid id (must be 0-7)."
	);

	data.pscBuff.lightsource[id] = { intensities.getVector4(), color.getColor4(), position.getVector4() };
	data.pPSCB->update(&data.pscBuff);
}

// If illumination is enabled clears all lights for the Surface.

void Surface::clearLights()
{
	USER_CHECK(isInit,
		"Trying to clear the lights on an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to clear the lights on a Surface with illumination disabled."
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

void Surface::getLight(unsigned id, Vector2f* intensities, Color* color, Vector3f* position)
{
	USER_CHECK(isInit,
		"Trying to get a light of an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	USER_CHECK(data.desc.enable_illuminated,
		"Trying to get a light of a Surface with illumination disabled."
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

Quaternion Surface::getRotation() const
{
	USER_CHECK(isInit,
		"Trying to get the rotation of an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	return data.rotation;
}

// Returns the current scene position.

Vector3f Surface::getPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the position of an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	return data.position;
}

// Returns the current distortion matrix.

Matrix Surface::getDistortion() const
{
	USER_CHECK(isInit,
		"Trying to get the distortion matrix of an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	return data.distortion;
}

// Returns the current screen position.

Vector2f Surface::getScreenPosition() const
{
	USER_CHECK(isInit,
		"Trying to get the screen position of an uninitialized Surface."
	);

	SurfaceInternals& data = *(SurfaceInternals*)surfaceData;

	return { data.vscBuff.displacement.x, data.vscBuff.displacement.y };
}
