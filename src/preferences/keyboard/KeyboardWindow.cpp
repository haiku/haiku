/*
 * Copyright 2004-2007, Haiku. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Jérôme Duval
 *		Marcus Overhagen
 */


#include "KeyboardMessages.h"
#include "KeyboardView.h"
#include "KeyboardWindow.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>
#include <TextControl.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "KeyboardWindow"

KeyboardWindow::KeyboardWindow()
	:
	BWindow(BRect(0, 0, 200, 200), B_TRANSLATE_SYSTEM_NAME("Keyboard"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	MoveTo(fSettings.WindowCorner());

	// Add the main settings view
	fSettingsView = new KeyboardView();
	BBox* fSettingsBox = new BBox("keyboard_box");
	fSettingsBox->AddChild(fSettingsView);

	// Add the "Default" button..
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"), new BMessage(BUTTON_DEFAULTS));

	// Add the "Revert" button...
	fRevertButton = new BButton(B_TRANSLATE("Revert"), new BMessage(BUTTON_REVERT));
	fRevertButton->SetEnabled(false);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fSettingsBox)
		.AddGroup(B_HORIZONTAL, 7)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	BSlider* slider = (BSlider* )FindView("key_repeat_rate");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatRate());

	slider = (BSlider* )FindView("delay_until_key_repeat");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatDelay());

	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

	// center window if it would be off-screen
	BScreen screen;
	if (screen.Frame().right < Frame().right
		|| screen.Frame().bottom < Frame().bottom) {
		CenterOnScreen();
	}

#ifdef DEBUG
	fSettings.Dump();
#endif

	Show();
}


bool
KeyboardWindow::QuitRequested()
{
	fSettings.SetWindowCorner(Frame().LeftTop());

#ifdef DEBUG
	fSettings.Dump();
#endif

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
KeyboardWindow::MessageReceived(BMessage* message)
{
	BSlider* slider = NULL;

	switch (message->what) {
		case BUTTON_DEFAULTS:
		{
			fSettings.Defaults();

			slider = (BSlider* )FindView("key_repeat_rate");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatRate());

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatDelay());

			fDefaultsButton->SetEnabled(false);

  			fRevertButton->SetEnabled(true);
			break;
		}
		case BUTTON_REVERT:
		{
			fSettings.Revert();

			slider = (BSlider* )FindView("key_repeat_rate");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatRate());

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider !=NULL)
				slider->SetValue(fSettings.KeyboardRepeatDelay());

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

  			fRevertButton->SetEnabled(false);
			break;
		}
		case SLIDER_REPEAT_RATE:
		{
			int32 rate;
			if (message->FindInt32("be:value", &rate) != B_OK)
				break;
			fSettings.SetKeyboardRepeatRate(rate);

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(true);
			break;
		}
		case SLIDER_DELAY_RATE:
		{
			int32 delay;
			if (message->FindInt32("be:value", &delay) != B_OK)
				break;

			// We need to look at the value from the slider and make it "jump"
			// to the next notch along. Setting the min and max values of the
			// slider to 1 and 4 doesn't work like the real Keyboard app.
			if (delay < 375000)
				delay = 250000;
			if (delay >= 375000 && delay < 625000)
				delay = 500000;
			if (delay >= 625000 && delay < 875000)
				delay = 750000;
			if (delay >= 875000)
				delay = 1000000;

			fSettings.SetKeyboardRepeatDelay(delay);

			slider = (BSlider* )FindView("delay_until_key_repeat");
			if (slider != NULL)
				slider->SetValue(delay);

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(true);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
