/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <debugger.h>
#include <OS.h>
#include "syscalls.h"

#include <stdio.h>
#include <stdlib.h>


void
debugger(const char *message)
{
	_kern_debugger(message);
}


const int
disable_debugger(int state)
{
	return _kern_disable_debugger(state);
}


status_t
install_default_debugger(port_id debuggerPort)
{
	return _kern_install_default_debugger(debuggerPort);
}


port_id
install_team_debugger(team_id team, port_id debuggerPort)
{
	return _kern_install_team_debugger(team, debuggerPort);
}


status_t
remove_team_debugger(team_id team)
{
	return _kern_remove_team_debugger(team);
}


status_t
debug_thread(thread_id thread)
{
	return _kern_debug_thread(thread);
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
_debugPrintf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	// TODO : we need locking here

	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
}


int
_sPrintf(const char *fmt, ...)
{
	va_list ap;
	int ret;
	char buffer[256]; // seems to be the size used in R5

	// TODO : we need locking here
	
	va_start(ap, fmt);
	ret = vsnprintf(buffer, 256, fmt, ap);
	va_end(ap);

	if (!ret)
		return ret;

	_kern_debug_output(buffer);	

	return 0;
}


int
_xdebugPrintf(const char * fmt, ...)
{
	va_list ap;
	int ret;
	
	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
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


