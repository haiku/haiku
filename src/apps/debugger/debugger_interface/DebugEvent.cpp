/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebugEvent.h"

#include "CpuState.h"


// #pragma mark - DebugEvent


DebugEvent::DebugEvent(int32 eventType, team_id team,
	thread_id thread)
	:
	fEventType(eventType),
	fTeam(team),
	fThread(thread),
	fThreadStopped(false)
{
}


DebugEvent::~DebugEvent()
{
}


void
DebugEvent::SetThreadStopped(bool stopped)
{
	fThreadStopped = stopped;
}


// #pragma mark - CpuStateEvent


CpuStateEvent::CpuStateEvent(debug_debugger_message eventType, team_id team,
	thread_id thread, CpuState* state)
	:
	DebugEvent(eventType, team, thread),
	fCpuState(state)
{
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


CpuStateEvent::~CpuStateEvent()
{
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}


// #pragma mark - ThreadDebuggedEvent


ThreadDebuggedEvent::ThreadDebuggedEvent(team_id team, thread_id thread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_THREAD_DEBUGGED, team, thread)
{
}


// #pragma mark - DebuggerCallEvent


DebuggerCallEvent::DebuggerCallEvent(team_id team, thread_id thread,
	target_addr_t message)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_DEBUGGER_CALL, team, thread),
	fMessage(message)
{
}


// #pragma mark - BreakpointHitEvent


BreakpointHitEvent::BreakpointHitEvent(team_id team, thread_id thread,
	CpuState* state)
	:
	CpuStateEvent(B_DEBUGGER_MESSAGE_BREAKPOINT_HIT, team, thread, state)
{
}


// #pragma mark - WatchpointHitEvent


WatchpointHitEvent::WatchpointHitEvent(team_id team, thread_id thread,
	CpuState* state)
	:
	CpuStateEvent(B_DEBUGGER_MESSAGE_WATCHPOINT_HIT, team, thread, state)
{
}



// #pragma mark - SingleStepEvent


SingleStepEvent::SingleStepEvent(team_id team, thread_id thread,
	CpuState* state)
	:
	CpuStateEvent(B_DEBUGGER_MESSAGE_SINGLE_STEP, team, thread, state)
{
}


// #pragma mark - ExceptionOccurredEvent


ExceptionOccurredEvent::ExceptionOccurredEvent(team_id team, thread_id thread,
	debug_exception_type exception)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED, team, thread),
	fException(exception)
{
}


// #pragma mark - TeamDeletedEvent


TeamDeletedEvent::TeamDeletedEvent(team_id team, thread_id thread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_TEAM_DELETED, team, thread)
{
}


// #pragma mark - TeamExecEvent


TeamExecEvent::TeamExecEvent(team_id team, thread_id thread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_TEAM_EXEC, team, thread)
{
}


// #pragma mark - ThreadCreatedEvent


ThreadCreatedEvent::ThreadCreatedEvent(team_id team, thread_id thread,
	thread_id newThread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_THREAD_CREATED, team, thread),
	fNewThread(newThread)
{
}


// #pragma mark - ThreadRenamedEvent


ThreadRenamedEvent::ThreadRenamedEvent(team_id team, thread_id thread,
	thread_id renamedThread, const char* newName)
	:
	DebugEvent(DEBUGGER_MESSAGE_THREAD_RENAMED, team, thread),
	fRenamedThread(renamedThread)
{
	strlcpy(fName, newName, sizeof(fName));
}


// #pragma mark - ThreadPriorityChangedEvent


ThreadPriorityChangedEvent::ThreadPriorityChangedEvent(team_id team,
	thread_id thread, thread_id changedThread, int32 newPriority)
	:
	DebugEvent(DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED, team, thread),
	fChangedThread(changedThread),
	fNewPriority(newPriority)
{
}


// #pragma mark - ThreadDeletedEvent


ThreadDeletedEvent::ThreadDeletedEvent(team_id team, thread_id thread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_THREAD_DELETED, team, thread)
{
}


// #pragma mark - ImageCreatedEvent


ImageCreatedEvent::ImageCreatedEvent(team_id team, thread_id thread,
	const ImageInfo& info)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_IMAGE_CREATED, team, thread),
	fInfo(info)
{
}


// #pragma mark - ImageDeletedEvent


ImageDeletedEvent::ImageDeletedEvent(team_id team, thread_id thread,
	const ImageInfo& info)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_IMAGE_DELETED, team, thread),
	fInfo(info)
{
}


// #pragma mark - HandedOverEvent


HandedOverEvent::HandedOverEvent(team_id team, thread_id thread,
	thread_id causingThread)
	:
	DebugEvent(B_DEBUGGER_MESSAGE_HANDED_OVER, team, thread),
	fCausingThread(causingThread)
{
}
