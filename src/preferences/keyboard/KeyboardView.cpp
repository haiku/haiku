/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#include <InterfaceDefs.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <Button.h>
#include <Slider.h>
#include <TextControl.h>
#include <Window.h>
#include <Font.h>

#include "KeyboardView.h"
#include "KeyboardMessages.h"


KeyboardView::KeyboardView(BRect rect)
 :	BView(rect, "keyboard_view", B_FOLLOW_LEFT | B_FOLLOW_TOP,
 		  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP)
{
	BRect frame;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fIconBitmap = BTranslationUtils::GetBitmap("key_bmap");
	fClockBitmap = BTranslationUtils::GetBitmap("clock_bmap");

	float labelwidth = StringWidth("Delay until key repeat")+20;
	
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	
	float labelheight = fontHeight.ascent + fontHeight.descent +
						fontHeight.leading;
	
	// Create the "Key repeat rate" slider...
	frame.Set(10,10,10 + labelwidth,10 + (labelheight*2) + 20);
	fRepeatSlider = new BSlider(frame,"key_repeat_rate",
										"Key repeat rate",
										new BMessage(SLIDER_REPEAT_RATE),
										20,300,B_BLOCK_THUMB,
										B_FOLLOW_LEFT,B_WILL_DRAW);
	fRepeatSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRepeatSlider->SetHashMarkCount(5);
	fRepeatSlider->SetLimitLabels("Slow","Fast");
	
	
	// Create the "Delay until key repeat" slider...
	frame.OffsetBy(0,frame.Height() + 10);
	fDelaySlider = new BSlider(frame,"delay_until_key_repeat",
						"Delay until key repeat",
						new BMessage(SLIDER_DELAY_RATE),250000,1000000,
						B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	fDelaySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDelaySlider->SetHashMarkCount(4);
	fDelaySlider->SetLimitLabels("Short","Long");
	
	// Create the "Typing test area" text box...
	frame.OffsetBy(0,frame.Height() + 15);
	frame.right = fDelaySlider->Frame().right + fIconBitmap->Bounds().Width() +
				  10;
	BTextControl *textcontrol = new BTextControl(frame,"typing_test_area",NULL,
									"Typing test area",
									new BMessage('TTEA'),
									B_FOLLOW_LEFT,B_WILL_DRAW);
	textcontrol->SetAlignment(B_ALIGN_LEFT,B_ALIGN_CENTER);
	
	float width, height;
	textcontrol->GetPreferredSize(&width, &height);
	textcontrol->ResizeTo(frame.Width(),height);
	
	// Create the box for the sliders...
	frame.left = frame.top = 10;
	frame.right = frame.left + fDelaySlider->Frame().right + 
				fClockBitmap->Bounds().Width() + 20;
	frame.bottom = textcontrol->Frame().bottom + 20;
	fBox = new BBox(frame,"keyboard_box",B_FOLLOW_LEFT,B_WILL_DRAW,
					B_FANCY_BORDER);
	AddChild(fBox);
	
	fBox->AddChild(fRepeatSlider);
	fBox->AddChild(fDelaySlider);
	fBox->AddChild(textcontrol);	

	//Add the "Default" button..	
	frame.left = 10;
	frame.top = fBox->Frame().bottom + 10;
	frame.right = frame.left + 1;
	frame.bottom = frame.top + 1;
	BButton *button = new BButton(frame,"keyboard_defaults","Defaults",
						new BMessage(BUTTON_DEFAULTS));
	button->ResizeToPreferred();
	AddChild(button);
	
	// Add the "Revert" button...
	frame = button->Frame();
	frame.OffsetBy(frame.Width() + 10, 0);
	button = new BButton(frame,"keyboard_revert","Revert",
						new BMessage(BUTTON_REVERT));
	button->SetEnabled(false);
	AddChild(button);
	
	ResizeTo(fBox->Frame().right + 10, button->Frame().bottom + 10);
}

void
KeyboardView::Draw(BRect updateFrame)
{
	BPoint pt;
	pt.x = fRepeatSlider->Frame().right + 10;
	pt.y = fRepeatSlider->Frame().bottom - 35 - 
			(fIconBitmap->Bounds().Height()/3);
	
	fBox->DrawBitmap(fIconBitmap,pt);
	
	pt.y = fDelaySlider->Frame().bottom - 35 - 
			(fIconBitmap->Bounds().Height()/3);
	fBox->DrawBitmap(fClockBitmap,pt);
}

void
KeyboardView::AttachedToWindow(void)
{
	Window()->ResizeTo(Bounds().Width(), Bounds().Height());
}

