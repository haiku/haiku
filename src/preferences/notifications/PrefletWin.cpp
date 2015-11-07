/*
 * Copyright 2010-2014, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include "PrefletWin.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <SeparatorView.h>

#include <notification/Notifications.h>

#include "PrefletView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefletWin"


const int32 kRevert = '_RVT';
const int32 kApply = '_APY';


PrefletWin::PrefletWin()
	:
	BWindow(BRect(0, 0, 1, 1), B_TRANSLATE_SYSTEM_NAME("Notifications"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	// Preflet container view
	fMainView = new PrefletView(this);
	fMainView->SetBorder(B_NO_BORDER);

	// Apply and revert buttons
	fRevert = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kRevert));
	fRevert->SetEnabled(false);
	fApply = new BButton("apply", B_TRANSLATE("Apply"), new BMessage(kApply));
	fApply->SetEnabled(false);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0)
		.Add(fMainView)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.Add(fRevert)
			.AddGlue()
			.Add(fApply)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING);

	ReloadSettings();

	// Center this window on screen and show it
	CenterOnScreen();
	Show();
}


void
PrefletWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kApply:
		{
			BPath path;

			status_t ret = B_OK;
			ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
			if (ret != B_OK)
				return;

			path.Append(kSettingsFile);

			BMessage settingsStore;
			for (int32 i = 0; i < fMainView->CountPages(); i++) {
				SettingsPane* pane =
					dynamic_cast<SettingsPane*>(fMainView->PageAt(i));
				if (pane) {
					if (pane->Save(settingsStore) == B_OK) {
						fApply->SetEnabled(false);
						fRevert->SetEnabled(true);
					} else
						break;
				}
			}

			// Save settings file
			BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
			ret = settingsStore.Flatten(&file);
			if (ret != B_OK) {
				BAlert* alert = new BAlert("",
					B_TRANSLATE("An error occurred saving the preferences.\n"
						"It's possible you are running out of disk space."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
					B_STOP_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				(void)alert->Go();
			}

			break;
		}
		case kRevert:
			for (int32 i = 0; i < fMainView->CountPages(); i++) {
				SettingsPane* pane =
					dynamic_cast<SettingsPane*>(fMainView->PageAt(i));
				if (pane) {
					if (pane->Revert() == B_OK)
						fRevert->SetEnabled(false);
				}
			}
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}


bool
PrefletWin::QuitRequested()
{
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
PrefletWin::SettingChanged()
{
	fApply->SetEnabled(true);
}


void
PrefletWin::ReloadSettings()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	// FIXME don't load this again here, share with other tabs!
	path.Append(kSettingsFile);

	BMessage settings;
	BFile file(path.Path(), B_READ_ONLY);
	settings.Unflatten(&file);

	for (int32 i = 0; i < fMainView->CountPages(); i++) {
		SettingsPane* pane =
			dynamic_cast<SettingsPane*>(fMainView->PageAt(i));
		if (pane)
			pane->Load(settings);
	}
}
