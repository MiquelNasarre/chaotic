#include "Mouse.h"

/*
--------------------------------------------------------------------------------------------
 Mouse Internal Function Defintions
--------------------------------------------------------------------------------------------
*/

// Internal function triggered by the MSG Handle to set a button as pressed.

void Mouse::setButtonPressed(Button button)
{
	buttonStates[button] = true;
}

// Internal function triggered by the MSG Handle to set a button as released.

void Mouse::setButtonReleased(Button button)
{
	buttonStates[button] = false;
}

// Internal function triggered by the MSG Handle to set the current position relative to the window.

void Mouse::setPosition(Vector2i position)
{
	Position = position;
}

// Internal function triggered by the MSG Handle to set the current position relative to the screen.

void Mouse::setScPosition(Vector2i position)
{
	ScPosition = position;
}

// Internal function triggered by the MSG Handle to add to the mouse wheel movement.

void Mouse::increaseWheel(int delta)
{
	deltaWheel += delta;
}

// Internal function triggered by the MSG Handle to push an event to the buffer.

void Mouse::pushEvent(event::Type type, Button button)
{
	unsigned int n = maxBuffer - 1u;
	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!buttonBuffer[i])
		{
			n = i;
			break;
		}
	}

	if (n == maxBuffer - 1u)
	{
		delete buttonBuffer[0];
		for (unsigned int i = 0; i < maxBuffer - 1u; i++)
			buttonBuffer[i] = buttonBuffer[i + 1];
	}

	buttonBuffer[n] = new event(Position, type, button);
}

/*
--------------------------------------------------------------------------------------------
 Keyboard User Function Definitions
--------------------------------------------------------------------------------------------
*/

// Sets the wheel movement back to 0. To be called after reading the wheel value.

void Mouse::resetWheel()
{
	deltaWheel = 0;
}

// Returns the current mouse wheel movement value. Does not reset it.

int Mouse::getWheel()
{
	int d = deltaWheel;
	deltaWheel = 0;
	return d;
}

// Returns the current mouse position in pixels relative to the window.

Vector2i Mouse::getPosition()
{
	return Position;
}

// Returns the current mouse position in pixels relative to the screen.

Vector2i Mouse::getScPosition()
{
	return ScPosition;
}

// Checks whether a button is being pressed.

bool Mouse::isButtonPressed(Button button)
{
	return buttonStates[button];
}

// Clears the mouse event buffer.

void Mouse::clearBuffer()
{
	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!buttonBuffer[i])
			break;
		delete buttonBuffer[i];
		buttonBuffer[i] = nullptr;
	}

}

// Checks whether the event buffer is empty.

bool Mouse::eventIsEmpty()
{
	if (!buttonBuffer[0])
		return true;
	return false;
}

// Pops last event of the buffer. To be used by an application event manager.

Mouse::event Mouse::popEvent()
{
	if (!buttonBuffer[0])
		return event();

	event ev = *buttonBuffer[0];
	delete buttonBuffer[0];

	for (unsigned int i = 0; i < maxBuffer - 1u; i++)
		buttonBuffer[i] = buttonBuffer[i + 1];
	buttonBuffer[maxBuffer - 1u] = nullptr;

	return ev;
}
