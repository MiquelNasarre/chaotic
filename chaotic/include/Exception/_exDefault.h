#pragma once
#include "Exception/Exception.h"

/* DEFAULT EXCEPTION CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the default exception class thrown by the
library when no specific exception is being thrown.

Contains the line and file and a description of the excetion that
can be entered as a single string or as a list of strings.
For user created exceptions please use this one.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#define INFO_EXCEPT(info)	InfoException(__LINE__, __FILE__, (info))

// Basic Exception class, stores the given information and adds it
// to the whatBuffer when the what() function is called.
class InfoException : public Exception
{
public:
	// Single message constructor, the message is stored in the info.
	InfoException(int line, const char* file, const char* msg) noexcept
		: Exception(line, file)
	{
		unsigned c = 0u;

		// Add intro to information.
		const char* intro = "\n[Error Info]\n";
		unsigned i = 0u;
		while (intro[i])
			info[c++] = intro[i++];

		// join all info messages with newlines into single string.
		i = 0u;
		while (msg[i] && c < 2047)
			info[c++] = msg[i++];

		if (c < 2047)
			info[c++] = '\n';

		// Add origin location.
		const char* origin = GetOriginString();
		i = 0u;
		while (origin[i] && c < 2047)
			info[c++] = origin[i++];

		// Add final EOS
		info[c] = '\0';
	}

	// Multiple messages constructor, the messages are stored in the info.
	InfoException(int line, const char* file, const char** infoMsgs = nullptr) noexcept
		:Exception(line, file)
	{
		unsigned i, j, c = 0u;

		// Add intro to information.
		const char* intro = "\n[Error Info]\n";
		i = 0u;
		while (intro[i] && c < 2047)
			info[c++] = intro[i++];

		// join all info messages with newlines into single string.
		i = 0u;
		while (infoMsgs[i])
		{
			j = 0u;
			while (infoMsgs[i][j] && c < 2047)
				info[c++] = infoMsgs[i][j++];

			if(c < 2047)
				info[c++]= '\n';
		}

		// Add origin location.
		const char* origin = GetOriginString();
		i = 0u;
		while (origin[i] && c < 2047)
			info[c++] = origin[i++];

		// Add final EOS
		info[c] = '\0';
	}

	// Info Exception type override.
	const char* GetType() const noexcept override { return "Graphics Info Exception"; }
};
