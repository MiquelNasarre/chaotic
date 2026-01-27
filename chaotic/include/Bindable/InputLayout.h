#pragma once
#include "Bindable.h"
#include "Bindable/VertexShader.h"

/* INPUT LAYOUT BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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