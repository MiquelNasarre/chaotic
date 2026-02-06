#include "WinHeader.h"
#include <stdio.h>
#include <stdlib.h>

// Default constructor, Only accessable to inheritance.
// Stores the code line and file where the error was found.

ChaoticError::ChaoticError(int line, const char* _file) noexcept
	: line(line)
{
	// Copy paths.
	snprintf(file, 512, "%s", _file);
	snprintf(origin, 512, "\n\n[File] %s\n\n[Line] %i", _file, line);
}

// Creates a default message box using Win32 with the error data.

void ChaoticError::PopMessageBoxAbort() const noexcept
{
	MessageBoxA(nullptr, info, GetType(), MB_OK | MB_ICONEXCLAMATION);
	ExitProcess(EXIT_FAILURE);
}

/*
--------------------------------------------------------------------------------------------
 User Error
--------------------------------------------------------------------------------------------
*/

// Single message constructor, the message is stored in the info.

UserError::UserError(int line, const char* file, const char* msg) noexcept
	: ChaoticError(line, file)
{
	// Print info error formatted string.
	snprintf(info, 2048, "\n[Error Info]\n%s%s", msg, origin);
}

/*
--------------------------------------------------------------------------------------------
 Graphics Error
--------------------------------------------------------------------------------------------
*/

// Single message constructor, the message is stored in the info.

GraphicsError::GraphicsError(int line, const char* file, const char* msg) noexcept
	: ChaoticError(line, file)
{
	// Print info error formatted string.
	snprintf(info, 2048, "\n[Error Info]\n%s%s", msg, origin);
}

// Constructor, takes in a FAILED HRESULT and an optional list of
// messages and stores it in memory for future info printing.

HrError::HrError(int line, const char* file, long hr, const char* infoMsgs) noexcept
	: ChaoticError(line, file)
{
	char description[512] = {};

	// Try Win32 message for HRESULT_FROM_WIN32 codes
	DWORD err = HRESULT_CODE(hr);
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS;

	char* msg = nullptr;

	// First: treat as Win32 error
	if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
	{
		if (FormatMessageA(flags, nullptr, err, 0, (LPSTR)&msg, 0, nullptr) && msg)
		{
			snprintf(description, 512, "%s", msg);
			LocalFree(msg);
			goto description_done;
		}
		msg = nullptr;
	}
	// Second: try directly on the HRESULT (sometimes works)
	if (FormatMessageA(flags, nullptr, (DWORD)hr, 0, (LPSTR)&msg, 0, nullptr) && msg)
	{
		snprintf(description, 512, "%s", msg);
		LocalFree(msg);
		goto description_done;
	}
	// Fallback
	snprintf(description, 512, "Unknown error (0x%8X)\n", (unsigned)hr);

description_done:
	snprintf(info, 2048,
		"\n[Error Code]  0x%08X"
		"\n\n[Description]\n%s"
		"\n[Error Info]\n%s"
		"%s"
		, (unsigned)hr, description, infoMsgs ? infoMsgs : "Not provided.", origin);

}

/*
--------------------------------------------------------------------------------------------
 Window Error
--------------------------------------------------------------------------------------------
*/

// Constructor initializes the public Error and stores the HRESULT.

WindowError::WindowError(int line, const char* file, DWORD dw) noexcept
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
		"\n[Error Code]\n0x%08X\n"
		"\n[Description]\n%s"
		"%s"
		, (unsigned)dw,
		nMsgLen ? pMsgBuf : "Unidentified error code.",
		origin
	);

	if (pMsgBuf)
		LocalFree(pMsgBuf);
}

/*
--------------------------------------------------------------------------------------------
 ImGui Error
--------------------------------------------------------------------------------------------
*/

// Create the imgui error class and funnel IM_ASSERT through CHAOTIC_FATAL.

class ImGuiError : public ChaoticError
{
public:
	// Single message constructor, the message is stored in the info.
	ImGuiError(int line, const char* file, const char* msg) noexcept
		: ChaoticError(line, file)
	{
		// Print info error formatted string.
		snprintf(info, 2048, "\n[Assertion Failed]\n%s%s", msg, origin);
	}

	// ImGui Error type override.
	const char* GetType() const noexcept override { return "ImGui Error"; }
};

// Feed forward through library pipeline.

[[noreturn]] void handleImGuiAssertion(int line, const char* file, const char* msg) noexcept
{
	CHAOTIC_FATAL(ImGuiError(line, file, msg));
}
