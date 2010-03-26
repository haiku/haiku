/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <DebugMessageHandler.h>


BDebugMessageHandler::~BDebugMessageHandler()
{
}


/*!	Handles the supplied debugger message.
	Can be overridded by subclasses. The base class implementation calls the
	respective Handle*() hook for the message.

	\param messageCode The (port) message code identifying the debugger message.
	\param message The message data.
	\return \c true, if the caller is supposed to continue the thread, \c false
		otherwise.
*/
bool
BDebugMessageHandler::HandleDebugMessage(int32 messageCode,
	const debug_debugger_message_data& message)
{
	switch (messageCode) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			return HandleThreadDebugged(message.thread_debugged);
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			return HandleDebuggerCall(message.debugger_call);
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			return HandleBreakpointHit(message.breakpoint_hit);
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			return HandleWatchpointHit(message.watchpoint_hit);
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			return HandleSingleStep(message.single_step);
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			return HandlePreSyscall(message.pre_syscall);
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			return HandlePostSyscall(message.post_syscall);
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			return HandleSignalReceived(message.signal_received);
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			return HandleExceptionOccurred(message.exception_occurred);
		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
			return HandleTeamCreated(message.team_created);
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			return HandleTeamDeleted(message.team_deleted);
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
			return HandleTeamExec(message.team_exec);
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			return HandleThreadCreated(message.thread_created);
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			return HandleThreadDeleted(message.thread_deleted);
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			return HandleImageCreated(message.image_created);
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			return HandleImageDeleted(message.image_deleted);
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
			return HandleProfilerUpdate(message.profiler_update);
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			return HandleHandedOver(message.handed_over);
		default:
			return true;
	}
}


bool
BDebugMessageHandler::HandleThreadDebugged(const debug_thread_debugged& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleDebuggerCall(const debug_debugger_call& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_DEBUGGER_CALL,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleBreakpointHit(const debug_breakpoint_hit& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_BREAKPOINT_HIT,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleWatchpointHit(const debug_watchpoint_hit& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_WATCHPOINT_HIT,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleSingleStep(const debug_single_step& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_SINGLE_STEP,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandlePreSyscall(const debug_pre_syscall& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_PRE_SYSCALL,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandlePostSyscall(const debug_post_syscall& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_POST_SYSCALL,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleSignalReceived(const debug_signal_received& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleExceptionOccurred(
	const debug_exception_occurred& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleTeamCreated(const debug_team_created& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_TEAM_CREATED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleTeamDeleted(const debug_team_deleted& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_TEAM_DELETED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleTeamExec(const debug_team_exec& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_TEAM_EXEC,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleThreadCreated(const debug_thread_created& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_THREAD_CREATED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleThreadDeleted(const debug_thread_deleted& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_THREAD_DELETED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleImageCreated(const debug_image_created& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_IMAGE_CREATED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleImageDeleted(const debug_image_deleted& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_IMAGE_DELETED,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleProfilerUpdate(const debug_profiler_update& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_PROFILER_UPDATE,
		(const debug_debugger_message_data&)message);
}


bool
BDebugMessageHandler::HandleHandedOver(const debug_handed_over& message)
{
	return UnhandledDebugMessage(B_DEBUGGER_MESSAGE_HANDED_OVER,
		(const debug_debugger_message_data&)message);
}


/*!	Called by the base class versions of the specific Handle*() methods.
	Can be overridded to handle any message not handled otherwise.
*/
bool
BDebugMessageHandler::UnhandledDebugMessage(int32 messageCode,
	const debug_debugger_message_data& message)
{
	return true;
}
