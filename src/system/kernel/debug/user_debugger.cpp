/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <arch/debug.h>
#include <arch/user_debugger.h>
#include <cpu.h>
#include <debugger.h>
#include <kernel.h>
#include <KernelExport.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <ksyscalls.h>
#include <port.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <user_debugger.h>
#include <vm/vm.h>
#include <vm/vm_types.h>

#include <AutoDeleter.h>
#include <util/AutoLock.h>

#include "BreakpointManager.h"


//#define TRACE_USER_DEBUGGER
#ifdef TRACE_USER_DEBUGGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// TODO: Since the introduction of team_debug_info::debugger_changed_condition
// there's some potential for simplifications. E.g. clear_team_debug_info() and
// destroy_team_debug_info() are now only used in nub_thread_cleanup() (plus
// arch_clear_team_debug_info() in install_team_debugger_init_debug_infos()).


static port_id sDefaultDebuggerPort = -1;
	// accessed atomically

static timer sProfilingTimers[B_MAX_CPU_COUNT];
	// a profiling timer for each CPU -- used when a profiled thread is running
	// on that CPU


static void schedule_profiling_timer(Thread* thread, bigtime_t interval);
static int32 profiling_event(timer* unused);
static status_t ensure_debugger_installed();
static void get_team_debug_info(team_debug_info &teamDebugInfo);


static inline status_t
kill_interruptable_write_port(port_id port, int32 code, const void *buffer,
	size_t bufferSize)
{
	return write_port_etc(port, code, buffer, bufferSize, B_KILL_CAN_INTERRUPT,
		0);
}


static status_t
debugger_write(port_id port, int32 code, const void *buffer, size_t bufferSize,
	bool dontWait)
{
	TRACE(("debugger_write(): thread: %ld, team %ld, port: %ld, code: %lx, message: %p, "
		"size: %lu, dontWait: %d\n", thread_get_current_thread()->id,
		thread_get_current_thread()->team->id, port, code, buffer, bufferSize,
		dontWait));

	status_t error = B_OK;

	// get the team debug info
	team_debug_info teamDebugInfo;
	get_team_debug_info(teamDebugInfo);
	sem_id writeLock = teamDebugInfo.debugger_write_lock;

	// get the write lock
	TRACE(("debugger_write(): acquiring write lock...\n"));
	error = acquire_sem_etc(writeLock, 1,
		dontWait ? (uint32)B_RELATIVE_TIMEOUT : (uint32)B_KILL_CAN_INTERRUPT, 0);
	if (error != B_OK) {
		TRACE(("debugger_write() done1: %lx\n", error));
		return error;
	}

	// re-get the team debug info
	get_team_debug_info(teamDebugInfo);

	if (teamDebugInfo.debugger_port != port
		|| (teamDebugInfo.flags & B_TEAM_DEBUG_DEBUGGER_HANDOVER)) {
		// The debugger has changed in the meantime or we are about to be
		// handed over to a new debugger. In either case we don't send the
		// message.
		TRACE(("debugger_write(): %s\n",
			(teamDebugInfo.debugger_port != port ? "debugger port changed"
				: "handover flag set")));
	} else {
		TRACE(("debugger_write(): writing to port...\n"));

		error = write_port_etc(port, code, buffer, bufferSize,
			dontWait ? (uint32)B_RELATIVE_TIMEOUT : (uint32)B_KILL_CAN_INTERRUPT, 0);
	}

	// release the write lock
	release_sem(writeLock);

	TRACE(("debugger_write() done: %lx\n", error));

	return error;
}


/*!	Updates the thread::flags field according to what user debugger flags are
	set for the thread.
	Interrupts must be disabled and the thread's debug info lock must be held.
*/
static void
update_thread_user_debug_flag(Thread* thread)
{
	if ((atomic_get(&thread->debug_info.flags) & B_THREAD_DEBUG_STOP) != 0)
		atomic_or(&thread->flags, THREAD_FLAGS_DEBUG_THREAD);
	else
		atomic_and(&thread->flags, ~THREAD_FLAGS_DEBUG_THREAD);
}


/*!	Updates the thread::flags THREAD_FLAGS_BREAKPOINTS_DEFINED bit of the
	given thread.
	Interrupts must be disabled and the thread debug info lock must be held.
*/
static void
update_thread_breakpoints_flag(Thread* thread)
{
	Team* team = thread->team;

	if (arch_has_breakpoints(&team->debug_info.arch_info))
		atomic_or(&thread->flags, THREAD_FLAGS_BREAKPOINTS_DEFINED);
	else
		atomic_and(&thread->flags, ~THREAD_FLAGS_BREAKPOINTS_DEFINED);
}


/*!	Updates the Thread::flags THREAD_FLAGS_BREAKPOINTS_DEFINED bit of all
	threads of the current team.
*/
static void
update_threads_breakpoints_flag()
{
	Team* team = thread_get_current_thread()->team;

	TeamLocker teamLocker(team);

	Thread* thread = team->thread_list;

	if (arch_has_breakpoints(&team->debug_info.arch_info)) {
		for (; thread != NULL; thread = thread->team_next)
			atomic_or(&thread->flags, THREAD_FLAGS_BREAKPOINTS_DEFINED);
	} else {
		for (; thread != NULL; thread = thread->team_next)
			atomic_and(&thread->flags, ~THREAD_FLAGS_BREAKPOINTS_DEFINED);
	}
}


/*!	Updates the thread::flags B_TEAM_DEBUG_DEBUGGER_INSTALLED bit of the
	given thread, which must be the current thread.
*/
static void
update_thread_debugger_installed_flag(Thread* thread)
{
	Team* team = thread->team;

	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED)
		atomic_or(&thread->flags, THREAD_FLAGS_DEBUGGER_INSTALLED);
	else
		atomic_and(&thread->flags, ~THREAD_FLAGS_DEBUGGER_INSTALLED);
}


/*!	Updates the thread::flags THREAD_FLAGS_DEBUGGER_INSTALLED bit of all
	threads of the given team.
	The team's lock must be held.
*/
static void
update_threads_debugger_installed_flag(Team* team)
{
	Thread* thread = team->thread_list;

	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		for (; thread != NULL; thread = thread->team_next)
			atomic_or(&thread->flags, THREAD_FLAGS_DEBUGGER_INSTALLED);
	} else {
		for (; thread != NULL; thread = thread->team_next)
			atomic_and(&thread->flags, ~THREAD_FLAGS_DEBUGGER_INSTALLED);
	}
}


/**
 *	For the first initialization the function must be called with \a initLock
 *	set to \c true. If it would be possible that another thread accesses the
 *	structure at the same time, `lock' must be held when calling the function.
 */
void
clear_team_debug_info(struct team_debug_info *info, bool initLock)
{
	if (info) {
		arch_clear_team_debug_info(&info->arch_info);
		atomic_set(&info->flags, B_TEAM_DEBUG_DEFAULT_FLAGS);
		info->debugger_team = -1;
		info->debugger_port = -1;
		info->nub_thread = -1;
		info->nub_port = -1;
		info->debugger_write_lock = -1;
		info->causing_thread = -1;
		info->image_event = 0;
		info->breakpoint_manager = NULL;

		if (initLock) {
			B_INITIALIZE_SPINLOCK(&info->lock);
			info->debugger_changed_condition = NULL;
		}
	}
}

/**
 *  `lock' must not be held nor may interrupts be disabled.
 *  \a info must not be a member of a team struct (or the team struct must no
 *  longer be accessible, i.e. the team should already be removed).
 *
 *	In case the team is still accessible, the procedure is:
 *	1. get `lock'
 *	2. copy the team debug info on stack
 *	3. call clear_team_debug_info() on the team debug info
 *	4. release `lock'
 *	5. call destroy_team_debug_info() on the copied team debug info
 */
static void
destroy_team_debug_info(struct team_debug_info *info)
{
	if (info) {
		arch_destroy_team_debug_info(&info->arch_info);

		// delete the breakpoint manager
		delete info->breakpoint_manager ;
		info->breakpoint_manager = NULL;

		// delete the debugger port write lock
		if (info->debugger_write_lock >= 0) {
			delete_sem(info->debugger_write_lock);
			info->debugger_write_lock = -1;
		}

		// delete the nub port
		if (info->nub_port >= 0) {
			set_port_owner(info->nub_port, B_CURRENT_TEAM);
			delete_port(info->nub_port);
			info->nub_port = -1;
		}

		// wait for the nub thread
		if (info->nub_thread >= 0) {
			if (info->nub_thread != thread_get_current_thread()->id) {
				int32 result;
				wait_for_thread(info->nub_thread, &result);
			}

			info->nub_thread = -1;
		}

		atomic_set(&info->flags, 0);
		info->debugger_team = -1;
		info->debugger_port = -1;
		info->causing_thread = -1;
		info->image_event = -1;
	}
}


void
init_thread_debug_info(struct thread_debug_info *info)
{
	if (info) {
		B_INITIALIZE_SPINLOCK(&info->lock);
		arch_clear_thread_debug_info(&info->arch_info);
		info->flags = B_THREAD_DEBUG_DEFAULT_FLAGS;
		info->debug_port = -1;
		info->ignore_signals = 0;
		info->ignore_signals_once = 0;
		info->profile.sample_area = -1;
		info->profile.samples = NULL;
		info->profile.buffer_full = false;
		info->profile.installed_timer = NULL;
	}
}


/*!	Clears the debug info for the current thread.
	Invoked with thread debug info lock being held.
*/
void
clear_thread_debug_info(struct thread_debug_info *info, bool dying)
{
	if (info) {
		// cancel profiling timer
		if (info->profile.installed_timer != NULL) {
			cancel_timer(info->profile.installed_timer);
			info->profile.installed_timer = NULL;
		}

		arch_clear_thread_debug_info(&info->arch_info);
		atomic_set(&info->flags,
			B_THREAD_DEBUG_DEFAULT_FLAGS | (dying ? B_THREAD_DEBUG_DYING : 0));
		info->debug_port = -1;
		info->ignore_signals = 0;
		info->ignore_signals_once = 0;
		info->profile.sample_area = -1;
		info->profile.samples = NULL;
		info->profile.buffer_full = false;
	}
}


void
destroy_thread_debug_info(struct thread_debug_info *info)
{
	if (info) {
		area_id sampleArea = info->profile.sample_area;
		if (sampleArea >= 0) {
			area_info areaInfo;
			if (get_area_info(sampleArea, &areaInfo) == B_OK) {
				unlock_memory(areaInfo.address, areaInfo.size, B_READ_DEVICE);
				delete_area(sampleArea);
			}
		}

		arch_destroy_thread_debug_info(&info->arch_info);

		if (info->debug_port >= 0) {
			delete_port(info->debug_port);
			info->debug_port = -1;
		}

		info->ignore_signals = 0;
		info->ignore_signals_once = 0;

		atomic_set(&info->flags, 0);
	}
}


