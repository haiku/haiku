/*
 * Copyright 2005-2006, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <debugger.h>
#include <kernel.h>
#include <KernelExport.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <ksyscalls.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <user_debugger.h>
#include <vm.h>
#include <vm_types.h>
#include <arch/user_debugger.h>

//#define TRACE_USER_DEBUGGER
#ifdef TRACE_USER_DEBUGGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static port_id sDefaultDebuggerPort = -1;
	// accessed atomically


static status_t ensure_debugger_installed(team_id teamID, port_id *port = NULL);
static void get_team_debug_info(team_debug_info &teamDebugInfo);


static ssize_t
kill_interruptable_read_port(port_id port, int32 *code, void *buffer,
	size_t bufferSize)
{
	return read_port_etc(port, code, buffer, bufferSize,
		B_KILL_CAN_INTERRUPT, 0);
}


static status_t
kill_interruptable_write_port(port_id port, int32 code, const void *buffer,
	size_t bufferSize)
{
	return write_port_etc(port, code, buffer, bufferSize,
		B_KILL_CAN_INTERRUPT, 0);
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

		if (initLock)
			info->lock = 0;
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
void
destroy_team_debug_info(struct team_debug_info *info)
{
	if (info) {
		arch_destroy_team_debug_info(&info->arch_info);

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
	}
}


void
clear_thread_debug_info(struct thread_debug_info *info, bool dying)
{
	if (info) {
		arch_clear_thread_debug_info(&info->arch_info);
		atomic_set(&info->flags,
			B_THREAD_DEBUG_DEFAULT_FLAGS | (dying ? B_THREAD_DEBUG_DYING : 0));
		info->debug_port = -1;
		info->ignore_signals = 0;
		info->ignore_signals_once = 0;
	}
}


void
destroy_thread_debug_info(struct thread_debug_info *info)
{
	if (info) {
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


void
user_debug_prepare_for_exec()
{
	struct thread *thread = thread_get_current_thread();
	struct team *team = thread->team;

	// If a debugger is installed for the team and the thread debug stuff
	// initialized, changed the ownership of the debug port for the thread
	// to the kernel team, since exec_team() deletes all ports owned by this
	// team. We change the ownership back later.
	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// get the port
		port_id debugPort = -1;

		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		if (thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED)
			debugPort = thread->debug_info.debug_port;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		// set the new port ownership
		if (debugPort >= 0)
			set_port_owner(debugPort, team_get_kernel_team_id());
	}
}


void
user_debug_finish_after_exec()
{
	struct thread *thread = thread_get_current_thread();
	struct team *team = thread->team;

	// If a debugger is installed for the team and the thread debug stuff
	// initialized for this thread, change the ownership of its debug port
	// back to this team.
	if (atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// get the port
		port_id debugPort = -1;

		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		if (thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED)
			debugPort = thread->debug_info.debug_port;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

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
	struct thread *thread = thread_get_current_thread();

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
	struct thread *thread = thread_get_current_thread();

	TRACE(("thread_hit_debug_event(): thread: %ld, event: %lu, message: %p, "
		"size: %ld\n", thread->id, (uint32)event, message, size));

	// check, if there's a debug port already
	bool setPort = !(atomic_get(&thread->debug_info.flags)
		& B_THREAD_DEBUG_INITIALIZED);

	// create a port, if there is none yet
	port_id port = -1;
	if (setPort) {
		char nameBuffer[128];
		snprintf(nameBuffer, sizeof(nameBuffer), "nub to thread %ld",
			thread->id);

		port = create_port(1, nameBuffer);
		if (port < 0) {
			dprintf("thread_hit_debug_event(): Failed to create debug port: "
				"%s\n", strerror(port));
			return port;
		}

		setPort = true;
	}

	// check the debug info structures once more: get the debugger port, set
	// the thread's debug port, and update the thread's debug flags
	port_id deletePort = port;
	port_id debuggerPort = -1;
	port_id nubPort = -1;
	status_t error = B_OK;
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

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

	RELEASE_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);
	RELEASE_THREAD_LOCK();
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
			ssize_t commandMessageSize = kill_interruptable_read_port(port,
				&command, &commandMessage, sizeof(commandMessage));
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
	GRAB_THREAD_LOCK();

	// check, if the team is still being debugged
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		// update the single-step flag
		if (singleStep) {
			atomic_or(&thread->debug_info.flags,
				B_THREAD_DEBUG_SINGLE_STEP);
		} else {
			atomic_and(&thread->debug_info.flags,
				~B_THREAD_DEBUG_SINGLE_STEP);
		}

		// unset the "stopped" state
		atomic_and(&thread->debug_info.flags, ~B_THREAD_DEBUG_STOPPED);

	} else {
		// the debugger is gone: cleanup our info completely
		threadDebugInfo = thread->debug_info;
		clear_thread_debug_info(&thread->debug_info, false);
		destroyThreadInfo = true;
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

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

	return result;
}


void
user_debug_pre_syscall(uint32 syscall, void *args)
{
	// check whether a debugger is installed
	struct thread *thread = thread_get_current_thread();
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
	struct thread *thread = thread_get_current_thread();
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
	if (sigaction(signal, NULL, &signalAction) == B_OK
		&& signalAction.sa_handler != SIG_DFL) {
		return true;
	}

	// ensure that a debugger is installed for this team
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_exception_occurred(): Failed to install debugger: "
			"thread: %ld: %s\n", thread->id, strerror(error));
		return true;
	}

	// prepare the message
	debug_exception_occurred message;
	message.exception = exception;
	message.signal = signal;

	status_t result = thread_hit_debug_event(
		B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED, &message, sizeof(message), true);
	return (result != B_THREAD_DEBUG_IGNORE_EVENT);
}


bool
user_debug_handle_signal(int signal, struct sigaction *handler, bool deadly)
{
	// check, if a debugger is installed and is interested in signals
	struct thread *thread = thread_get_current_thread();
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
	// ensure that a debugger is installed for this team
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_stop_thread(): Failed to install debugger: "
			"thread: %ld: %s\n", thread->id, strerror(error));
		return;
	}

	// prepare the message
	debug_thread_debugged message;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED, &message,
		sizeof(message), true);
}


void
user_debug_team_created(team_id teamID)
{
	// check, if a debugger is installed and is interested in team creation
	// events
	struct thread *thread = thread_get_current_thread();
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
			// TODO: Would it be OK to wait here?
	}
}


void
user_debug_thread_created(thread_id threadID)
{
	// check, if a debugger is installed and is interested in thread events
	struct thread *thread = thread_get_current_thread();
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
	// get the team debug flags and debugger port
	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	struct team *team = team_get_team_struct_locked(teamID);

	int32 teamDebugFlags = 0;
	port_id debuggerPort = -1;
	if (team) {
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		teamDebugFlags = atomic_get(&team->debug_info.flags);
		debuggerPort = team->debug_info.debugger_port;

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// check, if a debugger is installed and is interested in thread events
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_THREADS)) {
		return;
	}

	// notify the debugger
	if (debuggerPort >= 0) {
		debug_thread_deleted message;
		message.origin.thread = threadID;
		message.origin.team = teamID;
		message.origin.nub_port = -1;
		debugger_write(debuggerPort, B_DEBUGGER_MESSAGE_THREAD_DELETED,
			&message, sizeof(message), true);
			// TODO: Would it be OK to wait here?
	}
}


void
user_debug_image_created(const image_info *imageInfo)
{
	// check, if a debugger is installed and is interested in image events
	struct thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_IMAGES)) {
		return;
	}

	// prepare the message
	debug_image_created message;
	memcpy(&message.info, imageInfo, sizeof(image_info));

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_image_deleted(const image_info *imageInfo)
{
	// check, if a debugger is installed and is interested in image events
	struct thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_IMAGES)) {
		return;
	}

	// prepare the message
	debug_image_deleted message;
	memcpy(&message.info, imageInfo, sizeof(image_info));

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), true);
}


void
user_debug_breakpoint_hit(bool software)
{
	// ensure that a debugger is installed for this team
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_breakpoint_hit(): Failed to install debugger: "
			"thread: %ld: %s\n", thread->id, strerror(error));
		return;
	}

	// prepare the message
	debug_breakpoint_hit message;
	message.software = software;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_BREAKPOINT_HIT, &message,
		sizeof(message), true);
}


void
user_debug_watchpoint_hit()
{
	// ensure that a debugger is installed for this team
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_watchpoint_hit(): Failed to install debugger: "
			"thread: %ld: %s\n", thread->id, strerror(error));
		return;
	}

	// prepare the message
	debug_watchpoint_hit message;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_WATCHPOINT_HIT, &message,
		sizeof(message), true);
}


void
user_debug_single_stepped()
{
	// ensure that a debugger is installed for this team
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_watchpoint_hit(): Failed to install debugger: "
			"thread: %ld: %s\n", thread->id, strerror(error));
		return;
	}

	// prepare the message
	debug_single_step message;
	arch_get_debug_cpu_state(&message.cpu_state);

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_SINGLE_STEP, &message,
		sizeof(message), true);
}


/**	\brief Called by the debug nub thread of a team to broadcast a message
 *		   that are initialized for debugging (and thus have a debug port).
 */
