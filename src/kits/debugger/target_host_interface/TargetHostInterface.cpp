/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TargetHostInterface.h"

#include <stdio.h>

#include <AutoLocker.h>

#include "DebuggerInterface.h"
#include "MessageCodes.h"
#include "TeamDebugger.h"


// #pragma mark - TeamDebuggerOptions


TeamDebuggerOptions::TeamDebuggerOptions()
	:
	requestType(TEAM_DEBUGGER_REQUEST_UNKNOWN),
	commandLineArgc(0),
	commandLineArgv(NULL),
	team(-1),
	thread(-1),
	settingsManager(NULL),
	userInterface(NULL),
	coreFilePath(NULL)
{
}


// #pragma mark - TargetHostInterface


TargetHostInterface::TargetHostInterface()
	:
	BLooper(),
	fListeners(),
	fTeamDebuggers(20, false)
{
}


TargetHostInterface::~TargetHostInterface()
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TargetHostInterfaceQuit(this);
	}
}


status_t
TargetHostInterface::StartTeamDebugger(const TeamDebuggerOptions& options)
{
	// we only want to stop in main for teams we're responsible for
	// creating ourselves.
	bool stopInMain = options.requestType == TEAM_DEBUGGER_REQUEST_CREATE;
	team_id team = options.team;
	thread_id thread = options.thread;

	AutoLocker<TargetHostInterface> interfaceLocker(this);
	if (options.requestType == TEAM_DEBUGGER_REQUEST_CREATE) {
		status_t error = CreateTeam(options.commandLineArgc,
			options.commandLineArgv, team);
		if (error != B_OK)
			return error;
		thread = team;
	}

	if (options.requestType != TEAM_DEBUGGER_REQUEST_LOAD_CORE) {

		if (team < 0 && thread < 0)
			return B_BAD_VALUE;

		if (team < 0) {
			status_t error = FindTeamByThread(thread, team);
			if (error != B_OK)
				return error;
		}

		TeamDebugger* debugger = FindTeamDebugger(team);
		if (debugger != NULL) {
			debugger->Activate();
			return B_OK;
		}
	}

	return _StartTeamDebugger(team, options, stopInMain);
}


int32
TargetHostInterface::CountTeamDebuggers() const
{
	return fTeamDebuggers.CountItems();
}


TeamDebugger*
TargetHostInterface::TeamDebuggerAt(int32 index) const
{
	return fTeamDebuggers.ItemAt(index);
}


TeamDebugger*
TargetHostInterface::FindTeamDebugger(team_id team) const
{
	for (int32 i = 0; i < fTeamDebuggers.CountItems(); i++) {
		TeamDebugger* debugger = fTeamDebuggers.ItemAt(i);
		if (debugger->TeamID() == team && !debugger->IsPostMortem())
			return debugger;
	}

	return NULL;
}


status_t
TargetHostInterface::AddTeamDebugger(TeamDebugger* debugger)
{
	if (!fTeamDebuggers.BinaryInsert(debugger, &_CompareDebuggers))
		return B_NO_MEMORY;

	return B_OK;
}


void
TargetHostInterface::RemoveTeamDebugger(TeamDebugger* debugger)
{
	for (int32 i = 0; i < fTeamDebuggers.CountItems(); i++) {
		if (fTeamDebuggers.ItemAt(i) == debugger) {
			fTeamDebuggers.RemoveItemAt(i);
			break;
		}
	}
}


void
TargetHostInterface::AddListener(Listener* listener)
{
	AutoLocker<TargetHostInterface> interfaceLocker(this);
	fListeners.Add(listener);
}


void
TargetHostInterface::RemoveListener(Listener* listener)
{
	AutoLocker<TargetHostInterface> interfaceLocker(this);
	fListeners.Remove(listener);
}


void
TargetHostInterface::Quit()
{
	if (fTeamDebuggers.CountItems() == 0)
		BLooper::Quit();
}


