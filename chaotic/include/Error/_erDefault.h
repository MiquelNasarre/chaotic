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
#if defined _DEPLOYMENT && defined SOLUTION_DIR
#define USER_ERROR(_MSG)			CHAOTIC_FATAL(UserError(__LINE__, __FILE__ + sizeof(SOLUTION_DIR) - 1, _MSG))
#else
#define USER_ERROR(_MSG)			CHAOTIC_FATAL(UserError(__LINE__, __FILE__, _MSG))
#endif

// Default user check macro used along the library for non system errors.
// Can also be used by the user when designing their own apps.
#if defined _DEPLOYMENT && defined SOLUTION_DIR
#define USER_CHECK(_EXPR, _MSG)		CHAOTIC_CHECK(_EXPR, UserError(__LINE__, __FILE__ + sizeof(SOLUTION_DIR) - 1, _MSG))
#else
#define USER_CHECK(_EXPR, _MSG)		CHAOTIC_CHECK(_EXPR, UserError(__LINE__, __FILE__, _MSG))
#endif

// Basic Error class, stores the given information and adds it
// to the whatBuffer when the what() function is called.
class UserError : public ChaoticError
{
public:
	// Single message constructor, the message is stored in the info.
	UserError(int line, const char* file, const char* msg) noexcept;

	// Info Error type override.
	const char* GetType() const noexcept override { return "Default User Error"; }
};
