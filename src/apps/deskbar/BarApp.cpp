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


#include "BarApp.h"

#include <locale.h>
#include <strings.h>

#include <AppFileInfo.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Mime.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>

#include <DeskbarPrivate.h>
#include <RosterPrivate.h>
#include "tracker_private.h"

#include "BarView.h"
#include "BarWindow.h"
#include "DeskbarUtils.h"
#include "FSUtils.h"
#include "PreferencesWindow.h"
#include "ResourceSet.h"
#include "StatusView.h"
#include "Switcher.h"
#include "Utilities.h"

#include "icons.h"


BLocker TBarApp::sSubscriberLock;
BList TBarApp::sBarTeamInfoList;
BList TBarApp::sSubscribers;


const uint32 kShowDeskbarMenu		= 'BeMn';
const uint32 kShowTeamMenu			= 'TmMn';

static const color_space kIconColorSpace = B_RGBA32;


int
main()
{
	setlocale(LC_ALL, "");
	TBarApp app;
	app.Run();

	return B_OK;
}


TBarApp::TBarApp()
	:
	BServer(kDeskbarSignature, true, NULL),
	fSettingsFile(NULL),
	fClockSettingsFile(NULL),
	fPreferencesWindow(NULL)
{
	InitSettings();
	InitIconPreloader();

	fBarWindow = new TBarWindow();
	fBarView = fBarWindow->BarView();

	be_roster->StartWatching(this);

	gLocalizedNamePreferred
		= BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	sBarTeamInfoList.MakeEmpty();

	BList teamList;
	int32 numTeams;
	be_roster->GetAppList(&teamList);
	numTeams = teamList.CountItems();
	for (int32 i = 0; i < numTeams; i++) {
		app_info appInfo;
		team_id tID = (addr_t)teamList.ItemAt(i);
		if (be_roster->GetRunningAppInfo(tID, &appInfo) == B_OK) {
			AddTeam(appInfo.team, appInfo.flags, appInfo.signature,
				&appInfo.ref);
		}
	}

	sSubscribers.MakeEmpty();
	fSwitcherMessenger = BMessenger(new TSwitchManager(fSettings.switcherLoc));
	fBarWindow->Show();

	fBarWindow->Lock();
	fBarView->UpdatePlacement();
	fBarWindow->Unlock();

	// this messenger now targets the barview instead of the
	// statusview so that all additions to the tray
	// follow the same path
	fStatusViewMessenger = BMessenger(fBarWindow->FindView("BarView"));
}


TBarApp::~TBarApp()
{
	be_roster->StopWatching(this);

	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		delete barInfo;
	}

	int32 subsCount = sSubscribers.CountItems();
	for (int32 i = 0; i < subsCount; i++) {
		BMessenger* messenger
			= static_cast<BMessenger*>(sSubscribers.ItemAt(i));
		delete messenger;
	}

	SaveSettings();

	delete fSettingsFile;
	delete fClockSettingsFile;
}


bool
TBarApp::QuitRequested()
{
	// don't allow the user to quit
	if (CurrentMessage() && CurrentMessage()->FindBool("shortcut")) {
		// but close the preferences window
		QuitPreferencesWindow();
		return false;
	}

	// system quitting, call inherited to notify all loopers
	fBarWindow->SaveSettings();
	BApplication::QuitRequested();
	return true;
}


