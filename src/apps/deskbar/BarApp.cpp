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
#include <stdlib.h>
#include <string.h>

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
#include <Path.h>
#include <Roster.h>
#include <RosterPrivate.h>

#include "icons.h"
#include "tracker_private.h"
#include "BarView.h"
#include "BarWindow.h"
#include "PreferencesWindow.h"
#include "DeskbarUtils.h"
#include "FSUtils.h"
#include "PublicCommands.h"
#include "ResourceSet.h"
#include "StatusView.h"
#include "Switcher.h"
#include "Utilities.h"


BLocker TBarApp::sSubscriberLock;
BList TBarApp::sBarTeamInfoList;
BList TBarApp::sSubscribers;


const uint32 kShowDeskbarMenu = 'BeMn';
const uint32 kShowTeamMenu = 'TmMn';


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
	:	BApplication(kDeskbarSignature),
		fSettingsFile(NULL),
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
		team_id tID = (team_id)teamList.ItemAt(i);
		if (be_roster->GetRunningAppInfo(tID, &appInfo) == B_OK) {
			AddTeam(appInfo.team, appInfo.flags, appInfo.signature,
				&appInfo.ref);
		}
	}

	sSubscribers.MakeEmpty();

	fSwitcherMessenger = BMessenger(new TSwitchManager(fSettings.switcherLoc));

	fBarWindow->Show();

	// Call UpdatePlacement() after the window is shown because expanded apps
	// need to resize the window.
	if (fBarWindow->Lock()) {
		fBarView->UpdatePlacement();
		fBarWindow->Unlock();
	}

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
}


bool
TBarApp::QuitRequested()
{
	// don't allow user quitting
	if (CurrentMessage() && CurrentMessage()->FindBool("shortcut")) {
		// but allow quitting to hide fPreferencesWindow
		int32 index = 0;
		BWindow* window = NULL;
		while ((window = WindowAt(index++)) != NULL) {
			if (window == fPreferencesWindow) {
				if (fPreferencesWindow->Lock()) {
					if (fPreferencesWindow->IsActive())
						fPreferencesWindow->PostMessage(B_QUIT_REQUESTED);
					fPreferencesWindow->Unlock();
				}
				break;
			}
		}
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
		BMessage storedSettings;
		storedSettings.AddBool("vertical", fSettings.vertical);
		storedSettings.AddBool("left", fSettings.left);
		storedSettings.AddBool("top", fSettings.top);
		storedSettings.AddInt32("state", fSettings.state);
		storedSettings.AddFloat("width", fSettings.width);

		storedSettings.AddBool("showTime", fSettings.showTime);
		storedSettings.AddBool("showSeconds", fSettings.showSeconds);
		storedSettings.AddBool("showDayOfWeek", fSettings.showDayOfWeek);
		storedSettings.AddBool("showTimeZone", fSettings.showTimeZone);

		storedSettings.AddPoint("switcherLoc", fSettings.switcherLoc);
		storedSettings.AddInt32("recentAppsCount", fSettings.recentAppsCount);
		storedSettings.AddInt32("recentDocsCount", fSettings.recentDocsCount);
		storedSettings.AddBool("timeShowSeconds", fSettings.timeShowSeconds);
		storedSettings.AddInt32("recentFoldersCount",
			fSettings.recentFoldersCount);
		storedSettings.AddBool("alwaysOnTop", fSettings.alwaysOnTop);
		storedSettings.AddBool("timeFullDate", fSettings.timeFullDate);
		storedSettings.AddBool("trackerAlwaysFirst",
			fSettings.trackerAlwaysFirst);
		storedSettings.AddBool("sortRunningApps", fSettings.sortRunningApps);
		storedSettings.AddBool("superExpando", fSettings.superExpando);
		storedSettings.AddBool("expandNewTeams", fSettings.expandNewTeams);
		storedSettings.AddBool("hideLabels", fSettings.hideLabels);
		storedSettings.AddInt32("iconSize", fSettings.iconSize);
		storedSettings.AddBool("autoRaise", fSettings.autoRaise);
		storedSettings.AddBool("autoHide", fSettings.autoHide);
		storedSettings.AddBool("recentAppsEnabled",
			fSettings.recentAppsEnabled);
		storedSettings.AddBool("recentDocsEnabled",
			fSettings.recentDocsEnabled);
		storedSettings.AddBool("recentFoldersEnabled",
			fSettings.recentFoldersEnabled);

		storedSettings.Flatten(fSettingsFile);
	}
}


