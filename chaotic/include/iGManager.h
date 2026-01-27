#pragma once
#include "Window.h"
#ifdef _INCLUDE_IMGUI

/* IMGUI BASE CLASS MANAGER
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Given the difficulty of developing User Interface classes this library encourages 
the use of ImGui for the user interface of the developed applications.

To use ImGui inside your applications you just need to create an inherited iGManager
class and a constructor that feeds the widow to the base constructor.

Then you can create a render function that starts with newFrame and end with drawFrame
using the imGui provided functions and your ImGui interface will be drawn to the window.

For more information on how to develop ImGui interfaces please visit:
ImGui GitHub Repositiory: https://github.com/ocornut/imgui.git
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// ImGui Interface base class, contains the basic building blocks to connect the ImGui
// API to this library, any app that wishes to use ImGui needs to inherit this class and 
// call the constructor on the desired window.
class iGManager 
{
	// Needs access to the internals to feed messages to ImGui if used.
	friend class MSGHandlePipeline;

protected:
	// Constructor, initializes ImGui WIN32/DX11 for the specific instance and
	// binds Win32 to the specified window, for user interface.
	explicit iGManager(Window& _w);

	// Constructor, initializes ImGui DX11 for the specific instance. Does not 
	// bind user interface to any window. Bind must be called for interaction.
	explicit iGManager();

	// If it is the last class instance shuts down ImGui WIN32/DX11.
	virtual ~iGManager();

	// Function to be called at the beggining of an ImGui render function.
	// Calls new frame on Win32 and DX11 on the imGui API.
	void newFrame();

	// Function to be called at the end of an ImGui render function.
	// Calls the rendering method of DX11 on the imGui API.
	void drawFrame();

public:
	// Binds the objects user interaction to the specified window.
	void bind(Window& _w);

	// If it's bound to a window it unbinds it.
	void unbind();

	// InGui rendering function, needs to be implemented by inheritance and will be 
	// called by the bounded window just before pushing a frame. For custom behavior 
	// just leave it blank and create your onw rendering functions.
	virtual void render() = 0;

private:
	// Stores the pointer to the ImGui context of the specific window of the instance.
	void* Context = nullptr;

	// Stores the pointer to its corresponding window.
	Window* pWindow = nullptr;

	// Called by the MSGHandlePipeline lets ImGui handle the message flow
	// if imGui is active and currently has focus on the window.
	static bool WndProcHandler(void* hWnd, unsigned int msg, unsigned int wParam, unsigned int lParam);

	// No copies are allowed. For proper context management.
	iGManager(iGManager&&) = delete;
	iGManager operator=(iGManager&&) = delete;
	iGManager(const iGManager&) = delete;
	iGManager operator=(const iGManager&) = delete;
};
#endif