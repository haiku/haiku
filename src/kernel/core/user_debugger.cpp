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
#include <ksignal.h>
#include <ksyscalls.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <user_debugger.h>
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
		arch_clear_thread_debug_info(&info->arch_info);
		atomic_set(&info->flags,
			B_THREAD_DEBUG_DEFAULT_FLAGS | (dying ? B_THREAD_DEBUG_DYING : 0));
		info->debug_port = -1;
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

		atomic_set(&info->flags, 0);
	}
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


void
prepare_thread_stopped_message(debug_thread_stopped &message,
	debug_why_stopped whyStopped, port_id nubPort, void *data)
{
	struct thread *thread = thread_get_current_thread();

	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = nubPort;
	message.why = whyStopped;
	message.data = data;
	arch_get_debug_cpu_state(&message.cpu_state);
}


static status_t
thread_hit_debug_event(uint32 event, const void *message, int32 size,
	debug_why_stopped whyStopped, void *additionalData)
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
			return port;
		}

		setPort = true;
	}

	// check the debug info structures once more: get the debugger port, set
	// the thread's debug port, and update the thread's debug flags
	port_id deletePort = port;
	port_id debuggerPort = -1;
	status_t error = B_OK;
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();
	GRAB_TEAM_DEBUG_INFO_LOCK(thread->team->debug_info);

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
	TRACE(("thread_hit_debug_event(): thread: %ld, sending message to debugger "
		"port %ld\n", thread->id, debuggerPort));
	do {
		error = write_port(debuggerPort, event, message, size);
	} while (error == B_INTERRUPTED);

	status_t result = B_THREAD_DEBUG_HANDLE_EVENT;

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

					// update the single-step flag
					state = disable_interrupts();
					GRAB_THREAD_LOCK();

					if (commandMessage.continue_thread.single_step) {
						atomic_or(&thread->debug_info.flags,
							B_THREAD_DEBUG_SINGLE_STEP);
					} else {
						atomic_and(&thread->debug_info.flags,
							~B_THREAD_DEBUG_SINGLE_STEP);
					}

					RELEASE_THREAD_LOCK();
					restore_interrupts(state);

					done = true;
					break;

				case B_DEBUGGED_THREAD_GET_WHY_STOPPED:
				{
					// get the team debug info (just in case it has changed)
					team_debug_info teamDebugInfo;
					get_team_debug_info(teamDebugInfo);
					if (!(teamDebugInfo.flags
							& B_TEAM_DEBUG_DEBUGGER_INSTALLED)) {
						done = true;
						break;
					}
					debuggerPort = teamDebugInfo.debugger_port;

					// prepare the message
					debug_thread_stopped stoppedMessage;
					prepare_thread_stopped_message(stoppedMessage, whyStopped,
						teamDebugInfo.nub_port, additionalData);

					// send it
					error = kill_interruptable_write_port(debuggerPort, event,
						&stoppedMessage, sizeof(stoppedMessage));

					break;
				}

				case B_DEBUGGED_THREAD_SET_CPU_STATE:
				{
					TRACE(("thread_hit_debug_event(): thread: %ld: "
						"B_DEBUGGED_THREAD_SET_CPU_STATE\n",
						thread->id));
					arch_set_debug_cpu_state(
						&commandMessage.set_cpu_state.cpu_state);
					
					break;
				}
			}
		}
	} else {
		TRACE(("thread_hit_debug_event(): thread: %ld, failed to send "
			"message to debugger port %ld: %lx\n", thread->id,
			debuggerPort, error));
	}

	// unset the "stopped" state
	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	atomic_and(&thread->debug_info.flags, ~B_THREAD_DEBUG_STOPPED);

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return (error == B_OK ? result : error);
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	message.syscall = syscall;

	// copy the syscall args
	if (syscall < (uint32)kSyscallCount) {
		if (kSyscallInfos[syscall].parameter_size > 0)
			memcpy(message.args, args, kSyscallInfos[syscall].parameter_size);
	}

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_PRE_SYSCALL, &message,
		sizeof(message), B_PRE_SYSCALL_HIT, NULL);
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
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
		sizeof(message), B_POST_SYSCALL_HIT, NULL);
}


