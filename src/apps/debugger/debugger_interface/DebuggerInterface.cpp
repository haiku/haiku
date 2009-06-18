/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebuggerInterface.h"

#include <new>

#include <stdio.h>

#include "debug_utils.h"

#include "CpuState.h"
#include "DebugEvent.h"
#include "ImageInfo.h"
#include "ThreadInfo.h"


DebuggerInterface::DebuggerInterface(team_id teamID)
	:
	fTeamID(teamID),
	fDebuggerPort(-1),
	fNubPort(-1)
{
	fDebugContext.reply_port = -1;
}


DebuggerInterface::~DebuggerInterface()
{
	destroy_debug_context(&fDebugContext);

	Close();
}


status_t
DebuggerInterface::Init()
{
	// create debugger port
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "team %ld debugger", fTeamID);
	fDebuggerPort = create_port(100, buffer);
	if (fDebuggerPort < 0)
		return fDebuggerPort;

	// install as team debugger
	fNubPort = install_team_debugger(fTeamID, fDebuggerPort);
	if (fNubPort < 0)
		return fNubPort;

	// init debug context
	status_t error = init_debug_context(&fDebugContext, fTeamID, fNubPort);
	if (error != B_OK)
		return error;

	return B_OK;
}


void
DebuggerInterface::Close()
{
	if (fDebuggerPort >= 0)
		delete_port(fDebuggerPort);
}


status_t
DebuggerInterface::GetNextDebugEvent(DebugEvent*& _event)
{
	while (true) {
		debug_debugger_message_data message;
		int32 messageCode;
		ssize_t size = read_port(fDebuggerPort, &messageCode, &message,
			sizeof(message));
		if (size < 0) {
			if (size == B_INTERRUPTED)
				continue;
			return size;
		}

		if (message.origin.team != fTeamID)
			continue;

		bool ignore = false;
		status_t error = _CreateDebugEvent(messageCode, message, ignore,
			_event);
		if (error != B_OK)
			return error;

		if (ignore) {
			if (message.origin.thread >= 0 && message.origin.nub_port >= 0)
				continue_thread(message.origin.nub_port, message.origin.thread);
			continue;
		}

		return B_OK;
	}
}


status_t
DebuggerInterface::SetTeamDebuggingFlags(uint32 flags)
{
	set_team_debugging_flags(fNubPort, flags);
	return B_OK;
}


status_t
DebuggerInterface::ContinueThread(thread_id thread)
{
	continue_thread(fNubPort, thread);
	return B_OK;
}


status_t
DebuggerInterface::GetThreadInfos(BObjectList<ThreadInfo>& infos)
{
	thread_info threadInfo;
	int32 cookie = 0;
	while (get_next_thread_info(fTeamID, &cookie, &threadInfo) == B_OK) {
		ThreadInfo* info = new(std::nothrow) ThreadInfo(threadInfo.team,
			threadInfo.thread, threadInfo.name);
		if (info == NULL || !infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DebuggerInterface::GetImageInfos(BObjectList<ImageInfo>& infos)
{
	image_info imageInfo;
	int32 cookie = 0;
	while (get_next_image_info(fTeamID, &cookie, &imageInfo) == B_OK) {
		ImageInfo* info = new(std::nothrow) ImageInfo(fTeamID, imageInfo.id,
			imageInfo.name);
		if (info == NULL || !infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


status_t
DebuggerInterface::GetThreadInfo(thread_id thread, ThreadInfo& info)
{
	thread_info threadInfo;
	status_t error = get_thread_info(thread, &threadInfo);
	if (error != B_OK)
		return error;

	info.SetTo(threadInfo.team, threadInfo.thread, threadInfo.name);
	return B_OK;
}


status_t
DebuggerInterface::GetCpuState(thread_id thread, CpuState*& _state)
{
	debug_nub_get_cpu_state message;
	message.reply_port = fDebugContext.reply_port;
	message.thread = thread;

	debug_nub_get_cpu_state_reply reply;

	status_t error = send_debug_message(&fDebugContext,
		B_DEBUG_MESSAGE_GET_CPU_STATE, &message, sizeof(message), &reply,
		sizeof(reply));
	if (error != B_OK)
		return error;
	if (reply.error != B_OK)
		return reply.error;

	// TODO: Correctly create the state...
	CpuState* state = new(std::nothrow) CpuState;
	if (state == NULL)
		return B_NO_MEMORY;

	_state = state;
	return B_OK;
}


status_t
DebuggerInterface::_CreateDebugEvent(int32 messageCode,
	const debug_debugger_message_data& message, bool& _ignore,
	DebugEvent*& _event)
{
	DebugEvent* event = NULL;

	switch (messageCode) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			event = new(std::nothrow) ThreadDebuggedEvent(message.origin.team,
				message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			event = new(std::nothrow) DebuggerCallEvent(message.origin.team,
				message.origin.thread,
				(target_addr_t)message.debugger_call.message);
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			event = new(std::nothrow) BreakpointHitEvent(message.origin.team,
				message.origin.thread, NULL);
					// TODO: CpuState!
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			event = new(std::nothrow) WatchpointHitEvent(message.origin.team,
				message.origin.thread, NULL);
					// TODO: CpuState!
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			event = new(std::nothrow) SingleStepEvent(message.origin.team,
				message.origin.thread, NULL);
					// TODO: CpuState!
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			event = new(std::nothrow) ExceptionOccurredEvent(
				message.origin.team, message.origin.thread,
				message.exception_occurred.exception);
			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			if (message.origin.team != fTeamID) {
				_ignore = true;
				return B_OK;
			}
			event = new(std::nothrow) TeamDeletedEvent(message.origin.team,
				message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
			if (message.origin.team != fTeamID) {
				_ignore = true;
				return B_OK;
			}
			event = new(std::nothrow) TeamExecEvent(message.origin.team,
				message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			event = new(std::nothrow) ThreadCreatedEvent(message.origin.team,
				message.origin.thread, message.thread_created.new_thread);
			break;
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			event = new(std::nothrow) ThreadDeletedEvent(message.origin.team,
				message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
		{
			const image_info& info = message.image_created.info;
			event = new(std::nothrow) ImageCreatedEvent(message.origin.team,
				message.origin.thread, ImageInfo(fTeamID, info.id, info.name));
			break;
		}
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
		{
			const image_info& info = message.image_deleted.info;
			event = new(std::nothrow) ImageDeletedEvent(message.origin.team,
				message.origin.thread, ImageInfo(fTeamID, info.id, info.name));
			break;
		}
		default:
			printf("DebuggerInterface for team %ld: unknown message from "
				"kernel: %ld\n", fTeamID, messageCode);
			// fall through...
		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			_ignore = true;
			return B_OK;
	}

	if (event == NULL)
		return B_NO_MEMORY;

	if (message.origin.thread >= 0 && message.origin.nub_port >= 0)
		event->SetThreadStopped(true);

	_ignore = false;
	_event = event;

	return B_OK;
}
