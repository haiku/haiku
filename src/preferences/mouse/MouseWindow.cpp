/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Brecht Machiels (brecht@mos6581.org)
 */

#include <Alert.h>
#include <Application.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Debug.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>

#include "MouseWindow.h"
#include "MouseConstants.h"
#include "SettingsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MouseWindow"

MouseWindow::MouseWindow(BRect _rect)
	:
	BWindow(_rect, B_TRANSLATE_SYSTEM_NAME("Mouse"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	// Add the main settings view
	fSettingsView = new SettingsView(fSettings);
	fSettingsBox = new BBox("main box");
	fSettingsBox->AddChild(fSettingsView);

	// Add the "Default" button
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));
	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

	// Add the "Revert" button
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	SetPulseRate(100000);
		// we are using the pulse rate to scan pressed mouse
		// buttons and draw the selected imagery

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fSettingsBox)
		.AddGroup(B_HORIZONTAL, 5)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	// check if the window is on screen
	BRect rect = BScreen().Frame();
	rect.InsetBySelf(20, 20);

	BPoint position = fSettings.WindowPosition();
	BRect windowFrame = Frame().OffsetToSelf(position);
	if (!rect.Contains(windowFrame)) {
		// center window on screen as it doesn't fit on the saved position
		position.x = (rect.Width() - windowFrame.Width()) / 2;
		position.y = (rect.Height() - windowFrame.Height()) / 2;
		if (position.x < 0)
			position.x = 0;
		if (position.y < 0)
			position.y = 15;
	}
	MoveTo(position);
}


bool
MouseWindow::QuitRequested()
{
	fSettings.SetWindowPosition(Frame().LeftTop());
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
MouseWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDefaults: {
			// reverts to default settings
			fSettings.Defaults();
			fSettingsView->UpdateFromSettings();

			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(true);
			break;
		}

		case kMsgRevert: {
			// revert to last settings
			fSettings.Revert();
			fSettingsView->UpdateFromSettings();

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
			fRevertButton->SetEnabled(false);
			break;
		}

		case kMsgMouseType:
		{
			int32 type;
			if (message->FindInt32("index", &type) == B_OK) {
				fSettings.SetMouseType(++type);
				fSettingsView->SetMouseType(type);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgMouseFocusMode:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fSettings.SetMouseMode((mode_mouse)mode);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
				fSettingsView->fFocusFollowsMouseMenu->SetEnabled(
					mode == B_FOCUS_FOLLOWS_MOUSE);
				fSettingsView->fAcceptFirstClickBox->SetEnabled(
					mode != B_FOCUS_FOLLOWS_MOUSE);
			}
			break;
		}

		case kMsgFollowsMouseMode:
		{
			int32 mode;
			if (message->FindInt32("mode_focus_follows_mouse", &mode)
				== B_OK) {
				fSettings.SetFocusFollowsMouseMode(
					(mode_focus_follows_mouse)mode);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgAcceptFirstClick:
		{
			BHandler *handler;
			if (message->FindPointer("source",
				reinterpret_cast<void**>(&handler)) == B_OK) {
				bool acceptFirstClick = false;
				BCheckBox *acceptFirstClickBox =
					dynamic_cast<BCheckBox*>(handler);
				if (acceptFirstClickBox)
					acceptFirstClick = acceptFirstClickBox->Value()
						== B_CONTROL_ON;
				fSettings.SetAcceptFirstClick(acceptFirstClick);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgDoubleClickSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 1000000, fast = 0
				fSettings.SetClickSpeed(value * 1000);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgMouseSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 8192, fast = 524287
				fSettings.SetMouseSpeed((int32)pow(2,
					value * 6.0 / 1000) * 8192);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgAccelerationFactor:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 0, fast = 262144
				fSettings.SetAccelerationFactor((int32)pow(
					value * 4.0 / 1000, 2) * 16384);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
			}
			break;
		}

		case kMsgMouseMap:
		{
			int32 index;
			int32 button;
			if (message->FindInt32("index", &index) == B_OK
				&& message->FindInt32("button", &button) == B_OK) {
				int32 mapping = B_PRIMARY_MOUSE_BUTTON;
				switch (index) {
					case 1:
						mapping = B_SECONDARY_MOUSE_BUTTON;
						break;
					case 2:
						mapping = B_TERTIARY_MOUSE_BUTTON;
						break;
				}

				fSettings.SetMapping(button, mapping);
				fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
				fRevertButton->SetEnabled(true);
				fSettingsView->MouseMapUpdated();
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

