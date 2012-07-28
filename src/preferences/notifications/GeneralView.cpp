/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <Roster.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Alert.h>
#include <Font.h>
#include <Button.h>
#include <Catalog.h>
#include <StringView.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <String.h>
#include <FindDirectory.h>
#include <Node.h>
#include <notification/Notifications.h>
#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <Query.h>
#include <SymLink.h>

#include "GeneralView.h"
#include "SettingsHost.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GeneralView"
const int32 kServer = '_TSR';

const char* kStartServer = B_TRANSLATE("Enable notifications");
const char* kStopServer = B_TRANSLATE("Disable notifications");
const char* kStarted = B_TRANSLATE("Events are notified");
const char* kStopped = B_TRANSLATE("Events are not notified");


GeneralView::GeneralView(SettingsHost* host)
	:
	SettingsPane("general", host)
{
	BFont statusFont;

	// Set a smaller font for the status label
	statusFont.SetSize(be_plain_font->Size() * 0.8);

	// Status button and label
	fServerButton = new BButton("server", kStartServer, new BMessage(kServer));
	fStatusLabel = new BStringView("status", kStopped);
	fStatusLabel->SetFont(&statusFont);

	// Update status label and server button
	if (_IsServerRunning()) {
		fServerButton->SetLabel(kStopServer);
		fStatusLabel->SetText(kStarted);
	} else {
		fServerButton->SetLabel(kStartServer);
		fStatusLabel->SetText(kStopped);
	}

	// Autostart
	fAutoStart = new BCheckBox("autostart",
		B_TRANSLATE("Enable notifications at startup"),
		new BMessage(kSettingChanged));

	// Display time
	fTimeout = new BTextControl(B_TRANSLATE("Hide notifications from screen"
		" after"), NULL, new BMessage(kSettingChanged));
	BStringView* displayTimeLabel = new BStringView("dt_label",
		B_TRANSLATE("seconds of inactivity"));

	// Default position
	// TODO: Here will come a screen representation with the four corners clickable

	// Load settings
	Load();

	// Calculate inset
	float inset = ceilf(be_plain_font->Size() * 0.7f);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, inset)
		.AddGroup(B_HORIZONTAL, inset)
			.Add(fServerButton)
			.Add(fStatusLabel)
			.AddGlue()
		.End()

		.AddGroup(B_VERTICAL, inset)
			.Add(fAutoStart)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_HORIZONTAL, 2)
					.Add(fTimeout)
					.Add(displayTimeLabel)
				.End()
			.End()
		.End()
		.SetInsets(inset, inset, inset, inset)
		.AddGlue()
	);
}


void
GeneralView::AttachedToWindow()
{
	fServerButton->SetTarget(this);
	fAutoStart->SetTarget(this);
	fTimeout->SetTarget(this);
}


void
GeneralView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kServer: {
				entry_ref ref;

				// Check if server is available
				if (!_CanFindServer(&ref)) {
					BAlert* alert = new BAlert(B_TRANSLATE("Notifications"),
						B_TRANSLATE("The notifications server cannot be"
						" found, this means your InfoPopper installation was"
						" not successfully completed."), B_TRANSLATE("OK"),
						NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					(void)alert->Go();
					return;
				}

				if (_IsServerRunning()) {
					// Server team
					team_id team = be_roster->TeamFor(kNotificationServerSignature);

					// Establish a connection to infopopper_server
					status_t ret = B_ERROR;
					BMessenger messenger(kNotificationServerSignature, team, &ret);
					if (ret != B_OK) {
						BAlert* alert = new BAlert(B_TRANSLATE(
							"Notifications"), B_TRANSLATE("Notifications "
							"cannot be stopped, because the server can't be"
							" reached."), B_TRANSLATE("OK"), NULL, NULL,
							B_WIDTH_AS_USUAL, B_STOP_ALERT);
						(void)alert->Go();
						return;
					}

					// Send quit message
					if (messenger.SendMessage(new BMessage(B_QUIT_REQUESTED)) != B_OK) {
						BAlert* alert = new BAlert(B_TRANSLATE(
							"Notifications"), B_TRANSLATE("Cannot disable"
							" notifications because the server can't be "
							"reached."), B_TRANSLATE("OK"),
							NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
						(void)alert->Go();
						return;
					}

					fServerButton->SetLabel(kStartServer);
					fStatusLabel->SetText(kStopped);
				} else {
					// Start server
					status_t err = be_roster->Launch(kNotificationServerSignature);
					if (err != B_OK) {
						BAlert* alert = new BAlert(B_TRANSLATE(
							"Notifications"), B_TRANSLATE("Cannot enable"
							" notifications because the server cannot be "
							"found.\nThis means your InfoPopper installation"
							" was not successfully completed."),
							B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
							B_STOP_ALERT);
						(void)alert->Go();
						return;
					}

					fServerButton->SetLabel(kStopServer);
					fStatusLabel->SetText(kStarted);
				}
			} break;
		case kSettingChanged:
			SettingsPane::MessageReceived(msg);
			break;
		default:
			BView::MessageReceived(msg);
			break;
	}
}