static status_t
prepare_debugger_change(team_id teamID, ConditionVariable& condition,
	Team*& team)
{
	// We look up the team by ID, even in case of the current team, so we can be
	// sure, that the team is not already dying.
	if (teamID == B_CURRENT_TEAM)
		teamID = thread_get_current_thread()->team->id;

	while (true) {
		// get the team
		team = Team::GetAndLock(teamID);
		if (team == NULL)
			return B_BAD_TEAM_ID;
		BReference<Team> teamReference(team, true);
		TeamLocker teamLocker(team, true);

		// don't allow messing with the kernel team
		if (team == team_get_kernel_team())
			return B_NOT_ALLOWED;

		// check whether the condition is already set
		InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

		if (team->debug_info.debugger_changed_condition == NULL) {
			// nobody there yet -- set our condition variable and be done
			team->debug_info.debugger_changed_condition = &condition;
			return B_OK;
		}

		// we'll have to wait
		ConditionVariableEntry entry;
		team->debug_info.debugger_changed_condition->Add(&entry);

		debugInfoLocker.Unlock();
		teamLocker.Unlock();

		entry.Wait();
	}
}


static void
prepare_debugger_change(Team* team, ConditionVariable& condition)
{
	while (true) {
		// check whether the condition is already set
		InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

		if (team->debug_info.debugger_changed_condition == NULL) {
			// nobody there yet -- set our condition variable and be done
			team->debug_info.debugger_changed_condition = &condition;
			return;
		}

		// we'll have to wait
		ConditionVariableEntry entry;
		team->debug_info.debugger_changed_condition->Add(&entry);

		debugInfoLocker.Unlock();

		entry.Wait();
	}
}


static void
finish_debugger_change(Team* team)
{
	// unset our condition variable and notify all threads waiting on it
	InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

	ConditionVariable* condition = team->debug_info.debugger_changed_condition;
	team->debug_info.debugger_changed_condition = NULL;

	condition->NotifyAll(false);
}


void
user_debug_prepare_for_exec()
{
	Thread *thread = thread_get_current_thread();
	Team *team = thread->team;

	// If a debugger is installed for the team and the thread debug stuff
	// initialized, change the ownership of the debug port for the thread
	// to the kernel team, since exec_team() deletes all ports owned by this
	// team. We change the ownership back later.
	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// get the port
		port_id debugPort = -1;

		InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

		if ((thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED) != 0)
			debugPort = thread->debug_info.debug_port;

		threadDebugInfoLocker.Unlock();

		// set the new port ownership
		if (debugPort >= 0)
			set_port_owner(debugPort, team_get_kernel_team_id());
	}
}


void
user_debug_finish_after_exec()
{
	Thread *thread = thread_get_current_thread();
	Team *team = thread->team;

	// If a debugger is installed for the team and the thread debug stuff
	// initialized for this thread, change the ownership of its debug port
	// back to this team.
	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// get the port
		port_id debugPort = -1;

		InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

		if (thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED)
			debugPort = thread->debug_info.debug_port;

		threadDebugInfoLocker.Unlock();

		// set the new port ownership
		if (debugPort >= 0)
			set_port_owner(debugPort, team->id);
	}
}


void
init_user_debug()
{
	#ifdef ARCH_INIT_USER_DEBUG
		ARCH_INIT_USER_DEBUG();
	#endif
}


static void
get_team_debug_info(team_debug_info &teamDebugInfo)
{
	Thread *thread = thread_get_current_thread();

	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

	memcpy(&teamDebugInfo, &thread->team->debug_info, sizeof(team_debug_info));

	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	restore_interrupts(state);
}


static status_t
thread_hit_debug_event_internal(debug_debugger_message event,
	const void *message, int32 size, bool requireDebugger, bool &restart)
{
	restart = false;
	Thread *thread = thread_get_current_thread();

	TRACE(("thread_hit_debug_event(): thread: %ld, event: %lu, message: %p, "
		"size: %ld\n", thread->id, (uint32)event, message, size));

	// check, if there's a debug port already
	bool setPort = !(atomic_get(&thread->debug_info.flags)
		& B_THREAD_DEBUG_INITIALIZED);

	// create a port, if there is none yet
	port_id port = -1;
	if (setPort) {
		char nameBuffer[128];
		snprintf(nameBuffer, sizeof(nameBuffer), "nub to thread %" B_PRId32,
			thread->id);

		port = create_port(1, nameBuffer);
		if (port < 0) {
			dprintf("thread_hit_debug_event(): Failed to create debug port: "
				"%s\n", strerror(port));
			return port;
		}
	}

	// check the debug info structures once more: get the debugger port, set
	// the thread's debug port, and update the thread's debug flags
	port_id deletePort = port;
	port_id debuggerPort = -1;
	port_id nubPort = -1;
	status_t error = B_OK;
	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	uint32 threadFlags = thread->debug_info.flags;
	threadFlags &= ~B_THREAD_DEBUG_STOP;
	bool debuggerInstalled
		= (thread->team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED);
	if (thread->id == thread->team->debug_info.nub_thread) {
		// Ugh, we're the nub thread. We shouldn't be here.
		TRACE(("thread_hit_debug_event(): Misdirected nub thread: %ld\n",
			thread->id));

		error = B_ERROR;
	} else if (debuggerInstalled || !requireDebugger) {
		if (debuggerInstalled) {
			debuggerPort = thread->team->debug_info.debugger_port;
			nubPort = thread->team->debug_info.nub_port;
		}

		if (setPort) {
			if (threadFlags & B_THREAD_DEBUG_INITIALIZED) {
				// someone created a port for us (the port we've created will
				// be deleted below)
				port = thread->debug_info.debug_port;
			} else {
				thread->debug_info.debug_port = port;
				deletePort = -1;	// keep the port
				threadFlags |= B_THREAD_DEBUG_INITIALIZED;
			}
		} else {
			if (threadFlags & B_THREAD_DEBUG_INITIALIZED) {
				port = thread->debug_info.debug_port;
			} else {
				// someone deleted our port
				error = B_ERROR;
			}
		}
	} else
		error = B_ERROR;

	// update the flags
	if (error == B_OK)
		threadFlags |= B_THREAD_DEBUG_STOPPED;
	atomic_set(&thread->debug_info.flags, threadFlags);

	update_thread_user_debug_flag(thread);

	threadDebugInfoLocker.Unlock();
	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	restore_interrupts(state);

	// delete the superfluous port
	if (deletePort >= 0)
		delete_port(deletePort);

	if (error != B_OK) {
		TRACE(("thread_hit_debug_event() error: thread: %ld, error: %lx\n",
			thread->id, error));
		return error;
	}

	// send a message to the debugger port
	if (debuggerInstalled) {
		// update the message's origin info first
		debug_origin *origin = (debug_origin *)message;
		origin->thread = thread->id;
		origin->team = thread->team->id;
		origin->nub_port = nubPort;

		TRACE(("thread_hit_debug_event(): thread: %ld, sending message to "
			"debugger port %ld\n", thread->id, debuggerPort));

		error = debugger_write(debuggerPort, event, message, size, false);
	}

	status_t result = B_THREAD_DEBUG_HANDLE_EVENT;
	bool singleStep = false;

	if (error == B_OK) {
		bool done = false;
		while (!done) {
			// read a command from the debug port
			int32 command;
			debugged_thread_message_data commandMessage;
			ssize_t commandMessageSize = read_port_etc(port, &command,
				&commandMessage, sizeof(commandMessage), B_KILL_CAN_INTERRUPT,
				0);

			if (commandMessageSize < 0) {
				error = commandMessageSize;
				TRACE(("thread_hit_debug_event(): thread: %ld, failed "
					"to receive message from port %ld: %lx\n",
					thread->id, port, error));
				break;
			}

			switch (command) {
				case B_DEBUGGED_THREAD_MESSAGE_CONTINUE:
					TRACE(("thread_hit_debug_event(): thread: %ld: "
						"B_DEBUGGED_THREAD_MESSAGE_CONTINUE\n",
						thread->id));
					result = commandMessage.continue_thread.handle_event;

					singleStep = commandMessage.continue_thread.single_step;
					done = true;
					break;

				case B_DEBUGGED_THREAD_SET_CPU_STATE:
				{
					TRACE(("thread_hit_debug_event(): thread: %ld: "
						"B_DEBUGGED_THREAD_SET_CPU_STATE\n",
						thread->id));
					arch_set_debug_cpu_state(
						&commandMessage.set_cpu_state.cpu_state);

					break;
				}

				case B_DEBUGGED_THREAD_GET_CPU_STATE:
				{
					port_id replyPort = commandMessage.get_cpu_state.reply_port;

					// prepare the message
					debug_nub_get_cpu_state_reply replyMessage;
					replyMessage.error = B_OK;
					replyMessage.message = event;
					arch_get_debug_cpu_state(&replyMessage.cpu_state);

					// send it
					error = kill_interruptable_write_port(replyPort, event,
						&replyMessage, sizeof(replyMessage));

					break;
				}

				case B_DEBUGGED_THREAD_DEBUGGER_CHANGED:
				{
					// Check, if the debugger really changed, i.e. is different
					// than the one we know.
					team_debug_info teamDebugInfo;
					get_team_debug_info(teamDebugInfo);

					if (teamDebugInfo.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
						if (!debuggerInstalled
							|| teamDebugInfo.debugger_port != debuggerPort) {
							// debugger was installed or has changed: restart
							// this function
							restart = true;
							done = true;
						}
					} else {
						if (debuggerInstalled) {
							// debugger is gone: continue the thread normally
							done = true;
						}
					}

					break;
				}
			}
		}
	} else {
		TRACE(("thread_hit_debug_event(): thread: %ld, failed to send "
			"message to debugger port %ld: %lx\n", thread->id,
			debuggerPort, error));
	}

	// update the thread debug info
	bool destroyThreadInfo = false;
	thread_debug_info threadDebugInfo;

	state = disable_interrupts();
	threadDebugInfoLocker.Lock();

	// check, if the team is still being debugged
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// update the single-step flag
		if (singleStep) {
			atomic_or(&thread->debug_info.flags,
				B_THREAD_DEBUG_SINGLE_STEP);
			atomic_or(&thread->flags, THREAD_FLAGS_SINGLE_STEP);
		} else {
			atomic_and(&thread->debug_info.flags,
				~(int32)B_THREAD_DEBUG_SINGLE_STEP);
		}

		// unset the "stopped" state
		atomic_and(&thread->debug_info.flags, ~B_THREAD_DEBUG_STOPPED);

		update_thread_user_debug_flag(thread);

	} else {
		// the debugger is gone: cleanup our info completely
		threadDebugInfo = thread->debug_info;
		clear_thread_debug_info(&thread->debug_info, false);
		destroyThreadInfo = true;
	}

	threadDebugInfoLocker.Unlock();
	restore_interrupts(state);

	// enable/disable single stepping
	arch_update_thread_single_step();

	if (destroyThreadInfo)
		destroy_thread_debug_info(&threadDebugInfo);

	return (error == B_OK ? result : error);
}


