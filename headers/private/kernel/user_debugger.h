/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 *
 * Userland debugger support.
 */
#ifndef _KERNEL_USER_DEBUGGER_H
#define _KERNEL_USER_DEBUGGER_H

#include <OS.h>

struct team_debug_info {
	int32		flags;
		// Set atomically. So reading atomically is OK, even when the team
		// lock is not held (at least if it is certain, that the team struct
		// won't go).
	team_id		debugger_team;
	port_id		debugger_port;
	thread_id	nub_thread;
	port_id		nub_port;
		// the port the nub thread is waiting on for commands from the debugger
};

struct thread_debug_info {
	int32		flags;
		// Set atomically. So reading atomically is OK, even when the thread
		// lock is not held (at least if it is certain, that the thread struct
		// won't go).
	port_id		debug_port;
		// the port the thread is waiting on for commands from the nub thread
};

// team debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_TEAM_DEBUG_DEBUGGER_INSTALLED	= 0x0001,

	B_TEAM_DEBUG_KERNEL_FLAG_MASK	= 0xffff,
};

// thread debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_THREAD_DEBUG_INITIALIZED		= 0x0001,
	B_THREAD_DEBUG_DYING			= 0x0002,
	B_THREAD_DEBUG_STOP				= 0x0004,
	B_THREAD_DEBUG_STOPPED			= 0x0008,

	B_THREAD_DEBUG_KERNEL_FLAG_MASK	= 0xffff,
};

// messages sent from the debug nub thread to a debugged thread
enum {
	B_DEBUGGED_THREAD_MESSAGE_CONTINUE	= 0,
};

typedef union {
} debugged_thread_message;


// Return value of user_debug_handle_signal(), telling the caller how to
// proceed.
enum {
	B_THREAD_DEBUG_IGNORE_SIGNAL,	// ignore the signal and continue
	B_THREAD_DEBUG_HANDLE_SIGNAL,	// handle the signal normally
};

#ifdef __cplusplus
extern "C" {
#endif

// service calls

void clear_team_debug_info(struct team_debug_info *info);
void destroy_team_debug_info(struct team_debug_info *info);

void clear_thread_debug_info(struct thread_debug_info *info,
		bool dying);
void destroy_thread_debug_info(struct thread_debug_info *info);

void user_debug_pre_syscall(uint32 syscall, void *args);
void user_debug_post_syscall(uint32 syscall, void *args, uint64 returnValue,
		bigtime_t startTime);

uint32 user_debug_handle_signal(int signal, bool deadly);
void user_debug_stop_thread();


// syscalls

void		_user_debugger(const char *message);
int			_user_disable_debugger(int state);

status_t	_user_install_default_debugger(port_id debuggerPort);
port_id		_user_install_team_debugger(team_id team, port_id debuggerPort);
status_t	_user_remove_team_debugger(team_id team);
status_t	_user_debug_thread(thread_id thread);


#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_USER_DEBUGGER_H
