/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PCWorld.h"
#include "PCWindow.h"
#include "Preferences.h"
#include "ProcessController.h"
#include "Utilities.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessController"


class PCApplication : public BApplication {
public:
								PCApplication();
	virtual						~PCApplication();

	virtual	void				ReadyToRun();
	virtual	void				ArgvReceived(int32 argc, char** argv);
};


const char* kSignature = "application/x-vnd.Haiku-ProcessController";
const char* kTrackerSig = "application/x-vnd.Be-TRAK";
const char* kDeskbarSig = "application/x-vnd.Be-TSKB";
const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";
const char* kPreferencesFileName = "ProcessController Prefs";

const char*	kPosPrefName = "Position";
const char*	kVersionName = "Version";
const int kCurrentVersion = 311;

thread_id id = 0;


PCApplication::PCApplication()
	:
	BApplication(kSignature)
{
}


PCApplication::~PCApplication()
{
	if (id) {
		status_t returnValue;
		wait_for_thread(id, &returnValue);
	}
}


void
PCApplication::ReadyToRun()
{
	BDeskbar deskbar;
	if (!deskbar.HasItem(kDeskbarItemName)) {
		// We're not yet installed in the Deskbar, ask if we should
		BAlert* alert = new BAlert(B_TRANSLATE("Info"),
			B_TRANSLATE("You can run ProcessController in a window"
			" or install it in the Deskbar."), B_TRANSLATE("Run in window"),
			B_TRANSLATE("Install in Deskbar"),
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (alert->Go() != 0) {
			BDeskbar deskbar;
			if (!deskbar.HasItem(kDeskbarItemName))
				move_to_deskbar(deskbar);
			Quit();
			return;
		}
	} else {
		BAlert* alert = new BAlert(B_TRANSLATE("Info"),
			B_TRANSLATE("ProcessController is already installed in Deskbar."),
			B_TRANSLATE("OK"), NULL,
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}

	new PCWindow();

	// quit other eventually running instances
	BList list;
	be_roster->GetAppList(kSignature, &list);
	int32 count = list.CountItems();
	if (count > 1) {
		for (int32 i = 0; i < count - 1; i++) {
			BMessenger otherme(NULL, (addr_t)list.ItemAt(i));
			otherme.SendMessage(B_QUIT_REQUESTED);
		}
	}
}


void
PCApplication::ArgvReceived(int32 argc, char **argv)
{
	if (argc == 2 && strcmp(argv[1], "-desktop-reset") == 0) {
		team_id tracker = be_roster->TeamFor(kTrackerSig);
		if (tracker >= 0) {
			BMessenger messenger(NULL, tracker);
			messenger.SendMessage(B_QUIT_REQUESTED);
			int	k = 500;
			do {
				snooze(10000);
			} while (be_roster->IsRunning(kTrackerSig) && k-- > 0);
		}
		BPath shelfPath;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &shelfPath) == B_OK
			&& shelfPath.Append("Tracker/tracker_shelf") == B_OK) {
			remove(shelfPath.Path());
		}
		BPath trackerPath;
		if (find_directory(B_SYSTEM_DIRECTORY, &trackerPath) == B_OK
			&& trackerPath.Append("Tracker") == B_OK) {
			launch(kTrackerSig, trackerPath.Path());
		}
	} else if (argc == 2 && strcmp(argv[1], "-deskbar") == 0) {
		BDeskbar deskbar;
		if (!gInDeskbar && !deskbar.HasItem(kDeskbarItemName))
			move_to_deskbar(deskbar);
	} else if (argc > 1) {
		// print a simple usage string
		printf(B_TRANSLATE("Usage: %s [-deskbar]\n"), argv[0]);
		puts(B_TRANSLATE("(c) 1996-2001 Georges-Edouard Berenger, "
		"berenger@francenet.fr"));
	}

	Quit();
}


//	#pragma mark -


int
main()
{
	PCApplication application;
	application.Run();

	return B_OK;
}

