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
#include <malloc.h>
#include <string.h>

#include <AppFileInfo.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <Path.h>
#include <Roster.h>

#if __HAIKU__
#	include <RosterPrivate.h>
#endif

#include "FavoritesConfig.h"

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


// private Be API
extern void __set_window_decor(int32 theme);

BLocker TBarApp::sSubscriberLock;
BList TBarApp::sBarTeamInfoList;
BList TBarApp::sSubscribers;


const uint32 kShowBeMenu = 'BeMn';
const uint32 kShowTeamMenu = 'TmMn';

const BRect kIconSize(0.0f, 0.0f, 15.0f, 15.0f);

#if __HAIKU__
	static const color_space kIconFormat = B_RGBA32;
#else
	static const color_space kIconFormat = B_CMAP8;
#endif



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
		fConfigWindow(NULL)
{
	InitSettings();
	InitIconPreloader();

#ifdef __HAIKU__
	be_roster->StartWatching(this);
#endif

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
#ifdef __HAIKU__
	be_roster->StopWatching(this);
#endif

	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo *barInfo = (BarTeamInfo *)sBarTeamInfoList.ItemAt(i);
		delete barInfo->teams;
		free(barInfo->sig);
		delete barInfo->icon;
		free(barInfo->name);
		free(barInfo);
	}

	int32 subsCount = sSubscribers.CountItems();
	for (int32 i = 0; i < subsCount; i++) {
		BMessenger *messenger = static_cast<BMessenger *>(sSubscribers.ItemAt(i));
		delete messenger;
	}
	SaveSettings();
	delete fSettingsFile;
}


bool
TBarApp::QuitRequested()
{
	if (CurrentMessage() && CurrentMessage()->FindBool("shortcut")) 
		// don't allow user quitting
		return false;

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
		fSettingsFile->Write(&fSettings.timeShowMil, sizeof(bool));
		fSettingsFile->Write(&fSettings.recentFoldersCount, sizeof(int32));
		fSettingsFile->Write(&fSettings.timeShowEuro, sizeof(bool));
		fSettingsFile->Write(&fSettings.alwaysOnTop, sizeof(bool));
		fSettingsFile->Write(&fSettings.timeFullDate, sizeof(bool));
		fSettingsFile->Write(&fSettings.trackerAlwaysFirst, sizeof(bool));
		fSettingsFile->Write(&fSettings.sortRunningApps, sizeof(bool));
		fSettingsFile->Write(&fSettings.superExpando, sizeof(bool));
		fSettingsFile->Write(&fSettings.expandNewTeams, sizeof(bool));
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
	settings.timeShowMil = false;
	settings.recentFoldersCount = 0;	// default is hidden
	settings.timeShowEuro = false;
	settings.alwaysOnTop = false;
	settings.timeFullDate = false;
	settings.trackerAlwaysFirst = false;
	settings.sortRunningApps = false;
	settings.superExpando = false;
	settings.expandNewTeams = false;

	BPath dirPath;
	const char *settingsFileName = "Deskbar_settings";

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
			off_t theSize = 0;
			fSettingsFile->GetSize(&theSize);
			
			if (theSize >= kValidSettingsSize1) {
				fSettingsFile->Seek(0, SEEK_SET);
				fSettingsFile->Read(&settings.vertical, sizeof(bool));
				fSettingsFile->Read(&settings.left, sizeof(bool));
				fSettingsFile->Read(&settings.top, sizeof(bool));
				fSettingsFile->Read(&settings.ampmMode, sizeof(bool));
				fSettingsFile->Read(&settings.state, sizeof(uint32));
				fSettingsFile->Read(&settings.width, sizeof(float));
				fSettingsFile->Read(&settings.showTime, sizeof(bool));
			}
			if (theSize >= kValidSettingsSize2)
				fSettingsFile->Read(&settings.switcherLoc, sizeof(BPoint));
			if (theSize >= kValidSettingsSize3) {
				fSettingsFile->Read(&settings.recentAppsCount, sizeof(int32));
				fSettingsFile->Read(&settings.recentDocsCount, sizeof(int32));
			}
			if (theSize >= kValidSettingsSize4) {
				fSettingsFile->Read(&settings.timeShowSeconds, sizeof(bool));						
				fSettingsFile->Read(&settings.timeShowMil, sizeof(bool));
			}
			if (theSize >= kValidSettingsSize5)
				fSettingsFile->Read(&settings.recentFoldersCount, sizeof(int32));
			if (theSize >= kValidSettingsSize6) {
				fSettingsFile->Read(&settings.timeShowEuro, sizeof(bool));
				fSettingsFile->Read(&settings.alwaysOnTop, sizeof(bool));
			}
			if (theSize >= kValidSettingsSize7)
				fSettingsFile->Read(&settings.timeFullDate, sizeof(bool));
			if (theSize >= kValidSettingsSize8) {
				fSettingsFile->Read(&settings.trackerAlwaysFirst, sizeof(bool));
				fSettingsFile->Read(&settings.sortRunningApps, sizeof(bool));
			}
			if (theSize >= kValidSettingsSize9) {
				fSettingsFile->Read(&settings.superExpando, sizeof(bool));
				fSettingsFile->Read(&settings.expandNewTeams, sizeof(bool));
			}
		}
	}

	fSettings = settings;
}


