/*
 * Copyright 2008, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#ifndef TEAM_WINDOW_H
#define TEAM_WINDOW_H

#include <Window.h>
#include <debug_support.h>

class BFile;
class BMessage;


class TeamWindow : public BWindow {
public:
				TeamWindow(team_id team);
	virtual		~TeamWindow();

	virtual void	MessageReceived(BMessage* message);
	virtual bool	QuitRequested();

    team_id	Team()  { return fTeam; };

private:

	status_t		_InstallDebugger();
	status_t		_RemoveDebugger();
	
	static status_t _ListenerEntry(void *data);
	status_t 		_Listener();
	status_t		_HandleDebugMessage(debug_debugger_message message, 
						debug_debugger_message_data & data);

	status_t		_OpenSettings(BFile& file, uint32 mode);
	status_t		_LoadSettings(BMessage& settings);
	status_t		_SaveSettings();

	// ----

	team_id			fTeam;
	thread_id		fNubThread;
	port_id			fNubPort;
	debug_context	fDebugContext;
	
	thread_id		fListenerThread;
	port_id			fListenerPort;
};

#endif	// TEAM_WINDOW_H
