/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TEAM_DEBUGGER_H
#define _TEAM_DEBUGGER_H


#include <debugger.h>

#include <DebugContext.h>


class BPath;


class BTeamDebugger : public BDebugContext {
public:
								BTeamDebugger();
								~BTeamDebugger();

			status_t			Install(team_id team);
			status_t			Uninstall();

			status_t			LoadProgram(const char* const* args,
									int32 argCount, bool traceLoading);

			status_t			ReadDebugMessage(int32& _messageCode,
									debug_debugger_message_data& messageBuffer);

			port_id				DebuggerPort() const { return fDebuggerPort; }

private:
	static	thread_id			_LoadProgram(const char* const* args,
									int32 argCount, bool traceLoading);
	static	status_t			_FindProgram(const char* programName,
									BPath& resolvedPath);

private:
			port_id				fDebuggerPort;
};


#endif	// _TEAM_DEBUGGER_H
