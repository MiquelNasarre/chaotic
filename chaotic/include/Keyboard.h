#pragma once

/* KEYBOARD CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
In order to develop user interaction in your apps it is important to have access
to the message pipeline coming into your window. For that reason this keyboard 
abstraction class stores the keyboard events via the MSGHandlePipeline and gives
simple access to the user to those events.

It has a character buffer, to be easily used for text applications, an event buffer
and a key state array that stores the current state of any of the keys.

The keyCode used by this class is derived from the wParam of the Win32 keyboard messages:
- For key events, keyCodes are the values defined by the Win32 VK_* constants.

- For alphanumeric keys, the virtual-key codes match their uppercase ASCII values:
	'0'–'9' -> codes 0x30–0x39
	'A'–'Z' -> codes 0x41–0x5A
  So you can write Keyboard::isKeyPressed('M') or check for event::keyCode=='3', etc.

- For non-character keys (function keys, arrows, etc.), keyCode corresponds to the 
  appropriate Win32 virtual-key code (VK_F1, VK_LEFT, VK_ESCAPE, …) truncated to char. 
  These values do not correspond to ASCII characters. Check Win32 references if you 
  want to implement non alpha-numeric keys:
  https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Keyboard static class, contains a set of user functions to create interactions between
// the keyboard and your app. Stores events, characters and key states and lets the user 
// access them via the public functions.
class Keyboard 
{
	// Needs acces to the keyboard to push events from the message pipeline.
	friend class MSGHandlePipeline;
public:
	// Keyboard event class that stores the event type and key.
	class event 
	{
	public:
		// Keyboard event type enumerator.
		enum Type: unsigned char
		{
			Pressed,	// Key pressed event
			Released,	// Key released event
			Invalid		// Invalid type event
		};

		Type type		= Invalid;	// Stores the event type
		char keyCode	= '\0';		// Stores the event key
	};

private:
	static constexpr unsigned maxBuffer = 64u;	// Maximum amount of events stored on buffer.
	static constexpr unsigned nKeys = 256u;		// Total number of keys stored by Keyboard.

	static inline bool autoRepeat = true;						// Whether the keyboard is on autorepead mode.
	static inline bool keyStates[nKeys] = { false };			// Stores the state of each key, true if pressed.
	static inline char* charBuffer[maxBuffer] = { nullptr };	// Buffer of characters pushed to the keyboard.
	static inline event* keyBuffer[maxBuffer] = { nullptr };	// Buffer of the events pushed to the keyboard.

	// Internal function triggered by the MSG Handle to set a key as pressed.
	static void setKeyPressed(char keycode);

	// Internal function triggered by the MSG Handle to set a key as released.
	static void setKeyReleased(char keycode);

	// Internal function triggered by the MSG Handle to clear all key states.
	static void clearKeyStates();

	// Internal function triggered by the MSG Handle to push a character to the buffer.
	static void pushChar(char character);

	// Internal function triggered by the MSG Handle to push an event to the buffer.
	static void pushEvent(event::Type type, char keycode);

public:
	// Toggles the autorepeat behavior on or off as specified.
	static void setAutorepeat(bool state);

	// Returns the current autorepeat behavior. (Default On)
	static bool getAutorepeat();

	// Clears the character and event buffers.
	static void clearBuffers();

	// Checks whether a key is being pressed.
	static bool isKeyPressed(char keycode);

	// Checks whether the char buffer is empty.
	static bool charIsEmpty();

	// Checks whether the event buffer is empty.
	static bool eventIsEmpty();

	// Pops last character of the buffer. To be used by an application event manager.
	static char popChar();

	// Pops last event of the buffer. To be used by an application event manager.
	static event popEvent();
};