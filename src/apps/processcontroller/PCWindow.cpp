/*

	PCWindow.cpp

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
#include "Preferences.h"
#include "PCUtils.h"
#include <StringView.h>
#include <Dragger.h>
#include <Deskbar.h>
#include <Alert.h>
#include <Roster.h>

const char*	kPosPrefName = "Position";
const char*	kVersionName = "Version";
const int	kCurrentVersion = 310;

PCWindow::PCWindow():BWindow(BRect(100, 150, 590, 175), "ProcessController", B_TITLED_WINDOW, B_NOT_ZOOMABLE+B_NOT_RESIZABLE, 0)
{
	BView			*topview;
	BStringView		*manuel;
	BRect			rect;

	GebsPreferences tPreferences(kPreferencesFileName);
	int32	version = 0;
	tPreferences.ReadInt32 (version, kVersionName);
	if (version != kCurrentVersion)
	{
		BAlert* alert = new BAlert("", "Do you want ProcessController to live in the Deskbar?",
			"Don't", "Install", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		if (alert->Go())
		{
			team_id deskbar = be_roster->TeamFor (kDeskbarSig);
			if (deskbar >= 0)
			{
				BMessenger messenger (NULL, deskbar);
				messenger.SendMessage (B_QUIT_REQUESTED);
				int	k = 500;
				do {
					snooze (10000);
				} while (be_roster->IsRunning (kDeskbarSig) && k-- > 0);
			}
			be_roster->Launch (kDeskbarSig);
			int	k = 500;
			do {
				snooze (10000);
			} while (!be_roster->IsRunning (kDeskbarSig) && k-- > 0);
			BDeskbar db;
			if (!db.HasItem (kDeskbarItemName))
				move_to_deskbar (db);
		}
	}
	tPreferences.SaveInt32 (kCurrentVersion, kVersionName);
	tPreferences.LoadWindowPosition(this, kPosPrefName);
	rect = Bounds();
	AddChild(topview = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW));
	topview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rect.left = 28;
	rect.top = 9;
	rect.bottom = rect.top+16;
	manuel = new BStringView(rect, NULL, "Drag this Replicant out of here, or click on it, and ask it to \"Live in the Deskbar\"");
	topview->AddChild(manuel);
	manuel->SetFont(be_bold_font);

	// set up a rectangle && instantiate a new view
	// view rect should be same size as window rect but with left top at (0, 0)
	rect.Set(0, 0, 15, 15);
	rect.OffsetTo(8, 9);
	topview->AddChild(new ProcessController(rect));
	
	// make window visible
	Show();
	wasShowing = BDragger::AreDraggersDrawn ();
	BDragger::ShowAllDraggers ();
}

PCWindow::~PCWindow ()
{
	if (!wasShowing)
		BDragger::HideAllDraggers ();
}

bool PCWindow::QuitRequested()
{
	GebsPreferences tPreferences(kPreferencesFileName);
	tPreferences.SaveWindowPosition(this, kPosPrefName);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
