/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUGGER_H
#define TEAM_DEBUGGER_H

#include <debugger.h>
#include <Looper.h>

#include <debug_support.h>
#include <util/DoublyLinkedList.h>

#include "DebugEvent.h"
#include "TeamWindow.h"


class DebuggerInterface;
class Team;
class TeamDebugModel;


class TeamDebugger :  public DoublyLinkedListLinkImpl<TeamDebugger>,
	private BLooper, private TeamWindow::Listener {
public:
								TeamDebugger();
								~TeamDebugger();

			status_t			Init(team_id teamID, thread_id threadID,
									bool stopInMain);

			team_id				TeamID() const	{ return fTeamID; }

private:
	virtual	void				MessageReceived(BMessage* message);

	// TeamWindow::Listener
	virtual	void				ThreadActionRequested(TeamWindow* window,
									thread_id threadID, uint32 action);
	virtual	bool				TeamWindowQuitRequested(TeamWindow* window);

private:
	static	status_t			_DebugEventListenerEntry(void* data);
			status_t			_DebugEventListener();

			void				_HandleDebuggerMessage(DebugEvent* event);

			bool				_HandleThreadStopped(thread_id threadID,
									CpuState* cpuState);

			bool				_HandleThreadDebugged(
									ThreadDebuggedEvent* event);
			bool				_HandleDebuggerCall(
									DebuggerCallEvent* event);
			bool				_HandleBreakpointHit(
									BreakpointHitEvent* event);
			bool				_HandleWatchpointHit(
									WatchpointHitEvent* event);
			bool				_HandleSingleStep(
									SingleStepEvent* event);
			bool				_HandleExceptionOccurred(
									ExceptionOccurredEvent* event);
			bool				_HandleThreadCreated(
									ThreadCreatedEvent* event);
			bool				_HandleThreadDeleted(
									ThreadDeletedEvent* event);
			bool				_HandleImageCreated(
									ImageCreatedEvent* event);
			bool				_HandleImageDeleted(
									ImageDeletedEvent* event);

			void				_UpdateThreadState(::Thread* thread);

			void				_HandleThreadAction(thread_id threadID,
									uint32 action);

private:
			::Team*				fTeam;
			TeamDebugModel*		fDebugModel;
			team_id				fTeamID;
			port_id				fNubPort;
			DebuggerInterface*	fDebuggerInterface;
			thread_id			fDebugEventListener;
			TeamWindow*			fTeamWindow;
	volatile bool				fTerminating;
};

#endif	// TEAM_DEBUGGER_H
