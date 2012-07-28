/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <debugger.h>
#include <OS.h>
#include <Debug.h>
#include "syscalls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct debug_string_entry {
	const char	*string;
	uint32		code;
} debug_string_entry;

static const debug_string_entry sDebugMessageStrings[] = {
	{ "Thread not running",	B_DEBUGGER_MESSAGE_THREAD_DEBUGGED },
	{ "Debugger call",		B_DEBUGGER_MESSAGE_DEBUGGER_CALL },
	{ "Breakpoint hit",		B_DEBUGGER_MESSAGE_BREAKPOINT_HIT },
	{ "Watchpoint hit",		B_DEBUGGER_MESSAGE_WATCHPOINT_HIT },
	{ "Single step",		B_DEBUGGER_MESSAGE_SINGLE_STEP },
	{ "Before syscall",		B_DEBUGGER_MESSAGE_PRE_SYSCALL },
	{ "After syscall",		B_DEBUGGER_MESSAGE_POST_SYSCALL },
	{ "Signal received",	B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED },
	{ "Exception occurred",	B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED },
	{ "Team created",		B_DEBUGGER_MESSAGE_TEAM_CREATED },
	{ "Team deleted",		B_DEBUGGER_MESSAGE_TEAM_DELETED },
	{ "Thread created",		B_DEBUGGER_MESSAGE_THREAD_CREATED },
	{ "Thread created",		B_DEBUGGER_MESSAGE_THREAD_DELETED },
	{ "Image created",		B_DEBUGGER_MESSAGE_IMAGE_CREATED },
	{ "Image deleted",		B_DEBUGGER_MESSAGE_IMAGE_DELETED },
	{ NULL, 0 }
};

static const debug_string_entry sDebugExceptionTypeStrings[] = {
	{ "Non-maskable interrupt",		B_NON_MASKABLE_INTERRUPT },
	{ "Machine check exception",	B_MACHINE_CHECK_EXCEPTION },
	{ "Segment violation",			B_SEGMENT_VIOLATION },
	{ "Alignment exception",		B_ALIGNMENT_EXCEPTION },
	{ "Divide error",				B_DIVIDE_ERROR },
	{ "Overflow exception",			B_OVERFLOW_EXCEPTION },
	{ "Bounds check exception",		B_BOUNDS_CHECK_EXCEPTION },
	{ "Invalid opcode exception",	B_INVALID_OPCODE_EXCEPTION },
	{ "Segment not present",		B_SEGMENT_NOT_PRESENT },
	{ "Stack fault",				B_STACK_FAULT },
	{ "General protection fault",	B_GENERAL_PROTECTION_FAULT },
	{ "Floating point exception",	B_FLOATING_POINT_EXCEPTION },
	{ NULL, 0 }
};

bool _rtDebugFlag = true;


void
debugger(const char *message)
{
	debug_printf("%" B_PRId32 ": DEBUGGER: %s\n", find_thread(NULL), message);
	_kern_debugger(message);
}


int
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


status_t
set_debugger_breakpoint(void *address)
{
	return _kern_set_debugger_breakpoint(address, 0, 0, false);
}


status_t
clear_debugger_breakpoint(void *address)
{
	return _kern_clear_debugger_breakpoint(address, false);
}


status_t
set_debugger_watchpoint(void *address, uint32 type, int32 length)
{
	return _kern_set_debugger_breakpoint(address, type, length, true);
}


status_t
clear_debugger_watchpoint(void *address)
{
	return _kern_clear_debugger_breakpoint(address, true);
}


static void
get_debug_string(const debug_string_entry *stringEntries,
	const char *defaultString, uint32 code, char *buffer, int32 bufferSize)
{
	int i;

	if (!buffer || bufferSize <= 0)
		return;

	for (i = 0; stringEntries[i].string; i++) {
		if (stringEntries[i].code == code) {
			strlcpy(buffer, stringEntries[i].string, bufferSize);
			return;
		}
	}

	snprintf(buffer, bufferSize, defaultString, code);
}


void
get_debug_message_string(debug_debugger_message message, char *buffer,
	int32 bufferSize)
{
	get_debug_string(sDebugMessageStrings, "Unknown message %lu",
		(uint32)message, buffer, bufferSize);
}


void
get_debug_exception_string(debug_exception_type exception, char *buffer,
	int32 bufferSize)
{
	get_debug_string(sDebugExceptionTypeStrings, "Unknown exception %lu",
		(uint32)exception, buffer, bufferSize);
}


//	#pragma mark - Debug.h functions


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

	if (!_rtDebugFlag)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
}


int
_sPrintf(const char *fmt, ...)
{
	char buffer[512];
	va_list ap;
	int ret;

	if (!_rtDebugFlag)
		return 0;

	va_start(ap, fmt);
	ret = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if (ret >= 0)
		_kern_debug_output(buffer);	

	return ret;
}


int
_xdebugPrintf(const char *fmt, ...)
{
	va_list ap;
	int ret;
	
	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);

	return ret;
}


int
_debuggerAssert(const char *file, int line, const char *message)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer),
		"Assert failed: File: %s, Line: %d, %s",
		file, line, message);

	debug_printf("%" B_PRId32 ": ASSERT: %s:%d %s\n", find_thread(NULL), file,
		line, buffer);
	_kern_debugger(buffer);

	return 0;
}

// TODO: Remove. Temporary debug helper.
// (accidently these are more or less the same as _sPrintf())

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


void
ktrace_printf(const char *format, ...)
{
	va_list list;
	va_start(list, format);

	ktrace_vprintf(format, list);

	va_end(list);
}


void
ktrace_vprintf(const char *format, va_list args)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);

	_kern_ktrace_output(buffer);
}
