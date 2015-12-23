/*
 * Copyright 2010-2013, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>
#include <SymLink.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <notification/Notifications.h>

#include "GeneralView.h"
#include "SettingsHost.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GeneralView"
const int32 kToggleNotifications = '_TSR';

GeneralView::GeneralView(SettingsHost* host)
	:
	SettingsPane("general", host)
{
	// Notifications
	fNotificationBox = new BCheckBox("server",
		B_TRANSLATE("Enable notifications"),
		new BMessage(kToggleNotifications));

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
	// TODO: Here will come a screen representation with the four corners
	// clickable

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL, B_USE_WINDOW_SPACING)
			.Add(fNotificationBox)
			.AddGlue()
		.End()
		.AddGroup(B_VERTICAL, B_USE_WINDOW_SPACING)
			.Add(fAutoStart)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_HORIZONTAL, 2)
					.Add(fTimeout)
					.Add(displayTimeLabel)
				.End()
			.End()
		.End()
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGlue();
}


void
GeneralView::AttachedToWindow()
{
	fNotificationBox->SetTarget(this);
	fAutoStart->SetTarget(this);
	fTimeout->SetTarget(this);
}


void
GeneralView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kToggleNotifications:
		{
			entry_ref ref;

			// Check if server is available
			if (!_CanFindServer(&ref)) {
				BAlert* alert = new BAlert(B_TRANSLATE("Notifications"),
					B_TRANSLATE("The notifications server cannot be"
					" found, this means your InfoPopper installation was"
					" not successfully completed."), B_TRANSLATE("OK"),
					NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				(void)alert->Go();
				return;
			}

			if (fNotificationBox->Value() == B_CONTROL_OFF) {
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
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					(void)alert->Go();
					return;
				}

				// Send quit message
				if (messenger.SendMessage(B_QUIT_REQUESTED) != B_OK) {
					BAlert* alert = new BAlert(B_TRANSLATE(
						"Notifications"), B_TRANSLATE("Cannot disable"
						" notifications because the server can't be "
						"reached."), B_TRANSLATE("OK"),
						NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					(void)alert->Go();
					return;
				}
			} else if (!_IsServerRunning()) {
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
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					(void)alert->Go();
					return;
				}
			}
			break;
		}
		case kSettingChanged:
			SettingsPane::MessageReceived(msg);
			break;
		default:
			BView::MessageReceived(msg);
			break;
	}
}


status_t
GeneralView::Load(BMessage& settings)
{
	char buffer[255];

	fNotificationBox->SetValue(_IsServerRunning() ? B_CONTROL_ON : B_CONTROL_OFF);

	bool autoStart;
	if (settings.FindBool(kAutoStartName, &autoStart) != B_OK)
		autoStart = kDefaultAutoStart;
	fAutoStart->SetValue(autoStart ? B_CONTROL_ON : B_CONTROL_OFF);

	int32 timeout;
	if (settings.FindInt32(kTimeoutName, &timeout) != B_OK)
		timeout = kDefaultTimeout;
	(void)sprintf(buffer, "%" B_PRId32, timeout);
	fTimeout->SetText(buffer);

	return B_OK;
}


status_t
GeneralView::Save(BMessage& settings)
{
	bool autoStart = (fAutoStart->Value() == B_CONTROL_ON);
	settings.AddBool(kAutoStartName, autoStart);

	int32 timeout = atol(fTimeout->Text());
	settings.AddInt32(kTimeoutName, timeout);

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
		query->SetPredicate("(BEOS:APP_SIG==\"" kNotificationServerSignature
			"\")");
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
