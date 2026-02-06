#include "Window.h"

#include "Timer.h"
#include "Graphics.h"
#include "iGManager.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "WinHeader.h"

#include <cstdarg> // For formatted window titles

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

/*
-----------------------------------------------------------------------------------------------------------
 Window Internal Data struct
-----------------------------------------------------------------------------------------------------------
*/

// Custom message that signals a window close button pressed.
#define WM_APP_WINDOW_CLOSE		(WM_APP + 1)

// This structure contains the internal data allocated by every window.
struct WindowInternals
{
	Graphics* graphics = nullptr;	// Graphics object of the window.
#ifdef _INCLUDE_IMGUI
	iGManager* imGui = nullptr;		// Pointer to the current iGManager.
#endif
	static inline Timer timer;		// Timer object to keep track of the framerate.

	WINDOWPLACEMENT g_wpPrev;   // When entering fullscreen stores the previous placement data.

	Vector2i Dimensions = {};	// Dimensions of the window.
	Vector2i Position = {};		// Position of the window.
	HWND hWnd = nullptr;		// Handle to the window.
	char Name[512] = {};		// Name of the window.
	bool wallpaper = false;		// Whether it is a wallpaper window.
	bool w_persist = false;		// Whether the wallpaper persisits past destructor.
	int monitor_idx = 0;		// Stores the last monitor IDX used in wallpaper mode.

	static inline bool noFrameUpdate = false;	// Schedules next frame time to be skipped.
	static inline float frame = 0.f;			// Stores the time of the last frame push.
	static inline float Frametime = 0.f;		// Stores the specified time for each frame.
};

/*
-----------------------------------------------------------------------------------------------------------
 Helpers for Wallpaper mode
-----------------------------------------------------------------------------------------------------------
*/

static BOOL CALLBACK EnumWindowsFindWorkerW(HWND top, LPARAM lParam)
{
	HWND& phwnd = *(HWND*)lParam;

	// Does this top-level window host the desktop icons view?
	HWND defView = FindWindowExW(top, nullptr, L"SHELLDLL_DefView", nullptr);
	if (defView)
	{
		// The WorkerW we want is typically a sibling of this top-level window.
		// This finds the next WorkerW after 'top' in the Z-order.
		HWND workerw = FindWindowExW(nullptr, top, L"WorkerW", nullptr);
		if (workerw)
		{
			phwnd = workerw;
			return FALSE; // stop enumeration
		}
	}
	return TRUE;
}

struct MonitorPick
{
	int indexWanted = 0;
	int indexNow = 0;
	RECT rc = {};
	bool found = false;
};

static BOOL CALLBACK EnumMonProc(HMONITOR hMon, HDC, LPRECT, LPARAM p)
{
	auto& mp = *(MonitorPick*)p;

	MONITORINFO mi = {};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoW(hMon, &mi))
		return TRUE;

	if (mp.indexNow == mp.indexWanted)
	{
		mp.rc = mi.rcMonitor;     // virtual-desktop coordinates
		mp.found = true;
		return FALSE;             // stop
	}

	mp.indexNow++;
	return TRUE;
}

static RECT GetMonitorRectByIndex(int monitorIndex)
{
	MonitorPick mp = {};
	mp.indexWanted = monitorIndex;
	mp.indexNow = 0;
	mp.found = false;

	if (monitorIndex != -1)
		EnumDisplayMonitors(nullptr, nullptr, EnumMonProc, (LPARAM)&mp);

	if (!mp.found)
	{
		// fallback to virtual screen
		RECT vr = {};
		vr.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		vr.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
		vr.right = vr.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		vr.bottom = vr.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
		return vr;
	}
	return mp.rc;
}

/*
-----------------------------------------------------------------------------------------------------------
 MSG Pipeline Static Class
-----------------------------------------------------------------------------------------------------------
*/

