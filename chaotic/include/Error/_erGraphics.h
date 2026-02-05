#pragma once
#include "Error/ChaoticError.h"
#include "Error/DxgiInfoManager.h"

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
#if defined _DEPLOYMENT && defined SOLUTION_DIR

#ifdef _DEBUG
// GraphicsError
#define GRAPHICS_INFO_ERROR(_MSG)				CHAOTIC_FATAL(GraphicsError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,_MSG))
#define GRAPHICS_INFO_CHECK(_CALL)				do { DxgiInfoManager::Set(); (_CALL); { auto msg = DxgiInfoManager::GetMessages(); if(msg) { GRAPHICS_INFO_ERROR(msg); } } } while(0)
// HrError
#define GRAPHICS_HR_ERROR(_HR)					CHAOTIC_FATAL(HrError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,(long)(_HR), DxgiInfoManager::GetMessages()))
#define GRAPHICS_HR_CHECK(_HRCALL)				do { HRESULT hr; DxgiInfoManager::Set(); if( FAILED( hr = (_HRCALL) ) ) GRAPHICS_HR_ERROR(hr); } while(0)
// DeviceRemovedError
#define GRAPHICS_HR_DEVICE_REMOVED_ERROR(_HR)	CHAOTIC_FATAL(DeviceRemovedError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,(_HR),DxgiInfoManager::GetMessages()))
#else
// GraphicsError
#define GRAPHICS_INFO_ERROR(_MSG)				CHAOTIC_FATAL(GraphicsError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,_MSG))
#define GRAPHICS_INFO_CHECK(_CALL)				(_CALL)
// HrError
#define GRAPHICS_HR_ERROR(_HR)					CHAOTIC_FATAL(HrError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,(long)(_HR)))
#define GRAPHICS_HR_CHECK(_HRCALL)				do { HRESULT hr; if( FAILED( hr = (_HRCALL) ) ) GRAPHICS_HR_ERROR(hr); } while(0)
// DeviceRemovedError
#define GRAPHICS_HR_DEVICE_REMOVED_ERROR(_HR)	CHAOTIC_FATAL(DeviceRemovedError( __LINE__,__FILE__ + sizeof(SOLUTION_DIR) - 1,(_HR)))
#endif

#else

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

	// Graphics Info Error type override.
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
