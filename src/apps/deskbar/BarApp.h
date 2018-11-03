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


#include <Server.h>

#include "BarSettings.h"


/* ------------------------------------ */
// Private app_server defines that I need to use

#define _DESKTOP_W_TYPE_ 1024
#define _FLOATER_W_TYPE_ 4
#define _STD_W_TYPE_ 0


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

// from roster_private.h
const uint32 kShutdownSystem = 301;
const uint32 kRebootSystem = 302;
const uint32 kSuspendSystem = 304;

// icon size constants
const int32 kMinimumIconSize = 16;
const int32 kMaximumIconSize = 96;
const int32 kIconSizeInterval = 8;
const int32 kIconCacheCount = (kMaximumIconSize - kMinimumIconSize)
	/ kIconSizeInterval + 1;

// update preferences message constant
const uint32 kUpdatePreferences = 'Pref';

// realign replicants message constant
const uint32 kRealignReplicants = 'Algn';

/* --------------------------------------------- */

class BBitmap;
class BFile;
class BList;
class PreferencesWindow;
class TBarView;
class TBarWindow;

class BarTeamInfo {
public:
									BarTeamInfo(BList* teams, uint32 flags,
										char* sig, BBitmap* icon, char* name);
									BarTeamInfo(const BarTeamInfo &info);
									~BarTeamInfo();

private:
			void					_Init();

public:
			BList*					teams;
			uint32					flags;
			char*					sig;
			BBitmap*				icon;
			char*					name;
			BBitmap*				iconCache[kIconCacheCount];
};

class TBarApp : public BServer {
public:
									TBarApp();
	virtual							~TBarApp();

	virtual	bool					QuitRequested();
	virtual	void					MessageReceived(BMessage* message);
	virtual	void					RefsReceived(BMessage* refs);

			desk_settings*			Settings() { return &fSettings; }
			desk_settings*			DefaultSettings()
										{ return &fDefaultSettings; }
			clock_settings*			ClockSettings() { return &fClockSettings; }

			TBarView*				BarView() const { return fBarView; }
			TBarWindow*				BarWindow() const { return fBarWindow; }

	static	void					Subscribe(const BMessenger &subscriber,
										BList*);
	static	void					Unsubscribe(const BMessenger &subscriber);

			int32					IconSize();

private:
			void					AddTeam(team_id team, uint32 flags,
										const char* sig, entry_ref*);
			void					RemoveTeam(team_id);

			void					InitSettings();
			void					SaveSettings();

			void					ShowPreferencesWindow();
			void					QuitPreferencesWindow();

			void					ResizeTeamIcons();
			void					FetchAppIcon(BarTeamInfo* barInfo);

			BRect					IconRect();

private:
			TBarWindow*				fBarWindow;
			TBarView*				fBarView;
			BMessenger				fSwitcherMessenger;
			BMessenger				fStatusViewMessenger;
			BFile*					fSettingsFile;
			BFile*					fClockSettingsFile;
			desk_settings			fSettings;
			desk_settings			fDefaultSettings;
			clock_settings			fClockSettings;
			PreferencesWindow*		fPreferencesWindow;

	static	BLocker					sSubscriberLock;
	static	BList					sBarTeamInfoList;
	static	BList					sSubscribers;
};


#endif	// BAR_APP_H
