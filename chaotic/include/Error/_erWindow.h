#pragma once
#include "Error/ChaoticError.h"

#include <cstdio>

/* WIN32 ERROR CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the windows error class, to be used when calling Win32
API functions that can error but are not part of the DXGI API functions.

The class takes an HRESULT as an input and decodes it using the Win32 FormatMessage
functions. By the macros default it retrieves the last error by calling GetLastError().
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Macro to retrieve the last error code amd create a Window Error with it.
#if defined _DEPLOYMENT && defined SOLUTION_DIR
#define WINDOW_LAST_ERROR()		CHAOTIC_FATAL(WindowError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,GetLastError()))
#else
#define WINDOW_LAST_ERROR()		CHAOTIC_FATAL(WindowError( __LINE__,__FILE__,GetLastError()))
#endif
// Expression check macro for window errors.
#if defined _DEPLOYMENT && defined SOLUTION_DIR
#define WINDOW_CHECK(_EXPR)		CHAOTIC_CHECK(_EXPR, WindowError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,GetLastError()))
#else
#define WINDOW_CHECK(_EXPR)		CHAOTIC_CHECK(_EXPR, WindowError( __LINE__,__FILE__,GetLastError()))
#endif

// Win32 Error class, when an API function call returns something unexpected
// the HRESULT from that function is retrieved and used to translate the error code.
// This error code and string can be accessed via the what function.
class WindowError : public ChaoticError
{
public:
	// Constructor initializes the public Error and stores the HRESULT.
	WindowError(int line, const char* file, DWORD dw) noexcept;

	// Win32 Error type override.
	const char* GetType() const noexcept override { return "Win32 Error"; }
};

