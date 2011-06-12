/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Userland debugger support.
 */
#ifndef _KERNEL_USER_DEBUGGER_H
#define _KERNEL_USER_DEBUGGER_H


#include <debugger.h>

#include <arch/user_debugger.h>

#include <timer.h>


// limits
#define B_DEBUG_MIN_PROFILE_INTERVAL			10			/* in us */
#define B_DEBUG_STACK_TRACE_DEPTH				128
#define	B_DEBUG_PROFILE_BUFFER_FLUSH_THRESHOLD	70			/* in % */


struct BreakpointManager;
struct ConditionVariable;
struct function_profile_info;

namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


// Team related debugging data.
//
// Locking policy:
// 1) When accessing the structure it must be made sure, that the structure,
//    (i.e. the struct Team it lives in) isn't deleted. Thus one either needs to
//    get a team reference, lock the team, or one accesses the structure from a
//    thread of that team.
// 2) Access to the `flags' field is atomic. Reading via atomic_get()
//    requires no further locks (in addition to 1) that is). Writing requires
//    `lock' to be held and must be done atomically, too
//    (atomic_{set,and,or}()). Reading with `lock' being held doesn't need to
//    be done atomically.
// 3) Access to all other fields (read or write) requires `lock' to be held.
// 4) Locking order is scheduler lock -> Team -> Thread -> team_debug_info::lock
//    -> thread_debug_info::lock.
//
struct team_debug_info {
	spinlock	lock;
		// Guards the remaining fields. Should always be the innermost lock
		// to be acquired/released, save for thread_debug_info::lock.

	int32		flags;
		// Set atomically. So reading atomically is OK, even when the lock is
		// not held (at least if it is certain, that the team struct won't go).

	team_id		debugger_team;
	port_id		debugger_port;
	thread_id	nub_thread;
	port_id		nub_port;
		// the port the nub thread is waiting on for commands from the debugger
	sem_id		debugger_write_lock;
		// synchronizes writes to the debugger port with the setting (but not
		// clearing) of the B_TEAM_DEBUG_DEBUGGER_HANDOVER flag
	thread_id	causing_thread;
		// thread that caused the debugger to be attached; -1 for manual
		// debugger attachment (or no debugger installed)
	vint32		image_event;
		// counter incremented whenever an image is created/deleted

	struct ConditionVariable* debugger_changed_condition;
		// Set to a condition variable when going to change the debugger. Anyone
		// who wants to change the debugger as well, needs to wait until the
		// condition variable is unset again (waiting for the condition and
		// rechecking again). The field and the condition variable is protected
		// by 'lock'. After setting the a condition variable the team is
		// guaranteed not to be deleted (until it is unset) it might be removed
		// from the team hash table, though.

	struct BreakpointManager* breakpoint_manager;
		// manages hard- and software breakpoints

	struct arch_team_debug_info	arch_info;
};

// Thread related debugging data.
//
// Locking policy:
// 1) When accessing the structure it must be made sure, that the structure,
//    (i.e. the struct Thread it lives in) isn't deleted. Thus one either needs
//    to get a thread reference, lock the thread, or one accesses the structure
//    of the current thread.
// 2) Access to the `flags' field is atomic. Reading via atomic_get()
//    requires no further locks (in addition to 1) that is). Writing requires
//    `lock' to be held and must be done atomically, too
//    (atomic_{set,and,or}()). Reading with `lock' being held doesn't need to
//    be done atomically.
// 3) Access to all other fields (read or write) requires `lock' to be held.
// 4) Locking order is scheduler lock -> Team -> Thread -> team_debug_info::lock
//    -> thread_debug_info::lock.
//
struct thread_debug_info {
	spinlock	lock;
		// Guards the remaining fields. Should always be the innermost lock
		// to be acquired/released.

	int32		flags;
		// Set atomically. So reading atomically is OK, even when the lock is
		// not held (at least if it is certain, that the thread struct won't
		// go).
	port_id		debug_port;
		// the port the thread is waiting on for commands from the nub thread

