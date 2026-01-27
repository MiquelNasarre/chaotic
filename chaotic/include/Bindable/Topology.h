#pragma once
#include "Bindable.h"

/* PRIMITIVE TOPOLOGY BINDABLE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This bindable is used to store the primitive topoligy of a given drawable 
and bind it to the global context during draw calls.

The TOPOLOGY_TYPE enumerator is based on the D3D_PRIMITIVE_TOPOLOGY enumerator.
For more information on the different topoligies and when to apply them you 
can check the microsoft learn website:
https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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