void
TBarApp::SaveSettings()
{
	if (fSettingsFile->InitCheck() == B_OK) {
		fSettingsFile->Seek(0, SEEK_SET);
		BMessage prefs;
		prefs.AddBool("vertical", fSettings.vertical);
		prefs.AddBool("left", fSettings.left);
		prefs.AddBool("top", fSettings.top);
		prefs.AddInt32("state", fSettings.state);
		prefs.AddFloat("width", fSettings.width);
		prefs.AddPoint("switcherLoc", fSettings.switcherLoc);
		prefs.AddBool("showClock", fSettings.showClock);
		// applications
		prefs.AddBool("trackerAlwaysFirst", fSettings.trackerAlwaysFirst);
		prefs.AddBool("sortRunningApps", fSettings.sortRunningApps);
		prefs.AddBool("superExpando", fSettings.superExpando);
		prefs.AddBool("expandNewTeams", fSettings.expandNewTeams);
		prefs.AddBool("hideLabels", fSettings.hideLabels);
		prefs.AddInt32("iconSize", fSettings.iconSize);
		// recent items
		prefs.AddBool("recentDocsEnabled", fSettings.recentDocsEnabled);
		prefs.AddBool("recentFoldersEnabled", fSettings.recentFoldersEnabled);
		prefs.AddBool("recentAppsEnabled", fSettings.recentAppsEnabled);
		prefs.AddInt32("recentDocsCount", fSettings.recentDocsCount);
		prefs.AddInt32("recentFoldersCount", fSettings.recentFoldersCount);
		prefs.AddInt32("recentAppsCount", fSettings.recentAppsCount);
		// window
		prefs.AddBool("alwaysOnTop", fSettings.alwaysOnTop);
		prefs.AddBool("autoRaise", fSettings.autoRaise);
		prefs.AddBool("autoHide", fSettings.autoHide);

		prefs.Flatten(fSettingsFile);
	}

	if (fClockSettingsFile->InitCheck() == B_OK) {
		fClockSettingsFile->Seek(0, SEEK_SET);
		BMessage prefs;
		prefs.AddBool("showSeconds", fClockSettings.showSeconds);
		prefs.AddBool("showDayOfWeek", fClockSettings.showDayOfWeek);
		prefs.AddBool("showTimeZone", fClockSettings.showTimeZone);

		prefs.Flatten(fClockSettingsFile);
	}
}


