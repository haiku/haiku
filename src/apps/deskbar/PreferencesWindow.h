/*
 * Copyright 2009 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PREFERENCES_WINDOW_H
#define _PREFERENCES_WINDOW_H


#include <Window.h>

#include "BarSettings.h"


const uint32 kConfigShow			= 'PrSh';
const uint32 kConfigQuit			= 'PrQt';
const uint32 kUpdateRecentCounts	= 'upct';
const uint32 kEditInTracker			= 'etrk';

const uint32 kTrackerFirst			= 'TkFt';
const uint32 kSortRunningApps		= 'SAps';
const uint32 kSuperExpando			= 'SprE';
const uint32 kExpandNewTeams		= 'ExTm';
const uint32 kHideLabels			= 'hLbs';
const uint32 kResizeTeamIcons		= 'RTIs';
const uint32 kAutoRaise				= 'AtRs';
const uint32 kAutoHide				= 'AtHd';

const uint32 kDefaults				= 'dflt';
const uint32 kRevert				= 'rvrt';


class BCheckBox;
class BFile;
class BMessage;
class BRadioButton;
class BSlider;
class BTextControl;

class PreferencesWindow : public BWindow
{
public:
							PreferencesWindow(BRect frame);
							~PreferencesWindow();

	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	virtual	void			Show();

private:
			void			_EnableDisableDependentItems();

			bool			_IsDefaultable();
			bool			_IsRevertable();

			status_t		_InitSettingsFile(BFile* file, bool write);
			status_t		_LoadSettings(BMessage* settings);
			status_t		_SaveSettings(BMessage* settings);

			void			_SetInitialSettings();

			void			_UpdateButtons();
			void			_UpdatePreferences(desk_settings* settings);
			void			_UpdateRecentCounts();

private:
			BCheckBox*		fMenuRecentDocuments;
			BCheckBox*		fMenuRecentApplications;
			BCheckBox*		fMenuRecentFolders;

			BTextControl*	fMenuRecentDocumentCount;
			BTextControl*	fMenuRecentApplicationCount;
			BTextControl*	fMenuRecentFolderCount;

			BCheckBox*		fAppsSort;
			BCheckBox*		fAppsSortTrackerFirst;
			BCheckBox*		fAppsShowExpanders;
			BCheckBox*		fAppsExpandNew;
			BCheckBox*		fAppsHideLabels;
			BSlider*		fAppsIconSizeSlider;

			BCheckBox*		fWindowAlwaysOnTop;
			BCheckBox*		fWindowAutoRaise;
			BCheckBox*		fWindowAutoHide;

			BButton*		fDefaultsButton;
			BButton*		fRevertButton;

private:
			desk_settings	fSettings;
};


#endif	// _PREFERENCES_WINDOW_H
