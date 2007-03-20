/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 *
 * Userland debugger support.
 */
#ifndef _KERNEL_USER_DEBUGGER_H
#define _KERNEL_USER_DEBUGGER_H

#include <debugger.h>

#include <arch/user_debugger.h>

// Team related debugging data.
//
// Locking policy:
// 1) When accessing the structure it must be made sure, that the structure,
//    (i.e. the struct team it lives in) isn't deleted. Thus one either needs to
//    acquire the global team lock, or one accesses the structure from a thread
//    of that team.
// 2) Access to the `flags' field is atomically. Reading via atomic_get()
//    requires no further locks (in addition to 1) that is). Writing requires
//    `lock' being held and must be done atomically, too
//    (atomic_{set,and,or}()). Reading with `lock' being held doesn't need to
//    be done atomically.
// 3) Access to all other fields (read or write) requires `lock' being held.
//
struct team_debug_info {
	spinlock	lock;
		// Guards the remaining fields. Should always be the innermost lock
		// to be acquired/released.

	int32		flags;
		// Set atomically. So reading atomically is OK, even when the team
		// lock is not held (at least if it is certain, that the team struct
		// won't go).

	team_id		debugger_team;
	port_id		debugger_port;
	thread_id	nub_thread;
	port_id		nub_port;
		// the port the nub thread is waiting on for commands from the debugger
	sem_id		debugger_write_lock;
		// synchronizes writes to the debugger port with the setting (but not
		// clearing) of the B_TEAM_DEBUG_DEBUGGER_HANDOVER flag

	struct arch_team_debug_info	arch_info;
};

struct thread_debug_info {
	int32		flags;
		// Set atomically. So reading atomically is OK, even when the thread
		// lock is not held (at least if it is certain, that the thread struct
		// won't go).
	port_id		debug_port;
		// the port the thread is waiting on for commands from the nub thread

	sigset_t	ignore_signals;
		// the signals the debugger is not interested in
	sigset_t	ignore_signals_once;
		// the signals the debugger wishes not to be notified of, when they
		// occur the next time

	struct arch_thread_debug_info	arch_info;
};

#define GRAB_TEAM_DEBUG_INFO_LOCK(info)		acquire_spinlock(&(info).lock)
#define RELEASE_TEAM_DEBUG_INFO_LOCK(info)	release_spinlock(&(info).lock)

// team debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_TEAM_DEBUG_DEBUGGER_INSTALLED	= 0x0001,
	B_TEAM_DEBUG_DEBUGGER_HANDOVER	= 0x0002,

	B_TEAM_DEBUG_KERNEL_FLAG_MASK	= 0xffff,

	B_TEAM_DEBUG_DEFAULT_FLAGS		= B_TEAM_DEBUG_SIGNALS
									  | B_TEAM_DEBUG_PRE_SYSCALL
									  | B_TEAM_DEBUG_POST_SYSCALL,
};

// thread debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_THREAD_DEBUG_INITIALIZED		= 0x0001,
	B_THREAD_DEBUG_DYING			= 0x0002,
	B_THREAD_DEBUG_STOP				= 0x0004,
	B_THREAD_DEBUG_STOPPED			= 0x0008,
	B_THREAD_DEBUG_SINGLE_STEP		= 0x0010,

	B_THREAD_DEBUG_NUB_THREAD		= 0x0020,	// marks the nub thread

	B_THREAD_DEBUG_KERNEL_FLAG_MASK	= 0xffff,

	B_THREAD_DEBUG_DEFAULT_FLAGS	= 0,
};

// messages sent from the debug nub thread to a debugged thread
typedef enum {
	B_DEBUGGED_THREAD_MESSAGE_CONTINUE	= 0,
	B_DEBUGGED_THREAD_SET_CPU_STATE,
	B_DEBUGGED_THREAD_GET_CPU_STATE,
	B_DEBUGGED_THREAD_DEBUGGER_CHANGED,
} debugged_thread_message;

typedef struct {
	uint32	handle_event;
	bool	single_step;
} debugged_thread_continue;

typedef struct {
	port_id	reply_port;
} debugged_thread_get_cpu_state;

typedef struct {
	debug_cpu_state	cpu_state;
} debugged_thread_set_cpu_state;

typedef union {
	debugged_thread_continue		continue_thread;
	debugged_thread_set_cpu_state	set_cpu_state;
	debugged_thread_get_cpu_state	get_cpu_state;
} debugged_thread_message_data;


// internal messages sent to the nub thread
typedef enum {
	B_DEBUG_MESSAGE_HANDED_OVER		= -1,
} debug_nub_kernel_message;


#ifdef __cplusplus
extern "C" {
#endif

// service calls

void clear_team_debug_info(struct team_debug_info *info, bool initLock);
void destroy_team_debug_info(struct team_debug_info *info);

void clear_thread_debug_info(struct thread_debug_info *info,
		bool dying);
void destroy_thread_debug_info(struct thread_debug_info *info);

void user_debug_prepare_for_exec();
void user_debug_finish_after_exec();

void init_user_debug();

// debug event callbacks

void user_debug_pre_syscall(uint32 syscall, void *args);
void user_debug_post_syscall(uint32 syscall, void *args, uint64 returnValue,
		bigtime_t startTime);
bool user_debug_exception_occurred(debug_exception_type exception, int signal);
bool user_debug_handle_signal(int signal, struct sigaction *handler,
		bool deadly);
void user_debug_stop_thread();
void user_debug_team_created(team_id teamID);
void user_debug_team_deleted(team_id teamID, port_id debuggerPort);
void user_debug_thread_created(thread_id threadID);
void user_debug_thread_deleted(team_id teamID, thread_id threadID);
void user_debug_image_created(const image_info *imageInfo);
void user_debug_image_deleted(const image_info *imageInfo);
void user_debug_breakpoint_hit(bool software);
void user_debug_watchpoint_hit();
void user_debug_single_stepped();


// syscalls

void		_user_debugger(const char *message);
int			_user_disable_debugger(int state);

status_t	_user_install_default_debugger(port_id debuggerPort);
port_id		_user_install_team_debugger(team_id team, port_id debuggerPort);
status_t	_user_remove_team_debugger(team_id team);
status_t	_user_debug_thread(thread_id thread);
void		_user_wait_for_debugger(void);

status_t	_user_set_debugger_breakpoint(void *address, uint32 type,
				int32 length, bool watchpoint);
status_t	_user_clear_debugger_breakpoint(void *address, bool watchpoint);


#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_USER_DEBUGGER_H
