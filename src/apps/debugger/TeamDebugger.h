/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_DEBUGGER_H
#define TEAM_DEBUGGER_H

#include <debugger.h>
#include <Locker.h>


class Team;


class TeamDebugger {
public:
								TeamDebugger();
								~TeamDebugger();

			status_t			Init(team_id teamID);


private:
	static	status_t			_DebugEventListenerEntry(void* data);
			status_t			_DebugEventListener();

			void				_HandleDebuggerMessage(int32 messageCode,
									const debug_debugger_message_data& message);

private:
			BLocker				fLock;
			Team*				fTeam;
			team_id				fTeamID;
			port_id				fDebuggerPort;
			port_id				fNubPort;
			thread_id			fDebugEventListener;
	volatile bool				fTerminating;
};

#endif	// TEAM_DEBUGGER_H