static status_t
stop_thread(debug_why_stopped whyStopped, void *additionalData)
{
	// ensure that a debugger is installed for this team
	port_id nubPort;
	status_t error = ensure_debugger_installed(B_CURRENT_TEAM, &nubPort);
	if (error != B_OK) {
		dprintf("user_debug_stop_thread(): Failed to install debugger: "
			"thread: %ld: %s\n", thread_get_current_thread()->id,
			strerror(error));
		return error;
	}

	// prepare the message
	debug_thread_stopped message;
	prepare_thread_stopped_message(message, whyStopped, nubPort,
		additionalData);

	return thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_STOPPED, &message,
		sizeof(message), whyStopped, additionalData);
}


/**	\brief To be called when an unhandled processor fault (exception/error)
 *		   occurred.
 *	\param fault The debug_why_stopped value identifying the kind of fault.
 *	\return \c true, if the caller shall continue normally, i.e. usually send
 *			a deadly signal. \c false, if the debugger insists to continue the
 *			program (e.g. because it has solved the removed the cause of the
 *			problem).
 */
bool
user_debug_fault_occurred(debug_why_stopped fault)
{
	return (stop_thread(fault, NULL) != B_THREAD_DEBUG_IGNORE_EVENT);
}


bool
user_debug_handle_signal(int signal, struct sigaction *handler, bool deadly)
{
	// check, if a debugger is installed and is interested in team creation
	// events
	struct thread *thread = thread_get_current_thread();
	int32 teamDebugFlags = atomic_get(&thread->team->debug_info.flags);
	if (~teamDebugFlags
		& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_SIGNALS)) {
		return true;
	}

	// prepare the message
	debug_signal_received message;
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	message.signal = signal;
	message.handler = *handler;
	message.deadly = deadly;

	status_t result = thread_hit_debug_event(B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED,
		&message, sizeof(message), B_SIGNAL_RECEIVED, NULL);
	return (result != B_THREAD_DEBUG_IGNORE_EVENT);
}


void
user_debug_stop_thread()
{
	stop_thread(B_THREAD_NOT_RUNNING, NULL);
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	message.new_team = teamID;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_TEAM_CREATED, &message,
		sizeof(message), B_TEAM_CREATED, NULL);
}


void
user_debug_team_deleted(team_id teamID, port_id debuggerPort)
{
	if (debuggerPort >= 0) {
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	message.new_thread = threadID;

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_THREAD_CREATED, &message,
		sizeof(message), B_THREAD_CREATED, NULL);
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
		write_port_etc(debuggerPort, B_DEBUGGER_MESSAGE_THREAD_DELETED,
			&message, sizeof(message), B_RELATIVE_TIMEOUT, 0);
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	memcpy(&message.info, imageInfo, sizeof(image_info));

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), B_IMAGE_CREATED, NULL);
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
	message.origin.thread = thread->id;
	message.origin.team = thread->team->id;
	message.origin.nub_port = thread->team->debug_info.nub_port;
	memcpy(&message.info, imageInfo, sizeof(image_info));

	thread_hit_debug_event(B_DEBUGGER_MESSAGE_IMAGE_CREATED, &message,
		sizeof(message), B_IMAGE_DELETED, NULL);
}


void
user_debug_break_or_watchpoint_hit(bool watchpoint)
{
	stop_thread((watchpoint ? B_WATCHPOINT_HIT : B_BREAKPOINT_HIT), NULL);
}


void
user_debug_single_stepped()
{
	stop_thread(B_SINGLE_STEP, NULL);
}


