// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MouseView.cpp
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :   December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <InterfaceDefs.h>
#include <Button.h>
#include <Box.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TextControl.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Debug.h>
#include <Window.h>

#include "MouseView.h"
#include "MouseMessages.h"
#include "MouseBitmap.h"


BoxView::BoxView(BRect rect, MouseView *mouseView)
	: BBox(rect, "mouse_box"),
	fMouseView(mouseView)
{
}


void
BoxView::MouseDown(BPoint where)
{
	int32 index = fMouseView->mouseTypeMenu->IndexOf(fMouseView->mouseTypeMenu->FindMarked());

	fMouseView->fCurrentButton = -1;
	if ((index == 0) && BRect(50,41,105,73).Contains(where))
		fMouseView->fCurrentButton = 0;
	else if (index == 1) {
		if(BRect(50,41,77,73).Contains(where))
			fMouseView->fCurrentButton = 0;
		else if (BRect(77,41,105,73).Contains(where))
			fMouseView->fCurrentButton = 1;
	} else {
		if(BRect(50,41,68,73).Contains(where))
			fMouseView->fCurrentButton = 0;
		else if (BRect(86,41,105,73).Contains(where))
			fMouseView->fCurrentButton = 1;
		else if (BRect(68,41,86,73).Contains(where))
			fMouseView->fCurrentButton = 2;
	}

	if (fMouseView->fCurrentButton >= 0) {
		int32 number = 0;
		switch (fMouseView->fCurrentMouseMap.button[fMouseView->fCurrentButton]) {
			case B_PRIMARY_MOUSE_BUTTON: number = 0; break;
			case B_SECONDARY_MOUSE_BUTTON: number = 1; break;
			case B_TERTIARY_MOUSE_BUTTON: number = 2; break;
		}
		fMouseView->mouseMapMenu->ItemAt(number)->SetMarked(true);
		ConvertToScreen(&where);
		fMouseView->mouseMapMenu->Go(where, true);
	}
}


