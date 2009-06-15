/*
 * Copyright 2009, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#ifndef RUNNING_TEAMS_WINDOW_H
#define RUNNING_TEAMS_WINDOW_H

#include <Window.h>

class BListView;
class BFile;
class BMessage;


class RunningTeamsWindow : public BWindow {
public:
				RunningTeamsWindow();
	virtual		~RunningTeamsWindow();

	virtual void	MessageReceived(BMessage* message);
	virtual bool	QuitRequested();

private:

	status_t	_OpenSettings(BFile& file, uint32 mode);
	status_t	_LoadSettings(BMessage& settings);
	status_t	_SaveSettings();

	BListView*	fTeamsListView;

};

static const uint32 kMsgDebugThisTeam = 'dbtm';

#endif	// RUNNING_TEAMS_WINDOW_H
