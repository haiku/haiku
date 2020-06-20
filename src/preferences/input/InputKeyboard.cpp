/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputKeyboard.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TextControl.h>

#include "InputConstants.h"
#include "KeyboardView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputKeyboard"

InputKeyboard::InputKeyboard(BInputDevice* dev)
	:
	BView("InputKeyboard", B_WILL_DRAW)
{
	// Add the main settings view
	fSettingsView = new KeyboardView();

	// Add the "Default" button..
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
        new BMessage(kMsgDefaults));

	// Add the "Revert" button...
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
        new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, 0)
			.Add(fSettingsView)
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.Add(fDefaultsButton)
			.Add(fRevertButton)
			.AddGlue()
			.End();

	BSlider* slider = (BSlider* )FindView("key_repeat_rate");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatRate());

	slider = (BSlider* )FindView("delay_until_key_repeat");
	if (slider !=NULL)
		slider->SetValue(fSettings.KeyboardRepeatDelay());

	fDefaultsButton->SetEnabled(fSettings.IsDefaultable());
}

void
InputKeyboard::MessageReceived(BMessage* message)
{
	BSlider* slider = NULL;

	switch (message->what) {
		case kMsgDefaults:
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
		case kMsgRevert:
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
		case kMsgSliderrepeatrate:
		{
			int32 rate;
			if (message->FindInt32("be:value", &rate) != B_OK)
				break;
			fSettings.SetKeyboardRepeatRate(rate);

			fDefaultsButton->SetEnabled(fSettings.IsDefaultable());

			fRevertButton->SetEnabled(true);
			break;
		}
		case kMsgSliderdelayrate:
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
			BView::MessageReceived(message);
	}
}