	sigset_t	ignore_signals;
		// the signals the debugger is not interested in
	sigset_t	ignore_signals_once;
		// the signals the debugger wishes not to be notified of, when they
		// occur the next time

	// profiling related part; if samples != NULL, the thread is profiled
	struct {
		bigtime_t		interval;
			// sampling interval
		area_id			sample_area;
			// cloned sample buffer area
		addr_t*			samples;
			// sample buffer
		int32			max_samples;
			// maximum number of samples the buffer can hold
		int32			flush_threshold;
			// number of sample when the buffer is flushed (if possible)
		int32			sample_count;
			// number of samples the buffer currently holds
		int32			stack_depth;
			// number of return addresses to record per timer interval
		int32			dropped_ticks;
			// number of ticks that had to be dropped when the sample buffer was
			// full and couldn't be flushed
		int32			image_event;
			// number of the image event when the first sample was written into
			// the buffer
		int32			last_image_event;
			// number of the image event when the last sample was written into
			// the buffer
		bool			variable_stack_depth;
			// record a variable number of samples per hit
		bool			buffer_full;
			// indicates that the sample buffer is full
		union {
			bigtime_t	interval_left;
				// when unscheduled: the time left of the current sampling
				// interval
			bigtime_t	timer_end;
				// when running: the absolute time the timer is supposed to go
				// off
		};
		timer*			installed_timer;
			// when running and being profiled: the CPU's profiling timer
	} profile;

	struct arch_thread_debug_info	arch_info;
};

#define GRAB_TEAM_DEBUG_INFO_LOCK(info)		acquire_spinlock(&(info).lock)
#define RELEASE_TEAM_DEBUG_INFO_LOCK(info)	release_spinlock(&(info).lock)

// team debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_TEAM_DEBUG_DEBUGGER_INSTALLED		= 0x0001,
	B_TEAM_DEBUG_DEBUGGER_HANDOVER		= 0x0002,	// marked for hand-over
	B_TEAM_DEBUG_DEBUGGER_HANDING_OVER	= 0x0004,	// handing over
	B_TEAM_DEBUG_DEBUGGER_DISABLED		= 0x0008,

	B_TEAM_DEBUG_KERNEL_FLAG_MASK		= 0xffff,

	B_TEAM_DEBUG_DEFAULT_FLAGS			= 0,
	B_TEAM_DEBUG_INHERITED_FLAGS		= B_TEAM_DEBUG_DEBUGGER_DISABLED
};

// thread debugging flags (user-specifiable flags are in <debugger.h>)
enum {
	B_THREAD_DEBUG_INITIALIZED			= 0x0001,
	B_THREAD_DEBUG_DYING				= 0x0002,
	B_THREAD_DEBUG_STOP					= 0x0004,
	B_THREAD_DEBUG_STOPPED				= 0x0008,
	B_THREAD_DEBUG_SINGLE_STEP			= 0x0010,
	B_THREAD_DEBUG_NOTIFY_SINGLE_STEP	= 0x0020,

	B_THREAD_DEBUG_NUB_THREAD			= 0x0040,	// marks the nub thread

	B_THREAD_DEBUG_KERNEL_FLAG_MASK		= 0xffff,

	B_THREAD_DEBUG_DEFAULT_FLAGS		= 0,
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

void init_thread_debug_info(struct thread_debug_info *info);
void clear_thread_debug_info(struct thread_debug_info *info, bool dying);
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
void user_debug_team_exec();
void user_debug_update_new_thread_flags(Thread* thread);
void user_debug_thread_created(thread_id threadID);
void user_debug_thread_deleted(team_id teamID, thread_id threadID);
void user_debug_thread_exiting(Thread* thread);
void user_debug_image_created(const image_info *imageInfo);
void user_debug_image_deleted(const image_info *imageInfo);
void user_debug_breakpoint_hit(bool software);
void user_debug_watchpoint_hit();
void user_debug_single_stepped();

void user_debug_thread_unscheduled(Thread* thread);
void user_debug_thread_scheduled(Thread* thread);


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
