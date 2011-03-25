/*
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


#include "PCWorld.h"
#include "PCWindow.h"
#include "Preferences.h"
#include "ProcessController.h"
#include "Utilities.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Roster.h>
#include <StringView.h>


PCWindow::PCWindow()
	:
	BWindow(BRect(100, 150, 131, 181),
		B_TRANSLATE_SYSTEM_NAME("ProcessController"), B_TITLED_WINDOW,
		B_NOT_H_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	Preferences preferences(kPreferencesFileName);
	preferences.SaveInt32(kCurrentVersion, kVersionName);
	preferences.LoadWindowPosition(this, kPosPrefName);

	SetSizeLimits(Bounds().Width(), Bounds().Width(), 31, 32767);
	BRect rect = Bounds();

	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	// set up a rectangle && instantiate a new view
	// view rect should be same size as window rect but with left top at (0, 0)
	rect.Set(0, 0, 15, 15);
	rect.OffsetTo((Bounds().Width() - 16) / 2, (Bounds().Height() - 16) / 2);
	topView->AddChild(new ProcessController(rect));

	// make window visible
	Show();
	fDraggersAreDrawn = BDragger::AreDraggersDrawn();
	BDragger::ShowAllDraggers();
}


PCWindow::~PCWindow()
{
	if (!fDraggersAreDrawn)
		BDragger::HideAllDraggers();
}


bool
PCWindow::QuitRequested()
{
	Preferences tPreferences(kPreferencesFileName);
	tPreferences.SaveInt32(kCurrentVersion, kVersionName);
	tPreferences.SaveWindowPosition(this, kPosPrefName);

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