void
TargetHostInterface::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_TEAM_DEBUGGER_QUIT:
	{
		thread_id thread;
		if (message->FindInt32("thread", &thread) == B_OK)
			wait_for_thread(thread, NULL);
		break;
	}
	case MSG_TEAM_RESTART_REQUESTED:
	{
		int32 teamID;
		if (message->FindInt32("team", &teamID) != B_OK)
			break;

		TeamDebugger* debugger = FindTeamDebugger(teamID);

		UserInterface* userInterface = debugger->GetUserInterface()->Clone();
		if (userInterface == NULL)
			break;

		BReference<UserInterface> userInterfaceReference(userInterface, true);

		TeamDebuggerOptions options;
		options.requestType = TEAM_DEBUGGER_REQUEST_CREATE;
		options.commandLineArgc = debugger->ArgumentCount();
		options.commandLineArgv = debugger->Arguments();
		options.settingsManager = debugger->GetSettingsManager();
		options.userInterface = userInterface;
		status_t result = StartTeamDebugger(options);
		if (result == B_OK) {
			userInterfaceReference.Detach();
			debugger->PostMessage(B_QUIT_REQUESTED);
		}
		break;
	}
	default:
		BLooper::MessageReceived(message);
		break;
	}
}


void
TargetHostInterface::TeamDebuggerStarted(TeamDebugger* debugger)
{
	AutoLocker<TargetHostInterface> locker(this);
	AddTeamDebugger(debugger);
	_NotifyTeamDebuggerStarted(debugger);
}


void
TargetHostInterface::TeamDebuggerRestartRequested(TeamDebugger* debugger)
{
	BMessage message(MSG_TEAM_RESTART_REQUESTED);
	message.AddInt32("team", debugger->TeamID());
	PostMessage(&message);
}


void
TargetHostInterface::TeamDebuggerQuit(TeamDebugger* debugger)
{
	AutoLocker<TargetHostInterface> interfaceLocker(this);
	RemoveTeamDebugger(debugger);

	if (debugger->Thread() >= 0) {
		_NotifyTeamDebuggerQuit(debugger);
		BMessage message(MSG_TEAM_DEBUGGER_QUIT);
		message.AddInt32("thread", debugger->Thread());
		PostMessage(&message);
	}
}


status_t
TargetHostInterface::_StartTeamDebugger(team_id teamID,
	const TeamDebuggerOptions& options, bool stopInMain)
{
	UserInterface* userInterface = options.userInterface;
	if (userInterface == NULL) {
		fprintf(stderr, "Error: Requested team debugger start without "
			"valid user interface!\n");
		return B_BAD_VALUE;
	}

	thread_id threadID = options.thread;
	if (options.commandLineArgv != NULL)
		threadID = teamID;

	DebuggerInterface* interface = NULL;
	TeamDebugger* debugger = NULL;
	status_t error = B_OK;
	if (options.requestType != TEAM_DEBUGGER_REQUEST_LOAD_CORE) {
		error = Attach(teamID, options.thread, interface);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to attach to team %" B_PRId32
				": %s!\n", teamID, strerror(error));
			return error;
		}
	} else {
		error = LoadCore(options.coreFilePath, interface, threadID);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to load core file '%s': %s!\n",
				options.coreFilePath, strerror(error));
			return error;
		}
	}

	BReference<DebuggerInterface> debuggerInterfaceReference(interface,
		true);
	debugger = new(std::nothrow) TeamDebugger(this, userInterface,
		options.settingsManager);
	if (debugger != NULL) {
		error = debugger->Init(interface, threadID,
			options.commandLineArgc, options.commandLineArgv, stopInMain);
	}

	if (error != B_OK) {
		printf("Error: debugger for team %" B_PRId32 " on interface %s failed"
			" to init: %s!\n", interface->TeamID(), Name(), strerror(error));
		delete debugger;
		debugger = NULL;
	} else {
		printf("debugger for team %" B_PRId32 " on interface %s created and"
			" initialized successfully!\n", interface->TeamID(), Name());
	}

	return error;
}


void
TargetHostInterface::_NotifyTeamDebuggerStarted(TeamDebugger* debugger)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamDebuggerStarted(debugger);
	}
}


void
TargetHostInterface::_NotifyTeamDebuggerQuit(TeamDebugger* debugger)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->TeamDebuggerQuit(debugger);
	}
}


/*static*/ int
TargetHostInterface::_CompareDebuggers(const TeamDebugger* a,
	const TeamDebugger* b)
{
	return a->TeamID() < b->TeamID() ? -1 : 1;
}


// #pragma mark - TargetHostInterface::Listener


TargetHostInterface::Listener::~Listener()
{
}


void
TargetHostInterface::Listener::TeamDebuggerStarted(TeamDebugger* debugger)
{
}


void
TargetHostInterface::Listener::TeamDebuggerQuit(TeamDebugger* debugger)
{
}


void
TargetHostInterface::Listener::TargetHostInterfaceQuit(
	TargetHostInterface* interface)
{
}