static void
nub_thread_cleanup(struct thread *nubThread)
{
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
	bytesRead = 0;
	while (size > 0) {
		int32 toRead = size;
		if ((uint32)address % B_PAGE_SIZE + toRead > B_PAGE_SIZE)
			toRead = B_PAGE_SIZE - (uint32)address % B_PAGE_SIZE;

		status_t error = user_memcpy(buffer, address, toRead);

		// If reading fails, we only fail, if we haven't read anything yet.
		if (error != B_OK) {
			if (bytesRead > 0)
				return B_OK;
			return error;
		}

		bytesRead += toRead;
		address += toRead;
		buffer += toRead;
		size -= toRead;
	}

	return B_OK;
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

	RELEASE_TEAM_DEBUG_INFO_LOCK(nubThread->team->debug_info);
	restore_interrupts(state);

	TRACE(("debug_nub_thread() thread: %ld, team %ld, nub port: %ld\n",
		nubThread->id, nubThread->team->id, port));

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
			debug_nub_read_memory_reply		read_memory;
			debug_nub_write_memory_reply	write_memory;
			debug_nub_set_breakpoint_reply	set_breakpoint;
			debug_nub_set_watchpoint_reply	set_watchpoint;
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
					"reply port: %ld, address: %p, size: %ld, result: %lx\n",
					nubThread->id, replyPort, address, size, result));

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
			case B_DEBUG_MESSAGE_STEP_THREAD:
			{
				// get the parameters
				thread_id threadID;
				uint32 handleEvent;
				bool singleStep;

				if (command == B_DEBUG_MESSAGE_RUN_THREAD) {
					threadID = message.run_thread.thread;
					handleEvent = message.run_thread.handle_event;
					singleStep = false;
				} else {
					threadID = message.step_thread.thread;
					handleEvent = message.step_thread.handle_event;
					singleStep = true;
				}

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_%s_THREAD: "
					"thread: %ld, handle event: %lu\n", nubThread->id,
					(singleStep ? "STEP" : "RUN"), threadID,
					handleEvent));

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
					debugged_thread_continue commandMessage;
					commandMessage.handle_event = handleEvent;
					commandMessage.single_step = singleStep;

					write_port(threadDebugPort,
						B_DEBUGGED_THREAD_MESSAGE_CONTINUE,
						&commandMessage, sizeof(commandMessage));
				}

				break;
			}

			case B_DEBUG_MESSAGE_GET_WHY_STOPPED:
			{
				// get the parameters
				thread_id threadID = message.get_why_stopped.thread;

				TRACE(("nub thread %ld: B_DEBUG_MESSAGE_GET_WHY_STOPPED: "
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
						B_DEBUGGED_THREAD_GET_WHY_STOPPED, NULL, 0);
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
					debugged_thread_set_cpu_state commandMessage;
					memcpy(&commandMessage.cpu_state, &cpuState,
						sizeof(debug_cpu_state));
					write_port(threadDebugPort,
						B_DEBUGGED_THREAD_SET_CPU_STATE,
						&commandMessage, sizeof(commandMessage));
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
		}

		// send the reply, if necessary
		if (sendReply) {
			status_t error = kill_interruptable_write_port(replyPort, command,
				&reply, replySize);

			if (error != B_OK) {
				// The debugger port is either not longer existing or we got
				// interrupted by a kill signal. In either case we terminate.
				nub_thread_cleanup(nubThread);
				return error;
			}
		}
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

	// get a real team ID
	// We look up the team by ID, even in case of the current team, so we can be
	// sure, that the team is not already dying.
	if (teamID == B_CURRENT_TEAM)
		teamID = thread_get_current_thread()->team->id;

	struct team *team = team_get_team_struct_locked(teamID);

	if (team) {
		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		if (team == team_get_kernel_team()) {
			// don't allow to debug the kernel
			error = B_NOT_ALLOWED;
		} else if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// there's already a debugger installed
			error = (dontFail ? B_OK : B_BAD_VALUE);
			done = true;
			result = team->debug_info.nub_port;
		}

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);
	} else
		error = B_BAD_TEAM_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

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
				atomic_set(&team->debug_info.flags,
					B_TEAM_DEBUG_DEFAULT_FLAGS
					| B_TEAM_DEBUG_DEBUGGER_INSTALLED);
				team->debug_info.nub_port = nubPort;
				team->debug_info.nub_thread = nubThread;
				team->debug_info.debugger_team = debuggerTeam;
				team->debug_info.debugger_port = debuggerPort;

				RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);

				// set the user debug flags of all threads to the default
				GRAB_THREAD_LOCK();

				for (struct thread *thread = team->thread_list;
					 thread;
					 thread = thread->team_next) {
					if (thread->id != nubThread) {
						int32 flags = thread->debug_info.flags
							& ~B_THREAD_DEBUG_USER_FLAG_MASK;
						atomic_set(&thread->debug_info.flags,
							flags | B_THREAD_DEBUG_DEFAULT_FLAGS);
					}
				}

				RELEASE_THREAD_LOCK();
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

	// notify the debugger
	stop_thread(B_DEBUGGER_CALL, (void*)message);
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

