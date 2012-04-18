/*
 * Copyright 2009 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PREFERENCES_WINDOW_H
#define _PREFERENCES_WINDOW_H


#include <Window.h>


const uint32 kConfigShow			= 'show';
const uint32 kConfigClose			= 'canc';
const uint32 kUpdateRecentCounts	= 'upct';
const uint32 kEditMenuInTracker		= 'mtrk';

const uint32 kTrackerFirst			= 'TkFt';
const uint32 kSortRunningApps		= 'SAps';
const uint32 kSuperExpando			= 'SprE';
const uint32 kExpandNewTeams		= 'ExTm';
const uint32 kHideLabels			= 'hLbs';
const uint32 kResizeTeamIcons		= 'RTIs';
const uint32 kAutoRaise				= 'AtRs';
const uint32 kAutoHide				= 'AtHd';

const uint32 kShowHideTime			= 'ShTm';
const uint32 kShowSeconds			= 'SwSc';
const uint32 kShowDayOfWeek			= 'SwDw';

class BBox;
class BButton;
class BCheckBox;
class BRadioButton;
class BSlider;
class BStringView;
class BTextControl;

class PreferencesWindow : public BWindow
{
public:
							PreferencesWindow(BRect frame);
							~PreferencesWindow();

		virtual	void		MessageReceived(BMessage* message);
		virtual	void		WindowActivated(bool active);

				void		UpdateRecentCounts();
				void		EnableDisableDependentItems();

private:
			BBox*			fMenuBox;
			BBox*			fAppsBox;
			BBox*			fClockBox;
			BBox*			fWindowBox;

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

			BCheckBox*		fShowSeconds;
			BCheckBox*		fShowDayOfWeek;
};


#endif	// _PREFERENCES_WINDOW_H