void
TBarApp::InitSettings()
{
	desk_settings settings;
	settings.vertical = fDefaultSettings.vertical = true;
	settings.left = fDefaultSettings.left = false;
	settings.top = fDefaultSettings.top = true;
	settings.state = fDefaultSettings.state = kExpandoState;
	settings.width = fDefaultSettings.width = gMinimumWindowWidth;
	settings.switcherLoc = fDefaultSettings.switcherLoc = BPoint(5000, 5000);
	settings.showClock = fDefaultSettings.showClock = true;
	// applications
	settings.trackerAlwaysFirst = fDefaultSettings.trackerAlwaysFirst = false;
	settings.sortRunningApps = fDefaultSettings.sortRunningApps = false;
	settings.superExpando = fDefaultSettings.superExpando = false;
	settings.expandNewTeams = fDefaultSettings.expandNewTeams = false;
	settings.hideLabels = fDefaultSettings.hideLabels = false;
	settings.iconSize = fDefaultSettings.iconSize = kMinimumIconSize;
	// recent items
	settings.recentDocsEnabled = fDefaultSettings.recentDocsEnabled = true;
	settings.recentFoldersEnabled
		= fDefaultSettings.recentFoldersEnabled = true;
	settings.recentAppsEnabled = fDefaultSettings.recentAppsEnabled = true;
	settings.recentDocsCount = fDefaultSettings.recentDocsCount = 10;
	settings.recentFoldersCount = fDefaultSettings.recentFoldersCount = 10;
	settings.recentAppsCount = fDefaultSettings.recentAppsCount = 10;
	// window
	settings.alwaysOnTop = fDefaultSettings.alwaysOnTop = false;
	settings.autoRaise = fDefaultSettings.autoRaise = false;
	settings.autoHide = fDefaultSettings.autoHide = false;

	clock_settings clock;
	clock.showSeconds = false;
	clock.showDayOfWeek = false;
	clock.showTimeZone = false;

	BPath dirPath;
	const char* settingsFileName = "settings";
	const char* clockSettingsFileName = "clock_settings";

	find_directory(B_USER_DESKBAR_DIRECTORY, &dirPath, true);
		// just make it

	if (GetDeskbarSettingsDirectory(dirPath, true) == B_OK) {
		BPath filePath = dirPath;
		filePath.Append(settingsFileName);
		fSettingsFile = new BFile(filePath.Path(), O_RDWR);
		if (fSettingsFile->InitCheck() != B_OK) {
			BDirectory theDir(dirPath.Path());
			if (theDir.InitCheck() == B_OK)
				theDir.CreateFile(settingsFileName, fSettingsFile);
		}

		BMessage prefs;
		if (fSettingsFile->InitCheck() == B_OK
			&& prefs.Unflatten(fSettingsFile) == B_OK) {
			settings.vertical = prefs.GetBool("vertical",
				fDefaultSettings.vertical);
			settings.left = prefs.GetBool("left",
				fDefaultSettings.left);
			settings.top = prefs.GetBool("top",
				fDefaultSettings.top);
			settings.state = prefs.GetInt32("state",
				fDefaultSettings.state);
			settings.width = prefs.GetFloat("width",
				fDefaultSettings.width);
			settings.switcherLoc = prefs.GetPoint("switcherLoc",
				fDefaultSettings.switcherLoc);
			settings.showClock = prefs.GetBool("showClock",
				fDefaultSettings.showClock);
			// applications
			settings.trackerAlwaysFirst = prefs.GetBool("trackerAlwaysFirst",
				fDefaultSettings.trackerAlwaysFirst);
			settings.sortRunningApps = prefs.GetBool("sortRunningApps",
				fDefaultSettings.sortRunningApps);
			settings.superExpando = prefs.GetBool("superExpando",
				fDefaultSettings.superExpando);
			settings.expandNewTeams = prefs.GetBool("expandNewTeams",
				fDefaultSettings.expandNewTeams);
			settings.hideLabels = prefs.GetBool("hideLabels",
				fDefaultSettings.hideLabels);
			settings.iconSize = prefs.GetInt32("iconSize",
				fDefaultSettings.iconSize);
			// recent items
			settings.recentDocsEnabled = prefs.GetBool("recentDocsEnabled",
				fDefaultSettings.recentDocsEnabled);
			settings.recentFoldersEnabled
				= prefs.GetBool("recentFoldersEnabled",
					fDefaultSettings.recentFoldersEnabled);
			settings.recentAppsEnabled = prefs.GetBool("recentAppsEnabled",
				fDefaultSettings.recentAppsEnabled);
			settings.recentDocsCount = prefs.GetInt32("recentDocsCount",
				fDefaultSettings.recentDocsCount);
			settings.recentFoldersCount = prefs.GetInt32("recentFoldersCount",
				fDefaultSettings.recentFoldersCount);
			settings.recentAppsCount = prefs.GetInt32("recentAppsCount",
				fDefaultSettings.recentAppsCount);
			// window
			settings.alwaysOnTop = prefs.GetBool("alwaysOnTop",
				fDefaultSettings.alwaysOnTop);
			settings.autoRaise = prefs.GetBool("autoRaise",
				fDefaultSettings.autoRaise);
			settings.autoHide = prefs.GetBool("autoHide",
				fDefaultSettings.autoHide);
		}

		// constrain width setting within limits
		if (settings.width < gMinimumWindowWidth)
			settings.width = gMinimumWindowWidth;
		else if (settings.width > gMaximumWindowWidth)
			settings.width = gMaximumWindowWidth;

		filePath = dirPath;
		filePath.Append(clockSettingsFileName);
		fClockSettingsFile = new BFile(filePath.Path(), O_RDWR);
		if (fClockSettingsFile->InitCheck() != B_OK) {
			BDirectory theDir(dirPath.Path());
			if (theDir.InitCheck() == B_OK)
				theDir.CreateFile(clockSettingsFileName, fClockSettingsFile);
		}

		if (fClockSettingsFile->InitCheck() == B_OK
			&& prefs.Unflatten(fClockSettingsFile) == B_OK) {
			clock.showSeconds = prefs.GetBool("showSeconds", false);
			clock.showDayOfWeek = prefs.GetBool("showDayOfWeek", false);
			clock.showTimeZone = prefs.GetBool("showTimeZone", false);
		}
	}

	fSettings = settings;
	fClockSettings = clock;
}


