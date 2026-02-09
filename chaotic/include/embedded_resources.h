#pragma once

/* EMBEDDED RESOURCES HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This internal header file is used to embed the shaders and default icon into the 
library file. Files are loaded as bytecode blobs and accessed via these variables.

These are not to be used by any other reason other than to create a compact library 
file that contains all the necessary resources inside.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// If enabled, the function to genenrate the embeddings is also built in. Enable just 
// to generate embeddings, then disable again to avoid unnecessary compiled functions.
//#define _INCLUDE_EMBEDDED_GENERATION

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
unsigned long long getBlobSizeFromId(BLOB_ID id) noexcept;

#ifdef _INCLUDE_EMBEDDED_GENERATION
// Function used to generate the embedded resources.
void generateEmbedded(const char* out_filename);
#endif