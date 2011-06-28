/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef BAR_APP_H
#define BAR_APP_H


#include <Application.h>
#include <Catalog.h>
#include <List.h>
#include "BarWindow.h"
#include "PreferencesWindow.h"


/* ------------------------------------ */
// Private app_server defines that I need to use

#define _DESKTOP_W_TYPE_ 1024
#define _FLOATER_W_TYPE_ 4
#define _STD_W_TYPE_ 0


class BarTeamInfo {
public:
	BarTeamInfo(BList* teams, uint32 flags, char* sig, BBitmap* icon,
				char* name);
	BarTeamInfo(const BarTeamInfo &info);
	~BarTeamInfo();

	BList* teams;
	uint32 flags;
	char* sig;
	BBitmap* icon;
	char* name;
};

const uint32 kWin95 = 'Bill';
const uint32 kAmiga = 'Ncro';
const uint32 kMac = 'WcOS';
const uint32 kBe = 'Tabs';
const uint32 kAlwaysTop = 'TTop';
const uint32 kToggleDraggers = 'TDra';
const uint32 kUnsubscribe = 'Unsb';
const uint32 kAddTeam = 'AdTm';
const uint32 kRemoveTeam = 'RmTm';
const uint32 kRestart = 'Rtrt';
const uint32 kShutDown = 'ShDn';
const uint32 kTrackerFirst = 'TkFt';
const uint32 kSortRunningApps = 'SAps';
const uint32 kSuperExpando = 'SprE';
const uint32 kExpandNewTeams = 'ExTm';
const uint32 kHideLabels = 'hLbs';
const uint32 kResizeTeamIcons = 'RTIs';
const uint32 kAutoRaise = 'AtRs';
const uint32 kAutoHide = 'AtHd';
const uint32 kRestartTracker = 'Trak';

// from roster_private.h
const uint32 kShutdownSystem = 301;
const uint32 kRebootSystem = 302;
const uint32 kSuspendSystem = 304;

// icon size constants
const int32 kMinimumIconSize = 16;
const int32 kMaximumIconSize = 96;
const int32 kIconSizeInterval = 8;

/* --------------------------------------------- */

struct desk_settings {
	bool vertical;
	bool left;
	bool top;
	bool ampmMode;
	bool showTime;
	uint32 state;
	float width;
	BPoint switcherLoc;
	int32 recentAppsCount;
	int32 recentDocsCount;
	bool timeShowSeconds;
	int32 recentFoldersCount;
	bool alwaysOnTop;
	bool timeFullDate;
	bool trackerAlwaysFirst;
	bool sortRunningApps;
	bool superExpando;
	bool expandNewTeams;
	bool hideLabels;
	int32 iconSize;
	bool autoRaise;
	bool autoHide;
	bool recentAppsEnabled;
	bool recentDocsEnabled;
	bool recentFoldersEnabled;
};


class TBarView;
class BFile;

using namespace BPrivate;

class TBarApp : public BApplication {
	public:
		TBarApp();
		virtual ~TBarApp();

		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);
		virtual void RefsReceived(BMessage* refs);

		desk_settings* Settings()
			{ return &fSettings; }
		TBarView* BarView() const
			{ return fBarWindow->BarView(); }
		TBarWindow* BarWindow() const
			{ return fBarWindow; }

		static void Subscribe(const BMessenger &subscriber, BList*);
		static void Unsubscribe(const BMessenger &subscriber);
		int32 IconSize();

	private:
		void AddTeam(team_id team, uint32 flags, const char* sig, entry_ref*);
		void RemoveTeam(team_id);

		void InitSettings();
		void SaveSettings();

		void ShowPreferencesWindow();
		void ResizeTeamIcons();
		void FetchAppIcon(const char* signature, BBitmap* icon);
		BRect IconRect();

		TBarWindow* fBarWindow;
		BMessenger fSwitcherMessenger;
		BMessenger fStatusViewMessenger;
		BFile* fSettingsFile;
		desk_settings fSettings;

		PreferencesWindow* fPreferencesWindow;

		static BLocker sSubscriberLock;
		static BList sBarTeamInfoList;
		static BList sSubscribers;
};

#endif	// BAR_APP_H

