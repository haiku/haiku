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

#include <Debug.h>
#include <stdlib.h>
#include <string.h>

#include <AppFileInfo.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Catalog.h>
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
#include "BarApp.h"
#include "BarView.h"
#include "BarWindow.h"
#include "DeskBarUtils.h"
#include "FSUtils.h"
#include "PublicCommands.h"
#include "ResourceSet.h"
#include "Switcher.h"
#include "TeamMenu.h"
#include "WindowMenuItem.h"


BLocker TBarApp::sSubscriberLock;
BList TBarApp::sBarTeamInfoList;
BList TBarApp::sSubscribers;


const uint32 kShowBeMenu = 'BeMn';
const uint32 kShowTeamMenu = 'TmMn';

const BRect kIconSize(0.0f, 0.0f, 15.0f, 15.0f);

static const color_space kIconFormat = B_RGBA32;


int
main()
{
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

	be_roster->StartWatching(this);

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

	fBarWindow = new TBarWindow();
	fBarWindow->Show();

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
		fSettingsFile->Write(&fSettings.vertical, sizeof(bool));
		fSettingsFile->Write(&fSettings.left, sizeof(bool));
		fSettingsFile->Write(&fSettings.top, sizeof(bool));
		fSettingsFile->Write(&fSettings.ampmMode, sizeof(bool));
		fSettingsFile->Write(&fSettings.state, sizeof(uint32));
		fSettingsFile->Write(&fSettings.width, sizeof(float));
		fSettingsFile->Write(&fSettings.showTime, sizeof(bool));
		fSettingsFile->Write(&fSettings.switcherLoc, sizeof(BPoint));
		fSettingsFile->Write(&fSettings.recentAppsCount, sizeof(int32));
		fSettingsFile->Write(&fSettings.recentDocsCount, sizeof(int32));
		fSettingsFile->Write(&fSettings.timeShowSeconds, sizeof(bool));
		fSettingsFile->Write(&fSettings.recentFoldersCount, sizeof(int32));
		fSettingsFile->Write(&fSettings.alwaysOnTop, sizeof(bool));
		fSettingsFile->Write(&fSettings.timeFullDate, sizeof(bool));
		fSettingsFile->Write(&fSettings.trackerAlwaysFirst, sizeof(bool));
		fSettingsFile->Write(&fSettings.sortRunningApps, sizeof(bool));
		fSettingsFile->Write(&fSettings.superExpando, sizeof(bool));
		fSettingsFile->Write(&fSettings.expandNewTeams, sizeof(bool));
		fSettingsFile->Write(&fSettings.autoRaise, sizeof(bool));
		fSettingsFile->Write(&fSettings.recentAppsEnabled, sizeof(bool));
		fSettingsFile->Write(&fSettings.recentDocsEnabled, sizeof(bool));
		fSettingsFile->Write(&fSettings.recentFoldersEnabled, sizeof(bool));
	}
}


