/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <DebugContext.h>


BDebugContext::BDebugContext()
{
	fContext.team = -1;
}


BDebugContext::~BDebugContext()
{
	Uninit();
}


status_t
BDebugContext::Init(team_id team, port_id nubPort)
{
	Uninit();

	status_t error = init_debug_context(&fContext, team, nubPort);
	if (error != B_OK) {
		fContext.team = -1;
		return error;
	}

	return B_OK;
}


void
BDebugContext::Uninit()
{
	if (fContext.team >= 0) {
		destroy_debug_context(&fContext);
		fContext.team = -1;
	}
}


status_t
BDebugContext::SendDebugMessage(int32 messageCode, const void *message,
	size_t messageSize, void* reply, size_t replySize)
{
	return send_debug_message(&fContext, messageCode, message, messageSize,
		reply, replySize);
}


status_t
BDebugContext::SetTeamDebuggingFlags(int32 flags)
{
	debug_nub_set_team_flags message;
	message.flags = flags;

	return SendDebugMessage(B_DEBUG_MESSAGE_SET_TEAM_FLAGS, &message,
		sizeof(message), NULL, 0);
}


ssize_t
BDebugContext::ReadMemoryPartial(const void* address, void* buffer, size_t size)
{
	return debug_read_memory_partial(&fContext, address, buffer, size);
}


ssize_t
BDebugContext::ReadMemory(const void* address, void* buffer, size_t size)
{
	return debug_read_memory(&fContext, address, buffer, size);
}


ssize_t
BDebugContext::ReadString(const void* address, char* buffer, size_t size)
{
	return debug_read_string(&fContext, address, buffer, size);
}


status_t
BDebugContext::SetBreakpoint(void* address)
{
	debug_nub_set_breakpoint message;
	message.reply_port = fContext.reply_port;
	message.address = address;

	debug_nub_set_breakpoint_reply reply;
	status_t error = SendDebugMessage(B_DEBUG_MESSAGE_SET_BREAKPOINT, &message,
		sizeof(message), &reply, sizeof(reply));

	return error == B_OK ? reply.error : error;
}


status_t
BDebugContext::ClearBreakpoint(void* address)
{
	debug_nub_clear_breakpoint message;
	message.address = address;

	return SendDebugMessage(B_DEBUG_MESSAGE_CLEAR_BREAKPOINT, &message,
		sizeof(message), NULL, 0);
}


status_t
BDebugContext::SetWatchpoint(void* address, uint32 type, int32 length)
{
	debug_nub_set_watchpoint message;
	message.reply_port = fContext.reply_port;
	message.address = address;
	message.type = type;
	message.length = length;

	debug_nub_set_watchpoint_reply reply;
	status_t error = SendDebugMessage(B_DEBUG_MESSAGE_SET_WATCHPOINT, &message,
		sizeof(message), &reply, sizeof(reply));

	return error == B_OK ? reply.error : error;
}


status_t
BDebugContext::ClearWatchpoint(void* address)
{
	debug_nub_clear_watchpoint message;
	message.address = address;

	return SendDebugMessage(B_DEBUG_MESSAGE_CLEAR_WATCHPOINT, &message,
		sizeof(message), NULL, 0);
}


status_t
BDebugContext::ContinueThread(thread_id thread, bool singleStep)
{
	debug_nub_continue_thread message;
	message.thread = thread;
	message.handle_event = B_THREAD_DEBUG_HANDLE_EVENT;
	message.single_step = singleStep;

	return SendDebugMessage(B_DEBUG_MESSAGE_CONTINUE_THREAD, &message,
		sizeof(message), NULL, 0);
}


status_t
BDebugContext::SetThreadDebuggingFlags(thread_id thread, int32 flags)
{
	debug_nub_set_thread_flags message;
	message.thread = thread;
	message.flags = flags;

	return SendDebugMessage(B_DEBUG_MESSAGE_SET_THREAD_FLAGS, &message,
		sizeof(message), NULL, 0);
}


status_t
BDebugContext::GetThreadCpuState(thread_id thread,
	debug_debugger_message* _messageCode, debug_cpu_state* cpuState)
{
	return debug_get_cpu_state(&fContext, thread, _messageCode, cpuState);
}
