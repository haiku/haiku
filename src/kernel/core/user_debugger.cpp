/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <debugger.h>
#include <kernel.h>
#include <KernelExport.h>
#include <ksyscalls.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <user_debugger.h>

//#define TRACE_USER_DEBUGGER
#ifdef TRACE_USER_DEBUGGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static port_id sDefaultDebuggerPort = -1;
	// accessed atomically


static status_t ensure_debugger_installed(team_id teamID, port_id *port = NULL);


/**
 *  The team lock must be held.
 */
void
clear_team_debug_info(struct team_debug_info *info)
{
	if (info) {
		atomic_set(&info->flags, B_TEAM_DEBUG_SIGNALS
			| B_TEAM_DEBUG_PRE_SYSCALL | B_TEAM_DEBUG_POST_SYSCALL);
		info->debugger_team = -1;
		info->debugger_port = -1;
		info->nub_thread = -1;
		info->nub_port = -1;
	}
}

/**
 *  The team lock must not be held. \a info must not be a member of a
 *	team struct (or the team struct not longer be accessible, i.e. the team
 *	should already be removed).
 *
 *	In case the team is still accessible, the procedure is:
 *	1. get the team lock
 *	2. copy the team debug info on stack
 *	3. call clear_team_debug_info() on the team debug info
 *	4. release the team lock
 *	5. call destroy_team_debug_info() on the copied team debug info
 */
void
destroy_team_debug_info(struct team_debug_info *info)
{
	if (info) {
		// delete the nub port
		if (info->nub_port >= 0) {
			set_port_owner(info->nub_port, B_CURRENT_TEAM);
			delete_port(info->nub_port);
			info->nub_port = -1;
		}

		// wait for the nub thread
		if (info->nub_thread >= 0) {
			int32 result;
			wait_for_thread(info->nub_thread, &result);
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
		atomic_set(&info->flags, (dying ? B_THREAD_DEBUG_DYING : 0));
		info->debug_port = -1;
	}
}


void
destroy_thread_debug_info(struct thread_debug_info *info)
{
	if (info) {
		if (info->debug_port >= 0) {
			delete_port(info->debug_port);
			info->debug_port = -1;
		}

		atomic_set(&info->flags, 0);
	}
}


static void
thread_hit_debug_event(uint32 event, const void *message, int32 size)
{
	struct thread *thread = thread_get_current_thread();

	TRACE(("thread_hit_debug_event(): thread: %ld, event: %lu, message: %p, "
		"size: %ld\n", thread->id, event, message, size));

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
			return;
		}

		setPort = true;
	}

	// check the debug info structures once more: get the debugger port, set
	// the thread's debug port, and update the thread's debug flags
	port_id deletePort = port;
	port_id debuggerPort = -1;
	status_t error = B_OK;
	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();
	GRAB_THREAD_LOCK();

	uint32 threadFlags = thread->debug_info.flags;
	threadFlags &= ~B_THREAD_DEBUG_STOP;
	if (thread->team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
		debuggerPort = thread->team->debug_info.debugger_port;

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

	RELEASE_THREAD_LOCK();
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// delete the superfluous port
	if (deletePort >= 0)
		delete_port(deletePort);

	if (error != B_OK) {
		TRACE(("thread_hit_debug_event() error: thread: %ld, error: %lx\n",
			thread->id, error));
		return;
	}

	// send a message to the debugger port
	TRACE(("thread_hit_debug_event(): thread: %ld, sending message to debugger "
		"port %ld\n", thread->id, debuggerPort));
	error = write_port(debuggerPort, event, message, size);
	if (error == B_OK) {
		bool done = false;
		while (!done) {
			// read a command from the debug port
			int32 command;
			debugged_thread_message commandMessage;
			ssize_t commandMessageSize = read_port(port, &command,
				&commandMessage, sizeof(commandMessage));
			if (commandMessageSize < 0) {
				error = commandMessageSize;
				TRACE(("thread_hit_debug_event(): thread: %ld, failed to "
					"receive message from port %ld: %lx\n", thread->id, port,
					error));
				break;
			}

			switch (command) {
				case B_DEBUGGED_THREAD_MESSAGE_CONTINUE:
					TRACE(("thread_hit_debug_event(): thread: %ld: "
						"B_DEBUGGED_THREAD_MESSAGE_CONTINUE\n", thread->id));
					done = true;
					break;
			}
		}
	} else {
		TRACE(("thread_hit_debug_event(): thread: %ld, failed to send message "
			"to debugger port %ld: %lx\n", thread->id, debuggerPort, error));
	}

	// unset the "stopped" state
	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	atomic_and(&thread->debug_info.flags, ~B_THREAD_DEBUG_STOPPED);

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
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
	message.thread = thread->id;
	message.team = thread->team->id;
	message.syscall = syscall;

	// copy the syscall args
	if (!(teamDebugFlags & B_TEAM_DEBUG_SYSCALL_FAST_TRACE)
		&& syscall < (uint32)kSyscallCount) {
		if (kSyscallInfos[syscall].parameter_size > 0)
			memcpy(message.args, args, kSyscallInfos[syscall].parameter_size);
	}

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_PRE_SYSCALL, &message,
		sizeof(message));
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
	message.thread = thread->id;
	message.team = thread->team->id;
	message.start_time = startTime;
	message.end_time = system_time();
	message.return_value = returnValue;
	message.syscall = syscall;

	// copy the syscall args
	if (!(teamDebugFlags & B_TEAM_DEBUG_SYSCALL_FAST_TRACE)
		&& syscall < (uint32)kSyscallCount) {
		if (kSyscallInfos[syscall].parameter_size > 0)
			memcpy(message.args, args, kSyscallInfos[syscall].parameter_size);
	}

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_POST_SYSCALL, &message,
		sizeof(message));
}


