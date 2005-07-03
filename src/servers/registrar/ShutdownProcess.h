/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 *
 * Manages the shutdown process.
 */
#ifndef SHUTDOWN_PROCESS_H
#define SHUTDOWN_PROCESS_H

#include <hash_set>

#include <Looper.h>

#include "AppInfoList.h"
#include "EventMaskWatcher.h"

class EventQueue;
class TRoster;

// Note: EventMaskWatcher is inherited public due to a gcc bug/C++ feature:
// Once cast into a Watcher dynamic_cast<>()ing it back into an
// EventMaskWatcher fails otherwise.
class ShutdownProcess : public BLooper, public EventMaskWatcher {
public:
	ShutdownProcess(TRoster *roster, EventQueue *eventQueue);
	~ShutdownProcess();

	status_t Init(BMessage *request);

	virtual void MessageReceived(BMessage *message);

	static void SendReply(BMessage *request, status_t error);

private:
	void _NegativeQuitRequestReply(thread_id thread);

	void _PrepareShutdownMessage(BMessage &message) const;
	status_t _ShutDown();

	status_t _PushEvent(uint32 eventType, team_id team, int32 phase);
	status_t _GetNextEvent(uint32 &eventType, team_id &team, int32 &phase,
		bool block);

	void _SetPhase(int32 phase);
	void _ScheduleTimeoutEvent(bigtime_t timeout, team_id team = -1);

	bool _LockAppLists();
	void _UnlockAppLists();

	static status_t _WorkerEntry(void *data);
	status_t _Worker();

	void _WorkerDoShutdown();
	void _QuitUserApps();
	void _WaitForUserApps();
	void _KillUserApps();
	void _QuitSystemApps();
	void _QuitNonApps();

private:
	class TimeoutEvent;
	class InternalEvent;
	struct InternalEventList;
	class QuitRequestReplyHandler;

	friend class QuitRequestReplyHandler;

	BLocker					fWorkerLock;	// protects fields shared by looper
											// and worker
	BMessage				*fRequest;
	TRoster					*fRoster;
	EventQueue				*fEventQueue;
	hash_set<team_id>		fVitalSystemApps;
	AppInfoList				fSystemApps;
	AppInfoList				fUserApps;
	TimeoutEvent			*fTimeoutEvent;
	InternalEventList		*fInternalEvents;
	sem_id					fInternalEventSemaphore;
	QuitRequestReplyHandler	*fQuitRequestReplyHandler;
	thread_id				fWorker;
	int32					fCurrentPhase;
	status_t				fShutdownError;
	bool					fHasGUI;
};

#endif	// SHUTDOWN_PROCESS_H
