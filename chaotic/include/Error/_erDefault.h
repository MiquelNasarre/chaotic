#pragma once
#include "Error/ChaoticError.h"

/* USER ERROR CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the default error class and user check, used by the 
library when no internal error has ocurred. Mostly used for user driven 
errors due to failed API conditions.

Contains the line, file and a description of the error, as every other 
error funnels through the defalut error pipeline.

It can also be used by the user own developed apps, but given the conditions
outlined in the ChaoticError header, any expression inside a USER_CHEK must 
not modify internal variable because it might be nullyfied by the end user.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Default user error macro used for user error calls across the library. 
// This is used if no specific condition has been checked on the path or 
// if the condition check is necessary for normal library runtime.
#define USER_ERROR(_MSG)			CHAOTIC_FATAL(UserError(__LINE__, __FILE__, _MSG))

// Default user check macro used along the library for non system errors.
// Can also be used by the user when designing their own apps.
#define USER_CHECK(_EXPR, _MSG)		CHAOTIC_CHECK(_EXPR, UserError(__LINE__, __FILE__, _MSG))

// Basic Error class, stores the given information and adds it
// to the whatBuffer when the what() function is called.
class UserError : public ChaoticError
{
public:
	// Single message constructor, the message is stored in the info.
	UserError(int line, const char* file, const char* msg) noexcept
		: ChaoticError(line, file)
	{
		unsigned c = 0u;

		// Add intro to information.
		const char* intro = "\n[Error Info]\n";
		unsigned i = 0u;
		while (intro[i])
			info[c++] = intro[i++];

		// copy message into info.
		i = 0u;
		while (msg[i] && c < 2047)
			info[c++] = msg[i++];

		// Add origin location.
		i = 0u;
		while (origin[i] && c < 2047)
			info[c++] = origin[i++];

		// Add final EOS
		info[c] = '\0';
	}

	// Info Error type override.
	const char* GetType() const noexcept override { return "Default User Error"; }
};
