#include "Bindable/Topology.h"
#include "WinHeader.h"

// Handy helper pointers to the device and context.
#define _device ((ID3D11Device*)device())
#define _context ((ID3D11DeviceContext*)context())

/*
--------------------------------------------------------------------------------------------
 Input Layout Functions
--------------------------------------------------------------------------------------------
*/

// Stores the type in class memory.

Topology::Topology(TOPOLOGY_TYPE type)
	: type(type)
{
}

// Sets the topology type in the global context.

void Topology::Bind()
{
	GRAPHICS_INFO_CHECK(_context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)type));
}