static status_t
thread_hit_debug_event(debug_debugger_message event, const void *message,
	int32 size, bool requireDebugger)
{
	status_t result;
	bool restart;
	do {
		restart = false;
		result = thread_hit_debug_event_internal(event, message, size,
			requireDebugger, restart);
	} while (result >= 0 && restart);

	// Prepare to continue -- we install a debugger change condition, so no one
	// will change the debugger while we're playing with the breakpoint manager.
	// TODO: Maybe better use ref-counting and a flag in the breakpoint manager.
	Team* team = thread_get_current_thread()->team;
	ConditionVariable debugChangeCondition;
	prepare_debugger_change(team, debugChangeCondition);

	if (team->debug_info.breakpoint_manager != NULL) {
		bool isSyscall;
		void* pc = arch_debug_get_interrupt_pc(&isSyscall);
		if (pc != NULL && !isSyscall)
			team->debug_info.breakpoint_manager->PrepareToContinue(pc);
	}

	finish_debugger_change(team);

	return result;
}


static status_t
thread_hit_serious_debug_event(debug_debugger_message event,
	const void *message, int32 messageSize)
{
	// ensure that a debugger is installed for this team
	status_t error = ensure_debugger_installed();
	if (error != B_OK) {
		Thread *thread = thread_get_current_thread();
		dprintf("thread_hit_serious_debug_event(): Failed to install debugger: "
			"thread: %" B_PRId32 ": %s\n", thread->id, strerror(error));
		return error;
	}

	// enter the debug loop
	return thread_hit_debug_event(event, message, messageSize, true);
}


void
user_debug_pre_syscall(uint32 syscall, void *args)
{
	// check whether a debugger is installed
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (!(teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED))
		return;

	// check whether pre-syscall tracing is enabled for team or thread
	int32 threadDebugFlags = atomic_get(&thread->debug_info.flags);
	if (!(teamDebugFlags & B_TEAM_DEBUG_PRE_SYSCALL)
			&& !(threadDebugFlags & B_THREAD_DEBUG_PRE_SYSCALL)) {
		return;
	}

	// prepare the message
	debug_pre_syscall message;
	message.syscall = syscall;

	// copy the syscall args
	if (syscall < (uint32)kSyscallCount) {
		if (kSyscallInfos[syscall].parameter_size > 0)
			memcpy(message.args, args, kSyscallInfos[syscall].parameter_size);
	}

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_PRE_SYSCALL, &message,
		sizeof(message), true);
}


void
user_debug_post_syscall(uint32 syscall, void *args, uint64 returnValue,
	bigtime_t startTime)
{
	// check whether a debugger is installed
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (!(teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED))
		return;

	// check whether post-syscall tracing is enabled for team or thread
	int32 threadDebugFlags = atomic_get(&thread->debug_info.flags);
	if (!(teamDebugFlags & B_TEAM_DEBUG_POST_SYSCALL)
			&& !(threadDebugFlags & B_THREAD_DEBUG_POST_SYSCALL)) {
		return;
	}

	// prepare the message
	debug_post_syscall message;
	message.start_time = startTime;
	message.end_time = system_time();
	message.return_value = returnValue;
	message.syscall = syscall;

	// copy the syscall args
	if (syscall < (uint32)kSyscallCount) {
		if (kSyscallInfos[syscall].parameter_size > 0)
			memcpy(message.args, args, kSyscallInfos[syscall].parameter_size);
	}

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_POST_SYSCALL, &message,
		sizeof(message), true);
}


/**	\brief To be called when an unhandled processor exception (error/fault)
 *		   occurred.
 *	\param exception The debug_why_stopped value identifying the kind of fault.
 *	\param singal The signal corresponding to the exception.
 *	\return \c true, if the caller shall continue normally, i.e. usually send
 *			a deadly signal. \c false, if the debugger insists to continue the
 *			program (e.g. because it has solved the removed the cause of the
 *			problem).
 */
bool
user_debug_exception_occurred(debug_exception_type exception, int signal)
{
	// First check whether there's a signal handler installed for the signal.
	// If so, we don't want to install a debugger for the team. We always send
	// the signal instead. An already installed debugger will be notified, if
	// it has requested notifications of signal.
	struct sigaction signalAction;
	if (sigaction(signal, NULL, &signalAction) == 0
		&& signalAction.sa_handler != SIG_DFL) {
		return true;
	}

	// prepare the message
	debug_exception_occurred message;
	message.exception = exception;
	message.signal = signal;

	status_t result = thread_hit_serious_debug_event(
		B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED, &message, sizeof(message));
	return (result != B_THREAD_DEBUG_IGNORE_EVENT);
}


bool
user_debug_handle_signal(int signal, struct sigaction *handler, bool deadly)
{
	// check, if a debugger is installed and is interested in signals
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_SIGNALS)) {
		return true;
	}

	// prepare the message
	debug_signal_received message;
	message.signal = signal;
	message.handler = *handler;
	message.deadly = deadly;

	status_t result = thread_hit_debug_event(B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED,
		&message, sizeof(message), true);
	return (result != B_THREAD_DEBUG_IGNORE_EVENT);
}


void
user_debug_stop_thread()
{
	// check whether this is actually an emulated single-step notification
	Thread* thread = thread_get_current_thread();
	InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	bool singleStepped = false;
	if ((atomic_and(&thread->debug_info.flags,
				~B_THREAD_DEBUG_NOTIFY_SINGLE_STEP)
			& B_THREAD_DEBUG_NOTIFY_SINGLE_STEP) != 0) {
		singleStepped = true;
	}

	threadDebugInfoLocker.Unlock();

	if (singleStepped) {
		user_debug_single_stepped();
	} else {
		debug_thread_debugged message;
		thread_hit_serious_debug_event(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED,
			&message, sizeof(message));
	}
}


void
user_debug_team_created(team_id teamID)
{
	// check, if a debugger is installed and is interested in team creation
	// events
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_TEAM_CREATION)) {
		return;
	}

	// prepare the message
	debug_team_created message;
	message.new_team = teamID;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_TEAM_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_team_deleted(team_id teamID, port_id debuggerPort)
{
	if (debuggerPort >= 0) {
		TRACE(("user_debug_team_deleted(team: %ld, debugger port: %ld)\n",
			teamID, debuggerPort));

		debug_team_deleted message;
		message.origin.thread = -1;
		message.origin.team = teamID;
		message.origin.nub_port = -1;
		write_port_etc(debuggerPort, B_DEBUGGER_MESSAGE_TEAM_DELETED, &message,
			sizeof(message), B_RELATIVE_TIMEOUT, 0);
	}
}


void
user_debug_team_exec()
{
	// check, if a debugger is installed and is interested in team creation
	// events
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_TEAM_CREATION)) {
		return;
	}

	// prepare the message
	debug_team_exec message;
	message.image_event = atomic_add(&thread->team->debug_info.image_event, 1)
		+ 1;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_TEAM_EXEC, &message,
		sizeof(message), true);
}


/*!	Called by a new userland thread to update the debugging related flags of
	\c Thread::flags before the thread first enters userland.
	\param thread The calling thread.
*/
void
user_debug_update_new_thread_flags(Thread* thread)
{
	// lock it and update it's flags
	InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	update_thread_user_debug_flag(thread);
	update_thread_breakpoints_flag(thread);
	update_thread_debugger_installed_flag(thread);
}


void
user_debug_thread_created(thread_id threadID)
{
	// check, if a debugger is installed and is interested in thread events
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_THREADS)) {
		return;
	}

	// prepare the message
	debug_thread_created message;
	message.new_thread = threadID;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_thread_deleted(team_id teamID, thread_id threadID)
{
	// Things are a bit complicated here, since this thread no longer belongs to
	// the debugged team (but to the kernel). So we can't use debugger_write().

	// get the team debug flags and debugger port
	Team* team = Team::Get(teamID);
	if (team == NULL)
		return;
	BReference<Team> teamReference(team, true);

	InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

	int32 teamDebugFlags = atomic_get(&team->debug_info.flags);
	port_id debuggerPort = team->debug_info.debugger_port;
	sem_id writeLock = team->debug_info.debugger_write_lock;

	debugInfoLocker.Unlock();

	// check, if a debugger is installed and is interested in thread events
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_THREADS)) {
		return;
	}

	// acquire the debugger write lock
	status_t error = acquire_sem_etc(writeLock, 1, B_KILL_CAN_INTERRUPT, 0);
	if (error != B_OK)
		return;

	// re-get the team debug info -- we need to check whether anything changed
	debugInfoLocker.Lock();

	teamDebugFlags = atomic_get(&team->debug_info.flags);
	port_id newDebuggerPort = team->debug_info.debugger_port;

	debugInfoLocker.Unlock();

	// Send the message only if the debugger hasn't changed in the meantime or
	// the team is about to be handed over.
	if (newDebuggerPort == debuggerPort
		|| (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_HANDOVER) == 0) {
		debug_thread_deleted message;
		message.origin.thread = threadID;
		message.origin.team = teamID;
		message.origin.nub_port = -1;

		write_port_etc(debuggerPort, B_DEBUGGER_MESSAGE_THREAD_DELETED,
			&message, sizeof(message), B_KILL_CAN_INTERRUPT, 0);
	}

	// release the debugger write lock
	release_sem(writeLock);
}


/*!	Called for a thread that is about to die, cleaning up all user debug
	facilities installed for the thread.
	\param thread The current thread, the one that is going to die.
*/
void
user_debug_thread_exiting(Thread* thread)
{
	// thread is the current thread, so using team is safe
	Team* team = thread->team;

	InterruptsLocker interruptsLocker;

	GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	int32 teamDebugFlags = atomic_get(&team->debug_info.flags);
	port_id debuggerPort = team->debug_info.debugger_port;

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	// check, if a debugger is installed
	if ((teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) == 0
		|| debuggerPort < 0) {
		return;
	}

	// detach the profile info and mark the thread dying
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	thread_debug_info& threadDebugInfo = thread->debug_info;
	if (threadDebugInfo.profile.samples == NULL)
		return;

	area_id sampleArea = threadDebugInfo.profile.sample_area;
	int32 sampleCount = threadDebugInfo.profile.sample_count;
	int32 droppedTicks = threadDebugInfo.profile.dropped_ticks;
	int32 stackDepth = threadDebugInfo.profile.stack_depth;
	bool variableStackDepth = threadDebugInfo.profile.variable_stack_depth;
	int32 imageEvent = threadDebugInfo.profile.image_event;
	threadDebugInfo.profile.sample_area = -1;
	threadDebugInfo.profile.samples = NULL;
	threadDebugInfo.profile.buffer_full = false;

	atomic_or(&threadDebugInfo.flags, B_THREAD_DEBUG_DYING);

	threadDebugInfoLocker.Unlock();
	interruptsLocker.Unlock();

	// notify the debugger
	debug_profiler_update message;
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = -1;	// asynchronous message
	message.sample_count = sampleCount;
	message.dropped_ticks = droppedTicks;
	message.stack_depth = stackDepth;
	message.variable_stack_depth = variableStackDepth;
	message.image_event = imageEvent;
	message.stopped = true;
	debugger_write(debuggerPort, B_DEBUGGER_MESSAGE_PROFILER_UPDATE,
		&message, sizeof(message), false);

	if (sampleArea >= 0) {
		area_info areaInfo;
		if (get_area_info(sampleArea, &areaInfo) == B_OK) {
			unlock_memory(areaInfo.address, areaInfo.size, B_READ_DEVICE);
			delete_area(sampleArea);
		}
	}
}


