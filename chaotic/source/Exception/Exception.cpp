#include "Exception/Exception.h"
#include <stdio.h>

// Default constructor, Only accessable to inheritance.
// Stores the code line and file where the exception was thrown.

Exception::Exception(int line, const char* _file) noexcept
	: line(line)
{
	snprintf(file, 512, "%s", _file);
	snprintf(origin, 512, "[file] %s\n[Line] %i", _file, line);
}

// Creates a default message box using Win32 with the exception data.
#include "WinHeader.h"

void Exception::PopMessageBox() const noexcept
{
	MessageBoxA(nullptr, what(), GetType(), MB_OK | MB_ICONEXCLAMATION);
}