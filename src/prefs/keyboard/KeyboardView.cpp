/*
	
	KeyboardView.cpp
	
*/
#include <InterfaceDefs.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <Button.h>
#include <Slider.h>
#include <TextControl.h>

#include "KeyboardView.h"
#include "KeyboardMessages.h"


KeyboardView::KeyboardView(BRect rect)
	   	   : BBox(rect, "keyboard_view",
					B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER)
{
	BRect	frame;
	BButton *button;
	BSlider	*slider;
	BTextControl *textcontrol;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fIconBitmap = BTranslationUtils::GetBitmap("key_bmap");
	fClockBitmap = BTranslationUtils::GetBitmap("clock_bmap");

	//Add the "Default" button..	
	frame.Set(10,187,85,207);
	button = new BButton(frame,"keyboard_defaults","Defaults", new BMessage(BUTTON_DEFAULTS));
	AddChild(button);
	
	// Add the "Revert" button...
	frame.Set(92,187,167,207);
	button = new BButton(frame,"keyboard_revert","Revert", new BMessage(BUTTON_REVERT));
	button->SetEnabled(false);
	AddChild(button);

	// Create the box for the sliders...
	frame=Bounds();
	frame.left=frame.left+11;
	frame.top=frame.top+12;
	frame.right=frame.right-11;
	frame.bottom=frame.bottom-44;
	fBox = new BBox(frame,"keyboard_box",B_FOLLOW_LEFT,B_WILL_DRAW,B_FANCY_BORDER);
	
	// Create the "Key repeat rate" slider...
	frame.Set(10,10,172,50);
	slider = new BSlider(frame,"key_repeat_rate","Key repeat rate", new BMessage(SLIDER_REPEAT_RATE),20,300,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(5);
	slider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(slider);
	
	// Create the "Delay until key repeat" slider...
	frame.Set(10,65,172,115);
	slider = new BSlider(frame,"delay_until_key_repeat","Delay until key repeat", new BMessage(SLIDER_DELAY_RATE),250000,1000000,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(4);
	slider->SetLimitLabels("Short","Long");
	fBox->AddChild(slider);
	
	// Create the "Typing test area" text box...
	frame=Bounds();
	frame.left=frame.left+10;
	frame.top=135;
	frame.right=frame.right-34;
	frame.bottom=frame.bottom-11;
	textcontrol = new BTextControl(frame,"typing_test_area",NULL,"Typing test area", new BMessage('TTEA'),B_FOLLOW_LEFT,B_WILL_DRAW);
	textcontrol->SetAlignment(B_ALIGN_LEFT,B_ALIGN_CENTER);
	fBox->AddChild(textcontrol);	
	
	AddChild(fBox);

}

void KeyboardView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);
	fBox->DrawBitmap(fIconBitmap,BPoint(178,26));
	fBox->DrawBitmap(fClockBitmap,BPoint(178,83));	
}