static void
broadcast_debugged_thread_message(struct thread *nubThread, int32 code,
	const void *message, int32 size)
{
	// iterate through the threads
	thread_info threadInfo;
	int32 cookie = 0;
	while (get_next_thread_info(nubThread->team->id, &cookie, &threadInfo)
			== B_OK) {
		// find the thread and get its debug port
		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		port_id threadDebugPort = -1;
		thread_id threadID = -1;
		struct thread *thread
			= thread_get_thread_struct_locked(threadInfo.thread);
		if (thread && thread != nubThread && thread->team == nubThread->team
			&& thread->debug_info.flags & B_THREAD_DEBUG_INITIALIZED) {
			threadDebugPort = thread->debug_info.debug_port;
			threadID = thread->id;
		}

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		// send the message to the thread
		if (threadDebugPort >= 0) {
			status_t error = kill_interruptable_write_port(threadDebugPort,
				code, message, size);
			if (error != B_OK) {
				TRACE(("broadcast_debugged_thread_message(): Failed to send "
					"message to thread %ld: %lx\n", threadID, error));
			}
		}
	}
}


static void
nub_thread_cleanup(struct thread *nubThread)
{
	TRACE(("nub_thread_cleanup(%ld): debugger port: %ld\n", nubThread->id,
		nubThread->team->debug_info.debugger_port));

	team_debug_info teamDebugInfo;
	bool destroyDebugInfo = false;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();
	GRAB_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);

	team_debug_info &info = nubThread->team->debug_info;
	if (info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED
		&& info.nub_thread == nubThread->id) {
		teamDebugInfo = info;
		clear_team_debug_info(&info, false);
		destroyDebugInfo = true;
	}

	RELEASE_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (destroyDebugInfo)
		destroy_team_debug_info(&teamDebugInfo);

	// notify all threads that the debugger is gone
	broadcast_debugged_thread_message(nubThread,
		B_DEBUGGED_THREAD_DEBUGGER_CHANGED, NULL, 0);
}