void
user_debug_image_created(const image_info *imageInfo)
{
	// check, if a debugger is installed and is interested in image events
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_IMAGES)) {
		return;
	}

	// prepare the message
	debug_image_created message;
	memcpy(&message.info, imageInfo, sizeof(image_info));
	message.image_event = atomic_add(&thread->team->debug_info.image_event, 1)
		+ 1;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_image_deleted(const image_info *imageInfo)
{
	// check, if a debugger is installed and is interested in image events
	Thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_IMAGES)) {
		return;
	}

	// prepare the message
	debug_image_deleted message;
	memcpy(&message.info, imageInfo, sizeof(image_info));
	message.image_event = atomic_add(&thread->team->debug_info.image_event, 1)
		+ 1;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_breakpoint_hit(bool software)
{
	// prepare the message
	debug_breakpoint_hit message;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_serious_debug_event(B_DEBUGGER_MESSAGE_BREAKPOINT_HIT, &message,
		sizeof(message));
}


void
user_debug_watchpoint_hit()
{
	// prepare the message
	debug_watchpoint_hit message;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_serious_debug_event(B_DEBUGGER_MESSAGE_WATCHPOINT_HIT, &message,
		sizeof(message));
}


void
user_debug_single_stepped()
{
	// clear the single-step thread flag
	Thread* thread = thread_get_current_thread();
	atomic_and(&thread->flags, ~(int32)THREAD_FLAGS_SINGLE_STEP);

	// prepare the message
	debug_single_step message;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_serious_debug_event(B_DEBUGGER_MESSAGE_SINGLE_STEP, &message,
		sizeof(message));
}


/*!	Schedules the profiling timer for the current thread.
	The caller must hold the thread's debug info lock.
	\param thread The current thread.
	\param interval The time after which the timer should fire.
*/
static void
schedule_profiling_timer(Thread* thread, bigtime_t interval)
{
	struct timer* timer = &sProfilingTimers[thread->cpu->cpu_num];
	thread->debug_info.profile.installed_timer = timer;
	thread->debug_info.profile.timer_end = system_time() + interval;
	add_timer(timer, &profiling_event, interval, B_ONE_SHOT_RELATIVE_TIMER);
}


/*!	Samples the current thread's instruction pointer/stack trace.
	The caller must hold the current thread's debug info lock.
	\param flushBuffer Return parameter: Set to \c true when the sampling
		buffer must be flushed.
*/
static bool
profiling_do_sample(bool& flushBuffer)
{
	Thread* thread = thread_get_current_thread();
	thread_debug_info& debugInfo = thread->debug_info;

	if (debugInfo.profile.samples == NULL)
		return false;

	// Check, whether the buffer is full or an image event occurred since the
	// last sample was taken.
	int32 maxSamples = debugInfo.profile.max_samples;
	int32 sampleCount = debugInfo.profile.sample_count;
	int32 stackDepth = debugInfo.profile.stack_depth;
	int32 imageEvent = thread->team->debug_info.image_event;
	if (debugInfo.profile.sample_count > 0) {
		if (debugInfo.profile.last_image_event < imageEvent
			&& debugInfo.profile.variable_stack_depth
			&& sampleCount + 2 <= maxSamples) {
			// an image event occurred, but we use variable stack depth and
			// have enough room in the buffer to indicate an image event
			addr_t* event = debugInfo.profile.samples + sampleCount;
			event[0] = B_DEBUG_PROFILE_IMAGE_EVENT;
			event[1] = imageEvent;
			sampleCount += 2;
			debugInfo.profile.sample_count = sampleCount;
			debugInfo.profile.last_image_event = imageEvent;
		}

		if (debugInfo.profile.last_image_event < imageEvent
			|| debugInfo.profile.flush_threshold - sampleCount < stackDepth) {
			if (!IS_KERNEL_ADDRESS(arch_debug_get_interrupt_pc(NULL))) {
				flushBuffer = true;
				return true;
			}

			// We can't flush the buffer now, since we interrupted a kernel
			// function. If the buffer is not full yet, we add the samples,
			// otherwise we have to drop them.
			if (maxSamples - sampleCount < stackDepth) {
				debugInfo.profile.dropped_ticks++;
				return true;
			}
		}
	} else {
		// first sample -- set the image event
		debugInfo.profile.image_event = imageEvent;
		debugInfo.profile.last_image_event = imageEvent;
	}

	// get the samples
	addr_t* returnAddresses = debugInfo.profile.samples
		+ debugInfo.profile.sample_count;
	if (debugInfo.profile.variable_stack_depth) {
		// variable sample count per hit
		*returnAddresses = arch_debug_get_stack_trace(returnAddresses + 1,
			stackDepth - 1, 1, 0, STACK_TRACE_KERNEL | STACK_TRACE_USER);

		debugInfo.profile.sample_count += *returnAddresses + 1;
	} else {
		// fixed sample count per hit
		if (stackDepth > 1) {
			int32 count = arch_debug_get_stack_trace(returnAddresses,
				stackDepth, 1, 0, STACK_TRACE_KERNEL | STACK_TRACE_USER);

			for (int32 i = count; i < stackDepth; i++)
				returnAddresses[i] = 0;
		} else
			*returnAddresses = (addr_t)arch_debug_get_interrupt_pc(NULL);

		debugInfo.profile.sample_count += stackDepth;
	}

	return true;
}


static void
profiling_buffer_full(void*)
{
	// It is undefined whether the function is called with interrupts enabled
	// or disabled. We are allowed to enable interrupts, though. First make
	// sure interrupts are disabled.
	disable_interrupts();

	Thread* thread = thread_get_current_thread();
	thread_debug_info& debugInfo = thread->debug_info;

	SpinLocker threadDebugInfoLocker(debugInfo.lock);

	if (debugInfo.profile.samples != NULL && debugInfo.profile.buffer_full) {
		int32 sampleCount = debugInfo.profile.sample_count;
		int32 droppedTicks = debugInfo.profile.dropped_ticks;
		int32 stackDepth = debugInfo.profile.stack_depth;
		bool variableStackDepth = debugInfo.profile.variable_stack_depth;
		int32 imageEvent = debugInfo.profile.image_event;

		// notify the debugger
		debugInfo.profile.sample_count = 0;
		debugInfo.profile.dropped_ticks = 0;

		threadDebugInfoLocker.Unlock();
		enable_interrupts();

		// prepare the message
		debug_profiler_update message;
		message.sample_count = sampleCount;
		message.dropped_ticks = droppedTicks;
		message.stack_depth = stackDepth;
		message.variable_stack_depth = variableStackDepth;
		message.image_event = imageEvent;
		message.stopped = false;

		thread_hit_debug_event(B_DEBUGGER_MESSAGE_PROFILER_UPDATE, &message,
			sizeof(message), false);

		disable_interrupts();
		threadDebugInfoLocker.Lock();

		// do the sampling and reschedule timer, if still profiling this thread
		bool flushBuffer;
		if (profiling_do_sample(flushBuffer)) {
			debugInfo.profile.buffer_full = false;
			schedule_profiling_timer(thread, debugInfo.profile.interval);
		}
	}

	threadDebugInfoLocker.Unlock();
	enable_interrupts();
}


/*!	Profiling timer event callback.
	Called with interrupts disabled.
*/
static int32
profiling_event(timer* /*unused*/)
{
	Thread* thread = thread_get_current_thread();
	thread_debug_info& debugInfo = thread->debug_info;

	SpinLocker threadDebugInfoLocker(debugInfo.lock);

	bool flushBuffer = false;
	if (profiling_do_sample(flushBuffer)) {
		if (flushBuffer) {
			// The sample buffer needs to be flushed; we'll have to notify the
			// debugger. We can't do that right here. Instead we set a post
			// interrupt callback doing that for us, and don't reschedule the
			// timer yet.
			thread->post_interrupt_callback = profiling_buffer_full;
			debugInfo.profile.installed_timer = NULL;
			debugInfo.profile.buffer_full = true;
		} else
			schedule_profiling_timer(thread, debugInfo.profile.interval);
	} else
		debugInfo.profile.installed_timer = NULL;

	return B_HANDLED_INTERRUPT;
}


/*!	Called by the scheduler when a debugged thread has been unscheduled.
	The scheduler lock is being held.
*/
void
user_debug_thread_unscheduled(Thread* thread)
{
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	// if running, cancel the profiling timer
	struct timer* timer = thread->debug_info.profile.installed_timer;
	if (timer != NULL) {
		// track remaining time
		bigtime_t left = thread->debug_info.profile.timer_end - system_time();
		thread->debug_info.profile.interval_left = max_c(left, 0);
		thread->debug_info.profile.installed_timer = NULL;

		// cancel timer
		threadDebugInfoLocker.Unlock();
			// not necessary, but doesn't harm and reduces contention
		cancel_timer(timer);
			// since invoked on the same CPU, this will not possibly wait for
			// an already called timer hook
	}
}


/*!	Called by the scheduler when a debugged thread has been scheduled.
	The scheduler lock is being held.
*/
void
user_debug_thread_scheduled(Thread* thread)
{
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	if (thread->debug_info.profile.samples != NULL
		&& !thread->debug_info.profile.buffer_full) {
		// install profiling timer
		schedule_profiling_timer(thread,
			thread->debug_info.profile.interval_left);
	}
}


/*!	\brief Called by the debug nub thread of a team to broadcast a message to
		all threads of the team that are initialized for debugging (and
		thus have a debug port).
*/
static void
broadcast_debugged_thread_message(Thread *nubThread, int32 code,
	const void *message, int32 size)
{
	// iterate through the threads
	thread_info threadInfo;
	int32 cookie = 0;
	while (get_next_thread_info(nubThread->team->id, &cookie, &threadInfo)
			== B_OK) {
		// get the thread and lock it
		Thread* thread = Thread::GetAndLock(threadInfo.thread);
		if (thread == NULL)
			continue;

		BReference<Thread> threadReference(thread, true);
		ThreadLocker threadLocker(thread, true);

		// get the thread's debug port
		InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

		port_id threadDebugPort = -1;
		if (thread && thread != nubThread && thread->team == nubThread->team
			&& (thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED) != 0
			&& (thread->debug_info.flags & B_THREAD_DEBUG_STOPPED) != 0) {
			threadDebugPort = thread->debug_info.debug_port;
		}

		threadDebugInfoLocker.Unlock();
		threadLocker.Unlock();

		// send the message to the thread
		if (threadDebugPort >= 0) {
			status_t error = kill_interruptable_write_port(threadDebugPort,
				code, message, size);
			if (error != B_OK) {
				TRACE(("broadcast_debugged_thread_message(): Failed to send "
					"message to thread %ld: %lx\n", thread->id, error));
			}
		}
	}
}


