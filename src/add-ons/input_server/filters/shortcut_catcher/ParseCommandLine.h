/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#ifndef ParseCommandString_h
#define ParseCommandString_h

#include <String.h>

// Utility methods to extract arguments from a typed-in string.

// Returns an NULL-terminated argv array. Sets (setArgc) to the number of 
// valid arguments in the array. It becomes the responsibility of the calling
// code to delete[] the array and each string in it. If (padFront > 0), then 
// (padFront) extra "slots" will be allocated at the beginning of the returned
// argv array. These slots will be NULL, and will be counted in the (setArgc)
// result. It is the caller's responsibility to fill them...
char** ParseArgvFromString(const char* string, int32& setArgc);

// Call this to free the argv array returned by ParseArgvFromString().
char** CloneArgv(char** argv);

// Call this to free the argv array returned by ParseArgvFromString().
void FreeArgv(char** argv);

// Returns the first argument in the string.
BString ParseArgvZeroFromString(const char* string);

// Calls EscapeChars() on (string) for the following characters: backslash, 
// single quote, double quote, space, and tab. The returns string should be 
// parsable as a single word by the functions above.
bool DoStandardEscapes(BString& string);

// Modifies (string) by inserting slashes in front of each instance of badChar.
// Returns false iff no modifications were done.
bool EscapeChars(BString& string, char badChar);

// Launch an app, Tracker style. Put here so that it can be shared amongst 
// my various apps... 
status_t LaunchCommand(char** argv, int32 argc);

#endif
