#ifndef __beos_test_utils_h__
#define __beos_test_utils_h__

#include <string>
#include <SupportDefs.h>

// Handy defines :-)
#define CHK CPPUNIT_ASSERT
#define RES DecodeResult

// Prints out a description of the given status_t
// return code to standard out. Helpful for figuring
// out just what the R5 libraries are returning.
// Returns the same value passed in, so you can
// use it inline in tests if necessary.
status_t DecodeResult(status_t result);

// First parameter is equal to the second or third
template<typename A, typename B, typename C>
static
inline
bool
Equals(const A &a, const B &b, const C &c)
{
	return (a == b || a == c);
}

// Returns a string version of the given integer
string IntToStr(int i);

// Calls system() with the concatenated string of command and parameter.
void ExecCommand(const char *command, const char *parameter);

// Calls system() with the concatenated string of command, parameter1,
// " " and parameter2.
void ExecCommand(const char *command, const char *parameter1,
							const char *parameter2);

// Calls system() with the given command (kind of silly, but it's consistent :-)
void ExecCommand(const char *command);
	
#endif	// __beos_test_utils_h__
