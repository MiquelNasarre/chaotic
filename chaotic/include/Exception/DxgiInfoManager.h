#pragma once
#ifdef _DEBUG
/* DXGI INFO MANAGEMENT CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This class handles the message pump by the DXGI API. All necessary methods 
are nicely taken care by the functions in this file so you can access the 
error messages via the static functions.

To be used call Set() before doing a call and get messages right after, if 
there are messages throw and exception. This class is only meant for debug.
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