uint32
user_debug_handle_signal(int signal, bool deadly)
{
	// TODO: Maybe provide the signal handler as info for the debugger as well.
	// TODO: Implement!
	return B_THREAD_DEBUG_HANDLE_SIGNAL;
}


void
user_debug_stop_thread()
{
	// ensure that a debugger is installed for this team
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_stop_thread(): Failed to install debugger: "
			"thread: %ld: %s\n", thread_get_current_thread()->id,
			strerror(error));
		return;
	}

	struct thread *thread = thread_get_current_thread();

	// prepare the message
	debug_thread_stopped message;
	message.thread = thread->id;
	message.team = thread->team->id;
	message.why = B_THREAD_NOT_RUNNING;
	message.nub_port = nubPort;
	// TODO: Remaining message fields.

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_STOPPED, &message,
		sizeof(message));
}


static status_t
debug_nub_thread(void *)
{
	// check, if we're still the current nub thread and get our port
	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	struct thread *nubThread = thread_get_current_thread();
	if (nubThread->team->debug_info.nub_thread != nubThread->id)
		return 0;

	port_id port = nubThread->team->debug_info.nub_port;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	TRACE(("debug_nub_thread() thread: %ld, team %ld, nub port: %ld\n",
		nubThread->id, nubThread->team->id, port));

	// command processing loop
	while (true) {
		int32 command;
		debug_nub_message_data message;
		ssize_t messageSize = read_port(port, &command, &message,
			sizeof(message));
		if (messageSize < 0) {
			// The port is not longer valid: If we are still listed in the
			// team's debug info as nub thread, we need to update that.
			cpu_status state = disable_interrupts();
			GRAB_TEAM_LOCK();

			team_debug_info &info = nubThread->team->debug_info;
			if (info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED
				&& info.nub_thread == nubThread->id) {
				clear_team_debug_info(&info);
			}
		
			RELEASE_TEAM_LOCK();
			restore_interrupts(state);

			TRACE(("nub thread %ld: terminating: %lx\n", nubThread->id,
				messageSize));

			return messageSize;
		}

		bool sendReply = false;
		union {
			debug_nub_read_memory_reply		read_memory;
			debug_nub_write_memory_reply	write_memory;
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
				if (result == B_OK)
					result = user_memcpy(reply.read_memory.data, address, size);
				reply.read_memory.error = result;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_READ_MEMORY: "
					"reply port: %ld, address: %p, size: %ld, result: %lx\n",
					nubThread->id, replyPort, address, size, result));

				// send only as much data as necessary
				int32 bytesRead = (result == B_OK ? size : 0);
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

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_WRITE_MEMORY: "
					"reply port: %ld, address: %p, size: %ld\n", nubThread->id,
					replyPort, address, size));

				// check the parameters
				if (!IS_USER_ADDRESS(address))
					result = B_BAD_ADDRESS;
				else if (size <= 0 || size > realSize)
					result = B_BAD_VALUE;

				// write the memory
				if (result == B_OK)
					result = user_memcpy(address, data, size);
				reply.write_memory.error = result;

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
				GRAB_TEAM_LOCK();

				flags |= team->debug_info.flags & B_TEAM_DEBUG_KERNEL_FLAG_MASK;
				atomic_set(&team->debug_info.flags, flags);

				RELEASE_TEAM_LOCK();
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
				if (thread->team == thread_get_current_thread()->team) {
					flags |= thread->debug_info.flags
						& B_THREAD_DEBUG_KERNEL_FLAG_MASK;
					atomic_set(&thread->debug_info.flags, flags);
				}

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

				break;
			}

			case B_DEBUG_MESSAGE_RUN_THREAD:
			{
				// get the parameters
				thread_id threadID = message.run_thread.thread;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_RUN_THREAD: "
					"thread: %ld\n", nubThread->id, threadID));

				// find the thread and get its debug port
				state = disable_interrupts();
				GRAB_THREAD_LOCK();

				port_id threadDebugPort = -1;
				struct thread *thread
					= thread_get_thread_struct_locked(threadID);
				if (thread && thread->team == nubThread->team
					&& thread->debug_info.flags & B_THREAD_DEBUG_STOPPED) {
					threadDebugPort = thread->debug_info.debug_port;
				}

				RELEASE_THREAD_LOCK();
				restore_interrupts(state);

				// send a message to the debugged thread
				if (threadDebugPort >= 0) {
					write_port(threadDebugPort,
						B_DEBUGGED_THREAD_MESSAGE_CONTINUE, NULL, 0);
				}

				break;
			}
		}

		// send the reply, if necessary
		if (sendReply)
			write_port(replyPort, command, &reply, replySize);
	}
}


