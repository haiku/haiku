/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <debugger.h>
#include <OS.h>
#include "syscalls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char *const sDebugWhyStrings[] = {
	"Thread not running",		// B_THREAD_NOT_RUNNING
	"Signal received",			// B_SIGNAL_RECEIVED
	"Team created",				// B_TEAM_CREATED
	"Thread created",			// B_THREAD_CREATED
	"Image created",			// B_IMAGE_CREATED
	"Image deleted",			// B_IMAGE_DELETED
	"Debugger call",			// B_DEBUGGER_CALL
	"Breakpoint hit",			// B_BREAKPOINT_HIT
	"watchpoint hit",			// B_WATCHPOINT_HIT
	"Before syscall",			// B_PRE_SYSCALL_HIT
	"After syscall",			// B_POST_SYSCALL_HIT
	"Single step",				// B_SINGLE_STEP
};
static const int32 sDebugWhyStringCount = B_SINGLE_STEP + 1;

static const char *const sDebugExceptionTypeStrings[] = {
	"Non-maskable interrupt",	// B_NMI
	"Machine check exception",	// B_MACHINE_CHECK_EXCEPTION
	"Segment violation",		// B_SEGMENT_VIOLATION
	"Alignment exception",		// B_ALIGNMENT_EXCEPTION
	"Divide error",				// B_DIVIDE_ERROR
	"Overflow exception",		// B_OVERFLOW_EXCEPTION
	"Bounds check exception",	// B_BOUNDS_CHECK_EXCEPTION
	"Invalid opcode exception",	// B_INVALID_OPCODE_EXCEPTION
	"Segment not present",		// B_SEGMENT_NOT_PRESENT
	"Stack fault",				// B_STACK_FAULT
	"General protection fault",	// B_GENERAL_PROTECTION_FAULT
	"Floating point exception",	// B_FLOATING_POINT_EXCEPTION
};
static const int32 sDebugExceptionTypeStringCount
	= B_FLOATING_POINT_EXCEPTION + 1;


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


/**	\brief Suspends the thread until a debugger has been installed for this
 *		   team.
 *
 *	As soon as this happens (immediately, if a debugger is already installed)
 *	the thread stops for debugging. This is desirable for debuggers that spawn
 *	their debugged teams via fork() and want the child to wait till they have
 *	installed themselves as team debugger before continuing with exec*().
 */
void
wait_for_debugger(void)
{
	_kern_wait_for_debugger();
}


static void
get_debug_string(const char *const *strings, int32 stringCount,
	const char *defaultString, int32 index, char *buffer, int32 bufferSize)
{
	if (!buffer || bufferSize <= 0)
		return;

	if ((int32)index >= 0 && (int32)index < stringCount)
		strlcpy(buffer, strings[index], bufferSize);
	else
		snprintf(buffer, bufferSize, defaultString, index);
}


void
get_debug_why_stopped_string(debug_why_stopped whyStopped, char *buffer,
	int32 bufferSize)
{
	get_debug_string(sDebugWhyStrings, sDebugWhyStringCount,
		"Unknown reason %ld", (int32)whyStopped, buffer, bufferSize);
}

void
get_debug_exception_string(debug_exception_type exception, char *buffer,
	int32 bufferSize)
{
	get_debug_string(sDebugExceptionTypeStrings, sDebugExceptionTypeStringCount,
		"Unknown exception %ld", (int32)exception, buffer, bufferSize);
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

	if (ret < 0)
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