void
TBarApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// BDeskbar originating messages we can handle
		case kMsgIsAlwaysOnTop:
		{
			BMessage reply('rply');
			reply.AddBool("always on top", fSettings.alwaysOnTop);
			message->SendReply(&reply);
			break;
		}
		case kMsgIsAutoRaise:
		{
			BMessage reply('rply');
			reply.AddBool("auto raise", fSettings.autoRaise);
			message->SendReply(&reply);
			break;
		}
		case kMsgIsAutoHide:
		{
			BMessage reply('rply');
			reply.AddBool("auto hide", fSettings.autoHide);
			message->SendReply(&reply);
			break;
		}

		// pass rest of BDeskbar originating messages onto the window
		// (except for setters handled below)
		case kMsgLocation:
		case kMsgSetLocation:
		case kMsgIsExpanded:
		case kMsgExpand:
		case kMsgGetItemInfo:
		case kMsgHasItem:
		case kMsgCountItems:
		case kMsgMaxItemSize:
		case kMsgAddView:
		case kMsgRemoveItem:
		case kMsgAddAddOn:
			fBarWindow->PostMessage(message);
			break;

		case kConfigShow:
			ShowPreferencesWindow();
			break;

		case kConfigQuit:
			QuitPreferencesWindow();
			break;

		case kStateChanged:
			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kStateChanged);
			break;

		case kShowDeskbarMenu:
			if (fBarWindow->Lock()) {
				fBarWindow->ShowDeskbarMenu();
				fBarWindow->Unlock();
			}
			break;

		case kShowTeamMenu:
			if (fBarWindow->Lock()) {
				fBarWindow->ShowTeamMenu();
				fBarWindow->Unlock();
			}
			break;

		case kUpdateRecentCounts:
			int32 count;
			bool enabled;

			if (message->FindInt32("applications", &count) == B_OK)
				fSettings.recentAppsCount = count;
			if (message->FindBool("applicationsEnabled", &enabled) == B_OK)
				fSettings.recentAppsEnabled = enabled && count > 0;

			if (message->FindInt32("folders", &count) == B_OK)
				fSettings.recentFoldersCount = count;
			if (message->FindBool("foldersEnabled", &enabled) == B_OK)
				fSettings.recentFoldersEnabled = enabled && count > 0;

			if (message->FindInt32("documents", &count) == B_OK)
				fSettings.recentDocsCount = count;
			if (message->FindBool("documentsEnabled", &enabled) == B_OK)
				fSettings.recentDocsEnabled = enabled && count > 0;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);
			break;

		case B_SOME_APP_LAUNCHED:
		{
			team_id team = -1;
			message->FindInt32("be:team", &team);

			uint32 flags = 0;
			message->FindInt32("be:flags", (int32*)&flags);

			const char* signature = NULL;
			message->FindString("be:signature", &signature);

			entry_ref ref;
			message->FindRef("be:ref", &ref);

			AddTeam(team, flags, signature, &ref);
			break;
		}

		case B_SOME_APP_QUIT:
		{
			team_id team = -1;
			message->FindInt32("be:team", &team);
			RemoveTeam(team);
			break;
		}

		case B_ARCHIVED_OBJECT:
			// TODO: what's this???
			message->AddString("special", "Alex Osadzinski");
			fStatusViewMessenger.SendMessage(message);
			break;

		case kToggleDraggers:
			if (BDragger::AreDraggersDrawn())
				BDragger::HideAllDraggers();
			else
				BDragger::ShowAllDraggers();
			break;

		case kMsgAlwaysOnTop: // from BDeskbar
		case kAlwaysTop:
			fSettings.alwaysOnTop = !fSettings.alwaysOnTop;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			fBarWindow->SetFeel(fSettings.alwaysOnTop
				? B_FLOATING_ALL_WINDOW_FEEL
				: B_NORMAL_WINDOW_FEEL);
			break;

		case kMsgAutoRaise: // from BDeskbar
		case kAutoRaise:
			fSettings.autoRaise = fSettings.alwaysOnTop ? false
				: !fSettings.autoRaise;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);
			break;

		case kMsgAutoHide: // from BDeskbar
		case kAutoHide:
			fSettings.autoHide = !fSettings.autoHide;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			fBarWindow->Lock();
			fBarView->HideDeskbar(fSettings.autoHide);
			fBarWindow->Unlock();
			break;

		case kTrackerFirst:
			fSettings.trackerAlwaysFirst = !fSettings.trackerAlwaysFirst;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kSortRunningApps:
			fSettings.sortRunningApps = !fSettings.sortRunningApps;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kUnsubscribe:
		{
			BMessenger messenger;
			if (message->FindMessenger("messenger", &messenger) == B_OK)
				Unsubscribe(messenger);
			break;
		}

		case kSuperExpando:
			fSettings.superExpando = !fSettings.superExpando;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kExpandNewTeams:
			fSettings.expandNewTeams = !fSettings.expandNewTeams;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kHideLabels:
			fSettings.hideLabels = !fSettings.hideLabels;

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kResizeTeamIcons:
		{
			int32 oldIconSize = fSettings.iconSize;
			int32 iconSize;
			if (message->FindInt32("be:value", &iconSize) != B_OK)
				break;

			fSettings.iconSize = iconSize * kIconSizeInterval;

			// pin icon size between min and max values
			if (fSettings.iconSize < kMinimumIconSize)
				fSettings.iconSize = kMinimumIconSize;
			else if (fSettings.iconSize > kMaximumIconSize)
				fSettings.iconSize = kMaximumIconSize;

			// don't resize if icon size hasn't changed
			if (fSettings.iconSize == oldIconSize)
				break;

			ResizeTeamIcons();

			if (fPreferencesWindow != NULL)
				fPreferencesWindow->PostMessage(kUpdatePreferences);

			// if mini mode we don't need to update the view
			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			if (!fBarView->Vertical()) {
				// Must also resize the Deskbar menu and replicant tray in
				// horizontal mode
				fBarView->PlaceDeskbarMenu();
				fBarView->PlaceTray(false, false);
			}
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;
		}

		case 'TASK':
			fSwitcherMessenger.SendMessage(message);
			break;

		case kSuspendSystem:
			// TODO: Call BRoster?
			break;

		case kRebootSystem:
		case kShutdownSystem:
		{
			bool reboot = (message->what == kRebootSystem);
			bool confirm;
			message->FindBool("confirm", &confirm);

			BRoster roster;
			BRoster::Private rosterPrivate(roster);
			status_t error = rosterPrivate.ShutDown(reboot, confirm, false);
			if (error != B_OK)
				fprintf(stderr, "Shutdown failed: %s\n", strerror(error));

			break;
		}

		case kShowSplash:
			run_be_about();
			break;

		case B_LOCALE_CHANGED:
		{
			BLocaleRoster::Default()->Refresh();

			bool localize;
			if (message->FindBool("filesys", &localize) == B_OK)
				gLocalizedNamePreferred = localize;
		}
		// fall-through

		case kRealignReplicants:
		case kShowHideTime:
		case kShowSeconds:
		case kShowDayOfWeek:
		case kShowTimeZone:
		case kGetClockSettings:
			fStatusViewMessenger.SendMessage(message);
				// Notify the replicant tray (through BarView) that the time
				// interval has changed and it should update the time view
				// and reflow the tray icons.
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
TBarApp::RefsReceived(BMessage* refs)
{
	entry_ref ref;
	for (int32 i = 0; refs->FindRef("refs", i, &ref) == B_OK; i++) {
		BMessage refsReceived(B_REFS_RECEIVED);
		refsReceived.AddRef("refs", &ref);

		BEntry entry(&ref);
		if (!entry.IsDirectory())
			TrackerLaunch(&refsReceived, false);
	}
}


