/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_MESSAGE_HANDLER_H
#define _DEBUG_MESSAGE_HANDLER_H


#include <debugger.h>


class BDebugMessageHandler {
public:
	virtual						~BDebugMessageHandler();

	virtual	bool				HandleDebugMessage(int32 messageCode,
									const debug_debugger_message_data& message);

	virtual	bool				HandleThreadDebugged(
									const debug_thread_debugged& message);
	virtual	bool				HandleDebuggerCall(
									const debug_debugger_call& message);
	virtual	bool				HandleBreakpointHit(
									const debug_breakpoint_hit& message);
	virtual	bool				HandleWatchpointHit(
									const debug_watchpoint_hit& message);
	virtual	bool				HandleSingleStep(
									const debug_single_step& message);
	virtual	bool				HandlePreSyscall(
									const debug_pre_syscall& message);
	virtual	bool				HandlePostSyscall(
									const debug_post_syscall& message);
	virtual	bool				HandleSignalReceived(
									const debug_signal_received& message);
	virtual	bool				HandleExceptionOccurred(
									const debug_exception_occurred& message);
	virtual	bool				HandleTeamCreated(
									const debug_team_created& message);
	virtual	bool				HandleTeamDeleted(
									const debug_team_deleted& message);
	virtual	bool				HandleTeamExec(
									const debug_team_exec& message);
	virtual	bool				HandleThreadCreated(
									const debug_thread_created& message);
	virtual	bool				HandleThreadDeleted(
									const debug_thread_deleted& message);
	virtual	bool				HandleImageCreated(
									const debug_image_created& message);
	virtual	bool				HandleImageDeleted(
									const debug_image_deleted& message);
	virtual	bool				HandleProfilerUpdate(
									const debug_profiler_update& message);
	virtual	bool				HandleHandedOver(
									const debug_handed_over& message);

	virtual	bool				UnhandledDebugMessage(int32 messageCode,
									const debug_debugger_message_data& message);
};


#endif	// _DEBUG_MESSAGE_HANDLER_H