static void
nub_thread_cleanup(Thread *nubThread)
{
	TRACE(("nub_thread_cleanup(%ld): debugger port: %ld\n", nubThread->id,
		nubThread->team->debug_info.debugger_port));

	ConditionVariable debugChangeCondition;
	prepare_debugger_change(nubThread->team, debugChangeCondition);

	team_debug_info teamDebugInfo;
	bool destroyDebugInfo = false;

	TeamLocker teamLocker(nubThread->team);
		// required by update_threads_debugger_installed_flag()

	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);

	team_debug_info &info = nubThread->team->debug_info;
	if (info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED
		&& info.nub_thread == nubThread->id) {
		teamDebugInfo = info;
		clear_team_debug_info(&info, false);
		destroyDebugInfo = true;
	}

	// update the thread::flags fields
	update_threads_debugger_installed_flag(nubThread->team);

	RELEASE_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);
	restore_interrupts(state);

	teamLocker.Unlock();

	if (destroyDebugInfo)
		teamDebugInfo.breakpoint_manager->RemoveAllBreakpoints();

	finish_debugger_change(nubThread->team);

	if (destroyDebugInfo)
		destroy_team_debug_info(&teamDebugInfo);

	// notify all threads that the debugger is gone
	broadcast_debugged_thread_message(nubThread,
		B_DEBUGGED_THREAD_DEBUGGER_CHANGED, NULL, 0);
}


/**	\brief Debug nub thread helper function that returns the debug port of
 *		   a thread of the same team.
 */
