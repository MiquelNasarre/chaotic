#include "Keyboard.h"

/*
--------------------------------------------------------------------------------------------
 Keyboard Internal Function Defintions
--------------------------------------------------------------------------------------------
*/

// Internal function triggered by the MSG Handle to set a key as pressed.

void Keyboard::setKeyPressed(char keycode)
{
	keyStates[keycode] = true;
}

// Internal function triggered by the MSG Handle to set a key as released.

void Keyboard::setKeyReleased(char keycode)
{
	keyStates[keycode] = false;
}

// Internal function triggered by the MSG Handle to clear all key states.

void Keyboard::clearKeyStates()
{
	for (unsigned int i = 0; i < nKeys; i++)
		keyStates[i] = false;
}

// Internal function triggered by the MSG Handle to push a character to the buffer.

void Keyboard::pushChar(char character)
{
	unsigned int n = maxBuffer - 1u;
	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!charBuffer[i])
		{
			n = i;
			break;
		}
	}

	if (n == maxBuffer - 1u)
	{
		delete charBuffer[0];
		for (unsigned int i = 0; i < maxBuffer - 1u; i++)
			charBuffer[i] = charBuffer[i + 1];
	}

	charBuffer[n] = new char(character);
}

// Internal function triggered by the MSG Handle to push an event to the buffer.

void Keyboard::pushEvent(event::Type type, char keycode)
{
	unsigned int n = maxBuffer - 1u;
	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!keyBuffer[i])
		{
			n = i;
			break;
		}
	}

	if (n == maxBuffer - 1u)
	{
		delete keyBuffer[0];
		for (unsigned int i = 0; i < maxBuffer - 1u; i++)
			keyBuffer[i] = keyBuffer[i + 1];
	}

	keyBuffer[n] = new event(type, keycode);
}

/*
--------------------------------------------------------------------------------------------
 Keyboard User Function Definitions
--------------------------------------------------------------------------------------------
*/

// Toggles the autorepeat behavior on or off as specified.

void Keyboard::setAutorepeat(bool state)
{
	autoRepeat = state;
}

// Returns the current autorepeat behavior. (Default On)

bool Keyboard::getAutorepeat()
{
	return autoRepeat;
}

// Clears the character and event buffers.

void Keyboard::clearBuffers()
{
	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!charBuffer[i])
			break;
		delete charBuffer[i];
		charBuffer[i] = nullptr;
	}

	for (unsigned int i = 0; i < maxBuffer; i++)
	{
		if (!keyBuffer[i])
			break;
		delete keyBuffer[i];
		keyBuffer[i] = nullptr;
	}
}

// Checks whether a key is being pressed.

bool Keyboard::isKeyPressed(char keycode)
{
	return keyStates[keycode];
}

// Checks whether the char buffer is empty.

bool Keyboard::charIsEmpty()
{
	if (!charBuffer[0])
		return true;
	return false;
}

// Checks whether the event buffer is empty.

bool Keyboard::eventIsEmpty()
{
	if (!keyBuffer[0])
		return true;
	return false;
}

// Pops last character of the buffer. To be used by an application event manager.

char Keyboard::popChar()
{
	if (!charBuffer[0])
		return 0;

	char ev = *charBuffer[0];
	delete charBuffer[0];

	for (unsigned int i = 0; i < maxBuffer - 1u; i++)
		charBuffer[i] = charBuffer[i + 1];
	charBuffer[maxBuffer - 1u] = nullptr;

	return ev;
}

// Pops last event of the buffer. To be used by an application event manager.

Keyboard::event Keyboard::popEvent()
{
	if (!keyBuffer[0])
		return event();

	event ev = *keyBuffer[0];
	delete keyBuffer[0];

	for (unsigned int i = 0; i < maxBuffer - 1u; i++)
		keyBuffer[i] = keyBuffer[i + 1];
	keyBuffer[maxBuffer - 1u] = nullptr;

	return ev;
}
