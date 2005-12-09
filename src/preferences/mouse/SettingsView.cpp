// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			SettingsView.cpp
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
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

#include "SettingsView.h"
#include "MouseConstants.h"
#include "MouseSettings.h"
#include "MouseView.h"


static int32
mouse_mode_to_index(mode_mouse mode)
{
	switch (mode) {
		case B_NORMAL_MOUSE:
		default:
			return 0;
		case B_FOCUS_FOLLOWS_MOUSE:
			return 1;
		case B_WARP_MOUSE:
			return 2;
		case B_INSTANT_WARP_MOUSE:
			return 3;
	}
}


//	#pragma mark -


SettingsView::SettingsView(BRect rect, MouseSettings &settings)
	: BBox(rect, "main_view"),
	fSettings(settings)
{
	ResizeToPreferred();

	fDoubleClickBitmap = BTranslationUtils::GetBitmap("double_click_bmap");
	fSpeedBitmap = BTranslationUtils::GetBitmap("speed_bmap");
	fAccelerationBitmap = BTranslationUtils::GetBitmap("acceleration_bmap");

	// Add the "Mouse Type" pop up menu
	fTypeMenu = new BPopUpMenu("unknown");
	fTypeMenu->AddItem(new BMenuItem("1-Button", new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem("2-Button", new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem("3-Button", new BMessage(kMsgMouseType)));

	BMenuField *field = new BMenuField(BRect(7, 8, 155, 190), "mouse_type", "Mouse type:", fTypeMenu);
	field->ResizeToPreferred();
	field->SetAlignment(B_ALIGN_RIGHT);
	AddChild(field);

	fMouseView = new MouseView(BRect(7, 28, 155, 190), fSettings);
	fMouseView->ResizeToPreferred();
	fMouseView->MoveBy((142 - fMouseView->Bounds().Width()) / 2,
		(108 - fMouseView->Bounds().Height()) / 2);
	AddChild(fMouseView);

	// Add the "Focus follows mouse" pop up menu
	fFocusMenu = new BPopUpMenu("Disabled");
	
	const char *focusLabels[] = {"Disabled", "Enabled", "Warping", "Instant-Warping"};
	const mode_mouse focusModes[] = {B_NORMAL_MOUSE, B_FOCUS_FOLLOWS_MOUSE,
		B_WARP_MOUSE, B_INSTANT_WARP_MOUSE};

	for (int i = 0; i < 4; i++) {
		BMessage *message = new BMessage(kMsgMouseFocusMode);
		message->AddInt32("mode", focusModes[i]);

		fFocusMenu->AddItem(new BMenuItem(focusLabels[i], message));
	}

	BRect frame(165, 208, 440, 200);
	field = new BMenuField(frame, "ffm", "Focus follows mouse:", fFocusMenu);
	field->SetDivider(field->StringWidth(field->Label()) + kItemSpace);
	field->SetAlignment(B_ALIGN_RIGHT);
	AddChild(field);

	// Create the "Double-click speed slider...
	frame.Set(166, 11, 328, 50);
	fClickSpeedSlider = new BSlider(frame, "double_click_speed", "Double-click speed", 
		new BMessage(kMsgDoubleClickSpeed), 0, 1000);
	fClickSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fClickSpeedSlider->SetHashMarkCount(5);
	fClickSpeedSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fClickSpeedSlider);

	// Create the "Mouse Speed" slider...
	frame.Set(166,76,328,125);
	fMouseSpeedSlider = new BSlider(frame, "mouse_speed", "Mouse Speed", 
		new BMessage(kMsgMouseSpeed), 0, 1000);
	fMouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseSpeedSlider->SetHashMarkCount(7);
	fMouseSpeedSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fMouseSpeedSlider);

	// Create the "Mouse Acceleration" slider...
	frame.Set(166, 141, 328, 190);
	fAccelerationSlider = new BSlider(frame, "mouse_acceleration", "Mouse Acceleration", 
		new BMessage(kMsgAccelerationFactor), 0, 1000);
	fAccelerationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAccelerationSlider->SetHashMarkCount(5);
	fAccelerationSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fAccelerationSlider);

	// Create the "Double-click test area" text box...
	frame = Bounds();
	frame.left = frame.left + 9;
	frame.right = frame.right - 230;
	frame.top = frame.bottom - 30;
	BTextControl *textControl = new BTextControl(frame, "double_click_test_area", NULL,
		"Double-click test area", NULL);
	textControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
	AddChild(textControl);
}