void
TBarApp::Subscribe(const BMessenger &subscriber, BList* list)
{
	// called when ExpandoMenuBar, TeamMenu or Switcher are built/rebuilt
	list->MakeEmpty();

	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	int32 numTeams = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < numTeams; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		BarTeamInfo* newBarInfo = new (std::nothrow) BarTeamInfo(*barInfo);
		if (newBarInfo != NULL)
			list->AddItem(newBarInfo);
	}

	int32 subsCount = sSubscribers.CountItems();
	for (int32 i = 0; i < subsCount; i++) {
		BMessenger* messenger = (BMessenger*)sSubscribers.ItemAt(i);
		if (*messenger == subscriber)
			return;
	}

	sSubscribers.AddItem(new BMessenger(subscriber));
}


void
TBarApp::Unsubscribe(const BMessenger &subscriber)
{
	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	int32 count = sSubscribers.CountItems();
	for (int32 i = 0; i < count; i++) {
		BMessenger* messenger = (BMessenger*)sSubscribers.ItemAt(i);
		if (*messenger == subscriber) {
			sSubscribers.RemoveItem(i);
			delete messenger;
			break;
		}
	}
}


void
TBarApp::AddTeam(team_id team, uint32 flags, const char* sig, entry_ref* ref)
{
	if ((flags & B_BACKGROUND_APP) != 0
		|| strcasecmp(sig, kDeskbarSignature) == 0) {
		// don't add if a background app or Deskbar itself
		return;
	}

	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	// have we already seen this team, is this another instance of
	// a known app?
	BarTeamInfo* multiLaunchTeam = NULL;
	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		if (barInfo->teams->HasItem((void*)(addr_t)team))
			return;
		if (strcasecmp(barInfo->sig, sig) == 0)
			multiLaunchTeam = barInfo;
	}

	if (multiLaunchTeam != NULL) {
		multiLaunchTeam->teams->AddItem((void*)(addr_t)team);

		int32 subsCount = sSubscribers.CountItems();
		if (subsCount > 0) {
			BMessage message(kAddTeam);
			message.AddInt32("team", team);
			message.AddString("sig", multiLaunchTeam->sig);

			for (int32 i = 0; i < subsCount; i++)
				((BMessenger*)sSubscribers.ItemAt(i))->SendMessage(&message);
		}
		return;
	}

	BFile file(ref, B_READ_ONLY);
	BAppFileInfo appMime(&file);

	BString name;
	if (!gLocalizedNamePreferred
		|| BLocaleRoster::Default()->GetLocalizedFileName(name, *ref)
			!= B_OK) {
		name = ref->name;
	}

	BarTeamInfo* barInfo = new BarTeamInfo(new BList(), flags, strdup(sig),
		NULL, strdup(name.String()));
	FetchAppIcon(barInfo);
	barInfo->teams->AddItem((void*)(addr_t)team);
	sBarTeamInfoList.AddItem(barInfo);

	int32 subsCount = sSubscribers.CountItems();
	if (subsCount > 0) {
		for (int32 i = 0; i < subsCount; i++) {
			BMessenger* messenger = (BMessenger*)sSubscribers.ItemAt(i);
			BMessage message(B_SOME_APP_LAUNCHED);

			BList* tList = new BList(*(barInfo->teams));
			message.AddPointer("teams", tList);

			BBitmap* icon = new BBitmap(barInfo->icon);
			ASSERT(icon);

			message.AddPointer("icon", icon);

			message.AddInt32("flags", static_cast<int32>(barInfo->flags));
			message.AddString("name", barInfo->name);
			message.AddString("sig", barInfo->sig);

			messenger->SendMessage(&message);
		}
	}
}