// Static class that handles the process messages and sets custom procedures.
class MSGHandlePipeline
{
public:
	// Custom procedure for window message handling, Stores mouse and keyboard events and
	// handles other specific window events. Other events are sent to the defaul procedure.
	static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, WindowInternals* _data, Window* pWnd) noexcept
	{
		WindowInternals& data = *_data;

		//	This function handles all the messages send by the computer to the application

		// Priority cases I want to always take care of

		switch (msg)
		{
		case WM_CLOSE:
		{
			// Send a custom message to the thread's message queue
			PostThreadMessage(GetCurrentThreadId(), WM_APP_WINDOW_CLOSE, (WPARAM)pWnd->getID(), 0);
			return 0;
		}

		case WM_SIZE:
			data.Dimensions.x = GET_X_LPARAM(lParam);
			data.Dimensions.y = GET_Y_LPARAM(lParam);
			if (data.graphics)
				data.graphics->setWindowDimensions(data.Dimensions);
			break;

		case WM_MOVE:
			data.noFrameUpdate = true;
			data.Position.x = GET_X_LPARAM(lParam);
			data.Position.y = GET_Y_LPARAM(lParam);
			break;

		case WM_KILLFOCUS:
			Keyboard::clearKeyStates();
			Mouse::resetWheel();
			Mouse::clearBuffer();
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			if (Keyboard::getAutorepeat() || !Keyboard::isKeyPressed((unsigned char)wParam))
				Keyboard::pushEvent(Keyboard::event::Pressed, (unsigned char)wParam);
			Keyboard::setKeyPressed((unsigned char)wParam);
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			Keyboard::setKeyReleased((unsigned char)wParam);
			Keyboard::pushEvent(Keyboard::event::Released, (unsigned char)wParam);
			break;

		case WM_MOUSEMOVE:
			Mouse::setPosition(Vector2i(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
			Mouse::setScPosition(Vector2i(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)) + data.Position);
			Mouse::pushEvent(Mouse::event::Moved, Mouse::None);
			break;

		}
#ifdef _INCLUDE_IMGUI
		// Let ImGui handle the rest if he has focus

		if (iGManager::WndProcHandler(hWnd, msg, (unsigned int)wParam, (unsigned int)lParam))
			return DefWindowProc(hWnd, msg, wParam, lParam);
#endif

		// Other lesser cases

		switch (msg)
		{
		case WM_CHAR:
			Keyboard::pushChar((char)wParam);
			break;

		case WM_LBUTTONDOWN:
			Mouse::pushEvent(Mouse::event::Pressed, Mouse::Left);
			Mouse::setButtonPressed(Mouse::Left);
			SetCapture(hWnd);
			break;

		case WM_LBUTTONUP:
			Mouse::pushEvent(Mouse::event::Released, Mouse::Left);
			Mouse::setButtonReleased(Mouse::Left);
			ReleaseCapture();
			break;

		case WM_RBUTTONDOWN:
			Mouse::pushEvent(Mouse::event::Pressed, Mouse::Right);
			Mouse::setButtonPressed(Mouse::Right);
			break;

		case WM_RBUTTONUP:
			Mouse::pushEvent(Mouse::event::Released, Mouse::Right);
			Mouse::setButtonReleased(Mouse::Right);
			break;

		case WM_MBUTTONDOWN:
			Mouse::pushEvent(Mouse::event::Pressed, Mouse::Middle);
			Mouse::setButtonPressed(Mouse::Middle);
			break;

		case WM_MBUTTONUP:
			Mouse::pushEvent(Mouse::event::Released, Mouse::Middle);
			Mouse::setButtonReleased(Mouse::Middle);
			break;

		case WM_MOUSEWHEEL:
			Mouse::increaseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			Mouse::pushEvent(Mouse::event::Wheel, Mouse::None);
			break;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	// Trampoline used to call the custom message pipeline to the specific window data.
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// retrieve ptr to window class
		Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (!pWnd)
			return DefWindowProc(hWnd, msg, wParam, lParam);
		// forward message to window class handler
		return HandleMsg(hWnd, msg, wParam, lParam, (WindowInternals*)pWnd->WindowData, pWnd);
	}

	// Setup to create the trampoline method. Creates a virtual window that points to the 
	// actual window and is accessible by the trampoline via its handle.
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE)
		{
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
			// set the window handle inside the class data
			auto& data = *((WindowInternals*)pWnd->WindowData);
			data.hWnd = hWnd;
			// set WinAPI-managed user data to store ptr to window class
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
			// set message proc to normal (non-setup) handler now that setup is finished
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HandleMsgThunk));
			// forward message to window class handler
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		// if we get a message before the WM_NCCREATE message, handle with default handler
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
};

