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
	topView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
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
