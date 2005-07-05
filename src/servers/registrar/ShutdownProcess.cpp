/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

// TODO: While debugging only. Remove when implementation is done.
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <new>

#include <signal.h>
#include <unistd.h>

#include <Autolock.h>
#include <Message.h>
#include <MessagePrivate.h>
#include <RegistrarDefs.h>
#include <Roster.h>		// for B_REQUEST_QUIT

#include <TokenSpace.h>
#include <util/DoublyLinkedList.h>

#include <syscalls.h>

#include "AppInfoListMessagingTargetSet.h"
#include "Debug.h"
#include "EventQueue.h"
#include "MessageDeliverer.h"
#include "MessageEvent.h"
#include "Registrar.h"
#include "RosterAppInfo.h"
#include "ShutdownProcess.h"
#include "TRoster.h"

using namespace BPrivate;

// The time span user applications have after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t USER_APPS_QUIT_TIMEOUT = 5000000; // 5 s

// The time span system applications have after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t SYSTEM_APPS_QUIT_TIMEOUT = 3000000; // 3 s

// The time span non-app processes have after the HUP signal has been send
// to them before they get a KILL signal.
static const bigtime_t NON_APPS_QUIT_TIMEOUT = 500000; // 0.5 s


// message what fields
enum {
	MSG_PHASE_TIMED_OUT	= 'phto',
	MSG_DONE			= 'done',
};

// internal events
enum {
	NO_EVENT,
	ABORT_EVENT,
	TIMEOUT_EVENT,
	USER_APP_QUIT_EVENT,
	SYSTEM_APP_QUIT_EVENT,
};

// phases
enum {
	INVALID_PHASE						= -1,
	USER_APP_TERMINATION_PHASE			= 0,
	SYSTEM_APP_TERMINATION_PHASE		= 1,
	OTHER_PROCESSES_TERMINATION_PHASE	= 2,
	DONE_PHASE							= 3,
	ABORTED_PHASE						= 4,
};

// TimeoutEvent
class ShutdownProcess::TimeoutEvent : public MessageEvent {
public:
	TimeoutEvent(BHandler *target)
		: MessageEvent(0, target, MSG_PHASE_TIMED_OUT)
	{
		SetAutoDelete(false);
		
		fMessage.AddInt32("phase", INVALID_PHASE);
		fMessage.AddInt32("team", -1);
	}

	void SetPhase(int32 phase)
	{
		fMessage.ReplaceInt32("phase", phase);
	}

	void SetTeam(team_id team)
	{
		fMessage.ReplaceInt32("team", team);
	}

	static int32 GetMessagePhase(BMessage *message)
	{
		int32 phase;
		if (message->FindInt32("phase", &phase) != B_OK)
			phase = INVALID_PHASE;

		return phase;
	}

	static int32 GetMessageTeam(BMessage *message)
	{
		team_id team;
		if (message->FindInt32("team", &team) != B_OK)
			team = -1;

		return team;
	}
};


// InternalEvent
class ShutdownProcess::InternalEvent
	: public DoublyLinkedListLinkImpl<InternalEvent> {
public:
	InternalEvent(uint32 type, team_id team, int32 phase)
		: fType(type),
		  fTeam(team),
		  fPhase(phase)
	{
	}

	uint32 Type() const			{ return fType; }
	team_id Team() const		{ return fTeam; }
	int32 Phase() const			{ return fPhase; }

private:
	uint32	fType;
	int32	fTeam;
	int32	fPhase;
};


// InternalEventList
struct ShutdownProcess::InternalEventList : DoublyLinkedList<InternalEvent> {
};


// QuitRequestReplyHandler
class ShutdownProcess::QuitRequestReplyHandler : public BHandler {
public:
	QuitRequestReplyHandler(ShutdownProcess *shutdownProcess)
		: BHandler("shutdown quit reply handler"),
		  fShutdownProcess(shutdownProcess)
	{
	}

