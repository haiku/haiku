/*
	
	PCWorld.cpp
	
	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
*/

#include "PCWorld.h"
#include "PCWindow.h"
#include "PCView.h"
#include "PCUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <Roster.h>
#include <Deskbar.h>

const char *kSignature = "application/x-vnd.Geb-ProcessController";
const char *kTrackerSig = "application/x-vnd.Be-TRAK";
const char *kDeskbarSig = "application/x-vnd.Be-TSKB";
const char *kTerminalSig = "application/x-vnd.Be-SHEL";
const char*	kPreferencesFileName = "ProcessController Prefs";

thread_id		id = 0;

int main()
{
	PCApplication	application;
	application.Run();

	return B_OK;
}

PCApplication::PCApplication():BApplication(kSignature)
{
}

PCApplication::~PCApplication()
{
	status_t 	thread_return_value;
	if (id) {
		wait_for_thread(id, &thread_return_value);
		id = 0;
	}
}

void PCApplication::ReadyToRun ()
{
	new PCWindow();
	// quitter les autres ProcessController eventuels...
	BList	list;
	be_roster->GetAppList (kSignature, &list);
	long pc_count = list.CountItems ();
	if (pc_count > 1) {
		for (long k = 0; k < pc_count-1; k++) {	
			BMessenger* otherme = new BMessenger (NULL, (team_id) list.ItemAt (k));
			BMessage* message = new BMessage (B_QUIT_REQUESTED);
			otherme->SendMessage (message);
			delete otherme;
		}
	}
}

void PCApplication::ArgvReceived (int32 argc, char **argv)
{
	if (argc == 2 && strcmp (argv[1], "-desktop-reset") == 0)
	{
		team_id tracker = be_roster->TeamFor (kTrackerSig);
		if (tracker >= 0)
		{
			BMessenger messenger (NULL, tracker);
			messenger.SendMessage (B_QUIT_REQUESTED);
			int	k = 500;
			do {
				snooze (10000);
			} while (be_roster->IsRunning (kTrackerSig) && k-- > 0);
		}
		remove ("/boot/home/config/settings/Tracker/tracker_shelf");
		launch (kTrackerSig, "/boot/beos/system/Tracker");
	}
	else if (argc == 2 && strcmp (argv[1], "-deskbar") == 0)
	{
		BDeskbar db;
		if (!gInDeskbar && !db.HasItem (kDeskbarItemName))
			move_to_deskbar (db);
	}
	else if (argc > 1) {
		// print a simple usage string
		printf(	"Usage: %s [-deskbar]\n", argv[0]);
		printf(	"(c) 1997-2001 Georges-Edouard Berenger, berenger@francenet.fr\n");
	}
	Quit ();
}
