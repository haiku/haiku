/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 *
 * Manages the shutdown process.
 */
#ifndef SHUTDOWN_PROCESS_H
#define SHUTDOWN_PROCESS_H

#include <hash_set>

#include <Locker.h>
#include <Looper.h>

#include "AppInfoList.h"
#include "EventMaskWatcher.h"
#include "RosterAppInfo.h"

#if __GNUC__ >= 4
using __gnu_cxx::hash_set;
#endif


class EventQueue;
class TRoster;

// Note: EventMaskWatcher is inherited public due to a gcc bug/C++ feature:
// Once cast into a Watcher dynamic_cast<>()ing it back into an
// EventMaskWatcher fails otherwise.
class ShutdownProcess : public BLooper, public EventMaskWatcher {
public:
								ShutdownProcess(TRoster* roster,
									EventQueue* eventQueue);
	virtual						~ShutdownProcess();

			status_t			Init(BMessage* request);

	virtual	void				MessageReceived(BMessage* message);

	static	void				SendReply(BMessage* request, status_t error);

private:
			void				_SendReply(status_t error);

			void				_NegativeQuitRequestReply(thread_id thread);

			void				_PrepareShutdownMessage(BMessage& message) const;
			status_t			_ShutDown();

			status_t			_PushEvent(uint32 eventType, team_id team,
									int32 phase);
			status_t			_GetNextEvent(uint32& eventType, team_id& team,
									int32& phase, bool block);

			void				_SetPhase(int32 phase);
			void				_ScheduleTimeoutEvent(bigtime_t timeout,
									team_id team = -1);

			bool				_LockAppLists();
			void				_UnlockAppLists();

			void				_InitShutdownWindow();
			void				_SetShowShutdownWindow(bool show);
			void				_AddShutdownWindowApps(AppInfoList& infos);
			void				_RemoveShutdownWindowApp(team_id team);
			void				_SetShutdownWindowCurrentApp(team_id team);
			void				_SetShutdownWindowText(const char* text);
			void				_SetShutdownWindowCancelButtonEnabled(
									bool enabled);
			void				_SetShutdownWindowKillButtonEnabled(
									bool enabled);
			void				_SetShutdownWindowWaitForShutdown();
			void				_SetShutdownWindowWaitForAbortedOK();

	static	status_t			_WorkerEntry(void* data);
			status_t			_Worker();

			void				_WorkerDoShutdown();
			bool				_WaitForApp(team_id team, AppInfoList* list,
									bool systemApps);
			void				_QuitApps(AppInfoList& list, bool systemApps);
			void				_QuitBackgroundApps();
			void				_WaitForBackgroundApps();
			void				_KillBackgroundApps();
			void				_QuitNonApps();
			void				_QuitBlockingApp(AppInfoList& list, team_id team,
									const char* appName, bool cancelAllowed);
			void				_DisplayAbortingApp(team_id team);
			void				_WaitForDebuggedTeams();

private:
	class TimeoutEvent;
	class InternalEvent;
	struct InternalEventList;
	class QuitRequestReplyHandler;
	class ShutdownWindow;

	friend class QuitRequestReplyHandler;

			BLocker				fWorkerLock;
									// protects fields shared by looper
									// and worker
			BMessage*			fRequest;
			TRoster*			fRoster;
			EventQueue*			fEventQueue;
			hash_set<team_id>	fVitalSystemApps;
			AppInfoList			fSystemApps;
			AppInfoList			fUserApps;
			AppInfoList			fBackgroundApps;
			hash_set<team_id>	fDebuggedTeams;
			TimeoutEvent*		fTimeoutEvent;
			InternalEventList*	fInternalEvents;
			sem_id				fInternalEventSemaphore;
			QuitRequestReplyHandler* fQuitRequestReplyHandler;
			thread_id			fWorker;
			int32				fCurrentPhase;
			status_t			fShutdownError;
			bool				fHasGUI;
			bool				fReboot;
			bool				fRequestReplySent;
			ShutdownWindow*		fWindow;
};

#endif	// SHUTDOWN_PROCESS_H
