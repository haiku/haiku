#ifndef __beos_test_utils_h__
#define __beos_test_utils_h__

#include <string>
#include <SupportDefs.h>

#include <cppunit/Portability.h>
#include <cppunit/extensions/HelperMacros.h>

// Handy defines :-)
#define CHK CPPUNIT_ASSERT

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
extern CPPUNIT_API std::string IntToStr(int i);

// Calls system() with the concatenated string of command and parameter.
extern CPPUNIT_API void ExecCommand(const char *command, const char *parameter);

// Calls system() with the concatenated string of command, parameter1,
// " " and parameter2.
extern CPPUNIT_API void ExecCommand(const char *command, const char *parameter1,
									const char *parameter2);

// Calls system() with the given command (kind of silly, but it's consistent :-)
extern CPPUNIT_API void ExecCommand(const char *command);

#endif	// __beos_test_utils_h__
