/*

	MemorySpyWindow.cpp

	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
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

#include "MemorySpyWindow.h"

const char*	kPositionPrefName = "MemorySpyWindowPosition";

MemorySpyWindow::MemorySpyWindow (team_id team)
	: BWindow (BRect (100, 100, 200, 150)), B_EMPTY_STRING, B_FLOATING_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	team_info	tinfo;
	get
	get_team_name_and_icon ();
	be_roster->GetRunningAppInfo(tminfo->team, &info)
	GebsPreferences tPreferences (kPreferencesFileName);
	gPreferences.LoadWindowPosition (this, kPositionPrefName);
	SetPulseRate(150000);

	MMView			*mmView;
	BView			*view;

	// set up a rectangle and instantiate a new view
	// view rect should be same size as window rect but with left top at (0, 0)
	frame.OffsetTo(B_ORIGIN);
	view = new BView(frame, "Main", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(view);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// View About
	view->AddChild(new TriggerAboutView(frame.LeftBottom()));

	frame.InsetBy(10, 10);
	mmView = new MMView(frame);
	
	// add view to window
	view->AddChild(mmView);
	Show();
}

bool MemorySpyWindow::QuitRequested()
{
	GebsPreferences tPreferences (kPreferencesFileName);
	tPreferences.SaveWindowPosition (this, kPositionPrefName);
	be_app->PostMessage (B_QUIT_REQUESTED);
	return true;
}
