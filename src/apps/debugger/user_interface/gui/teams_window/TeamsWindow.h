/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAMS_WINDOW_H
#define TEAMS_WINDOW_H


#include <Window.h>

class BListView;
class BFile;
class BMessage;
class SettingsManager;


class TeamsWindow : public BWindow {
public:
								TeamsWindow(SettingsManager* settingsManager);
	virtual						~TeamsWindow();

	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

private:

			status_t			_OpenSettings(BFile& file, uint32 mode);
			status_t			_LoadSettings(BMessage& settings);
			status_t			_SaveSettings();

private:
			BListView*			fTeamsListView;
			SettingsManager*	fSettingsManager;

};

static const uint32 kMsgDebugThisTeam = 'dbtm';

#endif	// TEAMS_WINDOW_H
