// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseWindow.cpp
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>
#include <Button.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Debug.h>
#include <string.h>

#include "MouseWindow.h"
#include "MouseConstants.h"
#include "SettingsView.h"


MouseWindow::MouseWindow(BRect _rect)
	: BWindow(_rect, "Mouse", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new BView(Bounds(), "view", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	// Add the main settings view
	fSettingsView = new SettingsView(Bounds().InsetBySelf(kBorderSpace + 1, kBorderSpace + 1),
		fSettings);
	view->AddChild(fSettingsView);

	// Add the "Default" button
	BRect rect(kBorderSpace, fSettingsView->Frame().bottom + kItemSpace + 2,
		kBorderSpace + 75, fSettingsView->Frame().bottom + 20);
	BButton *button = new BButton(rect, "defaults", "Defaults", new BMessage(kMsgDefaults));
	button->ResizeToPreferred();
	view->AddChild(button);

	// Add the "Revert" button
	rect.OffsetBy(button->Bounds().Width() + kItemSpace, 0);
	fRevertButton = new BButton(rect, "revert", "Revert", new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);
	fRevertButton->ResizeToPreferred();
	view->AddChild(fRevertButton);

	SetPulseRate(100000);
		// we are using the pulse rate to scan pressed mouse
		// buttons and draw the selected imagery

	ResizeTo(fSettingsView->Frame().right + kBorderSpace,
		button->Frame().bottom + kBorderSpace - 1);

	// check if the window is on screen

	rect = BScreen().Frame();
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
MouseWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgDefaults: {
			// reverts to default settings
			fSettings.Defaults();
			fSettingsView->UpdateFromSettings();

			SetRevertable(true);
			break;
		}

		case kMsgRevert: {
			// revert to last settings
			fSettings.Revert();
			fSettingsView->UpdateFromSettings();

			SetRevertable(false);
			break;
		}

		case kMsgMouseType:
		{
			int32 type;
			if (message->FindInt32("index", &type) == B_OK) {
				fSettings.SetMouseType(++type);
				fSettingsView->SetMouseType(type);
				SetRevertable(true);
			}
			break;
		}

		case kMsgMouseFocusMode:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fSettings.SetMouseMode((mode_mouse)mode);
				SetRevertable(true);
			}
			break;
		}

		case kMsgDoubleClickSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 1000000, fast = 0
				fSettings.SetClickSpeed(value * 1000);
				SetRevertable(true);
			}
			break;
		}

		case kMsgMouseSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 8192, fast = 524287
				fSettings.SetMouseSpeed((int32)pow(2, value * 6 / 1000) * 8192);
				SetRevertable(true);
			}
			break;
		}

		case kMsgAccelerationFactor:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 0, fast = 262144
				fSettings.SetAccelerationFactor((int32)pow(value * 4 / 1000, 2) * 16384);
				SetRevertable(true);
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
				SetRevertable(true);
				fSettingsView->MouseMapUpdated();
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void 
MouseWindow::SetRevertable(bool revertable)
{
	fRevertButton->SetEnabled(revertable);
}