/*
-----------------------------------------------------------------------------------------------------------
 Instance creation Helper class
-----------------------------------------------------------------------------------------------------------
*/

// Global window class that creates the handle to the instance.
// Also assings a name and icon to the process.
class WindowClass
{
public:
	// Returns the name of the window class.
	static inline const LPCSTR GetName() noexcept { return wndClassName; }
	// Returns the handle to the instance.
	static inline HINSTANCE GetInstance() noexcept { return hInst; }

private:
	// Private constructor creates the instance.
	WindowClass() noexcept
	{
		// Get your process instance handle
		hInst = GetModuleHandle(nullptr);

		//	Register Window class
		WNDCLASSEXA wc = {};
		wc.cbSize = sizeof(wc);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = MSGHandlePipeline::HandleMsgSetup;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetInstance();
#ifndef _DEPLOYMENT
		wc.hIcon = static_cast<HICON>(LoadImageW(0, PROJECT_DIR L"resources/Icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_SHARED));
#else
		int cx = GetSystemMetrics(SM_CXICON);
		int cy = GetSystemMetrics(SM_CYICON);
		int id = LookupIconIdFromDirectoryEx(
			(PBYTE)getBlobFromId(BLOB_ID::BLOB_DEFAULT_ICON), TRUE, cx, cy, LR_DEFAULTCOLOR
		);
		wc.hIcon = CreateIconFromResourceEx(
			(PBYTE)getBlobFromId(BLOB_ID::BLOB_DEFAULT_ICON) + id,
			(DWORD)getBlobSizeFromId(BLOB_ID::BLOB_DEFAULT_ICON) - id,
			TRUE,
			0x00030000,
			cx, cy,
			LR_DEFAULTCOLOR
		);
#endif
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = GetName();
		wc.hIconSm = nullptr;
		RegisterClassExA(&wc);

		// Set context awareness to avoid looking blurry on resized windows.
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
	// Private destructor deletes the instance.
	~WindowClass()
	{
		UnregisterClassA(wndClassName, hInst);
	}

	static constexpr LPCSTR wndClassName = "Chaotic Window";
	static inline HINSTANCE hInst;
	static WindowClass wndClass;
};

WindowClass WindowClass::wndClass;

/*
-----------------------------------------------------------------------------------------------------------
 Window Class Functions
-----------------------------------------------------------------------------------------------------------
*/

// Returns a reference to the window internal Graphics object.

Graphics& Window::graphics()
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return *data.graphics;
}

// Returns a reference to the window internal Graphics object.

const Graphics& Window::graphics() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return *data.graphics;
}

// Loops throgh the messages, pushes them to the queue and translates them.
// It reuturns 0 unless a window close button is pressed, in that case it 
// returns the window ID of that specific window.

unsigned Window::processEvents()
{
	//	Message Pump
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
	{
		if (msg.message == WM_APP_WINDOW_CLOSE)
			return (unsigned)msg.wParam; // Close Window ID

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	handleFramerate();
	return 0;
}

// Closes the window.

void Window::close()
{
	// Send a custom message to the thread's message queue
	PostThreadMessage(GetCurrentThreadId(), WM_APP_WINDOW_CLOSE, (WPARAM)getID(), 0);
}

// Creates the window and its associated Graphics, expects a valid descriptor 
// pointer, if not provided it chooses the default descriptor settings.

Window::Window(const WINDOW_DESC* pDesc): w_id { next_id++ }
{
	//  If descriptor not provided, default
	WINDOW_DESC desc = {};
	if (pDesc)
		desc = *pDesc;

	//  Create internal data
	WindowData = new WindowInternals;
	WindowInternals& data = *((WindowInternals*)WindowData);
	data.wallpaper = (desc.window_mode == WINDOW_DESC::WINDOW_MODE_WALLPAPER);

	switch (desc.window_mode)
	{
		case WINDOW_DESC::WINDOW_MODE_WALLPAPER:
		{
			// Set persistance setting
			data.w_persist = desc.wallpaper_persist;
			data.monitor_idx = desc.monitor_idx;

			// Get access to the Explorer
			HWND progman = FindWindowW(L"Progman", nullptr);
			WINDOW_CHECK(progman);

			// Ask it kindly to spawn the workerW.
			DWORD_PTR result = 0;
			if (!SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result))
				WINDOW_LAST_ERROR();

			// Find that workerW window
			HWND workerw = nullptr;
			EnumWindows(EnumWindowsFindWorkerW, (LPARAM)&workerw);

			// If you cannot find it fallback to regular window and pop message.
			if (!workerw)
			{
				// Not a wallpaper anymore.
				data.wallpaper = false;

				// Use specified dimensions
				data.Dimensions = desc.window_dim;

				//	Calculate window size based on desired client region size
				RECT wr;
				wr.left = 100;
				wr.right = desc.window_dim.x + wr.left;
				wr.top = 100;
				wr.bottom = desc.window_dim.y + wr.top;
				if (!AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPEDWINDOW, FALSE))
					WINDOW_LAST_ERROR();

				//	Create Window & get hWnd
				HWND hWnd = CreateWindowExA(
					NULL,
					WindowClass::GetName(),
					NULL,
					WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					wr.right - wr.left,
					wr.bottom - wr.top,
					nullptr,
					nullptr,
					WindowClass::GetInstance(),
					this
				);

				//	Check for error
				WINDOW_CHECK(hWnd);

				// Pop fallback message box.
				auto messageBox = [](void*)
					{
						const char* info =
							"Chaotic could not attach itself behind your desktop icons.\n\n"
							"This feature relies on undocumented Windows behavior and is "
							"not guaranteed to work on all system configurations.\n\n"
							"Falling back to regular window creation.";

						MessageBoxA(nullptr, info, "Wallpaper Mode Unavailable", MB_ICONEXCLAMATION);
						return 0UL;
					};

				CloseHandle(CreateThread(
					NULL,
					0ULL,
					messageBox,
					nullptr,
					0UL,
					nullptr
				));
				
				break;
			}

			// Virtual desktop size. Depending on monitor.
			RECT vr = GetMonitorRectByIndex(desc.monitor_idx);

			// Set correct dimensions.
			data.Dimensions = { vr.right - vr.left, vr.bottom - vr.top };

			// Get correct positions
			POINT pt = { vr.left, vr.top };
			ScreenToClient(workerw, &pt);
			int childX = pt.x;
			int childY = pt.y;

			// Create a borderless window
			HWND hWnd = CreateWindowExA(
				WS_EX_TOOLWINDOW,
				WindowClass::GetName(),
				nullptr,
				WS_POPUP,
				childX,
				childY,
				vr.right - vr.left,
				vr.bottom - vr.top,
				nullptr,
				nullptr,
				WindowClass::GetInstance(),
				this
			);

			// Sanity check
			WINDOW_CHECK(hWnd);

			// Set workerW as parent window
			SetParent(hWnd, workerw);

			// Convert popup -> child after parenting
			LONG_PTR s = GetWindowLongPtrW(hWnd, GWL_STYLE);
			s &= ~WS_POPUP;
			s |= WS_CHILD;
			SetWindowLongPtrW(hWnd, GWL_STYLE, s);

			// Fit exactly to virtual screen; don't steal focus
			SetWindowPos(
				hWnd,
				nullptr,
				childX,
				childY,
				vr.right - vr.left,
				vr.bottom - vr.top,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED
			);
			break;
		}
	
		case WINDOW_DESC::WINDOW_MODE_NORMAL:
		default:
		{
			// Use specified dimensions
			data.Dimensions = desc.window_dim;

			//	Calculate window size based on desired client region size
			RECT wr;
			wr.left = 100;
			wr.right = desc.window_dim.x + wr.left;
			wr.top = 100;
			wr.bottom = desc.window_dim.y + wr.top;
			if (!AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_OVERLAPPEDWINDOW, FALSE))
				WINDOW_LAST_ERROR();

			//	Create Window & get hWnd
			HWND hWnd = CreateWindowExA(
				NULL,
				WindowClass::GetName(),
				NULL,
				WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				wr.right - wr.left,
				wr.bottom - wr.top,
				nullptr,
				nullptr,
				WindowClass::GetInstance(),
				this
			);

			//	Check for error
			WINDOW_CHECK(hWnd);
			break;
		}
	};

	//  Create graphics
	data.graphics = new Graphics(data.hWnd);

	//	Set title & dark theme
	setTitle(desc.window_title);
	if (!data.wallpaper)
		setDarkTheme(desc.dark_theme);

	//  Set icon if requested
	if (desc.icon_filename[0] != '\0')
		setIcon(desc.icon_filename);

	//	Create graphics buffers
	data.graphics->setWindowDimensions(data.Dimensions);
}

