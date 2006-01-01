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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef BAR_APP_H
#define BAR_APP_H


#include <Application.h>
#include <List.h>
#include "BarWindow.h"


/* ------------------------------------ */
// Private app_server defines that I need to use

#define _DESKTOP_W_TYPE_ 1024
#define _FLOATER_W_TYPE_ 4
#define _STD_W_TYPE_ 0


class BarTeamInfo {
public:
	BarTeamInfo(BList *teams, uint32 flags, char *sig, BBitmap *icon,
				char *name);
	~BarTeamInfo();

	BList *teams;
	uint32 flags;
	char *sig;
	BBitmap *icon;
	char *name;
};

const uint32 msg_Win95 = 'Bill';
const uint32 msg_Amiga = 'Ncro';
const uint32 msg_Mac = 'WcOS';
const uint32 msg_Be = 'Tabs';
const uint32 msg_AlwaysTop = 'TTop';
const uint32 msg_ToggleDraggers = 'TDra';
const uint32 msg_config_db = 'cnfg';
const uint32 msg_Unsubscribe = 'Unsb';
const uint32 msg_AddTeam = 'AdTm';
const uint32 msg_RemoveTeam = 'RmTm';
const uint32 msg_Restart = 'Rtrt';
const uint32 msg_ShutDown = 'ShDn';
const uint32 msg_trackerFirst = 'TkFt';
const uint32 msg_sortRunningApps = 'SAps';
const uint32 msg_superExpando = 'SprE';
const uint32 msg_expandNewTeams = 'ExTm';

// from roster_private.h
const uint32 CMD_SHUTDOWN_SYSTEM = 301;
const uint32 CMD_REBOOT_SYSTEM = 302;
const uint32 CMD_SUSPEND_SYSTEM = 304;

/* --------------------------------------------- */

// if you want to extend this structure, maintain the
// constants below (used in TBarApp::InitSettings())

struct desk_settings {
	bool vertical;				// version 1
	bool left;
	bool top;
	bool ampmMode;
	bool showTime;
	uint32 state;
	float width;
	BPoint switcherLoc;			// version 2
	int32 recentAppsCount;		// version 3
	int32 recentDocsCount;
	bool timeShowSeconds;		// version 4
	bool timeShowMil;
	int32 recentFoldersCount;	// version 5
	bool timeShowEuro;			// version 6
	bool alwaysOnTop;
	bool timeFullDate;			// version 7
	bool trackerAlwaysFirst;	// version 8
	bool sortRunningApps;
	bool superExpando;			// version 9
	bool expandNewTeams;
};

// the following structures are defined to compute
// valid sizes for "struct desk_settings"

const uint32 kValidSettingsSize1 = 5 * sizeof(bool) + sizeof(uint32) + sizeof(float);
const uint32 kValidSettingsSize2 = sizeof(BPoint) + kValidSettingsSize1;
const uint32 kValidSettingsSize3 = 2 * sizeof(int32) + kValidSettingsSize2;
const uint32 kValidSettingsSize4 = 2 * sizeof(bool) + kValidSettingsSize3;
const uint32 kValidSettingsSize5 = sizeof(int32) + kValidSettingsSize4;
const uint32 kValidSettingsSize6 = 2 * sizeof(bool) + kValidSettingsSize5;
const uint32 kValidSettingsSize7 = sizeof(bool) + kValidSettingsSize6;
const uint32 kValidSettingsSize8 = 2 * sizeof(bool) + kValidSettingsSize7;
const uint32 kValidSettingsSize9 = 2 * sizeof(bool) + kValidSettingsSize8;

class TBarView;
class BFile;

namespace BPrivate {
	class TFavoritesConfigWindow;
}

using namespace BPrivate;

class TBarApp : public BApplication {
	public:
		TBarApp();
		virtual ~TBarApp();

		virtual	bool QuitRequested();
		virtual void MessageReceived(BMessage *);

		desk_settings *Settings()
			{ return &fSettings; }
		TBarView *BarView() const
			{ return fBarWindow->BarView(); }
		TBarWindow *BarWindow() const
			{ return fBarWindow; }

		static void Subscribe(const BMessenger &subscriber, BList *);
		static void Unsubscribe(const BMessenger &subscriber);

	private:
		void AddTeam(team_id team, uint32 flags, const char	*sig, entry_ref	*);
		void RemoveTeam(team_id);

		void InitSettings();
		void SaveSettings();

		void ShowConfigWindow();

		TBarWindow *fBarWindow;
		BMessenger fSwitcherMessenger;
		BMessenger fStatusViewMessenger;
		BFile *fSettingsFile;
		desk_settings fSettings;

		TFavoritesConfigWindow *fConfigWindow;

		static BLocker sSubscriberLock;
		static BList sBarTeamInfoList;
		static BList sSubscribers;
};

#endif
