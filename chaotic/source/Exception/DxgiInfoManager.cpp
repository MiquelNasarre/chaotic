#ifdef _DEBUG
#include "Exception/DxgiInfoManager.h"

#include "WinHeader.h"
#include "Exception/_exGraphics.h"
#include "Exception/_exWindow.h"

#include <dxgidebug.h>
#include <memory>
#include <string>

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
	// create the COM pointer
	pDxgiInfoQueue = ComPtr<IDXGIInfoQueue>().Detach();

	// define function signature of DXGIGetDebugInterface
	typedef HRESULT(WINAPI* DXGIGetDebugInterface)(REFIID, void**);

	// load the dll that contains the function DXGIGetDebugInterface
	const auto hModDxgiDebug = LoadLibraryEx(L"dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hModDxgiDebug)
		throw WND_LAST_EXCEPT();

	// get address of DXGIGetDebugInterface in dll
	const auto DxgiGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface>(
		reinterpret_cast<void*>(GetProcAddress(hModDxgiDebug, "DXGIGetDebugInterface"))
		);
	if (!DxgiGetDebugInterface)
		throw WND_LAST_EXCEPT();

	if (FAILED (DxgiGetDebugInterface(__uuidof(IDXGIInfoQueue), &pDxgiInfoQueue)) )
		throw INFO_EXCEPT("Failed to laod the info queue for the DXGI Info Manager");
}

// Private destructor to release the pointer at the end of the process.

DxgiInfoManager::~DxgiInfoManager()
{
	((IDXGIInfoQueue*)pDxgiInfoQueue)->Release();
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

const char** DxgiInfoManager::GetMessages()
{
	const auto end = ((IDXGIInfoQueue*)pDxgiInfoQueue)->GetNumStoredMessages(DXGI_DEBUG_ALL);
	if (next == end) 
		return nullptr;

	const char** messages = (const char**)calloc(end + 1 - next, sizeof(const char*));
	for (size_t i = next; i < end; i++)
	{
		size_t messageLength;
		// get the size of message i in bytes
		if (FAILED( ((IDXGIInfoQueue*)pDxgiInfoQueue)->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &messageLength) ))
			throw INFO_EXCEPT("Unexpected error when trying to load error message from queue");
		// allocate memory for message
		auto bytes = std::make_unique<byte[]>(messageLength);
		auto pMessage = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes.get());
		// get the message and push its description into the vector
		if (FAILED( ((IDXGIInfoQueue*)pDxgiInfoQueue)->GetMessage(DXGI_DEBUG_ALL, i, pMessage, &messageLength) ))
			throw INFO_EXCEPT("Unexpected error when trying to load error message from queue");
		messages[i - next] = std::string(pMessage->pDescription).c_str();
	}
	return messages;
}
#endif