void
TBarApp::MessageReceived(BMessage *message)
{
	int32 count;
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
			if (message->FindInt32("count", &count) == B_OK)
				fSettings.recentDocsCount = count;
			break;

		case kUpdateAppsCount:
			if (message->FindInt32("count", &count) == B_OK)
				fSettings.recentAppsCount = count;
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

		case msg_config_db:
			ShowConfigWindow();
			break;

		case kConfigClose:
			if (message->FindInt32("applications", &count) == B_OK)				
				fSettings.recentAppsCount = count;
			if (message->FindInt32("folders", &count) == B_OK)				
				fSettings.recentFoldersCount = count;
			if (message->FindInt32("documents", &count) == B_OK)				
				fSettings.recentDocsCount = count;
				
			fConfigWindow = NULL;
			break;

		case B_SOME_APP_LAUNCHED:
		{
			team_id team = -1;
			message->FindInt32("be:team", &team);

			uint32 flags = 0;
			message->FindInt32("be:flags", (long *)&flags);

			const char *sig = NULL;
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

		case msg_Be:
			__set_window_decor(0);
			break;
			
		case msg_Win95:
			__set_window_decor(2);
			break;

		case msg_Amiga:
			__set_window_decor(1);
			break;

		case msg_Mac:
			__set_window_decor(3);
			break;

		case msg_ToggleDraggers:
			if (BDragger::AreDraggersDrawn())
				BDragger::HideAllDraggers();
			else
				BDragger::ShowAllDraggers();
			break;
			
		case msg_AlwaysTop:
 			fSettings.alwaysOnTop = !fSettings.alwaysOnTop;

 			fBarWindow->SetFeel(fSettings.alwaysOnTop ? 
 				B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
 			break;

		case msg_trackerFirst:
		{
			fSettings.trackerAlwaysFirst = !fSettings.trackerAlwaysFirst;

			TBarView *barView = static_cast<TBarApp *>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case msg_sortRunningApps:
		{
			fSettings.sortRunningApps = !fSettings.sortRunningApps;

			TBarView *barView = static_cast<TBarApp *>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case msg_Unsubscribe:
		{
			BMessenger messenger;
			if (message->FindMessenger("messenger", &messenger) == B_OK)
				Unsubscribe(messenger);
			break;
		}
		
		case msg_superExpando:
		{
			fSettings.superExpando = !fSettings.superExpando;

			TBarView *barView = static_cast<TBarApp *>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}
		
		case msg_expandNewTeams:
		{
			fSettings.expandNewTeams = !fSettings.expandNewTeams;

			TBarView *barView = static_cast<TBarApp *>(be_app)->BarView();
			fBarWindow->Lock();
			barView->UpdatePlacement();
			fBarWindow->Unlock();
			break;
		}

		case 'TASK': 
			fSwitcherMessenger.SendMessage(message);
			break;

#if __HAIKU__
		case CMD_SUSPEND_SYSTEM:
			break;

		case CMD_REBOOT_SYSTEM:
		case CMD_SHUTDOWN_SYSTEM: {
			bool reboot = (message->what == CMD_REBOOT_SYSTEM);
			BRoster roster;
			BRoster::Private rosterPrivate(roster);
			status_t error = rosterPrivate.ShutDown(reboot, true, false);
			if (error != B_OK)
				fprintf(stderr, "Shutdown failed: %s\n", strerror(error));

			break;
		}
#endif // __HAIKU__

		// in case Tracker is not running

		case kShowSplash:
#ifdef B_BEOS_VERSION_5
			run_be_about();
#endif
			break;

		default:
			BApplication::MessageReceived(message);		
			break;
	}
}


/**	In case Tracker is not running, the TBeMenu will use us as a target.
 *	We'll make sure the user won't be completely confused and take over
 *	Tracker's duties until it's back.
 */

void
TBarApp::RefsReceived(BMessage *refs)
{
	entry_ref ref;
	for (int32 i = 0; refs->FindRef("refs", i, &ref) == B_OK; i++) {
		BMessage refsReceived(B_REFS_RECEIVED);
		refsReceived.AddRef("refs", &ref);

		BEntry entry(&ref);
		if (!entry.IsDirectory())
			TrackerLaunch(&refsReceived, true);
	}
}


void
TBarApp::Subscribe(const BMessenger &subscriber, BList *list)
{
	// called when ExpandoMenuBar, TeamMenu or Switcher are built/rebuilt
	list->MakeEmpty();

	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	int32 numTeams = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < numTeams; i++) {
		BarTeamInfo	*barInfo = (BarTeamInfo *)sBarTeamInfoList.ItemAt(i);
		BList *tList = new BList(*(barInfo->teams));
		BBitmap *icon = new BBitmap(barInfo->icon);
		ASSERT(icon);
		list->AddItem(new BarTeamInfo(tList, barInfo->flags, strdup(barInfo->sig), icon, strdup(barInfo->name)));
	}

	int32 subsCount = sSubscribers.CountItems();
	for (int32 i = 0; i < subsCount; i++) {
		BMessenger *messenger = (BMessenger *)sSubscribers.ItemAt(i);
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
		BMessenger *messenger = (BMessenger *)sSubscribers.ItemAt(i);
		if (*messenger == subscriber) {
			sSubscribers.RemoveItem(i);
			delete (messenger);
			break;
		}
	}
}


void
TBarApp::AddTeam(team_id team, uint32 flags, const char *sig, entry_ref *ref)
{
	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	// have we already seen this team, is this another instance of 
	// a known app?
	BarTeamInfo *multiLaunchTeam = NULL;
	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo *barInfo = (BarTeamInfo *)sBarTeamInfoList.ItemAt(i);
		if (barInfo->teams->HasItem((void *)team))
			return;
		if (strcasecmp(barInfo->sig, sig) == 0)
			multiLaunchTeam = barInfo;
	}

	if (multiLaunchTeam != NULL) {
		multiLaunchTeam->teams->AddItem((void *)team);

		int32 subsCount = sSubscribers.CountItems();
		if (subsCount > 0) {
			BMessage message(msg_AddTeam);
			message.AddInt32("team", team);
			message.AddString("sig", multiLaunchTeam->sig);

			for (int32 i = 0; i < subsCount; i++)
				((BMessenger *)sSubscribers.ItemAt(i))->SendMessage(&message);		
		}
		return;
	}

	BFile file(ref, B_READ_ONLY);
	BAppFileInfo appMime(&file);

	BarTeamInfo *barInfo = new BarTeamInfo(new BList(), flags, strdup(sig), 
		new BBitmap(kIconSize, kIconFormat), strdup(ref->name));

	barInfo->teams->AddItem((void *)team);
	if (appMime.GetIcon(barInfo->icon, B_MINI_ICON) != B_OK) {
		const BBitmap* generic = AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_GenericAppIcon);
		if (generic)
			barInfo->icon->SetBits(generic->Bits(), barInfo->icon->BitsLength(),
				0, generic->ColorSpace());
	}

	sBarTeamInfoList.AddItem(barInfo);

	int32 subsCount = sSubscribers.CountItems();
	if (subsCount > 0) {
		for (int32 i = 0; i < subsCount; i++) {
			BMessenger *messenger = (BMessenger *)sSubscribers.ItemAt(i);
			BMessage message(B_SOME_APP_LAUNCHED);

			BList *tList = new BList(*(barInfo->teams));
			message.AddPointer("teams", tList);

			BBitmap *icon = new BBitmap(barInfo->icon);
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
TBarApp::RemoveTeam(team_id	team)
{
	BAutolock autolock(sSubscriberLock);
	if (!autolock.IsLocked())
		return;

	int32 teamCount = sBarTeamInfoList.CountItems();
	for (int32 i = 0; i < teamCount; i++) {
		BarTeamInfo *barInfo = (BarTeamInfo *)sBarTeamInfoList.ItemAt(i);
		if (barInfo->teams->HasItem((void *)team)) {
			int32 subsCount = sSubscribers.CountItems();
			if (subsCount > 0) {
				BMessage message((barInfo->teams->CountItems() == 1) ?
					 B_SOME_APP_QUIT : msg_RemoveTeam);

				message.AddInt32("team", team);
				for (int32 i = 0; i < subsCount; i++) {
					BMessenger *messenger = (BMessenger *)sSubscribers.ItemAt(i);
					messenger->SendMessage(&message);
				}
			}

			barInfo->teams->RemoveItem((void *)team);
			if (barInfo->teams->CountItems() < 1) {
				delete (BarTeamInfo *)sBarTeamInfoList.RemoveItem(i);
				return;
			}
		}
	} 
}


void
TBarApp::ShowConfigWindow()
{
	if (fConfigWindow)
		fConfigWindow->Activate();
 	else {
		//	always start at top, could be cached and we could start
		//	where we left off.
		BPath path;
		find_directory (B_USER_DESKBAR_DIRECTORY, &path);
		entry_ref startref;
		get_ref_for_path(path.Path(), &startref);	
	
		fConfigWindow = new TFavoritesConfigWindow(BRect(0, 0, 320, 240),
#ifdef __HAIKU__
			"Configure Leaf Menu", false, B_ANY_NODE,
#else
			"Configure Be Menu", false, B_ANY_NODE,
#endif
			BMessenger(this), &startref,
			fSettings.recentAppsCount, fSettings.recentDocsCount,
			fSettings.recentFoldersCount);
	}
}


//	#pragma mark -


BarTeamInfo::BarTeamInfo(BList *teams, uint32 flags, char *sig, BBitmap *icon, char *name)
	:	teams(teams),
		flags(flags),
		sig(sig),
		icon(icon),
		name(name)
{
}


BarTeamInfo::~BarTeamInfo()
{
	delete teams;
	free(sig);
	delete icon;
	free(name);
}


