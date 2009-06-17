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

#include "TeamWindow.h"


class Team;


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

private:
	static	status_t			_DebugEventListenerEntry(void* data);
			status_t			_DebugEventListener();

			void				_HandleDebuggerMessage(int32 messageCode,
									const debug_debugger_message_data& message);

			bool				_HandleThreadCreated(
									const debug_thread_created& message);
			bool				_HandleThreadDeleted(
									const debug_thread_deleted& message);
			bool				_HandleImageCreated(
									const debug_image_created& message);
			bool				_HandleImageDeleted(
									const debug_image_deleted& message);

private:
			::Team*				fTeam;
			team_id				fTeamID;
			port_id				fDebuggerPort;
			port_id				fNubPort;
			debug_context		fDebugContext;
			thread_id			fDebugEventListener;
			TeamWindow*			fTeamWindow;
	volatile bool				fTerminating;
};

#endif	// TEAM_DEBUGGER_H
