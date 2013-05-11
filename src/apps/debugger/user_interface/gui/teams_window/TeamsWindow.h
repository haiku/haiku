/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAMS_WINDOW_H
#define TEAMS_WINDOW_H


#include <Window.h>

class BButton;
class BListView;
class BFile;
class BMessage;
class SettingsManager;
class StartTeamWindow;
class TeamsListView;

class TeamsWindow : public BWindow {
public:
								TeamsWindow(SettingsManager* settingsManager);
	virtual						~TeamsWindow();

	static	TeamsWindow*		Create(SettingsManager* settingsManager);
									// throws

	virtual	void				Zoom(BPoint origin, float width, float height);
	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

private:
			void				_Init();
			status_t			_OpenSettings(BFile& file, uint32 mode);
			status_t			_LoadSettings(BMessage& settings);
			status_t			_SaveSettings();

private:
			team_id				fCurrentTeam;
			TeamsListView*		fTeamsListView;
			StartTeamWindow*	fStartTeamWindow;
			BButton*			fAttachTeamButton;
			BButton*			fCreateTeamButton;
			SettingsManager*	fSettingsManager;

};


#endif	// TEAMS_WINDOW_H
