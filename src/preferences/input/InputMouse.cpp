/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputMouse.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Slider.h>
#include <Screen.h>
#include <StringView.h>
#include <TabView.h>

#include "InputConstants.h"
#include "InputWindow.h"
#include "MouseSettings.h"
#include "MouseView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMouse"


InputMouse::InputMouse(BInputDevice* dev, MouseSettings* settings)
	: BView("InputMouse", B_WILL_DRAW)
{
	fSettings = settings;

	fSettingsView = new SettingsView(*fSettings);

	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kMsgDefaults));
	fDefaultsButton->SetEnabled(fSettings->IsDefaultable());

	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fSettingsView)
			.Add(new BSeparatorView(B_HORIZONTAL))
				.AddGroup(B_HORIZONTAL)
				.Add(fDefaultsButton)
				.Add(fRevertButton)
				.AddGlue()
				.End()
		.End();
}

InputMouse::~InputMouse()
{
}

void
InputMouse::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDefaults: {
			fSettings->Defaults();
			fSettingsView->UpdateFromSettings();

			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(fSettings->IsRevertable());
			break;
		}

		case kMsgRevert: {
			fSettings->Revert();
			fSettingsView->UpdateFromSettings();

			fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
			fRevertButton->SetEnabled(false);
			break;

		}

		case kMsgMouseType:
		{
			int32 type;
			if (message->FindInt32("be:value", &type) == B_OK) {
				if (type > 6)
					debugger("Mouse type is invalid");
				fSettings->SetMouseType(type);
				fSettingsView->SetMouseType(type);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgMouseFocusMode:
		{
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK) {
				fSettings->SetMouseMode((mode_mouse)mode);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
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
				fSettings->SetFocusFollowsMouseMode(
					(mode_focus_follows_mouse)mode);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgAcceptFirstClick:
		{
			BHandler *handler;
			if (message->FindPointer("source",
				reinterpret_cast<void**>(&handler)) == B_OK) {
				bool acceptFirstClick = true;
				BCheckBox *acceptFirstClickBox =
					dynamic_cast<BCheckBox*>(handler);
				if (acceptFirstClickBox)
					acceptFirstClick = acceptFirstClickBox->Value()
						== B_CONTROL_ON;
				fSettings->SetAcceptFirstClick(acceptFirstClick);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgDoubleClickSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 1000000, fast = 0
				fSettings->SetClickSpeed(value * 1000);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgMouseSpeed:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 8192, fast = 524287
				fSettings->SetMouseSpeed((int32)pow(2,
					value * 6.0 / 1000) * 8192);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgAccelerationFactor:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				// slow = 0, fast = 262144
				fSettings->SetAccelerationFactor((int32)pow(
					value * 4.0 / 1000, 2) * 16384);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
			}
			break;
		}

		case kMsgMouseMap:
		{
			int32 index;
			int32 button;
			if (message->FindInt32("index", &index) == B_OK
				&& message->FindInt32("button", &button) == B_OK) {
				int32 mapping = B_MOUSE_BUTTON(index + 1);
				fSettings->SetMapping(button, mapping);
				fDefaultsButton->SetEnabled(fSettings->IsDefaultable());
				fRevertButton->SetEnabled(fSettings->IsRevertable());
				fSettingsView->MouseMapUpdated();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}
