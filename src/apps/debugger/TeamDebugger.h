/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUGGER_H
#define TEAM_DEBUGGER_H

#include <debugger.h>
#include <Looper.h>

#include <debug_support.h>

#include "DebugEvent.h"
#include "Team.h"
#include "TeamWindow.h"
#include "Worker.h"


class DebuggerInterface;
class TeamDebugModel;


class TeamDebugger : private BLooper, private TeamWindow::Listener,
	private JobListener, private Team::Listener {
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

	// JobListener
	virtual	void				JobDone(Job* job);
	virtual	void				JobFailed(Job* job);
	virtual	void				JobAborted(Job* job);

	// Team::Listener
	virtual	void				ThreadStateChanged(
									const ::Team::ThreadEvent& event);
	virtual	void				ThreadCpuStateChanged(
									const ::Team::ThreadEvent& event);
	virtual	void				ThreadStackTraceChanged(
									const ::Team::ThreadEvent& event);

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
			void				_SetThreadState(::Thread* thread, uint32 state,
									CpuState* cpuState);

			void				_HandleThreadAction(thread_id threadID,
									uint32 action);

			void				_HandleThreadStateChanged(thread_id threadID);
			void				_HandleCpuStateChanged(thread_id threadID);
			void				_HandleStackTraceChanged(thread_id threadID);

private:
			::Team*				fTeam;
			TeamDebugModel*		fDebugModel;
			team_id				fTeamID;
			port_id				fNubPort;
			DebuggerInterface*	fDebuggerInterface;
			Worker*				fWorker;
			thread_id			fDebugEventListener;
			TeamWindow*			fTeamWindow;
	volatile bool				fTerminating;
};

#endif	// TEAM_DEBUGGER_H