static status_t
debug_nub_thread_get_thread_debug_port(Thread *nubThread,
	thread_id threadID, port_id &threadDebugPort)
{
	threadDebugPort = -1;

	// get the thread
	Thread* thread = Thread::GetAndLock(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// get the debug port
	InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	if (thread->team != nubThread->team)
		return B_BAD_VALUE;
	if ((thread->debug_info.flags & B_THREAD_DEBUG_STOPPED) == 0)
		return B_BAD_THREAD_STATE;

	threadDebugPort = thread->debug_info.debug_port;

	threadDebugInfoLocker.Unlock();

	if (threadDebugPort < 0)
		return B_ERROR;

	return B_OK;
}


static status_t
debug_nub_thread(void *)
{
	Thread *nubThread = thread_get_current_thread();

	// check, if we're still the current nub thread and get our port
	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);

	if (nubThread->team->debug_info.nub_thread != nubThread->id) {
		RELEASE_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);
		restore_interrupts(state);
		return 0;
	}

	port_id port = nubThread->team->debug_info.nub_port;
	sem_id writeLock = nubThread->team->debug_info.debugger_write_lock;
	BreakpointManager* breakpointManager
		= nubThread->team->debug_info.breakpoint_manager;

	RELEASE_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);
	restore_interrupts(state);

	TRACE(("debug_nub_thread() thread: %ld, team %ld, nub port: %ld\n",
		nubThread->id, nubThread->team->id, port));

	// notify all threads that a debugger has been installed
	broadcast_debugged_thread_message(nubThread,
		B_DEBUGGED_THREAD_DEBUGGER_CHANGED, NULL, 0);

	// command processing loop
	while (true) {
		int32 command;
		debug_nub_message_data message;
		ssize_t messageSize = read_port_etc(port, &command, &message,
			sizeof(message), B_KILL_CAN_INTERRUPT, 0);

		if (messageSize < 0) {
			// The port is no longer valid or we were interrupted by a kill
			// signal: If we are still listed in the team's debug info as nub
			// thread, we need to update that.
			nub_thread_cleanup(nubThread);

			TRACE(("nub thread %ld: terminating: %lx\n", nubThread->id,
				messageSize));

			return messageSize;
		}

		bool sendReply = false;
		union {
			debug_nub_read_memory_reply			read_memory;
			debug_nub_write_memory_reply		write_memory;
			debug_nub_get_cpu_state_reply		get_cpu_state;
			debug_nub_set_breakpoint_reply		set_breakpoint;
			debug_nub_set_watchpoint_reply		set_watchpoint;
			debug_nub_get_signal_masks_reply	get_signal_masks;
			debug_nub_get_signal_handler_reply	get_signal_handler;
			debug_nub_start_profiler_reply		start_profiler;
			debug_profiler_update				profiler_update;
		} reply;
		int32 replySize = 0;
		port_id replyPort = -1;

		// process the command
		switch (command) {
			case B_DEBUG_MESSAGE_READ_MEMORY:
			{
				// get the parameters
				replyPort = message.read_memory.reply_port;
				void *address = message.read_memory.address;
				int32 size = message.read_memory.size;
				status_t result = B_OK;

				// check the parameters
				if (!BreakpointManager::CanAccessAddress(address, false))
					result = B_BAD_ADDRESS;
				else if (size <= 0 || size > B_MAX_READ_WRITE_MEMORY_SIZE)
					result = B_BAD_VALUE;

				// read the memory
				size_t bytesRead = 0;
				if (result == B_OK) {
					result = breakpointManager->ReadMemory(address,
						reply.read_memory.data, size, bytesRead);
				}
				reply.read_memory.error = result;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_READ_MEMORY: "
					"reply port: %ld, address: %p, size: %ld, result: %lx, "
					"read: %ld\n", nubThread->id, replyPort, address, size,
					result, bytesRead));

				// send only as much data as necessary
				reply.read_memory.size = bytesRead;
				replySize = reply.read_memory.data + bytesRead - (char*)&reply;
				sendReply = true;
				break;
			}

			case B_DEBUG_MESSAGE_WRITE_MEMORY:
			{
				// get the parameters
				replyPort = message.write_memory.reply_port;
				void *address = message.write_memory.address;
				int32 size = message.write_memory.size;
				const char *data = message.write_memory.data;
				int32 realSize = (char*)&message + messageSize - data;
				status_t result = B_OK;

				// check the parameters
				if (!BreakpointManager::CanAccessAddress(address, true))
					result = B_BAD_ADDRESS;
				else if (size <= 0 || size > realSize)
					result = B_BAD_VALUE;

				// write the memory
				size_t bytesWritten = 0;
				if (result == B_OK) {
					result = breakpointManager->WriteMemory(address, data, size,
						bytesWritten);
				}
				reply.write_memory.error = result;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_WRITE_MEMORY: "
					"reply port: %ld, address: %p, size: %ld, result: %lx, "
					"written: %ld\n", nubThread->id, replyPort, address, size,
					result, bytesWritten));

				reply.write_memory.size = bytesWritten;
				sendReply = true;
				replySize = sizeof(debug_nub_write_memory_reply);
				break;
			}

			case B_DEBUG_MESSAGE_SET_TEAM_FLAGS:
			{
				// get the parameters
				int32 flags = message.set_team_flags.flags
					& B_TEAM_DEBUG_USER_FLAG_MASK;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_TEAM_FLAGS: "
					"flags: %lx\n", nubThread->id, flags));

				Team *team = thread_get_current_thread()->team;

				// set the flags
				cpu_status state = disable_interrupts();
				GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

				flags |= team->debug_info.flags & B_TEAM_DEBUG_KERNEL_FLAG_MASK;
				atomic_set(&team->debug_info.flags, flags);

				RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
				restore_interrupts(state);

				break;
			}

			case B_DEBUG_MESSAGE_SET_THREAD_FLAGS:
			{
				// get the parameters
				thread_id threadID = message.set_thread_flags.thread;
				int32 flags = message.set_thread_flags.flags
					& B_THREAD_DEBUG_USER_FLAG_MASK;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_THREAD_FLAGS: "
					"thread: %ld, flags: %lx\n", nubThread->id, threadID,
					flags));

				// set the flags
				Thread* thread = Thread::GetAndLock(threadID);
				if (thread == NULL)
					break;
				BReference<Thread> threadReference(thread, true);
				ThreadLocker threadLocker(thread, true);

				InterruptsSpinLocker threadDebugInfoLocker(
					thread->debug_info.lock);

				if (thread->team == thread_get_current_thread()->team) {
					flags |= thread->debug_info.flags
						& B_THREAD_DEBUG_KERNEL_FLAG_MASK;
					atomic_set(&thread->debug_info.flags, flags);
				}

				break;
			}

			case B_DEBUG_MESSAGE_CONTINUE_THREAD:
			{
				// get the parameters
				thread_id threadID;
				uint32 handleEvent;
				bool singleStep;

				threadID = message.continue_thread.thread;
				handleEvent = message.continue_thread.handle_event;
				singleStep = message.continue_thread.single_step;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_CONTINUE_THREAD: "
					"thread: %ld, handle event: %lu, single step: %d\n",
					nubThread->id, threadID, handleEvent, singleStep));

				// find the thread and get its debug port
				port_id threadDebugPort = -1;
				status_t result = debug_nub_thread_get_thread_debug_port(
					nubThread, threadID, threadDebugPort);

				// send a message to the debugged thread
				if (result == B_OK) {
					debugged_thread_continue commandMessage;
					commandMessage.handle_event = handleEvent;
					commandMessage.single_step = singleStep;

					result = write_port(threadDebugPort,
						B_DEBUGGED_THREAD_MESSAGE_CONTINUE,
						&commandMessage, sizeof(commandMessage));
				}

				break;
			}

			case B_DEBUG_MESSAGE_SET_CPU_STATE:
			{
				// get the parameters
				thread_id threadID = message.set_cpu_state.thread;
				const debug_cpu_state &cpuState
					= message.set_cpu_state.cpu_state;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_CPU_STATE: "
					"thread: %ld\n", nubThread->id, threadID));

				// find the thread and get its debug port
				port_id threadDebugPort = -1;
				status_t result = debug_nub_thread_get_thread_debug_port(
					nubThread, threadID, threadDebugPort);

				// send a message to the debugged thread
				if (result == B_OK) {
					debugged_thread_set_cpu_state commandMessage;
					memcpy(&commandMessage.cpu_state, &cpuState,
						sizeof(debug_cpu_state));
					write_port(threadDebugPort,
						B_DEBUGGED_THREAD_SET_CPU_STATE,
						&commandMessage, sizeof(commandMessage));
				}

				break;
			}

			case B_DEBUG_MESSAGE_GET_CPU_STATE:
			{
				// get the parameters
				thread_id threadID = message.get_cpu_state.thread;
				replyPort = message.get_cpu_state.reply_port;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_GET_CPU_STATE: "
					"thread: %ld\n", nubThread->id, threadID));

				// find the thread and get its debug port
				port_id threadDebugPort = -1;
				status_t result = debug_nub_thread_get_thread_debug_port(
					nubThread, threadID, threadDebugPort);

				// send a message to the debugged thread
				if (threadDebugPort >= 0) {
					debugged_thread_get_cpu_state commandMessage;
					commandMessage.reply_port = replyPort;
					result = write_port(threadDebugPort,
						B_DEBUGGED_THREAD_GET_CPU_STATE, &commandMessage,
						sizeof(commandMessage));
				}

				// send a reply to the debugger in case of error
				if (result != B_OK) {
					reply.get_cpu_state.error = result;
					sendReply = true;
					replySize = sizeof(reply.get_cpu_state);
				}

				break;
			}

			case B_DEBUG_MESSAGE_SET_BREAKPOINT:
			{
				// get the parameters
				replyPort = message.set_breakpoint.reply_port;
				void *address = message.set_breakpoint.address;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_BREAKPOINT: "
					"address: %p\n", nubThread->id, address));

				// check the address
				status_t result = B_OK;
				if (address == NULL
					|| !BreakpointManager::CanAccessAddress(address, false)) {
					result = B_BAD_ADDRESS;
				}

				// set the breakpoint
				if (result == B_OK)
					result = breakpointManager->InstallBreakpoint(address);

				if (result == B_OK)
					update_threads_breakpoints_flag();

				// prepare the reply
				reply.set_breakpoint.error = result;
				replySize = sizeof(reply.set_breakpoint);
				sendReply = true;

				break;
			}

			case B_DEBUG_MESSAGE_CLEAR_BREAKPOINT:
			{
				// get the parameters
				void *address = message.clear_breakpoint.address;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_CLEAR_BREAKPOINT: "
					"address: %p\n", nubThread->id, address));

				// check the address
				status_t result = B_OK;
				if (address == NULL
					|| !BreakpointManager::CanAccessAddress(address, false)) {
					result = B_BAD_ADDRESS;
				}

				// clear the breakpoint
				if (result == B_OK)
					result = breakpointManager->UninstallBreakpoint(address);

				if (result == B_OK)
					update_threads_breakpoints_flag();

				break;
			}

			case B_DEBUG_MESSAGE_SET_WATCHPOINT:
			{
				// get the parameters
				replyPort = message.set_watchpoint.reply_port;
				void *address = message.set_watchpoint.address;
				uint32 type = message.set_watchpoint.type;
				int32 length = message.set_watchpoint.length;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_WATCHPOINT: "
					"address: %p, type: %lu, length: %ld\n", nubThread->id,
					address, type, length));

				// check the address and size
				status_t result = B_OK;
				if (address == NULL
					|| !BreakpointManager::CanAccessAddress(address, false)) {
					result = B_BAD_ADDRESS;
				}
				if (length < 0)
					result = B_BAD_VALUE;

				// set the watchpoint
				if (result == B_OK) {
					result = breakpointManager->InstallWatchpoint(address, type,
						length);
				}

				if (result == B_OK)
					update_threads_breakpoints_flag();

				// prepare the reply
				reply.set_watchpoint.error = result;
				replySize = sizeof(reply.set_watchpoint);
				sendReply = true;

				break;
			}

			case B_DEBUG_MESSAGE_CLEAR_WATCHPOINT:
			{
				// get the parameters
				void *address = message.clear_watchpoint.address;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_CLEAR_WATCHPOINT: "
					"address: %p\n", nubThread->id, address));

				// check the address
				status_t result = B_OK;
				if (address == NULL
					|| !BreakpointManager::CanAccessAddress(address, false)) {
					result = B_BAD_ADDRESS;
				}

				// clear the watchpoint
				if (result == B_OK)
					result = breakpointManager->UninstallWatchpoint(address);

				if (result == B_OK)
					update_threads_breakpoints_flag();

				break;
			}

			case B_DEBUG_MESSAGE_SET_SIGNAL_MASKS:
			{
				// get the parameters
				thread_id threadID = message.set_signal_masks.thread;
				uint64 ignore = message.set_signal_masks.ignore_mask;
				uint64 ignoreOnce = message.set_signal_masks.ignore_once_mask;
				uint32 ignoreOp = message.set_signal_masks.ignore_op;
				uint32 ignoreOnceOp = message.set_signal_masks.ignore_once_op;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_SIGNAL_MASKS: "
					"thread: %ld, ignore: %llx (op: %lu), ignore once: %llx "
					"(op: %lu)\n", nubThread->id, threadID, ignore,
						ignoreOp, ignoreOnce, ignoreOnceOp));

				// set the masks
				Thread* thread = Thread::GetAndLock(threadID);
				if (thread == NULL)
					break;
				BReference<Thread> threadReference(thread, true);
				ThreadLocker threadLocker(thread, true);

				InterruptsSpinLocker threadDebugInfoLocker(
					thread->debug_info.lock);

				if (thread->team == thread_get_current_thread()->team) {
					thread_debug_info &threadDebugInfo = thread->debug_info;
					// set ignore mask
					switch (ignoreOp) {
						case B_DEBUG_SIGNAL_MASK_AND:
							threadDebugInfo.ignore_signals &= ignore;
							break;
						case B_DEBUG_SIGNAL_MASK_OR:
							threadDebugInfo.ignore_signals |= ignore;
							break;
						case B_DEBUG_SIGNAL_MASK_SET:
							threadDebugInfo.ignore_signals = ignore;
							break;
					}

					// set ignore once mask
					switch (ignoreOnceOp) {
						case B_DEBUG_SIGNAL_MASK_AND:
							threadDebugInfo.ignore_signals_once &= ignoreOnce;
							break;
						case B_DEBUG_SIGNAL_MASK_OR:
							threadDebugInfo.ignore_signals_once |= ignoreOnce;
							break;
						case B_DEBUG_SIGNAL_MASK_SET:
							threadDebugInfo.ignore_signals_once = ignoreOnce;
							break;
					}
				}

				break;
			}

			case B_DEBUG_MESSAGE_GET_SIGNAL_MASKS:
			{
				// get the parameters
				replyPort = message.get_signal_masks.reply_port;
				thread_id threadID = message.get_signal_masks.thread;
				status_t result = B_OK;

				// get the masks
				uint64 ignore = 0;
				uint64 ignoreOnce = 0;

				Thread* thread = Thread::GetAndLock(threadID);
				if (thread != NULL) {
					BReference<Thread> threadReference(thread, true);
					ThreadLocker threadLocker(thread, true);

					InterruptsSpinLocker threadDebugInfoLocker(
						thread->debug_info.lock);

					ignore = thread->debug_info.ignore_signals;
					ignoreOnce = thread->debug_info.ignore_signals_once;
				} else
					result = B_BAD_THREAD_ID;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_GET_SIGNAL_MASKS: "
					"reply port: %ld, thread: %ld, ignore: %llx, "
					"ignore once: %llx, result: %lx\n", nubThread->id,
					replyPort, threadID, ignore, ignoreOnce, result));

				// prepare the message
				reply.get_signal_masks.error = result;
				reply.get_signal_masks.ignore_mask = ignore;
				reply.get_signal_masks.ignore_once_mask = ignoreOnce;
				replySize = sizeof(reply.get_signal_masks);
				sendReply = true;
				break;
			}

			case B_DEBUG_MESSAGE_SET_SIGNAL_HANDLER:
			{
				// get the parameters
				int signal = message.set_signal_handler.signal;
				struct sigaction &handler = message.set_signal_handler.handler;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_SIGNAL_HANDLER: "
					"signal: %d, handler: %p\n", nubThread->id,
					signal, handler.sa_handler));

				// set the handler
				sigaction(signal, &handler, NULL);

				break;
			}

			case B_DEBUG_MESSAGE_GET_SIGNAL_HANDLER:
			{
				// get the parameters
				replyPort = message.get_signal_handler.reply_port;
				int signal = message.get_signal_handler.signal;
				status_t result = B_OK;

				// get the handler
				if (sigaction(signal, NULL, &reply.get_signal_handler.handler)
						!= 0) {
					result = errno;
				}

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_GET_SIGNAL_HANDLER: "
					"reply port: %ld, signal: %d, handler: %p\n", nubThread->id,
					replyPort, signal,
					reply.get_signal_handler.handler.sa_handler));

				// prepare the message
				reply.get_signal_handler.error = result;
				replySize = sizeof(reply.get_signal_handler);
				sendReply = true;
				break;
			}

			case B_DEBUG_MESSAGE_PREPARE_HANDOVER:
			{
				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_PREPARE_HANDOVER\n",
					nubThread->id));

				Team *team = nubThread->team;

				// Acquire the debugger write lock. As soon as we have it and
				// have set the B_TEAM_DEBUG_DEBUGGER_HANDOVER flag, no thread
				// will write anything to the debugger port anymore.
				status_t result = acquire_sem_etc(writeLock, 1,
					B_KILL_CAN_INTERRUPT, 0);
				if (result == B_OK) {
					// set the respective team debug flag
					cpu_status state = disable_interrupts();
					GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

					atomic_or(&team->debug_info.flags,
						B_TEAM_DEBUG_DEBUGGER_HANDOVER);
					BreakpointManager* breakpointManager
						= team->debug_info.breakpoint_manager;

					RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
					restore_interrupts(state);

					// remove all installed breakpoints
					breakpointManager->RemoveAllBreakpoints();

					release_sem(writeLock);
				} else {
					// We probably got a SIGKILL. If so, we will terminate when
					// reading the next message fails.
				}

				break;
			}

			case B_DEBUG_MESSAGE_HANDED_OVER:
			{
				// notify all threads that the debugger has changed
				broadcast_debugged_thread_message(nubThread,
					B_DEBUGGED_THREAD_DEBUGGER_CHANGED, NULL, 0);

				break;
			}

			case B_DEBUG_START_PROFILER:
			{
				// get the parameters
				thread_id threadID = message.start_profiler.thread;
				replyPort = message.start_profiler.reply_port;
				area_id sampleArea = message.start_profiler.sample_area;
				int32 stackDepth = message.start_profiler.stack_depth;
				bool variableStackDepth
					= message.start_profiler.variable_stack_depth;
				bigtime_t interval = max_c(message.start_profiler.interval,
					B_DEBUG_MIN_PROFILE_INTERVAL);
				status_t result = B_OK;

				TRACE(("nub thread %ld: B_DEBUG_START_PROFILER: "
					"thread: %ld, sample area: %ld\n", nubThread->id, threadID,
					sampleArea));

				if (stackDepth < 1)
					stackDepth = 1;
				else if (stackDepth > B_DEBUG_STACK_TRACE_DEPTH)
					stackDepth = B_DEBUG_STACK_TRACE_DEPTH;

				// provision for an extra entry per hit (for the number of
				// samples), if variable stack depth
				if (variableStackDepth)
					stackDepth++;

				// clone the sample area
				area_info areaInfo;
				if (result == B_OK)
					result = get_area_info(sampleArea, &areaInfo);

				area_id clonedSampleArea = -1;
				void* samples = NULL;
				if (result == B_OK) {
					clonedSampleArea = clone_area("profiling samples", &samples,
						B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
						sampleArea);
					if (clonedSampleArea >= 0) {
						// we need the memory locked
						result = lock_memory(samples, areaInfo.size,
							B_READ_DEVICE);
						if (result != B_OK) {
							delete_area(clonedSampleArea);
							clonedSampleArea = -1;
						}
					} else
						result = clonedSampleArea;
				}

				// get the thread and set the profile info
				int32 imageEvent = nubThread->team->debug_info.image_event;
				if (result == B_OK) {
					Thread* thread = Thread::GetAndLock(threadID);
					BReference<Thread> threadReference(thread, true);
					ThreadLocker threadLocker(thread, true);

					if (thread != NULL && thread->team == nubThread->team) {
						thread_debug_info &threadDebugInfo = thread->debug_info;

						InterruptsSpinLocker threadDebugInfoLocker(
							threadDebugInfo.lock);

						if (threadDebugInfo.profile.samples == NULL) {
							threadDebugInfo.profile.interval = interval;
							threadDebugInfo.profile.sample_area
								= clonedSampleArea;
							threadDebugInfo.profile.samples = (addr_t*)samples;
							threadDebugInfo.profile.max_samples
								= areaInfo.size / sizeof(addr_t);
							threadDebugInfo.profile.flush_threshold
								= threadDebugInfo.profile.max_samples
									* B_DEBUG_PROFILE_BUFFER_FLUSH_THRESHOLD
									/ 100;
							threadDebugInfo.profile.sample_count = 0;
							threadDebugInfo.profile.dropped_ticks = 0;
							threadDebugInfo.profile.stack_depth = stackDepth;
							threadDebugInfo.profile.variable_stack_depth
								= variableStackDepth;
							threadDebugInfo.profile.buffer_full = false;
							threadDebugInfo.profile.interval_left = interval;
							threadDebugInfo.profile.installed_timer = NULL;
							threadDebugInfo.profile.image_event = imageEvent;
							threadDebugInfo.profile.last_image_event
								= imageEvent;
						} else
							result = B_BAD_VALUE;
					} else
						result = B_BAD_THREAD_ID;
				}

				// on error unlock and delete the sample area
				if (result != B_OK) {
					if (clonedSampleArea >= 0) {
						unlock_memory(samples, areaInfo.size, B_READ_DEVICE);
						delete_area(clonedSampleArea);
					}
				}

				// send a reply to the debugger
				reply.start_profiler.error = result;
				reply.start_profiler.interval = interval;
				reply.start_profiler.image_event = imageEvent;
				sendReply = true;
				replySize = sizeof(reply.start_profiler);

				break;
			}

			case B_DEBUG_STOP_PROFILER:
			{
				// get the parameters
				thread_id threadID = message.stop_profiler.thread;
				replyPort = message.stop_profiler.reply_port;
				status_t result = B_OK;

				TRACE(("nub thread %ld: B_DEBUG_STOP_PROFILER: "
					"thread: %ld\n", nubThread->id, threadID));

				area_id sampleArea = -1;
				addr_t* samples = NULL;
				int32 sampleCount = 0;
				int32 stackDepth = 0;
				bool variableStackDepth = false;
				int32 imageEvent = 0;
				int32 droppedTicks = 0;

				// get the thread and detach the profile info
				Thread* thread = Thread::GetAndLock(threadID);
				BReference<Thread> threadReference(thread, true);
				ThreadLocker threadLocker(thread, true);

				if (thread && thread->team == nubThread->team) {
					thread_debug_info &threadDebugInfo = thread->debug_info;

					InterruptsSpinLocker threadDebugInfoLocker(
						threadDebugInfo.lock);

					if (threadDebugInfo.profile.samples != NULL) {
						sampleArea = threadDebugInfo.profile.sample_area;
						samples = threadDebugInfo.profile.samples;
						sampleCount = threadDebugInfo.profile.sample_count;
						droppedTicks = threadDebugInfo.profile.dropped_ticks;
						stackDepth = threadDebugInfo.profile.stack_depth;
						variableStackDepth
							= threadDebugInfo.profile.variable_stack_depth;
						imageEvent = threadDebugInfo.profile.image_event;
						threadDebugInfo.profile.sample_area = -1;
						threadDebugInfo.profile.samples = NULL;
						threadDebugInfo.profile.buffer_full = false;
						threadDebugInfo.profile.dropped_ticks = 0;
					} else
						result = B_BAD_VALUE;
				} else
					result = B_BAD_THREAD_ID;

				threadLocker.Unlock();

				// prepare the reply
				if (result == B_OK) {
					reply.profiler_update.origin.thread = threadID;
					reply.profiler_update.image_event = imageEvent;
					reply.profiler_update.stack_depth = stackDepth;
					reply.profiler_update.variable_stack_depth
						= variableStackDepth;
					reply.profiler_update.sample_count = sampleCount;
					reply.profiler_update.dropped_ticks = droppedTicks;
					reply.profiler_update.stopped = true;
				} else
					reply.profiler_update.origin.thread = result;

				replySize = sizeof(debug_profiler_update);
				sendReply = true;

				if (sampleArea >= 0) {
					area_info areaInfo;
					if (get_area_info(sampleArea, &areaInfo) == B_OK) {
						unlock_memory(samples, areaInfo.size, B_READ_DEVICE);
						delete_area(sampleArea);
					}
				}
			}
		}

		// send the reply, if necessary
		if (sendReply) {
			status_t error = kill_interruptable_write_port(replyPort, command,
				&reply, replySize);

			if (error != B_OK) {
				// The debugger port is either not longer existing or we got
				// interrupted by a kill signal. In either case we terminate.
				TRACE(("nub thread %ld: failed to send reply to port %ld: %s\n",
					nubThread->id, replyPort, strerror(error)));

				nub_thread_cleanup(nubThread);
				return error;
			}
		}
	}
}


