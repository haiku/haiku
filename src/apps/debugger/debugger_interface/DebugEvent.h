/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEBUG_EVENT_H
#define DEBUG_EVENT_H

#include <debugger.h>

#include "ImageInfo.h"
#include "Types.h"


class CpuState;


// constants for synthetic events generated via the
// start_system_watching() interface
enum {
	DEBUGGER_MESSAGE_THREAD_RENAMED				= 'dmtr',
	DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED	= 'dmpc'
};


class DebugEvent {
public:
								DebugEvent(int32 eventType,
									team_id team, thread_id thread);
	virtual						~DebugEvent();

			int32 				EventType() const		{ return fEventType; }
			team_id				Team() const			{ return fTeam; }
			thread_id			Thread() const			{ return fThread; }

			bool				ThreadStopped() const { return fThreadStopped; }
			void				SetThreadStopped(bool stopped);

private:
			int32 				fEventType;
			team_id				fTeam;
			thread_id			fThread;
			bool				fThreadStopped;
};


class CpuStateEvent : public DebugEvent {
public:
								CpuStateEvent(debug_debugger_message eventType,
									team_id team, thread_id thread,
									CpuState* state);
	virtual						~CpuStateEvent();

			CpuState*			GetCpuState() const	{ return fCpuState; }

private:
			CpuState*			fCpuState;
};


class ThreadDebuggedEvent : public DebugEvent {
public:
								ThreadDebuggedEvent(team_id team,
									thread_id thread);
};


class DebuggerCallEvent : public DebugEvent {
public:
								DebuggerCallEvent(team_id team,
									thread_id thread, target_addr_t message);

			target_addr_t		Message() const	{ return fMessage; }

private:
			target_addr_t		fMessage;
};


class BreakpointHitEvent : public CpuStateEvent {
public:
								BreakpointHitEvent(team_id team,
									thread_id thread, CpuState* state);
};


class WatchpointHitEvent : public CpuStateEvent {
public:
								WatchpointHitEvent(team_id team,
									thread_id thread, CpuState* state);
};


class SingleStepEvent : public CpuStateEvent {
public:
								SingleStepEvent(team_id team,
									thread_id thread, CpuState* state);
};


class ExceptionOccurredEvent : public DebugEvent {
public:
								ExceptionOccurredEvent(team_id team,
									thread_id thread,
									debug_exception_type exception);

			debug_exception_type Exception() const	{ return fException; }

private:
			debug_exception_type fException;
};


class TeamDeletedEvent : public DebugEvent {
public:
								TeamDeletedEvent(team_id team,
									thread_id thread);
};


class TeamExecEvent : public DebugEvent {
public:
								TeamExecEvent(team_id team, thread_id thread);
};


class ThreadCreatedEvent : public DebugEvent {
public:
								ThreadCreatedEvent(team_id team,
									thread_id thread, thread_id newThread);

			thread_id			NewThread() const	{ return fNewThread; }

private:
			thread_id			fNewThread;
};


class ThreadRenamedEvent : public DebugEvent {
public:
								ThreadRenamedEvent(team_id team,
									thread_id thread, thread_id renamedThread,
									const char* name);

			thread_id			RenamedThread() const { return fRenamedThread; }
			const char*			NewName() const	{ return fName; }

private:
			thread_id			fRenamedThread;
			char				fName[B_OS_NAME_LENGTH];
};


class ThreadPriorityChangedEvent : public DebugEvent {
public:
								ThreadPriorityChangedEvent(team_id team,
									thread_id thread, thread_id changedThread,
									int32 newPriority);

			thread_id			ChangedThread() const { return fChangedThread; }
			int32				NewPriority() const	{ return fNewPriority; }

private:
			thread_id			fChangedThread;
			int32				fNewPriority;
};


class ThreadDeletedEvent : public DebugEvent {
public:
								ThreadDeletedEvent(team_id team,
									thread_id thread);
};


class ImageCreatedEvent : public DebugEvent {
public:
								ImageCreatedEvent(team_id team,
									thread_id thread, const ImageInfo& info);

			const ImageInfo&	GetImageInfo() const	{ return fInfo; }

private:
			ImageInfo			fInfo;
};


class ImageDeletedEvent : public DebugEvent {
public:
								ImageDeletedEvent(team_id team,
									thread_id thread, const ImageInfo& info);

			const ImageInfo&	GetImageInfo() const	{ return fInfo; }

private:
			ImageInfo			fInfo;
};


class HandedOverEvent : public DebugEvent {
public:
								HandedOverEvent(team_id team,
									thread_id thread, thread_id causingThread);

			thread_id			CausingThread() const { return fCausingThread; }

private:
			thread_id			fCausingThread;
};


#endif	// DEBUG_EVENT_H
