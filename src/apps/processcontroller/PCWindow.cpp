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
#include <ControlLook.h>
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

	BView* topView = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	AddChild(topView);

	BSize size = be_control_look->ComposeIconSize(16);
	float spacing = be_control_look->ComposeSpacing(B_USE_ITEM_SPACING);

	BView* processController = new ProcessController(BRect(BPoint(0, 0),
		ProcessController::ComposeSize(size.Width() * 10, size.Height())), false);

	BSize limits(processController->Bounds().Width() + spacing * 2,
		processController->Bounds().Height() + spacing * 2);
	SetSizeLimits(limits.Width(), limits.Width(), limits.Height(), limits.Height());

	topView->AddChild(processController);
	processController->MoveTo(spacing, spacing);

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