/**	\brief Helper function for install_team_debugger(), that sets up the team
		   and thread debug infos.

	The caller must hold the team's lock as well as the team debug info lock.

	The function also clears the arch specific team and thread debug infos
	(including among other things formerly set break/watchpoints).
 */
static void
install_team_debugger_init_debug_infos(Team *team, team_id debuggerTeam,
	port_id debuggerPort, port_id nubPort, thread_id nubThread,
	sem_id debuggerPortWriteLock, thread_id causingThread)
{
	atomic_set(&team->debug_info.flags,
		B_TEAM_DEBUG_DEFAULT_FLAGS | B_TEAM_DEBUG_DEBUGGER_INSTALLED);
	team->debug_info.nub_port = nubPort;
	team->debug_info.nub_thread = nubThread;
	team->debug_info.debugger_team = debuggerTeam;
	team->debug_info.debugger_port = debuggerPort;
	team->debug_info.debugger_write_lock = debuggerPortWriteLock;
	team->debug_info.causing_thread = causingThread;

	arch_clear_team_debug_info(&team->debug_info.arch_info);

	// set the user debug flags and signal masks of all threads to the default
	for (Thread *thread = team->thread_list; thread;
			thread = thread->team_next) {
		SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

		if (thread->id == nubThread) {
			atomic_set(&thread->debug_info.flags, B_THREAD_DEBUG_NUB_THREAD);
		} else {
			int32 flags = thread->debug_info.flags
				& ~B_THREAD_DEBUG_USER_FLAG_MASK;
			atomic_set(&thread->debug_info.flags,
				flags | B_THREAD_DEBUG_DEFAULT_FLAGS);
			thread->debug_info.ignore_signals = 0;
			thread->debug_info.ignore_signals_once = 0;

			arch_clear_thread_debug_info(&thread->debug_info.arch_info);
		}
	}

	// update the thread::flags fields
	update_threads_debugger_installed_flag(team);
}


