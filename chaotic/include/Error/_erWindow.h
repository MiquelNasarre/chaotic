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
#define WINDOW_LAST_ERROR()		CHAOTIC_FATAL(WindowError( __LINE__,__FILE__,GetLastError()))
// Expression check macro for window errors.
#define WINDOW_CHECK(_EXPR)		CHAOTIC_CHECK(_EXPR, WindowError( __LINE__,__FILE__,GetLastError()))

// Win32 Error class, when an API function call returns something unexpected
// the HRESULT from that function is retrieved and used to translate the error code.
// This error code and string can be accessed via the what function.
class WindowError : public ChaoticError
{
public:
	// Constructor initializes the public Error and stores the HRESULT.
	WindowError(int line, const char* file, DWORD dw) noexcept
		: ChaoticError(line, file)
	{
		// Retrieves the error string from a given Win32 failed HRESULT 
		// using the FormatMessage method and returns the string.
		char* pMsgBuf = nullptr;
		// windows will allocate memory for err string and make our pointer point to it
		DWORD nMsgLen = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
		);

		// Store information gathered.
		snprintf(info, 2048,
			"\n[Error String]\n0x%8X\n"
			"\n[Description]\n%s"
			"%s"
			, (unsigned)dw, 
			nMsgLen ? pMsgBuf : "Unidentified error code", 
			origin
		);

		if (pMsgBuf)
			LocalFree(pMsgBuf);
	}

	// Win32 Error type override.
	const char* GetType() const noexcept override { return "Win32 Error"; }
};