void
TBarApp::RemoveTeam(team_id team)
{
	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		if (barInfo->teams->HasItem((void*)(addr_t)team)) {
			int32 subsCount = sSubscribers.CountItems();
			if (subsCount > 0) {
				BMessage message((barInfo->teams->CountItems() == 1)
					? B_SOME_APP_QUIT : kRemoveTeam);

				message.AddInt32("team", team);
				for (int32 i = 0; i < subsCount; i++) {
					BMessenger* messenger = (BMessenger*)sSubscribers.ItemAt(i);
					messenger->SendMessage(&message);
				}
			}

			barInfo->teams->RemoveItem((void*)(addr_t)team);
			if (barInfo->teams->CountItems() < 1) {
				delete (BarTeamInfo*)sBarTeamInfoList.RemoveItem(i);
				return;
			}
		}
	}
}


void
TBarApp::ResizeTeamIcons()
{
	for (int32 i = sBarTeamInfoList.CountItems() - 1; i >= 0; i--) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		if ((barInfo->flags & B_BACKGROUND_APP) == 0
			&& strcasecmp(barInfo->sig, kDeskbarSignature) != 0) {
			FetchAppIcon(barInfo);
		}
	}
}


int32
TBarApp::IconSize()
{
	return fSettings.iconSize;
}