SettingsView::~SettingsView()
{
	delete fDoubleClickBitmap;
	delete fSpeedBitmap;
	delete fAccelerationBitmap;
}


void
SettingsView::AttachedToWindow()
{
	UpdateFromSettings();
}


void 
SettingsView::GetPreferredSize(float *_width, float *_height)
{
	// ToDo: fixed values for now
	if (_width)
		*_width = 397 - 22;
	if (_height)
		*_height = 293 - 55;
}


void 
SettingsView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

	SetHighColor(120, 120, 120);
	SetLowColor(255, 255, 255);
	// Line above the test area
	StrokeLine(BPoint(8, 198), BPoint(149, 198), B_SOLID_HIGH);
	StrokeLine(BPoint(8, 199), BPoint(149, 199), B_SOLID_LOW);
	// Line above focus follows mouse
	StrokeLine(BPoint(164, 198), BPoint(367, 198), B_SOLID_HIGH);
	StrokeLine(BPoint(164, 199), BPoint(367, 199), B_SOLID_LOW);
	// Line in the middle
	StrokeLine(BPoint(156, 10), BPoint(156, 230), B_SOLID_HIGH);
	StrokeLine(BPoint(157, 11), BPoint(157, 230), B_SOLID_LOW);

	SetDrawingMode(B_OP_OVER);

	// Draw the icons
	BPoint doubleClickBmpPoint(344, 16);
	if (fDoubleClickBitmap != NULL 
		&& updateFrame.Intersects(BRect(doubleClickBmpPoint,
			doubleClickBmpPoint + fDoubleClickBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fDoubleClickBitmap, doubleClickBmpPoint);

	BPoint speedBmpPoint(333, 90);
	if (fSpeedBitmap != NULL 
		&& updateFrame.Intersects(BRect(speedBmpPoint,
			speedBmpPoint + fSpeedBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fSpeedBitmap, speedBmpPoint);
	
	BPoint accelerationBmpPoint(333, 155);
	if (fAccelerationBitmap != NULL 
		&& updateFrame.Intersects(BRect(accelerationBmpPoint,
			accelerationBmpPoint + fAccelerationBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fAccelerationBitmap, accelerationBmpPoint);
	

	SetDrawingMode(B_OP_COPY);
}


void 
SettingsView::SetMouseType(int32 type)
{
	fMouseView->SetMouseType(type);
}


void 
SettingsView::UpdateFromSettings()
{
	int32 value = int32(fSettings.ClickSpeed() / 1000);
		// slow = 1000000, fast = 0
	fClickSpeedSlider->SetValue(value);

	value = int32((log(fSettings.MouseSpeed() / 8192) / log(2)) * 1000 / 6);  
		// slow = 8192, fast = 524287
	fMouseSpeedSlider->SetValue(value);

	value = int32(sqrt(fSettings.AccelerationFactor() / 16384) * 1000 / 4);
		// slow = 0, fast = 262144
	fAccelerationSlider->SetValue(value);

	BMenuItem *item = fTypeMenu->ItemAt(fSettings.MouseType() - 1);
	if (item != NULL)
		item->SetMarked(true);

	fMouseView->SetMouseType(fSettings.MouseType());

	item = fFocusMenu->ItemAt(mouse_mode_to_index(fSettings.MouseMode()));
	if (item != NULL)
		item->SetMarked(true);
}

