/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"

#include <stdio.h>
#include <stdlib.h>


void
debugger(const char *message)
{
	// ToDo: implement debugger() properly
	puts("*** debugger call - not yet implemented ***");
	printf("%s\n", message);

	abort();
}


const int
disable_debugger(int state)
{
	// ToDo: implement disable_debugger()
	return 0;
}


// relating to Debug.h
// TODO: verify these functions
// TODO: add implementations for printfs

#include <Debug.h>

bool _rtDebugFlag = false;

bool
_debugFlag(void)
{
	return _rtDebugFlag;
}


bool
_setDebugFlag(bool flag)
{
	bool previous = _rtDebugFlag;
	_rtDebugFlag = flag;
	return previous;
}

	
int
_debugPrintf(const char * message, ...)
{
	puts("*** _debugPrintf call - not yet implemented ***");
	printf("%s\n", message);
	return 0;
}


int
_sPrintf(const char * message, ...)
{
	puts("*** _sPrintf call - not yet implemented ***");
	printf("%s\n", message);
	return 0;
}


int
_xdebugPrintf(const char * message, ...)
{
	puts("*** _xdebugPrintf call - not yet implemented ***");
	printf("%s\n", message);
	return 0;
}


int
_debuggerAssert(const char * file, int line, char * message)
{
	puts("*** _debuggerAssert call - not yet implemented ***");
	printf("%s:%d:%s\n", file, line, message);
	return 0;
}

// TODO: Remove. Temporary debug helper.

void
debug_printf(const char *format, ...)
{
	va_list list;
	va_start(list, format);

	debug_vprintf(format, list);

	va_end(list);
}

void
debug_vprintf(const char *format, va_list args)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);

	_kern_debug_output(buffer);
}