void
TBarApp::InitSettings()
{
	desk_settings settings;
	settings.vertical = true;
	settings.left = false;
	settings.top = true;
	settings.showTime = true;
	settings.showSeconds = false;
	settings.showDayOfWeek = false;
	settings.showTimeZone = false;
	settings.state = kExpandoState;
	settings.width = 0;
	settings.switcherLoc = BPoint(5000, 5000);
	settings.recentAppsCount = 10;
	settings.recentDocsCount = 10;
	settings.timeShowSeconds = false;
	settings.recentFoldersCount = 10;
	settings.alwaysOnTop = false;
	settings.timeFullDate = false;
	settings.trackerAlwaysFirst = false;
	settings.sortRunningApps = false;
	settings.superExpando = false;
	settings.expandNewTeams = false;
	settings.hideLabels = false;
	settings.iconSize = kMinimumIconSize;
	settings.autoRaise = false;
	settings.autoHide = false;
	settings.recentAppsEnabled = true;
	settings.recentDocsEnabled = true;
	settings.recentFoldersEnabled = true;

	BPath dirPath;
	const char* settingsFileName = "Deskbar_settings";

	find_directory(B_USER_DESKBAR_DIRECTORY, &dirPath, true);
		// just make it

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &dirPath, true) == B_OK) {
		BPath filePath = dirPath;
		filePath.Append(settingsFileName);
		fSettingsFile = new BFile(filePath.Path(), O_RDWR);
		if (fSettingsFile->InitCheck() != B_OK) {
			BDirectory theDir(dirPath.Path());
			if (theDir.InitCheck() == B_OK)
				theDir.CreateFile(settingsFileName, fSettingsFile);
		}

		BMessage storedSettings;
		if (fSettingsFile->InitCheck() == B_OK
			&& storedSettings.Unflatten(fSettingsFile) == B_OK) {
			if (storedSettings.FindBool("vertical", &settings.vertical)
					!= B_OK) {
				settings.vertical = true;
			}
			if (storedSettings.FindBool("left", &settings.left) != B_OK)
				settings.left = false;
			if (storedSettings.FindBool("top", &settings.top) != B_OK)
				settings.top = true;
			if (storedSettings.FindInt32("state", (int32*)&settings.state)
					!= B_OK) {
				settings.state = kExpandoState;
			}
			if (storedSettings.FindFloat("width", &settings.width) != B_OK)
				settings.width = 0;
			if (storedSettings.FindBool("showTime", &settings.showTime)
					!= B_OK) {
				settings.showTime = true;
			}
			if (storedSettings.FindBool("showSeconds", &settings.showSeconds)
					!= B_OK) {
				settings.showSeconds = false;
			}
			if (storedSettings.FindBool("showDayOfWeek", &settings.showDayOfWeek)
					!= B_OK) {
				settings.showDayOfWeek = false;
			}
			if (storedSettings.FindBool("showTimeZone", &settings.showTimeZone)
					!= B_OK) {
				settings.showTimeZone = false;
			}
			if (storedSettings.FindPoint("switcherLoc", &settings.switcherLoc)
					!= B_OK) {
				settings.switcherLoc = BPoint(5000, 5000);
			}
			if (storedSettings.FindInt32("recentAppsCount",
					&settings.recentAppsCount) != B_OK) {
				settings.recentAppsCount = 10;
			}
			if (storedSettings.FindInt32("recentDocsCount",
					&settings.recentDocsCount) != B_OK) {
				settings.recentDocsCount = 10;
			}
			if (storedSettings.FindBool("timeShowSeconds",
					&settings.timeShowSeconds) != B_OK) {
				settings.timeShowSeconds = false;
			}
			if (storedSettings.FindInt32("recentFoldersCount",
					&settings.recentFoldersCount) != B_OK) {
				settings.recentFoldersCount = 10;
			}
			if (storedSettings.FindBool("alwaysOnTop", &settings.alwaysOnTop)
					!= B_OK) {
				settings.alwaysOnTop = false;
			}
			if (storedSettings.FindBool("timeFullDate", &settings.timeFullDate)
					!= B_OK) {
				settings.timeFullDate = false;
			}
			if (storedSettings.FindBool("trackerAlwaysFirst",
					&settings.trackerAlwaysFirst) != B_OK) {
				settings.trackerAlwaysFirst = false;
			}
			if (storedSettings.FindBool("sortRunningApps",
					&settings.sortRunningApps) != B_OK) {
				settings.sortRunningApps = false;
			}
			if (storedSettings.FindBool("superExpando", &settings.superExpando)
					!= B_OK) {
				settings.superExpando = false;
			}
			if (storedSettings.FindBool("expandNewTeams",
					&settings.expandNewTeams) != B_OK) {
				settings.expandNewTeams = false;
			}
			if (storedSettings.FindBool("hideLabels", &settings.hideLabels)
					!= B_OK) {
				settings.hideLabels = false;
			}
			if (storedSettings.FindInt32("iconSize", (int32*)&settings.iconSize)
					!= B_OK) {
				settings.iconSize = kMinimumIconSize;
			}
			if (storedSettings.FindBool("autoRaise", &settings.autoRaise)
					!= B_OK) {
				settings.autoRaise = false;
			}
			if (storedSettings.FindBool("autoHide", &settings.autoHide) != B_OK)
				settings.autoHide = false;
			if (storedSettings.FindBool("recentAppsEnabled",
					&settings.recentAppsEnabled) != B_OK) {
				settings.recentAppsEnabled = true;
			}
			if (storedSettings.FindBool("recentDocsEnabled",
					&settings.recentDocsEnabled) != B_OK) {
				settings.recentDocsEnabled = true;
			}
			if (storedSettings.FindBool("recentFoldersEnabled",
					&settings.recentFoldersEnabled) != B_OK) {
				settings.recentFoldersEnabled = true;
			}
		}
	}

	fSettings = settings;
}


