/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_HOST_INTERFACE_H
#define TARGET_HOST_INTERFACE_H

#include <OS.h>
#include <Looper.h>

#include <ObjectList.h>

#include <util/DoublyLinkedList.h>

#include "controllers/TeamDebugger.h"


class DebuggerInterface;
class Settings;
class SettingsManager;
class TargetHost;
struct TeamDebuggerOptions;
class UserInterface;


class TargetHostInterface : public BLooper, private TeamDebugger::Listener {
public:
	class Listener;
								TargetHostInterface();
	virtual						~TargetHostInterface();

			status_t			StartTeamDebugger(const TeamDebuggerOptions& options);

			int32				CountTeamDebuggers() const;
			TeamDebugger*		TeamDebuggerAt(int32 index) const;
			TeamDebugger*		FindTeamDebugger(team_id team) const;
			status_t			AddTeamDebugger(TeamDebugger* debugger);
			void				RemoveTeamDebugger(TeamDebugger* debugger);

	virtual	status_t			Init(Settings* settings) = 0;
	virtual	void				Close() = 0;

	virtual	bool				IsLocal() const = 0;
	virtual	bool				Connected() const = 0;

	virtual	TargetHost*			GetTargetHost() = 0;

	virtual	status_t			Attach(team_id id, thread_id threadID,
									DebuggerInterface*& _interface) const = 0;
	virtual	status_t			CreateTeam(int commandLineArgc,
									const char* const* arguments,
									team_id& _teamID) const = 0;
	virtual	status_t			LoadCore(const char* coreFilePath,
									DebuggerInterface*& _interface,
									thread_id& _thread) const = 0;

	virtual	status_t			FindTeamByThread(thread_id thread,
									team_id& _teamID) const = 0;


			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

	// BLooper
	virtual	void				Quit();
	virtual	void				MessageReceived(BMessage* message);

private:
	// TeamDebugger::Listener
	virtual void 				TeamDebuggerStarted(TeamDebugger* debugger);
	virtual	void				TeamDebuggerRestartRequested(
									TeamDebugger* debugger);
	virtual void 				TeamDebuggerQuit(TeamDebugger* debugger);

private:
			status_t			_StartTeamDebugger(team_id teamID,
									const TeamDebuggerOptions& options,
									bool stopInMain);

			void				_NotifyTeamDebuggerStarted(
									TeamDebugger* debugger);
			void				_NotifyTeamDebuggerQuit(
									TeamDebugger* debugger);

	static	int					_CompareDebuggers(const TeamDebugger* a,
									const TeamDebugger* b);
private:
			typedef DoublyLinkedList<Listener> ListenerList;
			typedef BObjectList<TeamDebugger> TeamDebuggerList;

private:
			ListenerList		fListeners;
			TeamDebuggerList	fTeamDebuggers;
};


class TargetHostInterface::Listener
	: public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();
	virtual	void				TeamDebuggerStarted(TeamDebugger* debugger);
	virtual	void				TeamDebuggerQuit(TeamDebugger* debugger);
	virtual	void				TargetHostInterfaceQuit(
									TargetHostInterface* interface);
};


enum {
	TEAM_DEBUGGER_REQUEST_UNKNOWN = 0,
	TEAM_DEBUGGER_REQUEST_CREATE,
	TEAM_DEBUGGER_REQUEST_ATTACH,
	TEAM_DEBUGGER_REQUEST_LOAD_CORE
};


struct TeamDebuggerOptions {
						TeamDebuggerOptions();
	int					requestType;
	int					commandLineArgc;
	const char* const*	commandLineArgv;
	team_id				team;
	thread_id			thread;
	SettingsManager*	settingsManager;
	UserInterface*		userInterface;
	const char*			coreFilePath;
};


#endif	// TARGET_HOST_INTERFACE_H
