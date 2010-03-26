/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_CONTEXT_H
#define _DEBUG_CONTEXT_H


#include <debug_support.h>


class BDebugContext {
public:
								BDebugContext();
								~BDebugContext();

			status_t			Init(team_id team, port_id nubPort);
			void				Uninit();

			team_id				Team() const	{ return fContext.team; }
			port_id				NubPort() const	{ return fContext.nub_port; }
			port_id				ReplyPort() const
									{ return fContext.reply_port; }

			status_t			SendDebugMessage(int32 messageCode,
									const void *message, size_t messageSize,
									void* reply, size_t replySize);

			status_t			SetTeamDebuggingFlags(int32 flags);

			ssize_t				ReadMemoryPartial(const void* address,
									void* buffer, size_t size);
			ssize_t				ReadMemory(const void* address,
									void* buffer, size_t size);
			ssize_t				ReadString(const void* address,
									char* buffer, size_t size);

			status_t			SetBreakpoint(void* address);
			status_t			ClearBreakpoint(void* address);

			status_t			SetWatchpoint(void* address, uint32 type,
									int32 length);
			status_t			ClearWatchpoint(void* address);

			status_t			ContinueThread(thread_id thread,
									bool singleStep = false);
			status_t			SetThreadDebuggingFlags(thread_id thread,
									int32 flags);
			status_t			GetThreadCpuState(thread_id thread,
									debug_debugger_message* _messageCode,
									debug_cpu_state* cpuState);

protected:
			debug_context		fContext;
};


#endif	// _DEBUG_CONTEXT_H
