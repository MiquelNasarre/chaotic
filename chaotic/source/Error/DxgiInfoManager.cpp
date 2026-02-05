#ifdef _DEBUG
#include "Error/DxgiInfoManager.h"

#include "WinHeader.h"
#include "Error/_erGraphics.h"
#include "Error/_erWindow.h"

#include <dxgidebug.h>

// Private helper to run constructor/destructor.
DxgiInfoManager DxgiInfoManager::helper;

/*
--------------------------------------------------------------------------------------------
 DXGI Info Management Function definitions
--------------------------------------------------------------------------------------------
*/

// Private constructor to access the info queue and store it in pDxgiInfoQueue.

DxgiInfoManager::DxgiInfoManager()
{
	// define function signature of DXGIGetDebugInterface
	typedef HRESULT(WINAPI* DXGIGetDebugInterface)(REFIID, void**);

	// load the dll that contains the function DXGIGetDebugInterface
	hModDxgiDebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	WINDOW_CHECK(hModDxgiDebug);

	// get address of DXGIGetDebugInterface in dll
	const auto DxgiGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface>(
		reinterpret_cast<void*>(GetProcAddress((HMODULE)hModDxgiDebug, "DXGIGetDebugInterface"))
		);
	WINDOW_CHECK(DxgiGetDebugInterface);

	if(FAILED(DxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), &pDxgiInfoQueue)))
		GRAPHICS_INFO_ERROR("Failed to laod the info queue for the DXGI Info Manager");
}

// Private destructor to release the pointer at the end of the process.

DxgiInfoManager::~DxgiInfoManager()
{
	if (pDxgiInfoQueue)
		((IDXGIInfoQueue*)pDxgiInfoQueue)->Release();

	if (hModDxgiDebug)
		FreeLibrary((HMODULE)hModDxgiDebug);
}

// Sets the next pointer at the end of the message list. Only error
// messages after this point will be picked up by GetMessages().

void DxgiInfoManager::Set() noexcept
{
	// set the index (next) so that the next all to GetMessages()
	// will only get errors generated after this call
	next = ((IDXGIInfoQueue*)pDxgiInfoQueue)->GetNumStoredMessages(DXGI_DEBUG_ALL);
}

// Accesses the stored messages and if there are more than before returns
// a pointer to a list of this messages, otherwise returns nullptr.

const char* DxgiInfoManager::GetMessages()
{
	const auto end = ((IDXGIInfoQueue*)pDxgiInfoQueue)->GetNumStoredMessages(DXGI_DEBUG_ALL);
	if (next == end) 
		return nullptr;

	thread_local static char messages[2048];
	int position = 0;
	for (size_t i = next; i < end; i++)
	{
		size_t messageLength;
		// get the size of message i in bytes
		if (FAILED(((IDXGIInfoQueue*)pDxgiInfoQueue)->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &messageLength)))
			GRAPHICS_INFO_ERROR("Unexpected error when trying to load error message from queue");

		// allocate memory for message
		char* bytes = new char[messageLength];
		auto pMessage = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes);

		// get the message and push its description into the vector
		if(FAILED(((IDXGIInfoQueue*)pDxgiInfoQueue)->GetMessage(DXGI_DEBUG_ALL, i, pMessage, &messageLength)))
			GRAPHICS_INFO_ERROR("Unexpected error when trying to load error message from queue");

		// Print message to buffer.
		position += snprintf(&messages[position], 2048 - position, "%s", pMessage->pDescription);
		delete[] bytes;
		if (position >= 2048 - 2)
			break;

		if (i < end - 1)
		{
			messages[position++] = '\n';
			messages[position] = '\0';
		}
	}
	// Set the counter to end.
	next = end;

	// Return stored messages.
	return messages;
}
#endif