status_t
GeneralView::Load()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append(kSettingsDirectory);

	if (create_directory(path.Path(), 0755) != B_OK) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("There was a problem saving the preferences.\n"
				"It's possible you don't have write access to the "
				"settings directory."), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
	}

	path.Append(kGeneralSettings);

	BMessage settings;
	BFile file(path.Path(), B_READ_ONLY);
	settings.Unflatten(&file);

	char buffer[255];

	bool autoStart;
	if (settings.FindBool(kAutoStartName, &autoStart) != B_OK)
		autoStart = kDefaultAutoStart;
	fAutoStart->SetValue(autoStart ? B_CONTROL_ON : B_CONTROL_OFF);

	int32 timeout;
	if (settings.FindInt32(kTimeoutName, &timeout) != B_OK)
		timeout = kDefaultTimeout;
	(void)sprintf(buffer, "%ld", timeout);
	fTimeout->SetText(buffer);

	return B_OK;
}


status_t
GeneralView::Save()
{
	BPath path;

	status_t ret = B_OK;
	ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
		return ret;

	path.Append(kSettingsDirectory);
	path.Append(kGeneralSettings);

	BMessage settings;

	bool autoStart = (fAutoStart->Value() == B_CONTROL_ON);
	settings.AddBool(kAutoStartName, autoStart);

	int32 timeout = atol(fTimeout->Text());
	settings.AddInt32(kTimeoutName, timeout);

	// Save settings file
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	ret = settings.Flatten(&file);
	if (ret != B_OK) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("An error occurred saving the preferences.\n"
				"It's possible you are running out of disk space."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
		return ret;
	}

	// Find server path
	entry_ref ref;
	if (!_CanFindServer(&ref)) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("The notifications server cannot be found.\n"
			   "A possible cause is an installation not done correctly"),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
		return B_ERROR;
	}

	// UserBootscript command
	BPath serverPath(&ref);

	// Start server at boot time
	ret = find_directory(B_USER_BOOT_DIRECTORY, &path, true);
	if (ret != B_OK) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("Can't save preferences, you probably don't have "
			"write access to the boot settings directory."), B_TRANSLATE("OK"),
			NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
		return ret;
	}

	path.Append("launch");
	BDirectory directory(path.Path());
	BEntry entry(&directory, serverPath.Leaf());

	// Remove symbolic link
	entry.Remove();

	if (autoStart) {
		// Put a symlink into ~/config/boot/launch
		if ((ret = directory.CreateSymLink(serverPath.Leaf(),
			serverPath.Path(), NULL) != B_OK)) {
			BAlert* alert = new BAlert("",
				B_TRANSLATE("Can't enable notifications at startup time, "
				"you probably don't have write permission to the boot settings"
				" directory."), B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			(void)alert->Go();
			return ret;
		}
	}

	return B_OK;
}


status_t
GeneralView::Revert()
{
	return B_ERROR;
}


bool
GeneralView::_CanFindServer(entry_ref* ref)
{
	// Try searching with be_roster
	if (be_roster->FindApp(kNotificationServerSignature, ref) == B_OK)
		return true;

	// Try with a query and take the first result
	BVolumeRoster vroster;
	BVolume volume;
	char volName[B_FILE_NAME_LENGTH];

	vroster.Rewind();

	while (vroster.GetNextVolume(&volume) == B_OK) {
		if ((volume.InitCheck() != B_OK) || !volume.KnowsQuery())
			continue;

		volume.GetName(volName);

		BQuery *query = new BQuery();
		query->SetPredicate("(BEOS:APP_SIG==\""kNotificationServerSignature"\")");
		query->SetVolume(&volume);
		query->Fetch();

		if (query->GetNextRef(ref) == B_OK)
			return true;
	}

	return false;
}


bool
GeneralView::_IsServerRunning()
{
	return be_roster->IsRunning(kNotificationServerSignature);
}