/**	\brief Reads data from user memory.
 *
 *	Tries to read \a size bytes of data from user memory address \a address
 *	into the supplied buffer \a buffer. If only a part could be read the
 *	function won't fail. The number of bytes actually read is return through
 *	\a bytesRead.
 *
 *	\param address The user memory address from which to read.
 *	\param buffer The buffer into which to write.
 *	\param size The number of bytes to read.
 *	\param bytesRead Will be set to the number of bytes actually read.
 *	\return \c B_OK, if reading went fine. Then \a bytesRead will be set to
 *			the amount of data actually read. An error indicates that nothing
 *			has been read.
 */
static status_t
read_user_memory(const void *_address, void *_buffer, int32 size,
	int32 &bytesRead)
{
	const char *address = (const char*)_address;
	char *buffer = (char*)_buffer;

	// check the parameters
	if (!IS_USER_ADDRESS(address))
		return B_BAD_ADDRESS;
	if (size <= 0)
		return B_BAD_VALUE;

	// If the region to be read crosses page boundaries, we split it up into
	// smaller chunks.
	status_t error = B_OK;
	bytesRead = 0;
	while (size > 0) {
		// check whether we're still in user address space
		if (!IS_USER_ADDRESS(address)) {
			error = B_BAD_ADDRESS;
			break;
		}

		// don't cross page boundaries in a single read
		int32 toRead = size;
		int32 maxRead = B_PAGE_SIZE - (addr_t)address % B_PAGE_SIZE;
		if (toRead > maxRead)
			toRead = maxRead;

		error = user_memcpy(buffer, address, toRead);
		if (error != B_OK)
			break;

		bytesRead += toRead;
		address += toRead;
		buffer += toRead;
		size -= toRead;
	}

	// If reading fails, we only fail, if we haven't read anything yet.
	if (error != B_OK) {
		if (bytesRead > 0)
			return B_OK;
		return error;
	}

	return B_OK;
}