// Handles the proper deletion of the window data after its closing.

Window::~Window()
{
	WindowInternals& data = *((WindowInternals*)WindowData);
#ifdef _INCLUDE_IMGUI
	// If an imgui instance is bound to the window unbind it.
	if (data.imGui)
		data.imGui->unbind();
#endif
	delete data.graphics;

	// Detach from workerW before destroying
	if (data.wallpaper)
		SetParent(data.hWnd, nullptr);

	DestroyWindow(data.hWnd);

	// Redraw desktop background to avoid persistance
	if (data.wallpaper && !data.w_persist)
	{
		DwmFlush();
		SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, nullptr, SPIF_SENDCHANGE);
	}

	delete &data;
}

// Returs the window ID. 
// The one posted by process events when cluse button pressed.

unsigned Window::getID() const
{
	return w_id;
}

// Checks whether the window has focus.

bool Window::hasFocus() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);
	return (GetForegroundWindow() == data.hWnd);
}

// Sets the focus to the current window.

void Window::requestFocus()
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(!data.wallpaper,
		"Calling requestFocus() on a Wallpaper window is not allowed.\n"
		"Wallpaper windows do not get focus due to their nature.\n"
		"Please consider other means of interaction with Wallpaper windows."
	);

	SetFocus(data.hWnd);
}

