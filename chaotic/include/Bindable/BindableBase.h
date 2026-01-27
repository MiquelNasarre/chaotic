#pragma once

/* BINDABLES HELPER HEADER
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains all the bindables currently available in this library.
To be included in any drawable that might use most or all of them.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#include "Bindable/ConstantBuffer.h"	// Constant buffers for pixel or vertex shaders
#include "Bindable/VertexBuffer.h"		// Vertex buffer bindable
#include "Bindable/IndexBuffer.h"		// Index buffer bindable
#include "Bindable/VertexShader.h"		// Vertex Shader bindable
#include "Bindable/PixelShader.h"		// Pixel Shader bindable
#include "Bindable/InputLayout.h"		// Input layout bindable
#include "Bindable/Texture.h"			// Texture bindable for pixel shaders
#include "Bindable/Topology.h"			// Topology bindable
#include "Bindable/Sampler.h"			// Sampler bindable
#include "Bindable/Rasterizer.h"		// Rasterizer bindable
#include "Bindable/Blender.h"			// Blender bindable
#include "Bindable/DepthStencil.h"		// Depth Stencil bindable