static status_t
write_user_memory(void *_address, const void *_buffer, int32 size,
	int32 &bytesWritten)
{
	char *address = (char*)_address;
	const char *buffer = (const char*)_buffer;

	// check the parameters
	if (!IS_USER_ADDRESS(address))
		return B_BAD_ADDRESS;
	if (size <= 0)
		return B_BAD_VALUE;

	// If the region to be written crosses area boundaries, we split it up into
	// smaller chunks.
	status_t error = B_OK;
	bytesWritten = 0;
	while (size > 0) {
		// check whether we're still in user address space
		if (!IS_USER_ADDRESS(address)) {
			error = B_BAD_ADDRESS;
			break;
		}

		// get the area for the address (we need to use _user_area_for(), since
		// we're looking for a user area)
		area_id area = _user_area_for((void*)address);
		if (area < 0) {
			TRACE(("write_user_memory(): area not found for address: %p: "
				"%lx\n", address, area));
			error = area;
			break;
		}

		area_info areaInfo;
		status_t error = get_area_info(area, &areaInfo);
		if (error != B_OK) {
			TRACE(("write_user_memory(): failed to get info for area %ld: "
				"%lx\n", area, error));
			error = B_BAD_ADDRESS;
			break;
		}

		// restrict this round of writing to the found area
		int32 toWrite = size;
		int32 maxWrite = (char*)areaInfo.address + areaInfo.size - address;
		if (toWrite > maxWrite)
			toWrite = maxWrite;

		// if the area is read-only, we temporarily need to make it writable
		bool protectionChanged = false;
		if (!(areaInfo.protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA))) {
			error = set_area_protection(area,
				areaInfo.protection | B_WRITE_AREA);
			if (error != B_OK) {
				TRACE(("write_user_memory(): failed to set new protection for "
					"area %ld: %lx\n", area, error));
				break;
			}
			protectionChanged = true;
		}

		// copy the memory
		error = user_memcpy(address, buffer, toWrite);

		// reset the area protection
		if (protectionChanged)
			set_area_protection(area, areaInfo.protection);

		if (error != B_OK) {
			TRACE(("write_user_memory(): user_memcpy() failed: %lx\n", error));
			break;
		}

		bytesWritten += toWrite;
		address += toWrite;
		buffer += toWrite;
		size -= toWrite;
	}

	// If writing fails, we only fail, if we haven't written anything yet.
	if (error != B_OK) {
		if (bytesWritten > 0)
			return B_OK;
		return error;
	}

	return B_OK;
}