static port_id
install_team_debugger(team_id teamID, port_id debuggerPort,
	thread_id causingThread, bool useDefault, bool dontReplace)
{
	TRACE(("install_team_debugger(team: %ld, port: %ld, default: %d, "
		"dontReplace: %d)\n", teamID, debuggerPort, useDefault, dontReplace));

	if (useDefault)
		debuggerPort = atomic_get(&sDefaultDebuggerPort);

	// get the debugger team
	port_info debuggerPortInfo;
	status_t error = get_port_info(debuggerPort, &debuggerPortInfo);
	if (error != B_OK) {
		TRACE(("install_team_debugger(): Failed to get debugger port info: "
			"%lx\n", error));
		return error;
	}
	team_id debuggerTeam = debuggerPortInfo.team;

	// Check the debugger team: It must neither be the kernel team nor the
	// debugged team.
	if (debuggerTeam == team_get_kernel_team_id() || debuggerTeam == teamID) {
		TRACE(("install_team_debugger(): Can't debug kernel or debugger team. "
			"debugger: %ld, debugged: %ld\n", debuggerTeam, teamID));
		return B_NOT_ALLOWED;
	}

	// get the team
	Team* team;
	ConditionVariable debugChangeCondition;
	error = prepare_debugger_change(teamID, debugChangeCondition, team);
	if (error != B_OK)
		return error;

	// get the real team ID
	teamID = team->id;

	// check, if a debugger is already installed

	bool done = false;
	port_id result = B_ERROR;
	bool handOver = false;
	port_id oldDebuggerPort = -1;
	port_id nubPort = -1;

	TeamLocker teamLocker(team);
	cpu_status state = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	int32 teamDebugFlags = team->debug_info.flags;

	if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// There's already a debugger installed.
		if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_HANDOVER) {
			if (dontReplace) {
				// We're fine with already having a debugger.
				error = B_OK;
				done = true;
				result = team->debug_info.nub_port;
			} else {
				// a handover to another debugger is requested
				// Set the handing-over flag -- we'll clear both flags after
				// having sent the handed-over message to the new debugger.
				atomic_or(&team->debug_info.flags,
					B_TEAM_DEBUG_DEBUGGER_HANDING_OVER);

				oldDebuggerPort = team->debug_info.debugger_port;
				result = nubPort = team->debug_info.nub_port;
				if (causingThread < 0)
					causingThread = team->debug_info.causing_thread;

				// set the new debugger
				install_team_debugger_init_debug_infos(team, debuggerTeam,
					debuggerPort, nubPort, team->debug_info.nub_thread,
					team->debug_info.debugger_write_lock, causingThread);

				handOver = true;
				done = true;
			}
		} else {
			// there's already a debugger installed
			error = (dontReplace ? B_OK : B_BAD_VALUE);
			done = true;
			result = team->debug_info.nub_port;
		}
	} else if ((teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_DISABLED) != 0
		&& useDefault) {
		// No debugger yet, disable_debugger() had been invoked, and we
		// would install the default debugger. Just fail.
		error = B_BAD_VALUE;
	}

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	restore_interrupts(state);
	teamLocker.Unlock();

	if (handOver && set_port_owner(nubPort, debuggerTeam) != B_OK) {
		// The old debugger must just have died. Just proceed as
		// if there was no debugger installed. We may still be too
		// early, in which case we'll fail, but this race condition
		// should be unbelievably rare and relatively harmless.
		handOver = false;
		done = false;
	}

	if (handOver) {
		// prepare the handed-over message
		debug_handed_over notification;
		notification.origin.thread = -1;
		notification.origin.team = teamID;
		notification.origin.nub_port = nubPort;
		notification.debugger = debuggerTeam;
		notification.debugger_port = debuggerPort;
		notification.causing_thread = causingThread;

		// notify the new debugger
		error = write_port_etc(debuggerPort,
			B_DEBUGGER_MESSAGE_HANDED_OVER, &notification,
			sizeof(notification), B_RELATIVE_TIMEOUT, 0);
		if (error != B_OK) {
			dprintf("install_team_debugger(): Failed to send message to new "
				"debugger: %s\n", strerror(error));
		}

		// clear the handed-over and handing-over flags
		state = disable_interrupts();
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		atomic_and(&team->debug_info.flags,
			~(B_TEAM_DEBUG_DEBUGGER_HANDOVER
				| B_TEAM_DEBUG_DEBUGGER_HANDING_OVER));

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
		restore_interrupts(state);

		finish_debugger_change(team);

		// notify the nub thread
		kill_interruptable_write_port(nubPort, B_DEBUG_MESSAGE_HANDED_OVER,
			NULL, 0);

		// notify the old debugger
		error = write_port_etc(oldDebuggerPort,
			B_DEBUGGER_MESSAGE_HANDED_OVER, &notification,
			sizeof(notification), B_RELATIVE_TIMEOUT, 0);
		if (error != B_OK) {
			TRACE(("install_team_debugger(): Failed to send message to old "
				"debugger: %s\n", strerror(error)));
		}

		TRACE(("install_team_debugger() done: handed over to debugger: team: "
			"%ld, port: %ld\n", debuggerTeam, debuggerPort));

		return result;
	}

	if (done || error != B_OK) {
		TRACE(("install_team_debugger() done1: %ld\n",
			(error == B_OK ? result : error)));
		finish_debugger_change(team);
		return (error == B_OK ? result : error);
	}

	// create the debugger write lock semaphore
	char nameBuffer[B_OS_NAME_LENGTH];
	snprintf(nameBuffer, sizeof(nameBuffer), "team %" B_PRId32 " debugger port "
		"write", teamID);
	sem_id debuggerWriteLock = create_sem(1, nameBuffer);
	if (debuggerWriteLock < 0)
		error = debuggerWriteLock;

	// create the nub port
	snprintf(nameBuffer, sizeof(nameBuffer), "team %" B_PRId32 " debug", teamID);
	if (error == B_OK) {
		nubPort = create_port(1, nameBuffer);
		if (nubPort < 0)
			error = nubPort;
		else
			result = nubPort;
	}

	// make the debugger team the port owner; thus we know, if the debugger is
	// gone and can cleanup
	if (error == B_OK)
		error = set_port_owner(nubPort, debuggerTeam);

	// create the breakpoint manager
	BreakpointManager* breakpointManager = NULL;
	if (error == B_OK) {
		breakpointManager = new(std::nothrow) BreakpointManager;
		if (breakpointManager != NULL)
			error = breakpointManager->Init();
		else
			error = B_NO_MEMORY;
	}

	// spawn the nub thread
	thread_id nubThread = -1;
	if (error == B_OK) {
		snprintf(nameBuffer, sizeof(nameBuffer), "team %" B_PRId32 " debug task",
			teamID);
		nubThread = spawn_kernel_thread_etc(debug_nub_thread, nameBuffer,
			B_NORMAL_PRIORITY, NULL, teamID);
		if (nubThread < 0)
			error = nubThread;
	}

	// now adjust the debug info accordingly
	if (error == B_OK) {
		TeamLocker teamLocker(team);
		state = disable_interrupts();
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		team->debug_info.breakpoint_manager = breakpointManager;
		install_team_debugger_init_debug_infos(team, debuggerTeam,
			debuggerPort, nubPort, nubThread, debuggerWriteLock,
			causingThread);

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
		restore_interrupts(state);
	}

	finish_debugger_change(team);

	// if everything went fine, resume the nub thread, otherwise clean up
	if (error == B_OK) {
		resume_thread(nubThread);
	} else {
		// delete port and terminate thread
		if (nubPort >= 0) {
			set_port_owner(nubPort, B_CURRENT_TEAM);
			delete_port(nubPort);
		}
		if (nubThread >= 0) {
			int32 result;
			wait_for_thread(nubThread, &result);
		}

		delete breakpointManager;
	}

	TRACE(("install_team_debugger() done2: %ld\n",
		(error == B_OK ? result : error)));
	return (error == B_OK ? result : error);
}


static status_t
ensure_debugger_installed()
{
	port_id port = install_team_debugger(B_CURRENT_TEAM, -1,
		thread_get_current_thread_id(), true, true);
	return port >= 0 ? B_OK : port;
}


// #pragma mark -


void
_user_debugger(const char *userMessage)
{
	// install the default debugger, if there is none yet
	status_t error = ensure_debugger_installed();
	if (error != B_OK) {
		// time to commit suicide
		char buffer[128];
		ssize_t length = user_strlcpy(buffer, userMessage, sizeof(buffer));
		if (length >= 0) {
			dprintf("_user_debugger(): Failed to install debugger. Message is: "
				"`%s'\n", buffer);
		} else {
			dprintf("_user_debugger(): Failed to install debugger. Message is: "
				"%p (%s)\n", userMessage, strerror(length));
		}
		_user_exit_team(1);
	}

	// prepare the message
	debug_debugger_call message;
	message.message = (void*)userMessage;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_DEBUGGER_CALL, &message,
		sizeof(message), true);
}


int
_user_disable_debugger(int state)
{
	Team *team = thread_get_current_thread()->team;

	TRACE(("_user_disable_debugger(%d): team: %ld\n", state, team->id));

	cpu_status cpuState = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	int32 oldFlags;
	if (state) {
		oldFlags = atomic_or(&team->debug_info.flags,
			B_TEAM_DEBUG_DEBUGGER_DISABLED);
	} else {
		oldFlags = atomic_and(&team->debug_info.flags,
			~B_TEAM_DEBUG_DEBUGGER_DISABLED);
	}

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	restore_interrupts(cpuState);

	// TODO: Check, if the return value is really the old state.
	return !(oldFlags & B_TEAM_DEBUG_DEBUGGER_DISABLED);
}


status_t
_user_install_default_debugger(port_id debuggerPort)
{
	// if supplied, check whether the port is a valid port
	if (debuggerPort >= 0) {
		port_info portInfo;
		status_t error = get_port_info(debuggerPort, &portInfo);
		if (error != B_OK)
			return error;

		// the debugger team must not be the kernel team
		if (portInfo.team == team_get_kernel_team_id())
			return B_NOT_ALLOWED;
	}

	atomic_set(&sDefaultDebuggerPort, debuggerPort);

	return B_OK;
}


port_id
_user_install_team_debugger(team_id teamID, port_id debuggerPort)
{
	return install_team_debugger(teamID, debuggerPort, -1, false, false);
}


status_t
_user_remove_team_debugger(team_id teamID)
{
	Team* team;
	ConditionVariable debugChangeCondition;
	status_t error = prepare_debugger_change(teamID, debugChangeCondition,
		team);
	if (error != B_OK)
		return error;

	InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

	thread_id nubThread = -1;
	port_id nubPort = -1;

	if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// there's a debugger installed
		nubThread = team->debug_info.nub_thread;
		nubPort = team->debug_info.nub_port;
	} else {
		// no debugger installed
		error = B_BAD_VALUE;
	}

	debugInfoLocker.Unlock();

	// Delete the nub port -- this will cause the nub thread to terminate and
	// remove the debugger.
	if (nubPort >= 0)
		delete_port(nubPort);

	finish_debugger_change(team);

	// wait for the nub thread
	if (nubThread >= 0)
		wait_for_thread(nubThread, NULL);

	return error;
}


status_t
_user_debug_thread(thread_id threadID)
{
	TRACE(("[%ld] _user_debug_thread(%ld)\n", find_thread(NULL), threadID));

	// get the thread
	Thread* thread = Thread::GetAndLock(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// we can't debug the kernel team
	if (thread->team == team_get_kernel_team())
		return B_NOT_ALLOWED;

	InterruptsLocker interruptsLocker;
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	// If the thread is already dying, it's too late to debug it.
	if ((thread->debug_info.flags & B_THREAD_DEBUG_DYING) != 0)
		return B_BAD_THREAD_ID;

	// don't debug the nub thread
	if ((thread->debug_info.flags & B_THREAD_DEBUG_NUB_THREAD) != 0)
		return B_NOT_ALLOWED;

	// already marked stopped?
	if ((thread->debug_info.flags & B_THREAD_DEBUG_STOPPED) != 0)
		return B_OK;

	// set the flag that tells the thread to stop as soon as possible
	atomic_or(&thread->debug_info.flags, B_THREAD_DEBUG_STOP);

	update_thread_user_debug_flag(thread);

	// resume/interrupt the thread, if necessary
	threadDebugInfoLocker.Unlock();
	SpinLocker schedulerLocker(gSchedulerLock);

	switch (thread->state) {
		case B_THREAD_SUSPENDED:
			// thread suspended: wake it up
			scheduler_enqueue_in_run_queue(thread);
			break;

		default:
			// thread may be waiting: interrupt it
			thread_interrupt(thread, false);
				// TODO: If the thread is already in the kernel and e.g.
				// about to acquire a semaphore (before
				// thread_prepare_to_block()), we won't interrupt it.
				// Maybe we should rather send a signal (SIGTRAP).
			scheduler_reschedule_if_necessary_locked();
			break;
	}

	return B_OK;
}


void
_user_wait_for_debugger(void)
{
	debug_thread_debugged message;
	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED, &message,
		sizeof(message), false);
}


status_t
_user_set_debugger_breakpoint(void *address, uint32 type, int32 length,
	bool watchpoint)
{
	// check the address and size
	if (address == NULL || !BreakpointManager::CanAccessAddress(address, false))
		return B_BAD_ADDRESS;
	if (watchpoint && length < 0)
		return B_BAD_VALUE;

	// check whether a debugger is installed already
	team_debug_info teamDebugInfo;
	get_team_debug_info(teamDebugInfo);
	if (teamDebugInfo.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED)
		return B_BAD_VALUE;

	// We can't help it, here's a small but relatively harmless race condition,
	// since a debugger could be installed in the meantime. The worst case is
	// that we install a break/watchpoint the debugger doesn't know about.

	// set the break/watchpoint
	status_t result;
	if (watchpoint)
		result = arch_set_watchpoint(address, type, length);
	else
		result = arch_set_breakpoint(address);

	if (result == B_OK)
		update_threads_breakpoints_flag();

	return result;
}


status_t
_user_clear_debugger_breakpoint(void *address, bool watchpoint)
{
	// check the address
	if (address == NULL || !BreakpointManager::CanAccessAddress(address, false))
		return B_BAD_ADDRESS;

	// check whether a debugger is installed already
	team_debug_info teamDebugInfo;
	get_team_debug_info(teamDebugInfo);
	if (teamDebugInfo.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED)
		return B_BAD_VALUE;

	// We can't help it, here's a small but relatively harmless race condition,
	// since a debugger could be installed in the meantime. The worst case is
	// that we clear a break/watchpoint the debugger has just installed.

	// clear the break/watchpoint
	status_t result;
	if (watchpoint)
		result = arch_clear_watchpoint(address);
	else
		result = arch_clear_breakpoint(address);

	if (result == B_OK)
		update_threads_breakpoints_flag();

	return result;
}
