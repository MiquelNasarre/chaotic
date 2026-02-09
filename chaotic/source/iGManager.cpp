#include "iGManager.h"

#ifdef _INCLUDE_IMGUI
#include "WinHeader.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// Declares the use of the ImGui message procedure.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
--------------------------------------------------------------------------------------------
 iGManager Function Definitions
--------------------------------------------------------------------------------------------
*/

// Constructor, initializes the ImGui context for the specific window and 
// initializes ImGui WIN32/DX11 in general if called for the first time.
// Important to call ImGui is to be used in the application.

iGManager::iGManager(Window& _w, bool ini_file)
	: iGManager(ini_file)
{
	bind(_w);
}

// Constructor, initializes ImGui DX11 for the specific instance. Does not 
// bind user interface to any window. Bind must be called for interaction.

iGManager::iGManager(bool ini_file)
{
	IMGUI_CHECKVERSION();

	// If the global device are still not created create them.
	GlobalDevice::set_global_device();

	// Store the current imGui context if exists
	ImGuiContext* current = ImGui::GetCurrentContext();

	// Create a unique context for this window
	Context = ImGui::CreateContext();
	ImGui::SetCurrentContext((ImGuiContext*)Context);
	ImGui::StyleColorsDark();

	// Initialize directX 11 for this context.
	ImGui_ImplDX11_Init(
		(ID3D11Device*)GlobalDevice::get_device_ptr(),
		(ID3D11DeviceContext*)GlobalDevice::get_context_ptr()
	);

	if (!ini_file)
	{
		// Avoid the creation of imgui.ini file
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
	}

	// Go back to previous context if existed.
	if (current)
		ImGui::SetCurrentContext(current);
}

// If it is the last class instance shuts down ImGui WIN32/DX11.

iGManager::~iGManager()
{
	// Unbind yourself from the window.
	unbind();

	// Store current context.
	ImGuiContext* current = ImGui::GetCurrentContext();

	// Destroy this ImGui context
	ImGui::SetCurrentContext((ImGuiContext*)Context);
	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext((ImGuiContext*)Context);

	// If existed go back to previous context.
	if (current)
		ImGui::SetCurrentContext(current);
}

// Function to be called at the beggining of an ImGui render function.
// Calls new frame on Win32 and DX11 on the imGui API.

void iGManager::newFrame()
{
	ImGui::SetCurrentContext((ImGuiContext*)Context);
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

// Function to be called at the end of an ImGui render function.
// Calls the rendering method of DX11 on the imGui API.

void iGManager::drawFrame()
{
	ImGui::SetCurrentContext((ImGuiContext*)Context);
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// Binds the objects user interaction to the specified window.

void iGManager::bind(Window& _w)
{
	// If it's bound to a window unbind it.
	unbind();

	// If another iGManager is bound to this window error.
	if (*_w.imGuiPtrAdress())
		USER_ERROR("You cannot have multiple ImGui contexts bound to the same window at the same time.");

	// Set your context as current
	ImGuiContext* current = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext((ImGuiContext*)Context);

	ImGui_ImplWin32_Init(_w.getWindowHandle());

	// Return to the previous context.
	if (current)
		ImGui::SetCurrentContext(current);

	// Add yourself to the window pointer.
	*_w.imGuiPtrAdress() = this;
	pWindow = &_w;
}

// If it's bound to a window it unbinds it.

void iGManager::unbind()
{
	ImGuiContext* current = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext((ImGuiContext*)Context);

	// If a window is bound delete that bind.
	if (pWindow)
	{
		*pWindow->imGuiPtrAdress() = nullptr;
		ImGui_ImplWin32_Shutdown();
	}

	if (current)
		ImGui::SetCurrentContext(current);
}

// Whether ImGui is currently capturing mouse events.

bool iGManager::wantCaptureMouse()
{
	// Store the current imGui context if exists and set current.
	ImGuiContext* current = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext((ImGuiContext*)Context);

	// Check if current IO wants mouse capture.
	ImGuiIO& io = ImGui::GetIO();
	bool wants = io.WantCaptureMouse;

	// Go back to previous context if existed.
	if (current)
		ImGui::SetCurrentContext(current);

	return wants;
}

// Whether ImGui is currently capturing keyboard events.

bool iGManager::wantCaptureKeyboard()
{
	// Store the current imGui context if exists and set current.
	ImGuiContext* current = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext((ImGuiContext*)Context);

	// Check if current IO wants mouse capture.
	ImGuiIO& io = ImGui::GetIO();
	bool wants = io.WantCaptureKeyboard;

	// Go back to previous context if existed.
	if (current)
		ImGui::SetCurrentContext(current);

	return wants;
}

// Called by the MSGHandlePipeline lets ImGui handle the message flow
// if imGui is active and currently has focus on the window.

bool iGManager::WndProcHandler(void* hWnd, unsigned int msg, unsigned int wParam, unsigned int lParam)
{
	// Extract the imGui instance through the window
	Window* pWnd = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)hWnd, GWLP_USERDATA));
	iGManager* pImGui = (iGManager*)*pWnd->imGuiPtrAdress();

	// If no ImGui is associated with this window return false.
	if (!pImGui)
		return false;

	// Switch to the correct ImGui context for this window
	ImGuiContext* current = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext((ImGuiContext*)pImGui->Context);

	// Pass the proc to the ImGui Win32 WndProcHandler
	ImGui_ImplWin32_WndProcHandler((HWND)hWnd, msg, wParam, lParam);
	const auto& imio = ImGui::GetIO();

	// Return to other context if existed.
	if (current)
		ImGui::SetCurrentContext(current);

	// If ImGui wants to capture the procedures return true.
	if (imio.WantCaptureKeyboard || imio.WantCaptureMouse)
		return true;

	// Otherwise return false.
	return false;
}
#endif