void
TBarApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'gloc':
		case 'sloc':
		case 'gexp':
		case 'sexp':
		case 'info':
		case 'exst':
		case 'cwnt':
		case 'icon':
		case 'remv':
		case 'adon':
			// pass any BDeskbar originating messages on to the window
			fBarWindow->PostMessage(message);
			break;

		case kConfigShow:
			ShowPreferencesWindow();
			break;

		case kStateChanged:
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
			break;

		case kConfigClose:
			fPreferencesWindow = NULL;
			break;

		case B_SOME_APP_LAUNCHED:
		{
			team_id team = -1;
			message->FindInt32("be:team", &team);

			uint32 flags = 0;
			message->FindInt32("be:flags", (long*)&flags);

			const char* sig = NULL;
			message->FindString("be:signature", &sig);

			entry_ref ref;
			message->FindRef("be:ref", &ref);

			AddTeam(team, flags, sig, &ref);
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

		case kAlwaysTop:
			fSettings.alwaysOnTop = !fSettings.alwaysOnTop;
			fBarWindow->SetFeel(fSettings.alwaysOnTop ?
				B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
			fPreferencesWindow->PostMessage(kStateChanged);
			break;

		case kAutoRaise:
			fSettings.autoRaise = fSettings.alwaysOnTop ? false :
				!fSettings.autoRaise;

			fBarWindow->Lock();
			fBarView->UpdateEventMask();
			fBarWindow->Unlock();
			break;

		case kAutoHide:
			fSettings.autoHide = !fSettings.autoHide;

			fBarWindow->Lock();
			fBarView->UpdateEventMask();
			fBarView->HideDeskbar(fSettings.autoHide);
			fBarWindow->Unlock();
			break;

		case kTrackerFirst:
			fSettings.trackerAlwaysFirst = !fSettings.trackerAlwaysFirst;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kSortRunningApps:
			fSettings.sortRunningApps = !fSettings.sortRunningApps;

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

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kExpandNewTeams:
			fSettings.expandNewTeams = !fSettings.expandNewTeams;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kHideLabels:
			fSettings.hideLabels = !fSettings.hideLabels;

			fBarWindow->Lock();
			fBarView->PlaceApplicationBar();
			fBarWindow->Unlock();
			break;

		case kResizeTeamIcons:
		{
			int32 iconSize;

			if (message->FindInt32("be:value", &iconSize) != B_OK)
				break;

			fSettings.iconSize = iconSize * kIconSizeInterval;

			if (fSettings.iconSize < kMinimumIconSize)
				fSettings.iconSize = kMinimumIconSize;
			else if (fSettings.iconSize > kMaximumIconSize)
				fSettings.iconSize = kMaximumIconSize;

			ResizeTeamIcons();

			if (fBarView->MiniState())
				break;

			fBarWindow->Lock();
			if (fBarView->Vertical())
				fBarView->PlaceApplicationBar();
			else
				fBarView->UpdatePlacement();

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

		case kRestartTracker:
		{
			BRoster roster;
			roster.Launch(kTrackerSignature);
			break;
		}

		case B_LOCALE_CHANGED:
		{
			BLocaleRoster::Default()->Refresh();

			bool localize;
			if (message->FindBool("filesys", &localize) == B_OK)
				gLocalizedNamePreferred = localize;

			BMessenger(fBarWindow->FindView("_deskbar_tv_")).SendMessage(
				message);
				// Notify the TimeView that the format has changed and it should
				// recompute its size
			break;
		}

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
	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	// have we already seen this team, is this another instance of
	// a known app?
	BarTeamInfo* multiLaunchTeam = NULL;
	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		if (barInfo->teams->HasItem((void*)team))
			return;
		if (strcasecmp(barInfo->sig, sig) == 0)
			multiLaunchTeam = barInfo;
	}

	if (multiLaunchTeam != NULL) {
		multiLaunchTeam->teams->AddItem((void*)team);

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
		new BBitmap(IconRect(), kIconColorSpace), strdup(ref->name));

	if ((barInfo->flags & B_BACKGROUND_APP) == 0
		&& strcasecmp(barInfo->sig, kDeskbarSignature) != 0) {
		FetchAppIcon(barInfo->sig, barInfo->icon);
	}

	barInfo->teams->AddItem((void*)team);

	sBarTeamInfoList.AddItem(barInfo);

	if (fSettings.expandNewTeams)
		fBarView->AddExpandedItem(sig);

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
		if (barInfo->teams->HasItem((void*)team)) {
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

			barInfo->teams->RemoveItem((void*)team);
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
	for (int32 i = 0; i < sBarTeamInfoList.CountItems(); i++) {
		BarTeamInfo* barInfo = (BarTeamInfo*)sBarTeamInfoList.ItemAt(i);
		if ((barInfo->flags & B_BACKGROUND_APP) == 0
			&& strcasecmp(barInfo->sig, kDeskbarSignature) != 0) {
			delete barInfo->icon;
			barInfo->icon = new BBitmap(IconRect(), kIconColorSpace);
			FetchAppIcon(barInfo->sig, barInfo->icon);
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
	if (fPreferencesWindow)
		fPreferencesWindow->Activate();
	else {
		fPreferencesWindow = new PreferencesWindow(BRect(0, 0, 320, 240));
		fPreferencesWindow->Show();
	}
}


void
TBarApp::FetchAppIcon(const char* signature, BBitmap* icon)
{
	app_info appInfo;
	icon_size size = icon->Bounds().IntegerHeight() >= 32
		? B_LARGE_ICON : B_MINI_ICON;

	if (be_roster->GetAppInfo(signature, &appInfo) == B_OK) {
		// fetch the app icon
		BFile file(&appInfo.ref, B_READ_ONLY);
		BAppFileInfo appMime(&file);
		if (appMime.GetIcon(icon, size) == B_OK)
			return;
	}

	// couldn't find the app icon
	// fetch the generic 3 boxes icon
	BMimeType defaultAppMime;
	defaultAppMime.SetTo(B_APP_MIME_TYPE);
	if (defaultAppMime.GetIcon(icon, size) == B_OK)
		return;

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
	:	teams(teams),
		flags(flags),
		sig(sig),
		icon(icon),
		name(name)
{
}


BarTeamInfo::BarTeamInfo(const BarTeamInfo &info)
	:	teams(new BList(*info.teams)),
		flags(info.flags),
		sig(strdup(info.sig)),
		icon(new BBitmap(*info.icon)),
		name(strdup(info.name))
{
}


BarTeamInfo::~BarTeamInfo()
{
	delete teams;
	free(sig);
	delete icon;
	free(name);
}
