/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/


#include "KeyboardView.h"

#include <InterfaceDefs.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Slider.h>
#include <TextControl.h>
#include <Window.h>

#include "InputConstants.h"
#include "KeyboardSettings.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "KeyboardView"

KeyboardView::KeyboardView()
 :	BGroupView()
{
	// Create the "Key repeat rate" slider...
	fRepeatSlider = new BSlider("key_repeat_rate",
						B_TRANSLATE("Key repeat rate"),
						new BMessage(kMsgSliderrepeatrate),
						20, 300, B_HORIZONTAL);
	fRepeatSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRepeatSlider->SetHashMarkCount(5);
	fRepeatSlider->SetLimitLabels(B_TRANSLATE("Slow"),B_TRANSLATE("Fast"));
	fRepeatSlider->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));


	// Create the "Delay until key repeat" slider...
	fDelaySlider = new BSlider("delay_until_key_repeat",
						B_TRANSLATE("Delay until key repeat"),
						new BMessage(kMsgSliderdelayrate),
						250000, 1000000, B_HORIZONTAL);
	fDelaySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDelaySlider->SetHashMarkCount(4);
	fDelaySlider->SetLimitLabels(B_TRANSLATE("Short"),B_TRANSLATE("Long"));

	// Create the "Typing test area" text box...
	BTextControl* textcontrol = new BTextControl(NULL,
									B_TRANSLATE("Typing test area"),
									new BMessage('TTEA'));
	textcontrol->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
	textcontrol->SetExplicitMinSize(BSize(
		textcontrol->StringWidth(B_TRANSLATE("Typing test area")), B_SIZE_UNSET));

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fRepeatSlider)
		.Add(fDelaySlider)
		.Add(textcontrol)
		.AddGlue();
}


KeyboardView::~KeyboardView()
{

}


void
KeyboardView::Draw(BRect updateFrame)
{
	BPoint pt;
	pt.x = fRepeatSlider->Frame().right + 10;

	if (fIconBitmap != NULL) {
		pt.y = fRepeatSlider->Frame().bottom - 35
			- fIconBitmap->Bounds().Height() / 3;
		DrawBitmap(fIconBitmap, pt);
	}

	if (fClockBitmap != NULL) {
		pt.y = fDelaySlider->Frame().bottom - 35
			- fClockBitmap->Bounds().Height() / 3;
		DrawBitmap(fClockBitmap, pt);
	}
}