static port_id
install_team_debugger(team_id teamID, port_id debuggerPort, bool useDefault,
	bool dontFail)
{
TRACE(("install_team_debugger(team: %ld, port: %ld, default: %d, "
"dontFail: %d)\n", teamID, debuggerPort, useDefault, dontFail));

	if (useDefault)
		debuggerPort = atomic_get(&sDefaultDebuggerPort);

	// check, if a debugger is already installed

	status_t error = B_OK;
	bool done = false;
	port_id result = B_ERROR;

	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	struct team *team = (teamID == B_CURRENT_TEAM
		? thread_get_current_thread()->team
		: team_get_team_struct_locked(teamID));

	if (team) {
		if (team == team_get_kernel_team()) {
			// don't allow to debug the kernel
			error = B_NOT_ALLOWED;
		} else if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// there's already a debugger installed
			error = (dontFail ? B_OK : B_BAD_VALUE);
			done = true;
			result = team->debug_info.nub_port;
		}
	} else
		error = B_BAD_TEAM_ID;

//int32 teamDebugFlags = team->debug_info.flags;
//team_debug_info *teamDebugInfo = &team->debug_info;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

//if (error != B_OK) {
//dprintf("install_team_debugger(): First check failed: team: %p (%ld), "
//"sizeof(struct team): %lu, info: %p, flags: %lx, error: %lx\n", team, team->id, sizeof(struct team), teamDebugInfo, teamDebugFlags, error);
//dprintf("  dead_children: %p, dead_children.kernel_time: %p, aspace: %p, "
//"image_list: %p, arch_info: %p, debug_info: %p, dead_threads_kernel_time: %p, "
//"dead_threads_user_time: %p\n", &team->dead_children,
//&team->dead_children.kernel_time, &team->aspace, &team->image_list,
//&team->arch_info, &team->debug_info, &team->dead_threads_kernel_time,
//&team->dead_threads_user_time);
//}

	if (done || error != B_OK) {
		TRACE(("install_team_debugger() done1: %ld\n",
			(error == B_OK ? result : error)));
		return (error == B_OK ? result : error);
	}

	// get the debugger team
	port_info debuggerPortInfo;
	error = get_port_info(debuggerPort, &debuggerPortInfo);
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

	// create the nub port
	char nameBuffer[B_OS_NAME_LENGTH];
	snprintf(nameBuffer, sizeof(nameBuffer), "team %ld debug", teamID);
	port_id nubPort = create_port(1, nameBuffer);
	if (nubPort < 0)
		error = nubPort;
	else
		result = nubPort;

	// make the debugger team the port owner; thus we know, if the debugger is
	// gone and can cleanup
	if (error == B_OK)
		error = set_port_owner(nubPort, debuggerTeam);

	// spawn the nub thread
	thread_id nubThread = -1;
	if (error == B_OK) {
		snprintf(nameBuffer, sizeof(nameBuffer), "team %ld debug task", teamID);
		nubThread = spawn_kernel_thread_etc(debug_nub_thread, nameBuffer,
			B_NORMAL_PRIORITY, NULL, teamID);
		if (nubThread < 0)
			error = nubThread;
	}

	// now adjust the debug info accordingly
	if (error == B_OK) {
		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		team = (teamID == B_CURRENT_TEAM
			? thread_get_current_thread()->team
			: team_get_team_struct_locked(teamID));

		if (team) {
			if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
				// there's already a debugger installed
				error = (dontFail ? B_OK : B_BAD_VALUE);
				done = true;
				result = team->debug_info.nub_port;
			} else {
				atomic_or(&team->debug_info.flags,
					B_TEAM_DEBUG_DEBUGGER_INSTALLED);
				team->debug_info.nub_port = nubPort;
				team->debug_info.nub_thread = nubThread;
				team->debug_info.debugger_team = debuggerTeam;
				team->debug_info.debugger_port = debuggerPort;
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
_user_debugger(const char *message)
{
	// install the default debugger, if there is none yet
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM);
	if (error != B_OK) {
		// time to commit suicide
		dprintf("_user_debugger(): Failed to install debugger. Message is: "
			"`%s'\n", message);
		_user_exit_team(1);
	}

	// send the debugger message
// TODO:...
_user_exit_team(1);
}


int
_user_disable_debugger(int state)
{
	cpu_status cpuState = disable_interrupts();
	GRAB_TEAM_LOCK();

	team_debug_info &info = thread_get_current_thread()->team->debug_info;

	int32 oldFlags;
	if (state)
		oldFlags = atomic_and(&info.flags, ~B_TEAM_DEBUG_SIGNALS);
	else
		oldFlags = atomic_or(&info.flags, B_TEAM_DEBUG_SIGNALS);

	RELEASE_TEAM_LOCK();
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
		if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// there's a debugger installed
			info = team->debug_info;
			clear_team_debug_info(&team->debug_info);
		} else {
			// no debugger installed
			error = B_BAD_VALUE;
		}
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

