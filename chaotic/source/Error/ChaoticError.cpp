#include "Error/ChaoticError.h"
#include <stdio.h>
#include <stdlib.h>

// Default constructor, Only accessable to inheritance.
// Stores the code line and file where the error was found.

ChaoticError::ChaoticError(int line, const char* _file) noexcept
	: line(line)
{
	snprintf(file, 512, "%s", _file);
	snprintf(origin, 512, "\n\n[file] %s\n\n[Line] %i", _file, line);
}

// Creates a default message box using Win32 with the error data.
#include "WinHeader.h"

void ChaoticError::PopMessageBoxAbort() const noexcept
{
	MessageBoxA(nullptr, info, GetType(), MB_OK | MB_ICONEXCLAMATION);
	abort();
}