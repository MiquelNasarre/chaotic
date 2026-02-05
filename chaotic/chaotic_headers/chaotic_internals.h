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
internal files as well as some error classes used for DirectX11 and Win32 errors.

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
#include <cstdio> // For snprintf

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
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This class handles the message pump by the DXGI API. All necessary methods
are nicely taken care by the functions in this file so you can access the
error messages via the static functions.

To be used call Set() before doing a call and get messages right after, if
there are messages raises an error. This class is only meant for debug.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
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
	static const char* GetMessages();

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

	// Pointer to the debug dll masked as void*.
	static inline void* hModDxgiDebug = nullptr;
};
#endif


/* GRAPHICS ERROR CLASSES
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the graphics error classes to be used when
making DXGI API calls, some macros are alos defined for convenience.

WHen on debug the DXGI Info Manager will be used.to get access to the
error messages. Two classes are defined, the HrError, the macro
takes in an HRESULT and if non-zero creates an error.

And the device removed error for specific cases where that can happen.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
 Graphics Error Macros
-------------------------------------------------------------------------------------------------------
*/

// Checks and error macros for both classes.
// In debug they use the info manager to get the error messages.
#ifdef _DEBUG
// GraphicsError
#define GRAPHICS_INFO_ERROR(_MSG)				CHAOTIC_FATAL(GraphicsError( __LINE__,__FILE__,_MSG))
#define GRAPHICS_INFO_CHECK(_CALL)				do { DxgiInfoManager::Set(); (_CALL); { auto msg = DxgiInfoManager::GetMessages(); if(msg) { GRAPHICS_INFO_ERROR(msg); } } } while(0)
// HrError
#define GRAPHICS_HR_ERROR(_HR)					CHAOTIC_FATAL(HrError( __LINE__,__FILE__,(long)(_HR), DxgiInfoManager::GetMessages()))
#define GRAPHICS_HR_CHECK(_HRCALL)				do { HRESULT hr; DxgiInfoManager::Set(); if( FAILED( hr = (_HRCALL) ) ) GRAPHICS_HR_ERROR(hr); } while(0)
// DeviceRemovedError
#define GRAPHICS_HR_DEVICE_REMOVED_ERROR(_HR)	CHAOTIC_FATAL(DeviceRemovedError( __LINE__,__FILE__,(_HR),DxgiInfoManager::GetMessages()))
#else
// GraphicsError
#define GRAPHICS_INFO_ERROR(_MSG)				CHAOTIC_FATAL(GraphicsError( __LINE__,__FILE__,_MSG))
#define GRAPHICS_INFO_CHECK(_CALL)				(_CALL)
// HrError
#define GRAPHICS_HR_ERROR(_HR)					CHAOTIC_FATAL(HrError( __LINE__,__FILE__,(long)(_HR)))
#define GRAPHICS_HR_CHECK(_HRCALL)				do { HRESULT hr; if( FAILED( hr = (_HRCALL) ) ) GRAPHICS_HR_ERROR(hr); } while(0)
// DeviceRemovedError
#define GRAPHICS_HR_DEVICE_REMOVED_ERROR(_HR)	CHAOTIC_FATAL(DeviceRemovedError( __LINE__,__FILE__,(_HR)))
#endif

/*
-------------------------------------------------------------------------------------------------------
 Graphics Error Classes
-------------------------------------------------------------------------------------------------------
*/

// Info only Graphics Error class. Same behavior as UserError class, but called with 
// DXGI info messages for Graphics falied calls.
class GraphicsError : public ChaoticError
{
public:
	// Single message constructor, the message is stored in the info.
	GraphicsError(int line, const char* file, const char* msg) noexcept;

	// Info Error type override.
	const char* GetType() const noexcept override { return "Graphics Info Error"; }
};

// HRESULT Error class, stores the given HRESULT and an optional list of messages, when
// what() is called it retrieves the HRESULT information and adds the messages if they exist.
class HrError : public ChaoticError
{
public:
	// Constructor, takes in a FAILED HRESULT and an optional list of
	// messages and stores it in memory for future info printing.
	HrError(int line, const char* file, long hr, const char* infoMsgs = nullptr) noexcept;

	// HResult Error type override.
	const char* GetType() const noexcept override { return "Graphics HResult Error"; }
};

// Wrapper of an HRESULT Error that has the same constructor but get the type
// Device Removed. To be called with HRESULT of pDevice->GetDeviceRemovedReason().
class DeviceRemovedError : public HrError
{
	// Using same constructor as HRESULT Error.
	using HrError::HrError;
public:
	// [Device Removed] Error type override.
	const char* GetType() const noexcept override { return "Graphics [Device Removed] Error"; }
};

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
	WindowError(int line, const char* file, DWORD dw) noexcept;

	// Win32 Error type override.
	const char* GetType() const noexcept override { return "Win32 Error"; }
};
