/*
 * Copyright 2009 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PREFERENCES_WINDOW_H
#define _PREFERENCES_WINDOW_H


#include <Window.h>


const uint32 kConfigShow			= 'show';
const uint32 kUpdateRecentCounts	= 'upct';
const uint32 kOpenInTracker			= 'otrk';

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


struct pref_settings {
	// applications
	bool trackerAlwaysFirst;
	bool sortRunningApps;
	bool superExpando;
	bool expandNewTeams;
	bool hideLabels;
	int32 iconSize;
	// recent items
	int32 recentAppsCount;
	int32 recentDocsCount;
	int32 recentFoldersCount;
	bool recentAppsEnabled;
	bool recentDocsEnabled;
	bool recentFoldersEnabled;
	// window
	bool alwaysOnTop;
	bool autoRaise;
	bool autoHide;
};


class BCheckBox;
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
	virtual	void			WindowActivated(bool active);

private:
			void			_EnableDisableDependentItems();

			bool			_IsDefaultable();
			bool			_IsRevertable();

			void			_SetDefaultSettings();
			void			_SetInitialSettings();

			void			_UpdateButtons();
			void			_UpdatePreferences(pref_settings prefs);
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

			BButton*		fRevertButton;
			BButton*		fDefaultsButton;

private:
			pref_settings	fDefaultSettings;
			pref_settings	fInitialSettings;
};


#endif	/* _PREFERENCES_WINDOW_H */
