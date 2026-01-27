#pragma once
#include "Header.h"

/* MOUSE CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
In order to develop user interaction in your apps it is important to have access
to the message pipeline coming into your window. For that reason this mouse
abstraction class stores the mouse events via the MSGHandlePipeline and gives
simple access to the user to those events.

It has a button state that keeps track of which mouse buttons are being pressed, 
and an event buffer to store the events coming from the message pipeline and be 
processed by your application.

It also stores a the mouse position relative to the window and to the screen.
And has a buffer that stores the mouse wheel movement, it can be accessed and 
reset by the user.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Mouse static class, contains a set of user functions to create interactions between
// the mouse and your app. Stores events, button states and positions and lets the user 
// access them via the public functions.
class Mouse 
{
	// Needs acces to the Mouse to push events from the message pipeline.
	friend class MSGHandlePipeline;
public:
	// Mouse buttons enumerator.
	enum Button: unsigned
	{
		Left	= 0u,	// Left mouse button
		Right	= 1u,	// Right mouse buttos
		Middle	= 2u,	// Middle mouse button
		None	= 3u,	// No mouse button (used in move events)
	};

	// Mouse event class that stores the event type, button, 
	// and mouse position when the event was stored.
	class event 
	{
	public:
		// Mouse event type enumerator.
		enum Type: unsigned
		{
			Pressed,	// Button pressed event
			Released,	// Button released event
			Moved,		// Mouse moved event
			Wheel,		// Wheel moved event
			Invalid		// Invalid event
		};

		Vector2i position	= {};		// Stores the mouse position during event
		Type type			= Invalid;	// Stores the event type
		Button button		= None;		// Stores the event button
	};

private:
	static constexpr unsigned int maxBuffer = 64u;	// Maximum amount of events stored on buffer.
	static constexpr unsigned int nButtons = 4u;	// Total number of buttons stored by Mouse.

	static inline bool buttonStates[nButtons] = { false };		// Stores the state of each button, true if pressed.
	static inline event* buttonBuffer[maxBuffer] = { nullptr };	// Buffer of the events pushed to the mouse.
	static inline Vector2i Position = {};						// Current mouse position relative to window.
	static inline Vector2i ScPosition = {};						// Current mouse position relative to screen.
	static inline int deltaWheel = 0;							// Stores the movement of the mouse wheel.

	// Internal function triggered by the MSG Handle to set a button as pressed.
	static void setButtonPressed(Button button);

	// Internal function triggered by the MSG Handle to set a button as released.
	static void setButtonReleased(Button button);

	// Internal function triggered by the MSG Handle to set the current position relative to the window.
	static void setPosition(Vector2i Position);

	// Internal function triggered by the MSG Handle to set the current position relative to the screen.
	static void setScPosition(Vector2i Position);

	// Internal function triggered by the MSG Handle to add to the mouse wheel movement.
	static void increaseWheel(int delta);

	// Internal function triggered by the MSG Handle to push an event to the buffer.
	static void pushEvent(event::Type type, Button button);

public:
	// Sets the wheel movement back to 0. To be called after reading the wheel value.
	static void resetWheel();

	// Returns the current mouse wheel movement value. Does not reset it.
	static int getWheel();

	// Returns the current mouse position in pixels relative to the window.
	static Vector2i getPosition();

	// Returns the current mouse position in pixels relative to the screen.
	static Vector2i getScPosition();

	// Checks whether a button is being pressed.
	static bool isButtonPressed(Button button);

	// Clears the mouse event buffer.
	static void clearBuffer();

	// Checks whether the event buffer is empty.
	static bool eventIsEmpty();

	// Pops last event of the buffer. To be used by an application event manager.
	static event popEvent();
};
