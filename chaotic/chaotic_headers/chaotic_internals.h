#pragma once

/* LICENSE AND COPYRIGHT
-----------------------------------------------------------------------------------------------------------
 * Chaotic — a 3D renderer for mathematical applications
 * Copyright (c) 2025-2026 Miguel Nasarre Budiño
 * Licensed under the MIT License. See LICENSE file.
-----------------------------------------------------------------------------------------------------------
*/

/* CHAOTIC-INTERNALS HEADER FILE
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
Welcome to the Chaotic Internals header!! This header contains the tools you need to develop
your own Win32 and DirectX11 additions to the library, adding functionalities to the Window
or Graphics classes or creating your own bindables.

These ones that follow are the main external dependencies for the library included in all 
internal files as well as some exception classes used for DirectX11 and Win32 errors.

For implementation details I suggest checking the microsoft-learn resources and you can 
also read the Window, Graphics and bindables source files.

The classes are copied as they are in their original header files. For further reading
each class has its own description and all functions are explained. I suggest reading the
descriptions of all functions before using them.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Include all other library dependencies
#include "chaotic_customs.h"

// Toggle to enable and disable automatic library linkage inside the header.
#define _LINK_LIBRARIES

/* WIN32 / D3D11 HEADER
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains necessary libraries to be loaded for most the D3D11 dependencies
that are used across the library.

The define list at the beggining gets rid of some chunks of the code to not bloat the
built time, if those dependencies are needed just comment them from the list.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// target Windows 10 or later
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#pragma region Some defines you might wanna use
// The following #defines disable a bunch of unused windows stuff. If you 
// get weird errors when trying to do some windows stuff, try removing some
// (or all) of these defines (it will increase build time though).
// 
//#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
//#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
//#define NOCTLMGR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
#ifndef STRICT
#define STRICT
#endif
#pragma endregion

// Win32 and DX11 includes
#include <Windows.h>
#include <WinUser.h>
#include <d3d11.h>
#include <wrl.h>

#include <windowsx.h>
#include <dwmapi.h>
#include <dxgi1_6.h> // For GPU selection

// Using Com pointers
using Microsoft::WRL::ComPtr;

#ifdef _LINK_LIBRARIES
// Required libraries for DirectX11
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "D3Dcompiler.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "dxgi.lib")

#undef _LINK_LIBRARIES
#endif

#ifdef _DEBUG
/* DXGI INFO MANAGEMENT CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This class handles the message pump by the DXGI API. All necessary methods
are nicely taken care by the functions in this file so you can access the
error messages via the static functions.

To be used call Set() before doing a call and get messages right after, if
there are messages throw and exception. This class is only meant for debug.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Static DXGI debug info management class. When using DXGI methods
// call Set() before and GetMessages() after, if get messages is not empty
// an error has occurred inside the graphics API.
class DxgiInfoManager
{
public:
	// Sets the next pointer at the end of the message list. Only error
	// messages after this point will be picked up by GetMessages().
	static void Set() noexcept;

	// Accesses the stored messages and if there are more than before returns
	// a pointer to a list of this messages, otherwise returns nullptr.
	static const char** GetMessages();

private:
	// Private helper to run constructor/destructor.
	static DxgiInfoManager helper;

	// Private constructor to access the info queue and store it in pDxgiInfoQueue.
	DxgiInfoManager();

	// Private destructor to release the pointer at the end of the process.
	~DxgiInfoManager();

	// Size of the message queue when Set() was last called.
	static inline unsigned long long next = 0u;

	// Pointer to the IDXGIInfoQueue masked as void*.
	static inline void* pDxgiInfoQueue = nullptr;
};
#endif


/* GRAPHICS EXCEPTION CLASSES
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the graphics exception classes to be used when
making DXGI API calls, some macros are alos defined for convenience.

WHen on debug the DXGI Info Manager will be used.to get access to the
error messages. Two classes are defined, the HrException, the macro
takes in an HRESULT and if non-zero throws an exception.

And the device removed exception for specific cases where that can happen.
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

/*
-----------------------------------------------------------------------------------------------------------
 Graphics Exception Macros
-----------------------------------------------------------------------------------------------------------
*/

