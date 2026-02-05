#pragma once

/* ERROR BASE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the base class for all error types defined across the library.
It also contains the main macro where all fatal checks are funneled through, to be
modified by the user if so desires.

Failed check in this library are to be considered always fatal, when a check is 
failed, a ChaoticError object is created, and is always funneled through 
the CHAOTIC_FATAL macro, which by default calls PopMessageBoxAbort.

We distinguish errors between two types, user errors, which are given from improper
use of the API, this will appear as "Info Errors" upon crash. And system errors, 
which are failed checks on Win32 or DX11 functions, all of them do diagnostics if 
available and print it to the message box.

Checks are performed in both debug and release mode, due to their almost negligible
overhead and their really valuable diagnostics. However this can be disabled by the 
user, all expressions that make it into CHAOTIC_CHECK are intended to be skipped, 
they do not modify any state, so CHAOTIC_CHECK and CHAOTIC_FATAL can be nullyfied 
with no additional modifications needed.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// General macro where all failed checks are funneled through
#ifdef _DEBUG
#define CHAOTIC_FATAL(_ERROR)  (_ERROR).PopMessageBoxAbort()
#else
#define CHAOTIC_FATAL(_ERROR)  (_ERROR).PopMessageBoxAbort()
#endif

// Macro used for regular checks with no state modifination expressions.
#ifdef _DEBUG
#define CHAOTIC_CHECK(_EXPR, _ERROR)  do { if (!(_EXPR)) { CHAOTIC_FATAL(_ERROR); } } while(0)
#else
#define CHAOTIC_CHECK(_EXPR, _ERROR)  do { if (!(_EXPR)) { CHAOTIC_FATAL(_ERROR); } } while(0)
#endif

// Error Base class layout, all error classes must follow it, when 
// an error is found, PopMessageBoxAbort() is called.
class ChaoticError
{
protected:
	// Default constructor, Only accessable to inheritance.
	// Stores the code line and file where the error was found.
	ChaoticError(int line, const char* file) noexcept;

public:
	// Returns the error type, to be overwritten by inheritance.
	virtual const char* GetType() const noexcept = 0;

	// Getters for custom error behavior.
	int			GetLine()   const noexcept { return line;   }
	const char* GetFile()   const noexcept { return file;   }
	const char* GetOrigin() const noexcept { return origin; }
	const char* GetInfo()   const noexcept { return info;   }

	// Creates a default message box using Win32 with the error data.
	[[noreturn]] void PopMessageBoxAbort() const noexcept;
protected:
	int line;				// Stores the line where the error was found.
	char file[512] = {};	// Stores the file where the error was found.

	char origin[512] = {};	// Stores the origin string.

protected:
	// Pointer to the what buffer to be used by inheritance.
	char info[2048] = {};
};