void
TBarApp::InitSettings()
{
	desk_settings settings;
	settings.vertical = true;
	settings.left = false;
	settings.top = true;
	settings.ampmMode = true;
	settings.showTime = true;
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
	settings.autoRaise = false;
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

		if (fSettingsFile->InitCheck() == B_OK) {
			off_t size = 0;
			fSettingsFile->GetSize(&size);

			if (size >= kValidSettingsSize1) {
				fSettingsFile->Seek(0, SEEK_SET);
				fSettingsFile->Read(&settings.vertical, sizeof(bool));
				fSettingsFile->Read(&settings.left, sizeof(bool));
				fSettingsFile->Read(&settings.top, sizeof(bool));
				fSettingsFile->Read(&settings.ampmMode, sizeof(bool));
				fSettingsFile->Read(&settings.state, sizeof(uint32));
				fSettingsFile->Read(&settings.width, sizeof(float));
				fSettingsFile->Read(&settings.showTime, sizeof(bool));
			}
			if (size >= kValidSettingsSize2)
				fSettingsFile->Read(&settings.switcherLoc, sizeof(BPoint));
			if (size >= kValidSettingsSize3) {
				fSettingsFile->Read(&settings.recentAppsCount, sizeof(int32));
				fSettingsFile->Read(&settings.recentDocsCount, sizeof(int32));
			}
			if (size >= kValidSettingsSize4) {
				fSettingsFile->Read(&settings.timeShowSeconds, sizeof(bool));
			}
			if (size >= kValidSettingsSize5)
				fSettingsFile->Read(&settings.recentFoldersCount,
					sizeof(int32));
			if (size >= kValidSettingsSize6) {
				fSettingsFile->Read(&settings.alwaysOnTop, sizeof(bool));
			}
			if (size >= kValidSettingsSize7)
				fSettingsFile->Read(&settings.timeFullDate, sizeof(bool));
			if (size >= kValidSettingsSize8) {
				fSettingsFile->Read(&settings.trackerAlwaysFirst, sizeof(bool));
				fSettingsFile->Read(&settings.sortRunningApps, sizeof(bool));
			}
			if (size >= kValidSettingsSize9) {
				fSettingsFile->Read(&settings.superExpando, sizeof(bool));
				fSettingsFile->Read(&settings.expandNewTeams, sizeof(bool));
			}
			if (size >= kValidSettingsSize10)
				fSettingsFile->Read(&settings.autoRaise, sizeof(bool));

			if (size >= kValidSettingsSize11) {
				fSettingsFile->Read(&settings.recentAppsEnabled, sizeof(bool));
				fSettingsFile->Read(&settings.recentDocsEnabled, sizeof(bool));
				fSettingsFile->Read(&settings.recentFoldersEnabled, sizeof(bool));
			} else {
				settings.recentAppsEnabled = settings.recentAppsCount > 0;
				settings.recentDocsEnabled = settings.recentDocsCount > 0;
				settings.recentFoldersEnabled = settings.recentFoldersCount > 0;
			}
		}
	}

	fSettings = settings;
}


void
TBarApp::MessageReceived(BMessage* message)
{
	int32 count;
	bool enabled;
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

		case kShowBeMenu:
			if (fBarWindow->Lock()) {
				fBarWindow->ShowBeMenu();
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
 			break;

		case kAutoRaise:
		{
			fSettings.autoRaise = !fSettings.autoRaise;

			TBarView* barView = static_cast<TBarApp*>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdateAutoRaise();
			fBarWindow->Unlock();
			break;
		}

		case kTrackerFirst:
		{
			fSettings.trackerAlwaysFirst = !fSettings.trackerAlwaysFirst;

			TBarView* barView = static_cast<TBarApp*>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case kSortRunningApps:
		{
			fSettings.sortRunningApps = !fSettings.sortRunningApps;

			TBarView* barView = static_cast<TBarApp*>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case kUnsubscribe:
		{
			BMessenger messenger;
			if (message->FindMessenger("messenger", &messenger) == B_OK)
				Unsubscribe(messenger);
			break;
		}

		case kSuperExpando:
		{
			fSettings.superExpando = !fSettings.superExpando;

			TBarView* barView = static_cast<TBarApp*>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case kExpandNewTeams:
		{
			fSettings.expandNewTeams = !fSettings.expandNewTeams;

			TBarView* barView = static_cast<TBarApp*>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
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

	BarTeamInfo* barInfo = new BarTeamInfo(new BList(), flags, strdup(sig),
		new BBitmap(kIconSize, kIconFormat), strdup(ref->name));

	barInfo->teams->AddItem((void*)team);
	if (appMime.GetIcon(barInfo->icon, B_MINI_ICON) != B_OK)
		appMime.GetTrackerIcon(barInfo->icon, B_MINI_ICON);

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
		if (barInfo->teams->HasItem((void*)team)) {
			int32 subsCount = sSubscribers.CountItems();
			if (subsCount > 0) {
				BMessage message((barInfo->teams->CountItems() == 1) ?
					 B_SOME_APP_QUIT : kRemoveTeam);

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
TBarApp::ShowPreferencesWindow()
{
	if (fPreferencesWindow)
		fPreferencesWindow->Activate();
 	else {
		fPreferencesWindow = new PreferencesWindow(BRect(0, 0, 320, 240));
		fPreferencesWindow->Show();
	}
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

