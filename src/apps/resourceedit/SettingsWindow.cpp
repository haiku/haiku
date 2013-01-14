/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SettingsWindow.h"

#include "Constants.h"
#include "GenericSettingsView.h"
#include "SettingsFile.h"

#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <View.h>


SettingsWindow::SettingsWindow(SettingsFile* settings)
	:
	BWindow(BRect(200, 150, 600, 450), "Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	fSettings = settings;

	BRect bounds = Bounds();

	fBackView = new BView(bounds, "fBackView", B_FOLLOW_ALL, 0);
	fBackView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fListView = new BListView(BRect(8, 8, 100 + 8, bounds.bottom - 8),
		"fListView", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);

	// TODO: Implement selecting different categories of settings.
	// fListView->SetSelectionMessage(new BMessage(...));

	fListView->AddItem(new BStringItem("Generic"));
	fListView->Select(0);

	fScrollView = new BScrollView("fScrollView", fListView, B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	fGenericSettingsView = new GenericSettingsView(
		BRect(132, 8, bounds.right - 8, bounds.bottom - 8 - 40),
		"fGenericSettingsView", fSettings);
	// TODO: Add more settings views (2/4).

	fRevertButton = new BButton(BRect(bounds.right - 152, bounds.bottom - 32,
		bounds.right - 88, bounds.bottom - 8),
		"fRevertButton", "Revert",
		new BMessage(MSG_SETTINGS_REVERT));

	fApplyButton = new BButton(BRect(bounds.right - 72, bounds.bottom - 32,
		bounds.right - 8, bounds.bottom - 8), "fApplyButton", "Apply",
		new BMessage(MSG_SETTINGS_APPLY));

	AdaptSettings();

	fBackView->AddChild(fScrollView);
	fBackView->AddChild(fGenericSettingsView);
	fBackView->AddChild(fRevertButton);
	fBackView->AddChild(fApplyButton);
	AddChild(fBackView);

	Show();
}


SettingsWindow::~SettingsWindow()
{

}


bool
SettingsWindow::QuitRequested()
{
	be_app->PostMessage(MSG_SETTINGS_CLOSED);
	return true;
}

void
SettingsWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case MSG_SETTINGS_APPLY:
		{
			ApplySettings();
			fSettings->Save();
			be_app->PostMessage(MSG_SETTINGS_APPLY);
			break;
		}
		case MSG_SETTINGS_REVERT:
		{
			AdaptSettings();
			fSettings->Save();
			be_app->PostMessage(MSG_SETTINGS_APPLY);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

void
SettingsWindow::ApplySettings()
{
	fGenericSettingsView->ApplySettings();
	// TODO: Add more settings views (3/4).
}

void
SettingsWindow::AdaptSettings()
{
	fGenericSettingsView->AdaptSettings();
	// TODO: Add more settings views (4/4).
}