MouseView::MouseView(BRect rect)
	: BView(rect, "mouse_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	fCurrentButton(-1),
	fButtons(0),
	fOldButtons(0)
{
	BTextControl *textcontrol;
	BMenuField *menufield;
	BRect frame;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fDoubleClickBitmap = BTranslationUtils::GetBitmap("double_click_bmap");
	if (!fDoubleClickBitmap) {
		printf("can't load double_click_bmap with BTranslationUtils::GetBitmap\n");
		exit(1);
	}

	fSpeedBitmap = BTranslationUtils::GetBitmap("speed_bmap");
	if (!fSpeedBitmap) {
		printf("can't load speed_bmap with BTranslationUtils::GetBitmap\n");
		exit(1);
	}

	fAccelerationBitmap = BTranslationUtils::GetBitmap("acceleration_bmap");
	if (!fAccelerationBitmap) {
		printf("can't load acceleration_bmap with BTranslationUtils::GetBitmap\n");
		exit(1);
	}

	// i don't really understand this bitmap SetBits : i have to substract 4 lines and add 15 pixels
	BRect mouseRect(0,0,kMouseWidth-1,kMouseHeight-5); 
  	fMouseBitmap = new BBitmap(mouseRect, B_CMAP8);
  	fMouseBitmap->SetBits(kMouseBits, kMouseWidth*kMouseHeight + 15, 0, kMouseColorSpace);

	BRect mouseDownRect(0,0,kMouseDownWidth-1,kMouseDownHeight-1);
  	fMouseDownBitmap = new BBitmap(mouseDownRect, B_CMAP8);
  	fMouseDownBitmap->SetBits(kMouseDownBits, kMouseDownWidth*kMouseDownHeight + 30, 0, kMouseDownColorSpace);

	// Add the "Default" button..	
	frame.Set(10,259,85,279);
	defaultButton = new BButton(frame,"mouse_defaults","Defaults", new BMessage(BUTTON_DEFAULTS));
	AddChild(defaultButton);

	// Add the "Revert" button...
	frame.Set(92,259,167,279);
	revertButton = new BButton(frame,"mouse_revert","Revert", new BMessage(BUTTON_REVERT));
	revertButton->SetEnabled(false);
	AddChild(revertButton);

	// Create the main box for the controls...
	frame = Bounds();
	frame.left = frame.left+11;
	frame.top = frame.top+11;
	frame.right = frame.right-11;
	frame.bottom = frame.bottom-44;
	fBox = new BoxView(frame, this);	

	// Add the "Mouse Type" pop up menu
	mouseTypeMenu = new BPopUpMenu("Mouse Type Menu");
	mouseTypeMenu->AddItem(new BMenuItem("1-Button", new BMessage(POPUP_MOUSE_TYPE)));
	mouseTypeMenu->AddItem(new BMenuItem("2-Button", new BMessage(POPUP_MOUSE_TYPE)));
	mouseTypeMenu->AddItem(new BMenuItem("3-Button", new BMessage(POPUP_MOUSE_TYPE)));

	frame.Set(7,8,208,20);
	menufield = new BMenuField(frame, "mouse_type", "Mouse type", mouseTypeMenu);
	menufield->SetDivider(menufield->Divider() - 29);
	menufield->SetAlignment(B_ALIGN_RIGHT);
	fBox->AddChild(menufield);

	// Add the "Focus follows mouse" pop up menu
	focusMenu = new BPopUpMenu("Focus Follows Mouse Menu");
	focusMenu->AddItem(new BMenuItem("Disabled",new BMessage(POPUP_MOUSE_FOCUS)));
	focusMenu->AddItem(new BMenuItem("Enabled",new BMessage(POPUP_MOUSE_FOCUS)));
	focusMenu->AddItem(new BMenuItem("Warping",new BMessage(POPUP_MOUSE_FOCUS)));
	focusMenu->AddItem(new BMenuItem("Instant-Warping",new BMessage(POPUP_MOUSE_FOCUS)));

	frame.Set(165, 208, 440, 200);
	menufield = new BMenuField(frame, "Focus follows mouse", "Focus follows mouse", focusMenu);
	menufield->SetDivider(menufield->Divider() - 18);
	menufield->SetAlignment(B_ALIGN_RIGHT);
	fBox->AddChild(menufield);

	// Create the "Double-click speed slider...
	frame.Set(166, 11, 328, 50);
	dcSpeedSlider = new BSlider(frame,"double_click_speed","Double-click speed", 
		new BMessage(SLIDER_DOUBLE_CLICK_SPEED), 0, 1000, B_BLOCK_THUMB, B_FOLLOW_LEFT, B_WILL_DRAW);
	dcSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	dcSpeedSlider->SetHashMarkCount(5);
	dcSpeedSlider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(dcSpeedSlider);

	// Create the "Mouse Speed" slider...
	frame.Set(166,76,328,125);
	mouseSpeedSlider = new BSlider(frame,"mouse_speed","Mouse Speed", 
		new BMessage(SLIDER_MOUSE_SPEED),0,1000,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	mouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	mouseSpeedSlider->SetHashMarkCount(7);
	mouseSpeedSlider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(mouseSpeedSlider);

	// Create the "Mouse Acceleration" slider...
	frame.Set(166,141,328,190);
	mouseAccSlider = new BSlider(frame,"mouse_acceleration","Mouse Acceleration", 
		new BMessage(SLIDER_MOUSE_ACC),0,1000,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	mouseAccSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	mouseAccSlider->SetHashMarkCount(5);
	mouseAccSlider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(mouseAccSlider);

	// Create the "Double-click test area" text box...
	frame=fBox->Bounds();
	frame.left=frame.left+9;
	frame.right=frame.right-230;
	frame.top=frame.bottom-30;
	textcontrol = new BTextControl(frame,"double_click_test_area",NULL,"Double-click test area", new BMessage(DOUBLE_CLICK_TEST_AREA),B_FOLLOW_LEFT,B_WILL_DRAW);
	textcontrol->SetAlignment(B_ALIGN_LEFT,B_ALIGN_CENTER);
	fBox->AddChild(textcontrol);

	AddChild(fBox);

	mouseMapMenu = new BPopUpMenu("Mouse Map Menu", true, true);
	mouseMapMenu->AddItem(new BMenuItem("1", new BMessage(POPUP_MOUSE_MAP)));
	mouseMapMenu->AddItem(new BMenuItem("2", new BMessage(POPUP_MOUSE_MAP)));
	mouseMapMenu->AddItem(new BMenuItem("3", new BMessage(POPUP_MOUSE_MAP)));
	
}


void
MouseView::AttachedToWindow()
{
	get_click_speed(&fClickSpeed);
	get_mouse_speed(&fMouseSpeed);
	get_mouse_acceleration(&fMouseAcc);
	get_mouse_type(&fMouseType);
	fMouseMode = mouse_mode();
	get_mouse_map(&fMouseMap);
	get_mouse_map(&fCurrentMouseMap);
	Init();

	mouseMapMenu->SetTargetForItems(Window());
}


void
MouseView::Pulse()
{
	BPoint point;
	GetMouse(&point, &fButtons, true);

	if (fOldButtons != fButtons) {
		printf("buttons: old = %ld, new = %ld\n", fOldButtons, fButtons);
		Invalidate();
		fOldButtons = fButtons;
	}
}


void 
MouseView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

	fBox->SetHighColor(120,120,120);
	fBox->SetLowColor(255,255,255);
	// Line above the test area
	fBox->StrokeLine(BPoint(8,198),BPoint(149,198),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(8,199),BPoint(149,199),B_SOLID_LOW);
	// Line above focus follows mouse
	fBox->StrokeLine(BPoint(164,198),BPoint(367,198),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(164,199),BPoint(367,199),B_SOLID_LOW);
	// Line in the middle
	fBox->StrokeLine(BPoint(156,10),BPoint(156,230),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(157,11),BPoint(157,230),B_SOLID_LOW);

	// Draw the icons
	if(updateFrame.Intersects(	// i have to add 10 pixels width and height, so weird
			BRect(333,16,354+fDoubleClickBitmap->Bounds().Width(),
			165+fAccelerationBitmap->Bounds().Height()))) {
		fBox->DrawBitmapAsync(fDoubleClickBitmap,BPoint(344,16));
		fBox->DrawBitmapAsync(fSpeedBitmap,BPoint(333,90));	
		fBox->DrawBitmapAsync(fAccelerationBitmap,BPoint(333,155));
	}
	
	// Draw the mouse 
	// i have to add 10 pixels width and height, so weird
	if(updateFrame.Intersects(BRect(50,41,60+kMouseWidth,51+kMouseHeight))) {
		fBox->SetDrawingMode(B_OP_OVER);
	
		// Draw the mouse top
		fBox->DrawBitmapAsync(fMouseBitmap,BPoint(50, 41));
		int32 index = mouseTypeMenu->IndexOf(mouseTypeMenu->FindMarked());
		if (index == 0)	{	// 1 button
			if (fButtons & fCurrentMouseMap.button[0]) {
				fBox->DrawBitmapAsync(fMouseDownBitmap,BPoint(50, 47));
				fBox->SetHighColor(64,64,64);
			}
		} else if (index == 1) { 	// 2 button
			if ( fButtons & fCurrentMouseMap.button[0]) {
				fBox->DrawBitmapAsync(fMouseDownBitmap, BRect(0,0,27,28), BRect(50,47,77,75));
				fBox->SetHighColor(120,120,120);
			} else
				fBox->SetHighColor(184,184,184);
			fBox->StrokeLine(BPoint(76,49),BPoint(76,71),B_SOLID_HIGH);
					
			if (fButtons & fCurrentMouseMap.button[1]) {
				fBox->SetHighColor(120,120,120);
				fBox->DrawBitmapAsync(fMouseDownBitmap, BRect(27,0,54,28), BRect(77,47,104,75));
			} else
				fBox->SetHighColor(255,255,255);
			fBox->StrokeLine(BPoint(78,49),BPoint(78,71),B_SOLID_HIGH);
			
			if ((fButtons & fCurrentMouseMap.button[0]) || (fButtons & fCurrentMouseMap.button[1]))
				fBox->SetHighColor(48,48,48);
			else
				fBox->SetHighColor(88,88,88);
			fBox->StrokeLine(BPoint(77,48),BPoint(77,72),B_SOLID_HIGH);
			
		} else {	// 3 button
			if ( fButtons & fCurrentMouseMap.button[0]) {
				fBox->DrawBitmapAsync(fMouseDownBitmap, BRect(0,0,18,28), BRect(50,47,68,75));
				fBox->SetHighColor(120,120,120);
			} else
				fBox->SetHighColor(184,184,184);
			fBox->StrokeLine(BPoint(67,49),BPoint(67,71),B_SOLID_HIGH);
			
			if (fButtons & fCurrentMouseMap.button[2]) {
				fBox->DrawBitmapAsync(fMouseDownBitmap, BRect(18,0,36,28), BRect(68,47,86,75));
				fBox->SetHighColor(120,120,120);
			} else
				fBox->SetHighColor(184,184,184);
			fBox->StrokeLine(BPoint(85,49),BPoint(85,71),B_SOLID_HIGH);
					
			if (fButtons & fCurrentMouseMap.button[2]) 
				fBox->SetHighColor(120,120,120);
			else
				fBox->SetHighColor(255,255,255);
			fBox->StrokeLine(BPoint(69,49),BPoint(69,71),B_SOLID_HIGH);
					
			if (fButtons & fCurrentMouseMap.button[1]) {
				fBox->DrawBitmapAsync(fMouseDownBitmap, BRect(36,0,54,28), BRect(86,47,104,75));
				fBox->SetHighColor(120,120,120);
			} else
				fBox->SetHighColor(255,255,255);
			fBox->StrokeLine(BPoint(87,49),BPoint(87,71),B_SOLID_HIGH);
			
			if ((fButtons & fCurrentMouseMap.button[0]) || (fButtons & fCurrentMouseMap.button[2]))
				fBox->SetHighColor(48,48,48);
			else
				fBox->SetHighColor(88,88,88);
			fBox->StrokeLine(BPoint(68,48),BPoint(68,72),B_SOLID_HIGH);
			
			if ((fButtons & fCurrentMouseMap.button[2]) || (fButtons & fCurrentMouseMap.button[1]))
				fBox->SetHighColor(48,48,48);
			else
				fBox->SetHighColor(88,88,88);
			fBox->StrokeLine(BPoint(86,48),BPoint(86,72),B_SOLID_HIGH);
			
		}
		
		fBox->SetHighColor(32,32,32);
		fBox->SetFont(be_plain_font);
		for(int32 i=0; i<index+1; i++) {
			char number[2] = "1";
			switch (fCurrentMouseMap.button[i]) {
				case B_PRIMARY_MOUSE_BUTTON: *number = '1'; break;
				case B_SECONDARY_MOUSE_BUTTON: *number = '2'; break;
				case B_TERTIARY_MOUSE_BUTTON: *number = '3'; break;
			}
			int32 offsets[][3] = { 	{75},
							{61, 88},
							{57, 92, 74} }; 
			fBox->MovePenTo(offsets[index][i], 65);
			fBox->DrawString(number);
		}
	}
	
	fBox->Sync();
	
	fBox->SetDrawingMode(B_OP_COPY);
}


void 
MouseView::Init()
{
	int32 value;
	// slow = 1000000, fast = 0
	value = (int32) ((1000000 - fClickSpeed) / 1000);
	if (value > 1000) value = 1000;
	if (value < 0) value = 0;
	dcSpeedSlider->SetValue(value);
	
	// slow = 8192, fast = 524287
	value = (int32) (( log(fMouseSpeed / 8192) / log(2)) * 1000 / 6);  
	if (value > 1000) value = 1000;
	if (value < 0) value = 0;
	mouseSpeedSlider->SetValue(value);
	
	// slow = 0, fast = 262144
	value = (int32) (sqrt(fMouseAcc / 16384) * 1000 / 4);
	if (value > 1000) value = 1000;
	if (value < 0) value = 0;
	mouseAccSlider->SetValue(value);
	
	if (fMouseType < 1)
		fMouseType = 1;
	if (fMouseType > 3)
		fMouseType = 3;
	BMenuItem *item = mouseTypeMenu->ItemAt(fMouseType - 1);
	if (item)
		item->SetMarked(true);
	
	int32 mode;
	switch (fMouseMode) {
		case B_NORMAL_MOUSE: mode = 0; break;
		case B_FOCUS_FOLLOWS_MOUSE: mode = 1; break;
		case B_WARP_MOUSE: mode = 2; break;
		case B_INSTANT_WARP_MOUSE: mode = 3; break;
		default : mode = 0; break;
	}
	item = focusMenu->ItemAt(mode);
	if (item)
		item->SetMarked(true);
			
	printf("click_speed : %lli, mouse_speed : %li, mouse_acc : %li, mouse_type : %li\n", 
		fClickSpeed, fMouseSpeed, fMouseAcc, fMouseType);
}


void 
MouseView::SetRevertable(bool revertable)
{
	revertButton->SetEnabled(revertable);
}

