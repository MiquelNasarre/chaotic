#pragma once

/* GLOBAL LIBRARY HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the basic math and image handling classes to be adden in any 
file contained or using the library. This being the complete math library and image
library. Check the headers for more information on how to operate with them.

Since imGui is implemented throughout the library, but you may not need if for your
projects, you can also find a toggle to enable/disable imGui globally in the API.
If you decide to use imGui make sure you link it properly (should be by default).

It also includes a couple of helpers to include this library and the imGui library. 
If you are not using VisualStudio or linkage is different in your case you will have 
to deal with library linkage yourself.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Toggle to enable and disable all ImGui code inside the library.
// If you want to use ImGui make sure the library has access to the 
// ImGui headers and this definition is uncommented during build.
#define _INCLUDE_IMGUI

// Toggle that uses the raw bytecode shaders constructors with the embedded 
// resources instead of the files, so that the library is self sufficient.
#define _DEPLOYMENT

// Default API includes

#include "Math/Vectors.h"
#include "Math/Matrix.h"
#include "Math/Quaternion.h"
#include "Math/constants.h"
#include "Image/Image.h"