/**	\brief Debug nub thread helper function that returns the debug port of
 *		   a thread of the same team.
 */
static status_t
debug_nub_thread_get_thread_debug_port(struct thread *nubThread,
	thread_id threadID, port_id &threadDebugPort)
{
	status_t result = B_OK;
	threadDebugPort = -1;

	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	struct thread *thread = thread_get_thread_struct_locked(threadID);
	if (thread) {
		if (thread->team != nubThread->team)
			result = B_BAD_VALUE;
		else if (thread->debug_info.flags & B_THREAD_DEBUG_STOPPED)
			threadDebugPort = thread->debug_info.debug_port;
		else
			result = B_BAD_VALUE;
	} else
		result = B_BAD_THREAD_ID;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (result == B_OK && threadDebugPort < 0)
		result = B_ERROR;

	return result;
}


static status_t
debug_nub_thread(void *)
{
	struct thread *nubThread = thread_get_current_thread();

	// check, if we're still the current nub thread and get our port
	cpu_status state = disable_interrupts();

	GRAB_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);

	if (nubThread->team->debug_info.nub_thread != nubThread->id)
		return 0;

	port_id port = nubThread->team->debug_info.nub_port;
	sem_id writeLock = nubThread->team->debug_info.debugger_write_lock;

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
		ssize_t messageSize = kill_interruptable_read_port(port, &command,
			&message, sizeof(message));

		if (messageSize < 0) {
			// The port is not longer valid or we were interrupted by a kill
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
				if (!IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;
				else if (size <= 0 || size > B_MAX_READ_WRITE_MEMORY_SIZE)
					result = B_BAD_VALUE;

				// read the memory
				int32 bytesRead = 0;
				if (result == B_OK) {
					result = read_user_memory(address, reply.read_memory.data,
						size, bytesRead);
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
				if (!IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;
				else if (size <= 0 || size > realSize)
					result = B_BAD_VALUE;

				// write the memory
				int32 bytesWritten = 0;
				if (result == B_OK) {
					result = write_user_memory(address, data, size,
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

				struct team *team = thread_get_current_thread()->team;

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
				cpu_status state = disable_interrupts();
				GRAB_THREAD_LOCK();

				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread
					&& thread->team == thread_get_current_thread()->team) {
					flags |= thread->debug_info.flags
						& B_THREAD_DEBUG_KERNEL_FLAG_MASK;
					atomic_set(&thread->debug_info.flags, flags);
				}

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

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
				if (address == NULL || !IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;

				// set the breakpoint
				if (result == B_OK)
					result = arch_set_breakpoint(address);

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
				if (address == NULL || !IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;

				// clear the breakpoint
				if (result == B_OK)
					result = arch_clear_breakpoint(address);

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
				if (address == NULL || !IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;
				if (length < 0)
					result = B_BAD_VALUE;

				// set the watchpoint
				if (result == B_OK)
					result = arch_set_watchpoint(address, type, length);

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
				if (address == NULL || !IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;

				// clear the watchpoint
				if (result == B_OK)
					result = arch_clear_watchpoint(address);

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
				cpu_status state = disable_interrupts();
				GRAB_THREAD_LOCK();

				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread
					&& thread->team == thread_get_current_thread()->team) {
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

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

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
				
				cpu_status state = disable_interrupts();
				GRAB_THREAD_LOCK();

				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread) {
					ignore = thread->debug_info.ignore_signals;
					ignoreOnce = thread->debug_info.ignore_signals_once;
				} else
					result = B_BAD_THREAD_ID;

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

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
				thread_id threadID = message.set_signal_handler.thread;
				int signal = message.set_signal_handler.signal;
				struct sigaction &handler = message.set_signal_handler.handler;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_SET_SIGNAL_HANDLER: "
					"thread: %ld, signal: %d, handler: %p\n", nubThread->id,
					threadID, signal, handler.sa_handler));

				// check, if the thread exists and is ours
				cpu_status state = disable_interrupts();
				GRAB_THREAD_LOCK();

				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread
					&& thread->team != thread_get_current_thread()->team) {
					thread = NULL;
				}

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

				// set the handler
				if (thread)
					sigaction_etc(threadID, signal, &handler, NULL);

				break;
			}

			case B_DEBUG_MESSAGE_GET_SIGNAL_HANDLER:
			{
				// get the parameters
				replyPort = message.get_signal_handler.reply_port;
				thread_id threadID = message.get_signal_handler.thread;
				int signal = message.get_signal_handler.signal;
				status_t result = B_OK;

				// check, if the thread exists and is ours
				cpu_status state = disable_interrupts();
				GRAB_THREAD_LOCK();

				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread) {
					if (thread->team != thread_get_current_thread()->team)
						result = B_BAD_VALUE;
				} else
					result = B_BAD_THREAD_ID;

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

				// get the handler
				if (result == B_OK) {
					result = sigaction_etc(threadID, signal, NULL,
						&reply.get_signal_handler.handler);
				}

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_GET_SIGNAL_HANDLER: "
					"reply port: %ld, thread: %ld, signal: %d, "
					"handler: %p\n", nubThread->id, replyPort,
					threadID, signal,
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

				struct team *team = nubThread->team;

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

					RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
					restore_interrupts(state);

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
 *		   and thread debug infos.
 *
 *	Interrupts must be enabled and the team debug info lock of the team to be
 *	debugged must be held. The function will release the lock, but leave
 *	interrupts disabled.
 */
static void
install_team_debugger_init_debug_infos(struct team *team, team_id debuggerTeam,
	port_id debuggerPort, port_id nubPort, thread_id nubThread,
	sem_id debuggerPortWriteLock)
{
	atomic_set(&team->debug_info.flags,
		B_TEAM_DEBUG_DEFAULT_FLAGS | B_TEAM_DEBUG_DEBUGGER_INSTALLED);
	team->debug_info.nub_port = nubPort;
	team->debug_info.nub_thread = nubThread;
	team->debug_info.debugger_team = debuggerTeam;
	team->debug_info.debugger_port = debuggerPort;
	team->debug_info.debugger_write_lock = debuggerPortWriteLock;

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	// set the user debug flags and signal masks of all threads to the default
	GRAB_THREAD_LOCK();

	for (struct thread *thread = team->thread_list;
		 thread;
		 thread = thread->team_next) {
		if (thread->id == nubThread) {
			atomic_set(&thread->debug_info.flags, B_THREAD_DEBUG_NUB_THREAD);
		} else {
			int32 flags = thread->debug_info.flags
				& ~B_THREAD_DEBUG_USER_FLAG_MASK;
			atomic_set(&thread->debug_info.flags,
				flags | B_THREAD_DEBUG_DEFAULT_FLAGS);
			thread->debug_info.ignore_signals = 0;
			thread->debug_info.ignore_signals_once = 0;
		}
	}

	RELEASE_THREAD_LOCK();
}


static port_id
install_team_debugger(team_id teamID, port_id debuggerPort, bool useDefault,
	bool dontFail)
{
	TRACE(("install_team_debugger(team: %ld, port: %ld, default: %d, "
		"dontFail: %d)\n", teamID, debuggerPort, useDefault, dontFail));

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

	// check the debugger team: It must neither be the kernel team nor the
	// debugged team
	if (debuggerTeam == team_get_kernel_team_id() || debuggerTeam == teamID) {
		TRACE(("install_team_debugger(): Can't debug kernel or debugger team. "
			"debugger: %ld, debugged: %ld\n", debuggerTeam, teamID));
		return B_NOT_ALLOWED;
	}

	// check, if a debugger is already installed

	bool done = false;
	port_id result = B_ERROR;
	bool handOver = false;
	bool releaseDebugInfoLock = true;
	port_id oldDebuggerPort = -1;
	port_id nubPort = -1;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	// get a real team ID
	// We look up the team by ID, even in case of the current team, so we can be
	// sure, that the team is not already dying.
	if (teamID == B_CURRENT_TEAM)
		teamID = thread_get_current_thread()->team->id;

	struct team *team = team_get_team_struct_locked(teamID);

	if (team) {
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		int32 teamDebugFlags = team->debug_info.flags;

		if (team == team_get_kernel_team()) {
			// don't allow to debug the kernel
			error = B_NOT_ALLOWED;
		} else if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			if (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_HANDOVER) {
				// a handover to another debugger is requested
				// clear the flag
				atomic_and(& team->debug_info.flags,
					~B_TEAM_DEBUG_DEBUGGER_HANDOVER);

				oldDebuggerPort = team->debug_info.debugger_port;
				result = nubPort = team->debug_info.nub_port;

				// set the new debugger
				install_team_debugger_init_debug_infos(team, debuggerTeam,
					debuggerPort, nubPort, team->debug_info.nub_thread,
					team->debug_info.debugger_write_lock);

				releaseDebugInfoLock = false;
				handOver = true;
				done = true;

				// finally set the new port owner
				if (set_port_owner(nubPort, debuggerTeam) != B_OK) {
					// The old debugger must just have died. Just proceed as
					// if there was no debugger installed. We may still be too
					// early, in which case we'll fail, but this race condition
					// should be unbelievably rare and relatively harmless.
					handOver = false;
					done = false;
				}

			} else {
				// there's already a debugger installed
				error = (dontFail ? B_OK : B_BAD_VALUE);
				done = true;
				result = team->debug_info.nub_port;
			}
		}

		// in case of a handover the lock has already been released
		if (releaseDebugInfoLock)
			RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	} else
		error = B_BAD_TEAM_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (handOver) {
		// notify the nub thread
		kill_interruptable_write_port(nubPort, B_DEBUG_MESSAGE_HANDED_OVER,
			NULL, 0);

		// notify the old debugger
		debug_handed_over notification;
		notification.origin.thread = -1;
		notification.origin.team = teamID;
		notification.origin.nub_port = nubPort;
		notification.debugger = debuggerTeam;
		notification.debugger_port = debuggerPort;

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
		return (error == B_OK ? result : error);
	}

	// create the debugger write lock semaphore
	char nameBuffer[B_OS_NAME_LENGTH];
	snprintf(nameBuffer, sizeof(nameBuffer), "team %ld debugger port write",
		teamID);
	sem_id debuggerWriteLock = create_sem(1, nameBuffer);
	if (debuggerWriteLock < 0)
		error = debuggerWriteLock;

	// create the nub port
	snprintf(nameBuffer, sizeof(nameBuffer), "team %ld debug", teamID);
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

	// spawn the nub thread
	thread_id nubThread = -1;
	if (error == B_OK) {
		snprintf(nameBuffer, sizeof(nameBuffer), "team %ld debug task", teamID);
		nubThread = spawn_kernel_thread_etc(debug_nub_thread, nameBuffer,
			B_NORMAL_PRIORITY, NULL, teamID, -1);
		if (nubThread < 0)
			error = nubThread;
	}

	// now adjust the debug info accordingly
	if (error == B_OK) {
		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		// look up again, to make sure the team isn't dying
		team = team_get_team_struct_locked(teamID);

		if (team) {
			GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

			if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
				// there's already a debugger installed
				error = (dontFail ? B_OK : B_BAD_VALUE);
				done = true;
				result = team->debug_info.nub_port;

				RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
			} else {
				install_team_debugger_init_debug_infos(team, debuggerTeam,
					debuggerPort, nubPort, nubThread, debuggerWriteLock);
			}
		} else
			error = B_BAD_TEAM_ID;

		RELEASE_TEAM_LOCK();
		restore_interrupts(state);
	}

	// if everything went fine, resume the nub thread, otherwise clean up
	if (error == B_OK && !done) {
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
	}

	TRACE(("install_team_debugger() done2: %ld\n",
		(error == B_OK ? result : error)));
	return (error == B_OK ? result : error);
}


static status_t
ensure_debugger_installed(team_id teamID, port_id *_port)
{
	port_id port = install_team_debugger(teamID, -1, true, true);
	if (port < 0)
		return port;

	if (_port)
		*_port = port;
	return B_OK;
}


// #pragma mark -


void
_user_debugger(const char *userMessage)
{
	// install the default debugger, if there is none yet
	struct thread *thread = thread_get_current_thread();
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
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
	struct team *team = thread_get_current_thread()->team;

	cpu_status cpuState = disable_interrupts();
	GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	int32 oldFlags;
	if (state)
		oldFlags = atomic_and(&team->debug_info.flags, ~B_TEAM_DEBUG_SIGNALS);
	else
		oldFlags = atomic_or(&team->debug_info.flags, B_TEAM_DEBUG_SIGNALS);

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	restore_interrupts(cpuState);

	// TODO: Check, if the return value is really the old state.
	return !(oldFlags & B_TEAM_DEBUG_SIGNALS);
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
	return install_team_debugger(teamID, debuggerPort, false, false);
}


status_t
_user_remove_team_debugger(team_id teamID)
{
	struct team_debug_info info;

	status_t error = B_OK;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	struct team *team = (teamID == B_CURRENT_TEAM
		? thread_get_current_thread()->team
		: team_get_team_struct_locked(teamID));

	if (team) {
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// there's a debugger installed
			info = team->debug_info;
			clear_team_debug_info(&team->debug_info, false);
		} else {
			// no debugger installed
			error = B_BAD_VALUE;
		}

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	} else
		error = B_BAD_TEAM_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// clean up the info, if there was a debugger installed
	if (error == B_OK)
		destroy_team_debug_info(&info);

	return error;
}


status_t
_user_debug_thread(thread_id threadID)
{
	// tell the thread to stop as soon as possible
	status_t error = B_OK;
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	struct thread *thread = thread_get_thread_struct_locked(threadID);
	if (!thread) {
		// thread doesn't exist any longer
		error = B_BAD_THREAD_ID;
	} else if (thread->team == team_get_kernel_team()) {
		// we can't debug the kernel team
		error = B_NOT_ALLOWED;
	} else if (thread->debug_info.flags & B_THREAD_DEBUG_DYING) {
		// the thread is already dying -- too late to debug it
		error = B_BAD_THREAD_ID;
	} else if (thread->debug_info.flags & B_THREAD_DEBUG_NUB_THREAD) {
		// don't debug the nub thread
		error = B_NOT_ALLOWED;
	} else if (!(thread->debug_info.flags & B_THREAD_DEBUG_STOPPED)) {
		// set the flag that tells the thread to stop as soon as possible
		atomic_or(&thread->debug_info.flags, B_THREAD_DEBUG_STOP);

		switch (thread->state) {
			case B_THREAD_SUSPENDED:
				// thread suspended: wake it up
				thread->state = thread->next_state = B_THREAD_READY;
				scheduler_enqueue_in_run_queue(thread);
				break;

			case B_THREAD_WAITING:
				// thread waiting: interrupt it
				sem_interrupt_thread(thread);
				break;
		}
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return error;
}

void
_user_wait_for_debugger(void)
{
	debug_thread_debugged message;
	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED, &message,
		sizeof(message), false);
}


