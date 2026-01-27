#pragma once
#include "Exception/Exception.h"
#include "Exception/_exDefault.h"
#include "DxgiInfoManager.h"

/* GRAPHICS EXCEPTION CLASSES
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the graphics exception classes to be used when
making DXGI API calls, some macros are alos defined for convenience.

WHen on debug the DXGI Info Manager will be used.to get access to the 
error messages. Two classes are defined, the HrException, the macro 
takes in an HRESULT and if non-zero throws an exception.

And the device removed exception for specific cases where that can happen.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------------------------------
 Graphics Exception Macros
-------------------------------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------------------------------
 Graphics Exception Classes
-------------------------------------------------------------------------------------------------------
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