// Exception and throw test macros for both classes.
// In debug they use the info manager to get the error messages.
#ifdef _DEBUG
#define GFX_EXCEPT(hr)					HrException( __LINE__,__FILE__,(long)(hr), DxgiInfoManager::GetMessages() )
#define GFX_THROW_INFO(hrcall)			{ HRESULT hr; DxgiInfoManager::Set(); if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr); }
#define GFX_THROW_INFO_ONLY(call)		{ DxgiInfoManager::Set(); (call); { auto v = DxgiInfoManager::GetMessages(); if(v) throw INFO_EXCEPT(v); } }
#define GFX_DEVICE_REMOVED_EXCEPT(hr)	DeviceRemovedException( __LINE__,__FILE__,(hr),DxgiInfoManager::GetMessages() )
#else
#define GFX_EXCEPT(hr)					HrException( __LINE__,__FILE__,(long)(hr) )
#define GFX_THROW_INFO(hrcall)			{ HRESULT hr; if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr); }
#define GFX_THROW_INFO_ONLY(call)		(call);
#define GFX_DEVICE_REMOVED_EXCEPT(hr)	DeviceRemovedException( __LINE__,__FILE__,(hr) )
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Graphics Exception Classes
-----------------------------------------------------------------------------------------------------------
*/

// HRESULT Exception class, stores the given HRESULT and an optional list of messages, when
// what() is called it retrieves the HRESULT information and adds the messages if they exist.
class HrException : public Exception
{
public:
	// Constructor, takes in a FAILED HRESULT and an optional list of
	// messages and stores it in memory for future what() call.
	HrException(int line, const char* file, long hr, const char** infoMsgs = nullptr) noexcept
		: Exception(line, file), hr(hr)
	{
		// join all info messages with newlines into single string
		char msgs[2048] = {};

		unsigned i = 0u, j = 0u, c = 0u;
		while (infoMsgs && infoMsgs[i])
		{
			while (infoMsgs[i][j] && c < 2047)
				msgs[c++] = infoMsgs[i][j];

			while (infoMsgs[++i] && c < 2047)
				msgs[c++] = '\n';
		}
		msgs[c] = '\0';

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
		snprintf(description, 512, "Unknown error (0x%8X)", (unsigned)hr);
	description_done:

		char e_string[64] = {};
		snprintf(e_string, 64, "0x%8X", (unsigned)hr);

		if (msgs[0])
			snprintf(info, 2048,
				"[Error Code]\n%lu\n "
				"[Error String]\n%s\n"
				"[Description]\n%s\n"
				"[Error Info]\n%s\n"
				"%s"
				, hr, e_string, description, msgs, GetOriginString());

		else
			snprintf(info, 2048,
				"[Error Code]\n%lu\n "
				"[Error String]\n%s\n"
				"[Description]\n%s\n"
				"%s"
				, hr, e_string, description, GetOriginString());
	}

	// HResult Exception type override.
	const char* GetType() const noexcept override { return "Graphics HResult Exception"; }

private:
	// Stores the FAILED HRESULT
	long hr;
};

// Wrapper of an HRESULT Exception that has the same constructor but get the type
// Device Removed. To be called with HRESULT of pDevice->GetDeviceRemovedReason().
class DeviceRemovedException : public HrException
{
	// Using same constructor as HRESULT Exception.
	using HrException::HrException;
public:
	// [Device Removed] Exception type override.
	const char* GetType() const noexcept override { return "Graphics [Device Removed] Exception (DXGI_ERROR_DEVICE_REMOVED)"; }
};


#include <cstdio>
/* WIN32 EXCEPTION CLASS
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
This header contains the windows exception class, to be used when calling Win32
API functions that can throw errors but are not part of the DXGI API functions.

The class takes an HRESULT as an input and decodes it using the Win32 FormatMessage
functions. By the macros default it retrieves the last error by calling GetLastError().
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
*/

// Macro to retrieve the last error code amd create a Window Exception with it.
#define WND_LAST_EXCEPT()	WindowException( __LINE__,__FILE__,GetLastError() )

// Win32 Exception class, when an API function call returns something unexpected
// the HRESULT from that function is retrieved and used to translate the error code.
// This error code and string can be accessed via the what function.
class WindowException : public Exception
{
public:
	// Constructor initializes the public Exception and stores the HRESULT.
	WindowException(int line, const char* file, HRESULT hr) noexcept
		: Exception(line, file), hr(hr)
	{
		// Retrieves the error string from a given Win32 failed HRESULT 
		// using the FormatMessage method and returns the string.
		char* pMsgBuf = nullptr;
		// windows will allocate memory for err string and make our pointer point to it
		DWORD nMsgLen = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
		);

		if (nMsgLen && pMsgBuf)
		{
			snprintf(info, 2048,
				"[Error Code]\n%i\n"
				"[Description]\n%s\n"
				"%s"
				, hr, pMsgBuf, GetOriginString()
			);

			LocalFree(pMsgBuf);
		}

		else
			snprintf(info, 2048,
				"[Error Code]\n%i\n"
				"[Description]\nUnidentified error code\n"
				"%s"
				, hr, GetOriginString()
			);
	}

	// Win32 Exception type override.
	const char* GetType() const noexcept override { return "Win32 Exception"; }

private:
	// Stores the FAILED HRESULT
	HRESULT hr;
};