	virtual void MessageReceived(BMessage *message)
	{
		switch (message->what) {
			case B_REPLY:
			{
				bool result;
				thread_id thread;
				if (message->FindBool("result", &result) == B_OK
					&& message->FindInt32("thread", &thread) == B_OK) {
					if (!result)
						fShutdownProcess->_NegativeQuitRequestReply(thread);
				}

				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	ShutdownProcess	*fShutdownProcess;
};


// #pragma mark -

// constructor
ShutdownProcess::ShutdownProcess(TRoster *roster, EventQueue *eventQueue)
	: BLooper("shutdown process"),
	  EventMaskWatcher(BMessenger(this), B_REQUEST_QUIT),
	  fWorkerLock("worker lock"),
	  fRequest(NULL),
	  fRoster(roster),
	  fEventQueue(eventQueue),
	  fTimeoutEvent(NULL),
	  fInternalEvents(NULL),
	  fInternalEventSemaphore(-1),
	  fQuitRequestReplyHandler(NULL),
	  fWorker(-1),
	  fCurrentPhase(INVALID_PHASE),
	  fShutdownError(B_ERROR),
	  fHasGUI(false)
{
}

// destructor
ShutdownProcess::~ShutdownProcess()
{
	// remove and delete the quit request reply handler
	if (fQuitRequestReplyHandler) {
		BAutolock _(this);
		RemoveHandler(fQuitRequestReplyHandler);
		delete fQuitRequestReplyHandler;
	}

	// remove and delete the timeout event
	if (fTimeoutEvent) {
		fEventQueue->RemoveEvent(fTimeoutEvent);

		delete fTimeoutEvent;
	}

	// delete the internal event semaphore
	if (fInternalEventSemaphore >= 0)
		delete_sem(fInternalEventSemaphore);

	// wait for the worker thread to terminate
	if (fWorker >= 0) {
		int32 result;
		wait_for_thread(fWorker, &result);
	}

	// delete all internal events and the queue
	if (fInternalEvents) {
		while (InternalEvent *event = fInternalEvents->First()) {
			fInternalEvents->Remove(event);
			delete event;
		}

		delete fInternalEvents;
	}

	// send a reply to the request and delete it
	SendReply(fRequest, fShutdownError);
	delete fRequest;
}

// Init
status_t
ShutdownProcess::Init(BMessage *request)
{
	PRINT(("ShutdownProcess::Init()\n"));

	// create and add the quit request reply handler
	fQuitRequestReplyHandler = new(nothrow) QuitRequestReplyHandler(this);
	if (!fQuitRequestReplyHandler)
		RETURN_ERROR(B_NO_MEMORY);
	AddHandler(fQuitRequestReplyHandler);

	// create the timeout event
	fTimeoutEvent = new(nothrow) TimeoutEvent(this);
	if (!fTimeoutEvent)
		RETURN_ERROR(B_NO_MEMORY);

	// create the event list
	fInternalEvents = new(nothrow) InternalEventList;
	if (!fInternalEvents)
		RETURN_ERROR(B_NO_MEMORY);

	// create the event sempahore
	fInternalEventSemaphore = create_sem(0, "shutdown events");
	if (fInternalEventSemaphore < 0)
		RETURN_ERROR(fInternalEventSemaphore);

	// init the app server connection
	fHasGUI = (Registrar::App()->InitGUIContext() == B_OK);

	// tell TRoster not to accept new applications anymore
	fRoster->SetShuttingDown(true);

	// start watching application quits
	status_t error = fRoster->AddWatcher(this);
	if (error != B_OK) {
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	// get a list of all applications to shut down and sort them
	error = fRoster->GetShutdownApps(fUserApps, fSystemApps, fVitalSystemApps);
	if (error != B_OK) {
		fRoster->RemoveWatcher(this);
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	// TODO: sort the system apps by descending registration time

	// display alert
	// TODO:...

	// start the worker thread
	fWorker = spawn_thread(_WorkerEntry, "shutdown worker", B_NORMAL_PRIORITY,
		this);
	if (fWorker < 0) {
		fRoster->RemoveWatcher(this);
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(fWorker);
	}

	// everything went fine: now we own the request
	fRequest = request;

	resume_thread(fWorker);

	PRINT(("ShutdownProcess::Init() done\n"));

	return B_OK;
}

// MessageReceived
void
ShutdownProcess::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SOME_APP_QUIT:
		{
			// get the team
			team_id team;
			if (message->FindInt32("be:team", &team) != B_OK) {
				// should not happen
				return;
			}

			PRINT(("ShutdownProcess::MessageReceived(): B_SOME_APP_QUIT: %ld\n",
				team));

			// remove the app info from the respective list
			_LockAppLists();

			uint32 event;

			RosterAppInfo *info = fUserApps.InfoFor(team);
			if (info) {
				fUserApps.RemoveInfo(info);
				event = USER_APP_QUIT_EVENT;
			} else if ((info = fSystemApps.InfoFor(team))) {
				fSystemApps.RemoveInfo(info);
				event = SYSTEM_APP_QUIT_EVENT;
			} else	// not found
				return;

			int32 phase = fCurrentPhase;

			_UnlockAppLists();

			// post the event
			_PushEvent(event, team, phase);

			delete info;

			break;
		}

		case MSG_PHASE_TIMED_OUT:
		{
			// get the phase the event is intended for
			int32 phase = TimeoutEvent::GetMessagePhase(message);
			team_id team = TimeoutEvent::GetMessageTeam(message);;

			BAutolock _(fWorkerLock);

			if (phase == INVALID_PHASE || phase != fCurrentPhase)
				return;

			// post the event
			_PushEvent(TIMEOUT_EVENT, team, phase);

			break;
		}

		case MSG_DONE:
		{
			// notify the registrar that we're done
			be_app->PostMessage(B_REG_SHUTDOWN_FINISHED, be_app);

			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// SendReply
void
ShutdownProcess::SendReply(BMessage *request, status_t error)
{
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}
}

// _SetPhase
void
ShutdownProcess::_SetPhase(int32 phase)
{
	BAutolock _(fWorkerLock);

	if (phase == fCurrentPhase)
		return;

	fCurrentPhase = phase;

	// remove the timeout event scheduled for the previous phase
	fEventQueue->RemoveEvent(fTimeoutEvent);
}

// _ScheduleTimeoutEvent
void
ShutdownProcess::_ScheduleTimeoutEvent(bigtime_t timeout, team_id team)
{
	BAutolock _(fWorkerLock);

	// remove the timeout event
	fEventQueue->RemoveEvent(fTimeoutEvent);

	// set the event's phase, team and time
	fTimeoutEvent->SetPhase(fCurrentPhase);
	fTimeoutEvent->SetTeam(team);
	fTimeoutEvent->SetTime(system_time() + timeout);

	// add the event
	fEventQueue->AddEvent(fTimeoutEvent);
}

// _LockAppLists
bool
ShutdownProcess::_LockAppLists()
{
	return fWorkerLock.Lock();
}

// _UnlockAppLists
void
ShutdownProcess::_UnlockAppLists()
{
	fWorkerLock.Unlock();
}

// _NegativeQuitRequestReply
void
ShutdownProcess::_NegativeQuitRequestReply(thread_id thread)
{
	BAutolock _(fWorkerLock);

	// Note: team ID == team main thread ID under Haiku. When testing under R5
	// using the team ID in case of an ABORT_EVENT won't work correctly. But
	// this is done only for system apps.
	_PushEvent(ABORT_EVENT, thread, fCurrentPhase);
}

// _PrepareShutdownMessage
void
ShutdownProcess::_PrepareShutdownMessage(BMessage &message) const
{
	message.what = B_QUIT_REQUESTED;
	message.AddBool("_shutdown_", true);

	BMessage::Private(message).SetReply(BMessenger(fQuitRequestReplyHandler));
}

// _ShutDown
status_t
ShutdownProcess::_ShutDown()
{
	// get the reboot flag
	bool reboot;
	if (fRequest->FindBool("reboot", &reboot) != B_OK)
		reboot = false;

	#ifdef __HAIKU__
		return _kern_shutdown(reboot);
	#else
		// we can't do anything on R5
		return B_ERROR;
	#endif
}


// _PushEvent
status_t
ShutdownProcess::_PushEvent(uint32 eventType, team_id team, int32 phase)
{
	InternalEvent *event = new(nothrow) InternalEvent(eventType, team, phase);
	if (!event) {
		ERROR(("ShutdownProcess::_PushEvent(): Failed to create event!\n"));

		return B_NO_MEMORY;
	}

	BAutolock _(fWorkerLock);

	fInternalEvents->Add(event);
	release_sem(fInternalEventSemaphore);

	return B_OK;
}

// _GetNextEvent
status_t
ShutdownProcess::_GetNextEvent(uint32 &eventType, thread_id &team, int32 &phase,
	bool block)
{
	while (true) {
		// acquire the semaphore
		if (block) {
			status_t error;
			do {
				error = acquire_sem(fInternalEventSemaphore);
			} while (error == B_INTERRUPTED);
	
			if (error != B_OK)
				return error;
	
		} else {
			status_t error = acquire_sem_etc(fInternalEventSemaphore, 1,
				B_RELATIVE_TIMEOUT, 0);
			if (error != B_OK) {
				eventType = NO_EVENT;
				return B_OK;
			}
		}
	
		// get the event
		BAutolock _(fWorkerLock);
	
		InternalEvent *event = fInternalEvents->Head();
		fInternalEvents->Remove(event);

		eventType = event->Type();
		team = event->Team();
		phase = event->Phase();

		delete event;

		// if the event is an obsolete timeout event, we drop it right here
		if (eventType == TIMEOUT_EVENT && phase != fCurrentPhase)
			continue;

		break;
	}

	return B_OK;
}

// _WorkerEntry
status_t
ShutdownProcess::_WorkerEntry(void *data)
{
	return ((ShutdownProcess*)data)->_Worker();
}

// _Worker
status_t
ShutdownProcess::_Worker()
{
	try {
		_WorkerDoShutdown();
		fShutdownError = B_OK;
	} catch (status_t error) {
		PRINT(("ShutdownProcess::_Worker(): caught exception: %s\n",
			strerror(error)));

		fShutdownError = error;
	}

	// this can happen only, if the shutdown process failed or was aborted:
	// notify the looper
	_SetPhase(DONE_PHASE);
	PostMessage(MSG_DONE);

	return B_OK;
}

// _WorkerDoShutdown
void
ShutdownProcess::_WorkerDoShutdown()
{
	PRINT(("ShutdownProcess::_WorkerDoShutdown()\n"));

	// TODO: Set alert text to "Tidying things up a bit".
	PRINT(("  sync()...\n"));
	sync();

	// phase 1: terminate the user apps
	_SetPhase(USER_APP_TERMINATION_PHASE);
	_QuitUserApps();
	_ScheduleTimeoutEvent(USER_APPS_QUIT_TIMEOUT);
	_WaitForUserApps();
	_KillUserApps();

	// phase 2: terminate the system apps
	_SetPhase(SYSTEM_APP_TERMINATION_PHASE);
	_QuitSystemApps();

	// phase 3: terminate the other processes
	_SetPhase(OTHER_PROCESSES_TERMINATION_PHASE);
	_QuitNonApps();

	// we're through: do the shutdown
	_SetPhase(DONE_PHASE);
	_ShutDown();

	PRINT(("  _kern_shutdown() failed\n"));

	// shutdown failed: This can happen for power off mode -- reboot will
	// always work. We close the alert, and display a new one with
	// a "Reboot System" button.
	if (fHasGUI) {
		// TODO:...
	} else {
		// there's no GUI: we enter the kernel debugger instead
		// TODO:...
	}
}

// _QuitUserApps
void
ShutdownProcess::_QuitUserApps()
{
	PRINT(("ShutdownProcess::_QuitUserApps()\n"));

	// TODO: Set alert text to "Asking user applications to quit"

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// send shutdown messages to user apps
	_LockAppLists();

	AppInfoListMessagingTargetSet targetSet(fUserApps);

	if (targetSet.HasNext()) {
		PRINT(("  sending shutdown message to %ld apps\n",
			fUserApps.CountInfos()));

		status_t error = MessageDeliverer::Default()->DeliverMessage(
			&message, targetSet);
		if (error != B_OK) {
			WARNING(("ShutdownProcess::_Worker(): Failed to deliver "
				"shutdown message to all applications: %s\n",
				strerror(error)));
		}
	}

	_UnlockAppLists();

	PRINT(("ShutdownProcess::_QuitUserApps() done\n"));
}

// _WaitForUserApps
void
ShutdownProcess::_WaitForUserApps()
{
	PRINT(("ShutdownProcess::_WaitForUserApps()\n"));

	// wait for user apps
	bool moreApps = true;
	while (moreApps) {
		_LockAppLists();
		moreApps = !fUserApps.IsEmpty();
		_UnlockAppLists();

		if (moreApps) {
			uint32 event;
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, true);
			if (error != B_OK)
				throw error;

			if (event == ABORT_EVENT)
				throw B_SHUTDOWN_CANCELLED;

			if (event == TIMEOUT_EVENT)	
				return;
		}
	}

	PRINT(("ShutdownProcess::_WaitForUserApps() done\n"));
}

// _KillUserApps
void
ShutdownProcess::_KillUserApps()
{
	PRINT(("ShutdownProcess::_KillUserApps()\n"));

	while (true) {
		// eat events (we need to be responsive for an abort event)
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw error;

			if (event == ABORT_EVENT)
				throw B_SHUTDOWN_CANCELLED;

		} while (event != NO_EVENT);

		// get the first team to kill
		_LockAppLists();

		team_id team = -1;
		AppInfoList &list = fUserApps;
		if (!list.IsEmpty()) {
			RosterAppInfo *info = *list.It();
			team = info->team;
		}

		_UnlockAppLists();

		if (team < 0) {
			PRINT(("ShutdownProcess::_KillUserApps() done\n"));
			return;
		}

		// TODO: Set alert text...

		// TODO: check whether the app blocks on a modal alert
		if (false) {
			// ...
		} else {
			// it does not: kill it
			PRINT(("  killing team %ld\n", team));

			kill_team(team);

			// remove the app (the roster will note eventually and send us
			// a notification, but we want to be sure)
			_LockAppLists();
	
			if (RosterAppInfo *info = list.InfoFor(team)) {
				list.RemoveInfo(info);
				delete info;
			}
	
			_UnlockAppLists();
		}
	}
}

// _QuitSystemApps
void
ShutdownProcess::_QuitSystemApps()
{
	PRINT(("ShutdownProcess::_QuitSystemApps()\n"));

	// TODO: Disable the abort button.

	// check one last time for abort events
	uint32 event;
	do {
		team_id team;
		int32 phase;
		status_t error = _GetNextEvent(event, team, phase, false);
		if (error != B_OK)
			throw error;

		if (event == ABORT_EVENT)
			throw B_SHUTDOWN_CANCELLED;

	} while (event != NO_EVENT);

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// now iterate through the list of system apps
	while (true) {
		// eat events
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw error;
		} while (event != NO_EVENT);

		// get the first app to quit
		_LockAppLists();

		AppInfoList &list = fSystemApps;
		team_id team = -1;
		port_id port = -1;
		if (!list.IsEmpty()) {
			RosterAppInfo *info = *list.It();
			team = info->team;
			port = info->port;
		}

		_UnlockAppLists();

		if (team < 0) {
			PRINT(("ShutdownProcess::_QuitSystemApps() done\n"));
			return;
		}

		// TODO: Set alert text...

		// send the shutdown message to the app
		PRINT(("  sending team %ld (port: %ld) a shutdown message\n", team,
			port));
		SingleMessagingTargetSet target(port, B_PREFERRED_TOKEN);
		MessageDeliverer::Default()->DeliverMessage(&message, target);

		// schedule a timeout event
		_ScheduleTimeoutEvent(SYSTEM_APPS_QUIT_TIMEOUT, team);

		// wait for the app to die or for the timeout to occur
		do {
			team_id eventTeam;
			int32 phase;
			status_t error = _GetNextEvent(event, eventTeam, phase, true);
			if (error != B_OK)
				throw error;

			if (event == TIMEOUT_EVENT && eventTeam == team)
				break;

			// If the app requests aborting the shutdown, we don't need to
			// wait any longer. It has processed the request and won't quit by
			// itself. We'll have to kill it.
			if (event == ABORT_EVENT && eventTeam == team)
				break;

			BAutolock _(fWorkerLock);
			if (!list.InfoFor(team))
				break;

		} while (event != NO_EVENT);

		// TODO: Set alert text...

		// TODO: check whether the app blocks on a modal alert
		if (false) {
			// ...
		} else {
			// it does not: kill it
			PRINT(("  killing team %ld\n", team));

			kill_team(team);

			// remove the app (the roster will note eventually and send us
			// a notification, but we want to be sure)
			_LockAppLists();
	
			if (RosterAppInfo *info = list.InfoFor(team)) {
				list.RemoveInfo(info);
				delete info;
			}
	
			_UnlockAppLists();
		}
	}
}

// _QuitNonApps
void
ShutdownProcess::_QuitNonApps()
{
	PRINT(("ShutdownProcess::_QuitNonApps()\n"));

	// TODO: Set alert text...

	// iterate through the remaining teams and send them the HUP signal
	int32 cookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (fVitalSystemApps.find(teamInfo.team) == fVitalSystemApps.end()) {
			PRINT(("  sending team %ld HUP signal\n", teamInfo.team));

			#ifdef __HAIKU__
				// Note: team ID == team main thread ID under Haiku
				send_signal(teamInfo.team, SIGHUP);
			#else
				// We don't want to do this when testing under R5, since it
				// would kill all teams besides our app server and registrar.
			#endif
		}
	}

	// give them a bit of time to terminate
	// TODO: Instead of just waiting we could periodically check whether the
	// processes are already gone to shorten the process.
	snooze(NON_APPS_QUIT_TIMEOUT);

	// iterate through the remaining teams and kill them
	cookie = 0;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (fVitalSystemApps.find(teamInfo.team) == fVitalSystemApps.end()) {
			PRINT(("  killing team %ld\n", teamInfo.team));

			#ifdef __HAIKU__
				kill_team(teamInfo.team);
			#else
				// We don't want to do this when testing under R5, since it
				// would kill all teams besides our app server and registrar.
			#endif
		}
	}

	PRINT(("ShutdownProcess::_QuitNonApps() done\n"));
}