void
TBarApp::ShowPreferencesWindow()
{
	if (fPreferencesWindow == NULL) {
		fPreferencesWindow = new PreferencesWindow(BRect(100, 100, 320, 240));
		fPreferencesWindow->Show();
	} else if (fPreferencesWindow->Lock()) {
		if (fPreferencesWindow->IsHidden())
			fPreferencesWindow->Show();
		else
			fPreferencesWindow->Activate();

		fPreferencesWindow->Unlock();
	}
}


void
TBarApp::QuitPreferencesWindow()
{
	if (fPreferencesWindow == NULL)
		return;

	if (fPreferencesWindow->Lock()) {
		fPreferencesWindow->Quit();
			// Quit() destroys the window so don't unlock
		fPreferencesWindow = NULL;
	}
}


void
TBarApp::FetchAppIcon(BarTeamInfo* barInfo)
{
	int32 width = IconSize();
	int32 index = (width - kMinimumIconSize) / kIconSizeInterval;

	// first look in the icon cache
	barInfo->icon = barInfo->iconCache[index];
	if (barInfo->icon != NULL)
		return;

	// icon wasn't in cache, get it from be_roster and cache it
	app_info appInfo;
	icon_size size = width >= 31 ? B_LARGE_ICON : B_MINI_ICON;
	BBitmap* icon = new BBitmap(IconRect(), kIconColorSpace);
	if (be_roster->GetAppInfo(barInfo->sig, &appInfo) == B_OK) {
		// fetch the app icon
		BFile file(&appInfo.ref, B_READ_ONLY);
		BAppFileInfo appMime(&file);
		if (appMime.GetIcon(icon, size) == B_OK) {
			delete barInfo->iconCache[index];
			barInfo->iconCache[index] = barInfo->icon = icon;
			return;
		}
	}

	// couldn't find the app icon
	// fetch the generic 3 boxes icon and cache it
	BMimeType defaultAppMime;
	defaultAppMime.SetTo(B_APP_MIME_TYPE);
	if (defaultAppMime.GetIcon(icon, size) == B_OK) {
		delete barInfo->iconCache[index];
		barInfo->iconCache[index] = barInfo->icon = icon;
		return;
	}

	// couldn't find generic 3 boxes icon
	// fill with transparent
	uint8* iconBits = (uint8*)icon->Bits();
	if (icon->ColorSpace() == B_RGBA32) {
		int32 i = 0;
		while (i < icon->BitsLength()) {
			iconBits[i++] = B_TRANSPARENT_32_BIT.red;
			iconBits[i++] = B_TRANSPARENT_32_BIT.green;
			iconBits[i++] = B_TRANSPARENT_32_BIT.blue;
			iconBits[i++] = B_TRANSPARENT_32_BIT.alpha;
		}
	} else {
		// Assume B_CMAP8
		for (int32 i = 0; i < icon->BitsLength(); i++)
			iconBits[i] = B_TRANSPARENT_MAGIC_CMAP8;
	}

	delete barInfo->iconCache[index];
	barInfo->iconCache[index] = barInfo->icon = icon;
}


BRect
TBarApp::IconRect()
{
	int32 iconSize = IconSize();
	return BRect(0, 0, iconSize - 1, iconSize - 1);
}


//	#pragma mark -


BarTeamInfo::BarTeamInfo(BList* teams, uint32 flags, char* sig, BBitmap* icon,
	char* name)
	:
	teams(teams),
	flags(flags),
	sig(sig),
	icon(icon),
	name(name)
{
	_Init();
}


BarTeamInfo::BarTeamInfo(const BarTeamInfo &info)
	:
	teams(new BList(*info.teams)),
	flags(info.flags),
	sig(strdup(info.sig)),
	icon(new BBitmap(*info.icon)),
	name(strdup(info.name))
{
	_Init();
}


BarTeamInfo::~BarTeamInfo()
{
	delete teams;
	free(sig);
	free(name);
	for (int32 i = 0; i < kIconCacheCount; i++)
		delete iconCache[i];
}


void
BarTeamInfo::_Init()
{
	for (int32 i = 0; i < kIconCacheCount; i++)
		iconCache[i] = NULL;
}
