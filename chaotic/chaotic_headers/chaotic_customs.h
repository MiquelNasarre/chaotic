#pragma once

/* LICENSE AND COPYRIGHT
-----------------------------------------------------------------------------------------------------------
 * Chaotic — a 3D renderer for mathematical applications
 * Copyright (c) 2025-2026 Miguel Nasarre Budiño
 * Licensed under the MIT License. See LICENSE file.
-----------------------------------------------------------------------------------------------------------
*/

/* CHAOTIC-CUSTOMS HEADER FILE
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Welcome to the Chaotic Customs header!! This header contains the tools you need to create 
your own Drawables for this library and its shaders. Sometimes we need something different 
or slightly faster than what this library provides, for this reason this header will give 
you access to all the tools you need to create your own Drawable and write GPU code, while 
still not depending on DirectX11 or Win32 API dependencies.

For an example implementation of the code you can check any of the library default drawables
source files. 

This header contains the intermediate lavel API classes, with no external dependencies.
The classes included are the following:

 chaotic.h and its classes.		— Main header including Bindable class.

 All the bindable classes:
 * Blender						— Decides how the object will blend with the target.
 * ConstantBuffer				— Constant GPU buffers to be accessed by the shaders.
 * DepthStencil					— Decides how the depth operation used for blending.
 * IndexBuffer					— Contains all the ordered vertex indices to create shapes.
 * PixelShader					— Stores the pixel shader bytecode used for drawing.
 * Rasterizer					— Decides which pixels to draw from a given VS output.
 * Sampler						— Decides how the colors will be sampled from a texture.
 * Texture						— Class to store GPU textures given by Image objects.
 * Topology						— Decides how the vertexs will be aranged.
 * VertexBuffer					— Stores the information of each vertex to be used by VS.
 * VertexShader					— Stores the vertex shader bytecode used for drawing.
 * InputLayout					— Specifies the information provided for each vertex.

 Exception default classes.		— Exception thrown during drawable errors.

 Embedded Resources				— Functions to retrieve default shader bytecodes from library (optional).

The classes are copied as they are in their original header files. For further reading
each class has its own description and all functions are explained. I suggest reading the
descriptions of all functions before using them.

The shaders are built as HLSL files and compiled to CSO. Pixel and Vertex Shader support
CSO file paths, they will read them to memory and store them as blobs. The default shaders
are already stored as bytecode inside LIB for convenience, and are accessible via the 
embedded resources functions. Check default drawables to see implementation.

If you decide to expand the functionalities of the Window class or the Graphics class
or create your own bindables, you need to include "chaotic_internals.h", contains all
the DirectX11 and Win32 dependencies used to create the internals of the library.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Toggle to enable and disable embedded resources inside the header.
#define _INCLUDE_EMBEDDED

// Imports chaotic.h classes with Bindable class
#define _CHAOTIC_CUSTOMS
#include "chaotic.h"
#undef _CHAOTIC_CUSTOMS


/* BLENDER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
The blend state is the last step when drawing to a render target, and it decides how 
the output color of the pixel is combined with the existent color in the buffer
according to the colors distances and alpha value.

This bindable supports four different modes:

- By default it is mode opaque, where all pixels that pass the depth test will override 
  the previous color and the alpha channel will be ignored.

- Blend mode additive is for a glow like effect, if it passes the depth test it will 
  simply add its color on top of the current pixel, ignoring the alpha channel. This
  combined with a no-write depth stencil can be used for lightsources for example.

- Blend mode alpha is simple additive transparency, if it passed the depth test it will
  combine C_src * A_src + C_dst * (1 - A_src). This is great for simple scenery building
  with a couple transparent objects but it fails when dealing with arbitrarily ordered 
  multiple transparent objects.

- Blend mode Order Independent Transparency. Since this library is meant to be used for
  math rendering intersecting surfaces and functions are very common, therefore it is 
  important to have a reliable way of making them transparent without enforcing ordering.
  This is where this mode comes in handy. 
  
  If the mode is weighted OIT the Blender is just a holder that will tell the graphics object 
  that a transparent object is coming, and the Graphics object will deal with the rest. If 
  your object is transparent it requires a pixel shader that will write in two render targets. 
  The first is a color accumulator, expects a premultiplied (C_src * A_src, A_src). The alpha 
  can be z_value dependent for better depth distinction. The second one is for the final 
  resolve and expects a single float A_src. Transparent objects must always be drawn after 
  opaque objects.

  For more information on how OITransparency works you can check the source code or some
  of the shaders of the default drawables that implement it. The original paper or weighted
  OIT can be found at: https://jcgt.org/published/0002/02/09/

For more information on how blend states work in general in DirectX11 you can check
the microsoft learn resources at their website:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_blend_desc
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Enumerator that defines the blending mode used by the blender.
enum BLEND_MODE
{
	BLEND_MODE_OPAQUE,			// Opaque surfaces
	BLEND_MODE_ALPHA,			// Standard alpha blending
	BLEND_MODE_ADDITIVE,		// Additive blending
	BLEND_MODE_OIT_WEIGHTED,	// Order Independent Transparency
};

// Blender bindable, decides how the colors are blended in the screen by the current drawable.
class Blender : public Bindable
{
	// Needs to acces the blending mode.
	friend class Drawable;
public:
	// Takes the blending mode as an input and creates the blending state accordingly.
	Blender(BLEND_MODE mode);

	// Releases the GPU pointer and deletes the data.
	~Blender() override;

	// Binds the Blender to the global context.
	void Bind() override;

private:
	// To be called by the draw call to check for OIT.
	BLEND_MODE getMode();

	// Pointer to the internal Blender data.
	void* BindableData = nullptr;
};


/* CONSTANT BUFFER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Our drawables might sometimes have general information that we would like all the shader calls to
have access to, like rotations, global color, lights, etc. For this reason the constant buffer
exists, it stores information available and constant to all shader during a draw call that can
be modified by the CPU across different calls.

To create a Constant Buffer you need to define a structure with the information you would like
the PS or VS to have access to and send the pointer to the constructor. The constructor only
expects 1 object, if you want to structure it different you can also use the raw constructor.

DirectX11 enforces 16 Byte alignment, so if the size is not divisible by 16 the constructor will
throw an exception, to avoid that I strongly recommend using alignas(16) in your struct definition.

In the specific case of this library given its 3D nature the Graphics object associated to a Window
reserves the right to the first VS Constant Buffer slot to store the current scene perspective.

For more information on how to structure a Constant Buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Enumerator to determine whether the constant buffer will 
// be accessed by the Pixel or the Vertex Shader.
enum CONSTANT_BUFFER_TYPE
{
	VERTEX_CONSTANT_BUFFER,
	PIXEL_CONSTANT_BUFFER
};

// Due to the first VS slot being reserved by the graphics, the default 
// constant buffer slot will be 1 for the VS and 0 for the PS.
#define CONSTANT_BUFFER_DEFAULT_SLOT -1

// Constant buffer bindable, takes a pointer to the structured data and manages the
// Constant Buffer inside the GPU, binding it during draw calls of its drawable.
class ConstantBuffer : public Bindable
{
public:
	// Expects a valid pointer to a single structure object and a type specifier, sends the
	// data to the raw constructor that creates the GPU object and stores the pointer.
	template<typename C>
	ConstantBuffer(const C* consts, CONSTANT_BUFFER_TYPE type, const int slot = CONSTANT_BUFFER_DEFAULT_SLOT)
		: ConstantBuffer((void*)consts, sizeof(C), type, slot) {
	}

	// Raw constructor, expects a valid pointer to the data with the specified byte size, 
	// creates the GPU Constant Buffer and stores the pointer for binding and update calls.
	ConstantBuffer(const void* data, unsigned size, CONSTANT_BUFFER_TYPE type, const int slot);

	// Releases the GPU pointer and deletes the data.
	~ConstantBuffer() override;

	// Expects a valid pointer to a single structure object of the same size as the one used 
	// in the constructor, feeds it to the raw update which updates the data on the GPU.
	template<typename C>
	void update(const C* consts)
	{
		update((void*)consts, sizeof(C));
	}

	// Raw update function, expects a valid pointer and the byte size to match the one used in 
	// the constructor, otherwise will throw. Updates the data in the GPU with the new data.
	void update(const void* data, unsigned size);

	// Binds the Constant Buffer to the global context according to its type.
	void Bind() override;

private:
	// Pointer to the internal Constant Buffer data.
	void* BindableData = nullptr;
};


/* DEPTH STENCIL BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
The depth stencil state is an important part of the last step when drawing to a 
render target, and it decides how the drawn object depth will interact with the 
bound depth buffer.

By default the depth stencil overrides the depth if the object is closer, but for
some cases like glow effects, light sources, etc, we might want different settings.

By default the depth stencil is alway set to override the objects depth if closer
in this library. But if one of your drawables requires a different interaction
you can use this bindable to set it.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Enumerator that defines the depth stencil mode.
enum DEPTH_STENCIL_MODE
{
	DEPTH_STENCIL_MODE_DEFAULT,			// Only writes if the object is closer.
	DEPTH_STENCIL_MODE_NOWRITE,			// Never writes to the depth buffer but still tests.
	DEPTH_STENCIL_MODE_NOWRITE_NOTEST,	// Never writes to the depth buffer, always draws.
	DEPTH_STENCIL_MODE_OVERRIDE,		// Always writes to the depth buffer and draws.
};

// Depth Stencil bindable, decides the interaction between the object drawn and the depth buffer.
class DepthStencil : public Bindable
{
public:
	// Takes the depth stencil mode mode as an input and creates the depth stencil state accordingly.
	DepthStencil(DEPTH_STENCIL_MODE mode);

	// Releases the GPU pointer and deletes the data.
	~DepthStencil() override;

	// Binds the Depth Stencil State to the global context.
	void Bind() override;

private:
	// Pointer to the internal Depth Stencil data.
	void* BindableData = nullptr;
};


/* INDEX BUFFER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
When a vertex buffer is sent to the GPU, it is important for that GPU to know how those
vertices will be organized, for that reason you need to index those vertices into a long 
list so that later the topology knows which vertexs to use and in which order.

The topology of the object drawn (lines, triangles, points) is decided by the Topology 
bindable, you can check it for further reference.

The index buffer tells the GPU which vertices to use and in which order to use them. Every
vertex has its index assigned by its position in the array sent to the Vertex Buffer, and 
the index buffer will point to each one of the one by one to create the desired shape.

For more information on how to structure an index buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Index buffer bindable, takes an array of unsigneds and manages the Index Buffer
// inside the GPU, binding it during draw calls of its drawable.
class IndexBuffer : public Bindable
{
	// Needs access to the index count.
	friend class Drawable;
public:
	// Takes an unsigned array and the count as input, 
	// creates the Index Buffer on GPU and stores the pointer.
	IndexBuffer(const unsigned* indices, unsigned count);

	// Releases the GPU pointer and deletes the data.
	~IndexBuffer() override;

	// Binds the Index Buffer to the global context.
	void Bind() override;

private:
	// To be accessed during the draw call to retriece the index count.
	unsigned getCount() const;

	// Pointer to the internal Index Buffer data.
	void* BindableData = nullptr;
};


/* PIXEL SHADER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This bindable is used to create the bytecode for a (*.cso) pixel shader file and bind it to
the current context. To create such pixel shader you can write an HLSL file and compile it
to a CSO. For examples on how to write an HLSL file you can look at the shaders used by the
drawables in this library.

Your pixel shader will be called on every pixel of your drawable you want to draw in the screen
and will evaluate the color corresponding to that pixel given the input it receives from the
vertex shader.

For further information on how to create shaders using HLSL check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Pixel Shader bindable class, expects a CSO file path created from an HLSL pixel shader.
// It binds the pixel shader to the GPU during draw calls when added to a drawable.
class PixelShader : public Bindable
{
public:
	// Given a (*.cso) file path it creates the bytecode and the pixel shader object.
	PixelShader(const wchar_t* path);

	// Raw constructor for deployment, uses embedded resources. Expects valid blobs.
	PixelShader(const void* bytecode, unsigned long long size);

	// Releases the pointers and deletes the internal data.
	~PixelShader() override;

	// Binds the Pixel Shader to the global context.
	void Bind() override;

private:
	// Pointer to the internal Pixel Shader data.
	void* BindableData = nullptr;
};


/* RASTERIZER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This bindable controls the middle step between the vertex shader and the pixel shader
in the pipeline. Decides which pixels will be drawn from the output of the vertex shader.

The default setting is set to draw both sides of triangles and draw the full triangle. 
This is due to the library being invisioned as a math rendering library. You can disable
double sided view and also enable wireframe drawings via the constructor.

For more information on what the rasterizer is and its role on the pipeline you can check
the microsoft learn sources at the following link:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Rasterizer bindable, tells the pipeline which triangles to draw and how to draw those 
// triangles. From the constructor you can control if both sides are drawn, if you are 
// drawing a wire frame and which side is considered front of a triangle.
class Rasterizer : public Bindable
{
public:
	// Creates the Rasterizer object and GPU pointer as specified in the arguments.
	Rasterizer(bool doubleSided = true, bool wireFrame = false, bool frontCounterClockwise = true);

	// Releases the GPU pointer and deletes the data.
	~Rasterizer() override;

	// Binds the Rasterizer to the global context.
	void Bind() override;

private:
	// Pointer to the internal Rasterizer data.
	void* BindableData = nullptr;
};


/* SAMPLER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
The sampler state controls the function inside pixel shaders of Texture::Sample().
It determines how to assign a color to a given texture coordinate from the texture.

Two main variables are controlled in this class:

- The filter, that tells you how the point maps to the texture, it can be mapping to
  the nearest pixel, getting therefore a pixelated view in low resolution. Linearly
  combining the nearest pixels getting a blurred effect, or non-linearly combining them
  with the anisotropic filter, giving better texture quality in some cases.

- The address mode, controls what happens when the texture coordinate gets out of bounds,
  since the textures are defined between 0 and 1 in all directions.

For more information on how to choose a sampler for texture rendering you can check
the microsoft learn sources at the following link:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_sampler_desc
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Sample filter enumerator mimquing D3D11_FILTER. Decides how the texture 
// color will be interpolated from the neighboring pixels in the shader.
enum SAMPLE_FILTER
{
	SAMPLE_FILTER_POINT = 0,            // Nearest pixel
	SAMPLE_FILTER_LINEAR = 0x15,        // interpolate pixels
	SAMPLE_FILTER_ANISOTROPIC = 0x55,   // Non linear for improved perspective (default x8)
};

// Sample address mode filter enumerator mimiquing D3D11_TEXTURE_ADDRESS_MODE.
// Decides how the texture coordinate will be assigned when it gets out of bounds.
enum SAMPLE_ADDRESS_MODE
{
	SAMPLE_ADDRESS_WRAP = 1,            // Repeating pattern
	SAMPLE_ADDRESS_MIRROR = 2,          // Repeating pattern mirrored
	SAMPLE_ADDRESS_CLAMP = 3,           // Clamped to edge of texture
	SAMPLE_ADDRESS_BORDER = 4,          // Border color assigned
	SAMPLE_ADDRESS_MIRROR_ONCE = 5      // Mirror negative axis then clamp
};

// Sampler bindable, decides how given texture coordinates inside pixel shaders will be 
// mapped to a color. From the constructor the filter and the address mode are specified.
class Sampler : public Bindable
{
public:
	// Creates the Sampler object and GPU pointer as specified in the arguments.
	Sampler(SAMPLE_FILTER filter, SAMPLE_ADDRESS_MODE address_mode = SAMPLE_ADDRESS_WRAP, unsigned slot = 0u);

	// Releases the GPU pointer and deletes the data.
	~Sampler() override;

	// Binds the Sampler to the global context.
	void Bind() override;

	// Sets the slot at which the Sampler will be bound.
	void setSlot(unsigned slot);

private:
	// Pointer to the internal Sampler data.
	void* BindableData = nullptr;
};


/* TEXTURE BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
A texture from a graphics engine point of view it is simply a grid of texels sent to
the GPU that the Pixel Shader can sample from and use it to output its pixel color.

For the Pixel Shader to sample from a texture a sampler state needs to be defined, refer
to the Sampler bindable header for more information on how to set a sampler state.

To be able to send your Images to the GPU this class handles the creation of textures and 
their GPU binding. To create a texture the Image class is to be used. Internally contains
a pointer to an array of colors and the image dimensions. It supports transparencies and 
image loading and saving from memory.

This bindable also supports cube-maps for background creation. The image uploaded must contain
the six faces of the cube stacked on top of each other in the order [+X,-X,+Y,-Y,+Z,-Z].
And the orientation must correspond to what a camera at the origin would see when looking 
along that axis direction, with +Y as “up” in world.

The Image header contains a small library of functions called ToCube, for simple conversion
from common spherical projections to cube-maps. I strongly recommend checking them out if you
have other type of spherical projected images.

For more information on how to use the Image class I strongly suggest checking its header.
For more information on how textures are used in Pixel Shaders and DX11 in general you can 
check the microsoft learn documentation at:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Usage specifier, if the Drawable intends to update the Texture, this can be done from 
// the Texture class itself without needing to replace it, set to dynamic if intended.
enum TEXTURE_USAGE
{
	TEXTURE_USAGE_DEFAULT,
	TEXTURE_USAGE_DYNAMIC,
};

// For background images that support fields of view cubemaps are a very common choice to
// minimize distortion while making the math simple on the GPU side. If you want to generate 
// a cube-map the uploaded image dimensions must be (width, 6 * width).
enum TEXTURE_TYPE
{
	TEXTURE_TYPE_IMAGE,
	TEXTURE_TYPE_CUBEMAP,
};

// Texture bindable class, from an Image it creates a texture and sends it to the GPU to 
// be read by the Pixel Shader at the specified slot.
class Texture : public Bindable
{
public:
	// Expects a valid image pointer and creates the texture in the GPU.
	Texture(const Image* image, TEXTURE_USAGE usage = TEXTURE_USAGE_DEFAULT, TEXTURE_TYPE type = TEXTURE_TYPE_IMAGE, unsigned slot = 0u);

	// Releases the GPU pointer and deletes the data.
	~Texture() override;

	// Binds the Texture to the global context at the specified slot.
	void Bind() override;

	// Sets the slot at which the Texture will be bound.
	void setSlot(unsigned slot);

	// If usage is dynamic updates the texture with the new image.
	// Dimensions must match the initial image dimensions.
	void update(const Image* image);

private:
	// Pointer to the internal Texture data.
	void* BindableData = nullptr;
};


/* PRIMITIVE TOPOLOGY BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This bindable is used to store the primitive topoligy of a given drawable 
and bind it to the global context during draw calls.

The TOPOLOGY_TYPE enumerator is based on the D3D_PRIMITIVE_TOPOLOGY enumerator.
For more information on the different topoligies and when to apply them you 
can check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Topology type enumerator mimiquing D3D_PRIMITIVE_TOPOLOGY.
enum TOPOLOGY_TYPE: unsigned
{
	POINT_LIST = 1,			// All vertex are treated as single points
	LINE_LIST = 2,			// Vertexs are joined by pairs to form lines (no repetition)
	LINE_STRIP = 3,			// Adjacent vertexs are joined to form lines (repetition)
	TRIANGLE_LIST = 4,		// Vertexs are joined by trios to form triangles (no repetition)
	TRIANGLE_STRIP = 5,		// Adjacent vertexs are joined to form triangles (repetition)
};

// Primitive topology bindable, tells the pipeline how to interpret the index/vertex buffer.
// Decides whether you are painting points, or connected/disconnected lines or triangles.
class Topology : public Bindable
{
public:
	// Stores the type in class memory.
	Topology(TOPOLOGY_TYPE type);

	// Sets the topology type in the global context.
	void Bind() override;
private:
	// Topology type storage
	const TOPOLOGY_TYPE type;
};


/* VERTEX BUFFER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
The first information accessed by the GPU on the graphics pipeline is the Vertex Buffer, this
data will feed into every Vertex Shader call for every vertex as specified by the Input Layout.

To create a Vertex Buffer you need to define a structure with all the information you would
like every vertex to have, for example 3D position, normal vector, color, etc. Then create an
array of that struct and send the pointer to the constructor specifying the amount of vertices.

For more information on possible padding issues in your structure and how to access the struct
information inside the vertex shader I recommend checking the Input Layout header.

For more information on how to structure a Vertex Buffer creation you can check the default
drawables used in this library. For more information on DX11 buffers and how they work you
can check the microsoft learn resources at the website:
https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-intro
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Usage specifier, if the Drawable intends to update the vertices, this can be done from 
// the Vertex Buffer itself without needing to replace it, set to dynamic if intended.
enum VERTEX_BUFFER_USAGE
{
	VB_USAGE_DEFAULT,
	VB_USAGE_DYNAMIC,
};

// Vertex buffer bindable, takes an array of custom Vertices and manages the
// Vertex Buffer inside the GPU, binding it during draw calls of its drawable.
class VertexBuffer : public Bindable
{
public:
	// Templated Vertex Buffer constructor, expects an array of custom Vertices and 
	// calls the raw constructor to create the GPU object and store the pointer.
	template<typename V>
	VertexBuffer(const V* vertices, unsigned count, VERTEX_BUFFER_USAGE usage = VB_USAGE_DEFAULT)
		: VertexBuffer((const void*)vertices, sizeof(V), count, usage) {
	}

	// Raw constructor, expects an ordered array of Vertices with the specified stride 
	// per vertex, creates the GPU Vertex Buffer and stores the pointer for binding calls.
	VertexBuffer(const void* vertices, unsigned stride, unsigned count, VERTEX_BUFFER_USAGE usage = VB_USAGE_DEFAULT);

	// Releases the GPU pointer and deletes the data.
	~VertexBuffer() override;

	// If the Vertex Buffer has dynamic usage it updates the data with the new Vertices
	// information. If byteWidth is bigger or usage is not dynamic it will cause assertion.
	template<typename V>
	void updateVertices(const V* vertices, unsigned count)
	{
		updateVertices((const void*)vertices, sizeof(V), count);
	}

	// If the Vertex Buffer has dynamic usage it updates the data with the new Vertices
	// information. If byteWidth is bigger or usage is not dynamic it will cause assertion.
	void updateVertices(const void* vertices, unsigned stride, unsigned count);

	// Binds the Vertex Buffer to the global context.
	void Bind() override;

private:
	// Pointer to the internal Vertex Buffer data.
	void* BindableData = nullptr;
};


/* VERTEX SHADER BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This bindable is used to create the bytecode for a (*.cso) vertex shader file and bind it to
the current context. To create such vertex shader you can write an HLSL file and compile it
to a CSO. For examples on how to write an HLSL file you can look at the shaders used by the
drawables in this library.

This class is connected to the input layout because it need to be able to match the data sent
from the CPU to the data expected by the GPU, and that is doe via an input layout. Therefore
you need to store the pointer and later use it to create the input layout.

Check the input layout header for more information on how to properly create both bindables.
For further information on how to create shaders using HLSL check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Vertex Shader bindable class, expects a CSO file path created from an HLSL vertex shader.
// It binds the vertex shader to the GPU during draw calls when added to a drawable.
class VertexShader : public Bindable
{
	// Needs access to the bytecode.
	friend class InputLayout;
public:
	// Given a (*.cso) file path it creates the bytecode and the vertex shader object.
	VertexShader(const wchar_t* path);

	// Raw constructor for deployment, uses embedded resources. Expects valid blobs.
	VertexShader(const void* bytecode, unsigned long long size);

	// Releases the pointers and deletes the internal data.
	~VertexShader();

	// Binds the Vertex Shader to the global context.
	void Bind() override;

private:
	// To be used by InputLayout. Returns the shader ID3DBlob* masked as a void*.
	void* GetBytecode() const noexcept;

	// Pointer to the internal Vertex Shader data.
	void* BindableData = nullptr;
};


/* INPUT LAYOUT BINDABLE CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This bindable is used to match the type of data collected by the Vertex Shaders
in the GPU and the data that is actually sent by the CPU from the Vertex Buffer.

For that reason the basic elements it needs is the format of the data sent and the
name associated to it that will be picked up by the vertex shader.

For proper abstraction from the D3D11 API an element descriptor has been created 
that mimics a simplified version of the D3D11_INPUT_ELEMENT_DESC, amd the most common
formats of that descriptor have been added to the DATA_FORMAT enum albeit in a simpler
name. For more data formatting options you can check the header 'dxgiformat.h'.

A typical use case scenareo would be, inside a drawable constructor, 
after adding a vertex shader:
'''
INPUT_ELEMENT_DESC ied[2] = 
{
    { "Position", _4_FLOAT },
    { "Normal",   _3_FLOAT },
}
AddBind(new InputLayout(ied, 2, pVertexShader))
'''

And then the Vertex Shader code would expect to see this kind of main declaration:
'''
VSOut main(float4 pos : Position, float3 norm : Normal) { ... }
'''

Depending on data alignment some settings might fail, the default constructor assumes no 
padding between the data, if your structure gets padded by the compiler you have to take
that into account when writing the input layout. For more information you can check:
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_input_element_desc

If you wish to send a specific D3D11 layout you can do so by masking it as a void* and
using the alternative Input Layout constructor, the source code will interpret it as a 
D3D11_INPUT_ELEMENT_DESC* and create the ID3D11InputLayout object with it.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Enumerator mimiquing the DXGI_FORMAT enum. Each format specified expects
// to find the same format inside the vertex shader bytecode.
enum DATA_FORMAT : unsigned
{
    _4_FLOAT = 2,
    _4_UINT = 3,
    _4_SINT = 4,

    _3_FLOAT = 6,
    _3_UINT = 7,
    _3_SINT = 8,

    _4_SHORT_FLOAT = 10,
    _4_SHORT_UINT = 12,
    _4_SHORT_SINT = 14,

    _2_FLOAT = 16,
    _2_UINT = 17,
    _2_SINT = 18,

    _4_UCHAR = 30,
    _4_CHAR = 32,
    _4_BGRA_COLOR = 87,

    _2_SHORT_FLOAT = 34,
    _2_SHORT_UINT = 36,
    _2_SHORT_SINT = 38,

    _1_FLOAT = 41,
    _1_UINT = 42,
    _1_SINT = 43,
};

// Simplified input element descriptor, contains a name and a format.
// These are expected to match inside the Vertex Shader code.
struct INPUT_ELEMENT_DESC
{
	const char* name;
    DATA_FORMAT fmt;
};

// Input layout bindable class, its function is to communicate between the 
// CPU and the GPU how the data will be sent to the vertex shader, including 
// a name and a format for each variable.
class InputLayout : public Bindable
{
public:
    // Constructor, expects a valid pointer to a list of 'size' element descriptors
    // and a valid pointer to a VertexShader to extract the bytecode.
    InputLayout(const INPUT_ELEMENT_DESC* layout, unsigned size, const VertexShader* pVS);

    // Alternative constructor, expects a valid D3D11_INPUT_ELEMENT_DESC* masked as a void*
    // of 'size' elements and a valid pointer to a VertexShader to extract the bytecode.
	InputLayout(const void* d3d11_layout, unsigned size, const VertexShader* pVS);

    // Releases the GPU pointer and deletes the data.
	~InputLayout() override;

    // Binds the Input Layout to the global context.
	void Bind() override;

private:
    // Pointer to the internal Input Layout data.
	void* BindableData = nullptr;
};


/* DEFAULT EXCEPTION CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the default exception class thrown by the
library when no specific exception is being thrown.

Contains the line and file and a description of the excetion that
can be entered as a single string or as a list of strings.
For user created exceptions please use this one.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

#define INFO_EXCEPT(info)	InfoException(__LINE__, __FILE__, (info))

// Basic Exception class, stores the given information and adds it
// to the whatBuffer when the what() function is called.
class InfoException : public Exception
{
public:
	// Single message constructor, the message is stored in the info.
	InfoException(int line, const char* file, const char* msg) noexcept
		: Exception(line, file)
	{
		unsigned c = 0u;

		// Add intro to information.
		const char* intro = "\n[Error Info]\n";
		unsigned i = 0u;
		while (intro[i])
			info[c++] = intro[i++];

		// join all info messages with newlines into single string.
		i = 0u;
		while (msg[i] && c < 2047)
			info[c++] = msg[i++];

		if (c < 2047)
			info[c++] = '\n';

		// Add origin location.
		const char* origin = GetOriginString();
		i = 0u;
		while (origin[i] && c < 2047)
			info[c++] = origin[i++];

		// Add final EOS
		info[c] = '\0';
	}

	// Multiple messages constructor, the messages are stored in the info.
	InfoException(int line, const char* file, const char** infoMsgs = nullptr) noexcept
		:Exception(line, file)
	{
		unsigned i, j, c = 0u;

		// Add intro to information.
		const char* intro = "\n[Error Info]\n";
		i = 0u;
		while (intro[i] && c < 2047)
			info[c++] = intro[i++];

		// join all info messages with newlines into single string.
		i = 0u;
		while (infoMsgs[i])
		{
			j = 0u;
			while (infoMsgs[i][j] && c < 2047)
				info[c++] = infoMsgs[i][j++];

			if (c < 2047)
				info[c++] = '\n';
		}

		// Add origin location.
		const char* origin = GetOriginString();
		i = 0u;
		while (origin[i] && c < 2047)
			info[c++] = origin[i++];

		// Add final EOS
		info[c] = '\0';
	}

	// Info Exception type override.
	const char* GetType() const noexcept override { return "Graphics Info Exception"; }
};

#ifdef _INCLUDE_EMBEDDED

/* EMBEDDED RESOURCES HEADER FILE
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This internal header file is used to embed the shaders and default icon into the
library file. Files are loaded as bytecode blobs and accessed via these variables.

These are not to be used by any other reason other than to create a compact library
file that contains all the necessary resources inside.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// List of all the different files stored as bytecode blobs.
enum class BLOB_ID
{
	BLOB_DEFAULT_ICON,
	BLOB_BACKGROUND_PS,
	BLOB_BACKGROUND_VS,
	BLOB_COLOR_CURVE_VS,
	BLOB_CUBE_TEXTURE_PS,
	BLOB_CURVE_VS,
	BLOB_DYNAMIC_BG_PS,
	BLOB_DYNAMIC_BG_VS,
	BLOB_GLOBAL_COLOR_PS,
	BLOB_GLOBAL_COLOR_VS,
	BLOB_LIGHT_PS,
	BLOB_LIGHT_VS,
	BLOB_OIT_CUBE_TEXTURE_PS,
	BLOB_OIT_GLOBAL_COLOR_PS,
	BLOB_OIT_RESOLVE_PS,
	BLOB_OIT_RESOLVE_VS,
	BLOB_OIT_UNLIT_CUBE_TEXTURE_PS,
	BLOB_OIT_UNLIT_GLOBAL_COLOR_PS,
	BLOB_OIT_UNLIT_VERTEX_COLOR_PS,
	BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS,
	BLOB_OIT_VERTEX_COLOR_PS,
	BLOB_OIT_VERTEX_TEXTURE_PS,
	BLOB_UNLIT_CUBE_TEXTURE_PS,
	BLOB_UNLIT_GLOBAL_COLOR_PS,
	BLOB_UNLIT_VERTEX_COLOR_PS,
	BLOB_UNLIT_VERTEX_TEXTURE_PS,
	BLOB_VERTEX_COLOR_PS,
	BLOB_VERTEX_COLOR_VS,
	BLOB_VERTEX_TEXTURE_PS,
	BLOB_VERTEX_TEXTURE_VS,
};

// Returns a pointer to the bytecode data of the blobs.
const void* getBlobFromId(BLOB_ID id) noexcept;

// Returns the size in bytes of the blob data.
size_t getBlobSizeFromId(BLOB_ID id) noexcept;

#undef _INCLUDE_EMBEDDED
#endif // _INCLUDE_EMBEDDED