// Set the title of the window, allows for formatted strings.

void Window::setTitle(const char* fmt_name, ...)
{
	USER_CHECK(fmt_name,
		"Found nullptr when trying to access the name to set the window title.\n"
		"Please send a valid const char* when setting the window title."
	);

	va_list ap;

	WindowInternals& data = *((WindowInternals*)WindowData);
	
	// Unwrap the format
	va_start(ap, fmt_name);
	if (vsnprintf(data.Name, 512, fmt_name, ap) < 0) return;
	va_end(ap);

	if (!IsWindow(data.hWnd))
		WINDOW_LAST_ERROR();
	if (!SetWindowTextA(data.hWnd, data.Name))
		WINDOW_LAST_ERROR();
}

// Sets the icon of the window via its filename (Has to be a .ico file).

void Window::setIcon(const char* filename)
{
	USER_CHECK(filename,
		"Found nullptr when trying to access the filename to set the window icon.\n"
		"Please send a valid file path when setting the window icon."
	);

	WindowInternals& data = *((WindowInternals*)WindowData);

	HANDLE hIcon = LoadImageA(0, filename, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE | LR_SHARED);
	if (hIcon) {
		//	Change both icons to the same icon handle
		SendMessage(data.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(data.hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		//	This will ensure that the application icon gets changed too
		SendMessage(GetWindow(data.hWnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(GetWindow(data.hWnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
	else WINDOW_LAST_ERROR();
}

// Sets the dimensions of the window to the ones specified.

void Window::setDimensions(Vector2i Dim)
{
	int width = Dim.x;
	int height = Dim.y;
	
	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(!data.wallpaper,
		"Calling setDimensions on a wallpaper window is not allowed.\n"
		"Please use setWallpaperMonitor to adjust wallpaper window."
	);

	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	if (!AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX | WS_OVERLAPPEDWINDOW, FALSE))
		WINDOW_LAST_ERROR();

	if (!SetWindowPos(data.hWnd, data.hWnd, 0, 0, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER))
		WINDOW_LAST_ERROR();
}

// Sets the position of the window to the one specified.

void Window::setPosition(Vector2i Pos)
{
	int X = Pos.x, Y = Pos.y;

	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(!data.wallpaper,
		"Calling setPosition on a wallpaper window is not allowed.\n"
		"Please use setWallpaperMonitor to adjust wallpaper window."
	);

	int eX = -8;
	int eY = -31;
	X += eX;
	Y += eY;

	if (!SetWindowPos(data.hWnd, data.hWnd, X, Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER))
		WINDOW_LAST_ERROR();
}

// Selects the monitor where the wallpaper will be shown. If -1
// then the wallpaper window will expand to all monitors in use.

void Window::setWallpaperMonitor(int monitor_idx)
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(data.wallpaper,
		"Calling setWallpaperMonitor on a non wallpaper window is not allowed.\n"
		"If you want to create a wallpaper window you must initialize it with wallpaper mode in the descriptor."
	);

	// Spawn the workerW
	HWND progman = FindWindowW(L"Progman", nullptr);
	WINDOW_CHECK(progman);

	// This function makes the workerW appear
	DWORD_PTR result = 0;
	if (!SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result))
		WINDOW_LAST_ERROR();

	// Find that workerW window
	HWND workerw = nullptr;
	EnumWindows(EnumWindowsFindWorkerW, (LPARAM)&workerw);
	WINDOW_CHECK(workerw);

	// Detach
	SetParent(data.hWnd, nullptr);

	// Update IDX
	data.monitor_idx = monitor_idx;

	// Get new rectangle
	RECT vr = GetMonitorRectByIndex(data.monitor_idx);

	// Set correct dimensions.
	data.Dimensions = { vr.right - vr.left, vr.bottom - vr.top };

	// Get correct positions
	POINT pt = { vr.left, vr.top };
	ScreenToClient(workerw, &pt);
	int childX = pt.x;
	int childY = pt.y;

	// Fit exactly to virtual screen; don't steal focus
	SetWindowPos(
		data.hWnd,
		nullptr,
		childX,
		childY,
		vr.right - vr.left,
		vr.bottom - vr.top,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED
	);

	// Reshape graphics buffers
	data.graphics->setWindowDimensions(data.Dimensions);

	// Flush and re-attach to clean last monitor
	DwmFlush();
	SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, nullptr, SPIF_SENDCHANGE);
	SetParent(data.hWnd, workerw);
}

// In the event of a failed Wallpaper creation the window fallsback to 
// regular. This function returns whether the window is in wallpaper mode.

bool Window::isWallpaperWindow() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return data.wallpaper;
}

// For the wallpaper mode, it tells you if a specific monitor 
// exists or not. Expand option, -1, is not considered a monitor.

bool Window::hasMonitor(int monitor_idx)
{
	MonitorPick mp = {};
	mp.indexWanted = monitor_idx;
	mp.indexNow = 0;
	mp.found = false;

	EnumDisplayMonitors(nullptr, nullptr, EnumMonProc, (LPARAM)&mp);
	return mp.found;
}

// Sets the maximum framerate of the widow to the one specified.

void Window::setFramerateLimit(int fps)
{
	WindowInternals::Frametime = 1.f / float(fps);
	WindowInternals::timer.setMax(fps);
}

// Toggles the dark theme of the window on or off as specified.

void Window::setDarkTheme(bool DARK_THEME)
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(!data.wallpaper,
		"Calling setDarkTheme on a wallpaper window is not allowed.\n"
		"Please use setWallpaperMonitor to adjust wallpaper window."
	);

	BOOL theme = DARK_THEME;
	if (FAILED(DwmSetWindowAttribute(data.hWnd, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, &theme, sizeof(BOOL))))
		WINDOW_LAST_ERROR();

	// Window dimensions wiggle to force and udate.
	setDimensions(data.Dimensions - Vector2i(1, 1));
	setDimensions(data.Dimensions + Vector2i(1, 1));
}

// Toggles the window on or off of full screen as specified.

void Window::setFullScreen(bool FULL_SCREEN)
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	USER_CHECK(!data.wallpaper,
		"Calling setFullScreen on a wallpaper window is not allowed.\n"
		"Please use setWallpaperMonitor to adjust wallpaper window."
	);

	DWORD dwStyle = GetWindowLong(data.hWnd, GWL_STYLE);
	if (dwStyle & WS_OVERLAPPEDWINDOW && FULL_SCREEN) {
		MONITORINFO mi = { sizeof(mi) };
		if (GetWindowPlacement(data.hWnd, &data.g_wpPrev) && GetMonitorInfo(MonitorFromWindow(data.hWnd,MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLongPtr(data.hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(data.hWnd, HWND_TOP,
				mi.rcMonitor.left, 
				mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED
			);
		}
	}
	else if (!FULL_SCREEN)
	{
		SetWindowLongPtr(data.hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		// Forces WM_SIZE message for reshaping
		SetWindowPos(
			data.hWnd, nullptr,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE |
			SWP_NOZORDER | SWP_NOOWNERZORDER |
			SWP_FRAMECHANGED
		);
		SetWindowPlacement(data.hWnd, &data.g_wpPrev);
	}
}

// Returns a string pointer to the title of the window.

const char* Window::getTitle() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return data.Name;
}

// Returns the dimensions vector of the window.

Vector2i Window::getDimensions() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return data.Dimensions;
}

// Returns the position vector of the window.

Vector2i Window::getPosition() const
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return data.Position;
}

// Returs the current framerate of the window.

float Window::getFramerate()
{
	return 1.f / WindowInternals::timer.average();
}

// Returns the HWND of the window.

void* Window::getWindowHandle()
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return data.hWnd;
}

// Waits for a certain amount of time to keep the window
// running stable at the desired framerate.

void Window::handleFramerate()
{
	if (WindowInternals::noFrameUpdate)
	{
		WindowInternals::noFrameUpdate = false;
		WindowInternals::frame = WindowInternals::timer.skip();
		return;
	}

	if (WindowInternals::timer.check() < WindowInternals::Frametime)
		Timer::sleep_for(unsigned long(1000 * (WindowInternals::Frametime - WindowInternals::timer.check())));

	WindowInternals::frame = WindowInternals::timer.mark();
}

#ifdef _INCLUDE_IMGUI
// Returns adress to the pointer to the iGManager bound to the window.
// This is to be accessed by the iGManager and set the data accordingly.

void** Window::imGuiPtrAdress()
{
	WindowInternals& data = *((WindowInternals*)WindowData);

	return (void**)&data.imGui;
}
#endif
