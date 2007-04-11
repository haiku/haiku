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
	field->SetDivider(field->StringWidth(field->Label()) + kItemSpace);
	field->SetAlignment(B_ALIGN_RIGHT);
	AddChild(field);

	BFont font = be_plain_font;
	float length = font.StringWidth("Mouse type: [3-Button]") + 20;
	fLeftArea.Set(8, 7, length + 8, 198);
	if (fLeftArea.Width() < 125)
		fLeftArea.right = fLeftArea.left + 125;

	fRightArea.Set(fLeftArea.right + 10, 11, 200, 7);

	// Create the "Double-click speed slider...
	fClickSpeedSlider = new BSlider(fRightArea, "double_click_speed", "Double-click speed", 
		new BMessage(kMsgDoubleClickSpeed), 0, 1000);
	fClickSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fClickSpeedSlider->SetHashMarkCount(5);
	fClickSpeedSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fClickSpeedSlider);

	length = fClickSpeedSlider->Bounds().Height() + 6;
	fDoubleClickBmpPoint.y = fRightArea.top + 
		(length - fDoubleClickBitmap->Bounds().Height()) / 2;
	fRightArea.top += length;

	// Create the "Mouse Speed" slider...
	fMouseSpeedSlider = new BSlider(fRightArea, "mouse_speed", "Mouse Speed", 
		new BMessage(kMsgMouseSpeed), 0, 1000);
	fMouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseSpeedSlider->SetHashMarkCount(7);
	fMouseSpeedSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fMouseSpeedSlider);

	fSpeedBmpPoint.y = fRightArea.top + 
		(length - fSpeedBitmap->Bounds().Height()) / 2;
	fRightArea.top += length;

	// Create the "Mouse Acceleration" slider...
	fAccelerationSlider = new BSlider(fRightArea, "mouse_acceleration", "Mouse Acceleration", 
		new BMessage(kMsgAccelerationFactor), 0, 1000);
	fAccelerationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAccelerationSlider->SetHashMarkCount(5);
	fAccelerationSlider->SetLimitLabels("Slow", "Fast");
	AddChild(fAccelerationSlider);

	fAccelerationBmpPoint.y = fRightArea.top + 
		(length - fAccelerationBitmap->Bounds().Height()) / 2;
	fRightArea.top += length - 3;
	
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

	BRect frame(fRightArea.left, fRightArea.top + 10, fRightArea.left +
			font.StringWidth("Focus follows mouse:") + 
			font.StringWidth(focusLabels[0]) + 30, 200);
	field = new BMenuField(frame, "ffm", "Focus follows mouse:", fFocusMenu, true);
	field->SetDivider(field->StringWidth(field->Label()) + kItemSpace);
	field->SetAlignment(B_ALIGN_RIGHT);
	AddChild(field);

	// Finalize the areas
	fRightArea.bottom = fRightArea.top;
	fRightArea.top = 11;
	fRightArea.right = frame.right + 8;
	if (fRightArea.Width() < 200)
		fRightArea.right = fRightArea.left + 200;
	fLeftArea.bottom = fRightArea.bottom;

	// Position mouse bitmaps
	fDoubleClickBmpPoint.x = fRightArea.right - fDoubleClickBitmap->Bounds().right - 15; 
	fSpeedBmpPoint.x = fRightArea.right - fSpeedBitmap->Bounds().right - 15;
	fAccelerationBmpPoint.x = fRightArea.right - fAccelerationBitmap->Bounds().right - 15;

	// Resize sliders to equal size
	length = fRightArea.left - 5;
	fClickSpeedSlider->ResizeTo(fDoubleClickBmpPoint.x - length - 11, 
			fClickSpeedSlider->Bounds().Height());
	fMouseSpeedSlider->ResizeTo(fSpeedBmpPoint.x - length, 
			fMouseSpeedSlider->Bounds().Height());
	fAccelerationSlider->ResizeTo(fAccelerationBmpPoint.x - length, 
			fAccelerationSlider->Bounds().Height());

	// Mouse image...
	frame.Set(0, 0, 148, 162);
	fMouseView = new MouseView(frame, fSettings);
	fMouseView->ResizeToPreferred();
	fMouseView->MoveBy((fLeftArea.right - fMouseView->Bounds().Width()) / 2,
		(fLeftArea.bottom - fMouseView->Bounds().Height()) / 2);
	AddChild(fMouseView);

	// Create the "Double-click test area" text box...
	frame.Set(fLeftArea.left, fLeftArea.bottom + 10, fLeftArea.right, 0);
	BTextControl *textControl = new BTextControl(frame, "double_click_test_area", NULL,
		"Double-click test area", NULL);
	textControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
	AddChild(textControl);

	ResizeTo(fRightArea.right + 5, fLeftArea.bottom + 60);
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
	if (_width)
		*_width = fRightArea.right + 5;
	if (_height)
		*_height = fLeftArea.bottom + 60;
}


void 
SettingsView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

	SetHighColor(120, 120, 120);
	SetLowColor(255, 255, 255);
	
	// Line above the test area
	fLeft = fLeftArea.LeftBottom();
	fRight = fLeftArea.RightBottom();
	StrokeLine(fLeft, fRight, B_SOLID_HIGH);
	fLeft.y++; fRight.y++;
	StrokeLine(fLeft, fRight, B_SOLID_LOW);
	
	// Line above focus follows mouse
	fLeft = fRightArea.LeftBottom();
	fRight = fRightArea.RightBottom();
	StrokeLine(fLeft, fRight, B_SOLID_HIGH);
	fLeft.y++; fRight.y++;
	StrokeLine(fLeft, fRight, B_SOLID_LOW);
	
	// Line in the middle
	fLeft = fLeftArea.RightTop();
	fRight = fLeftArea.RightBottom();
	fLeft.x += 5;
	fRight.x += 5;
	StrokeLine(fLeft, fRight, B_SOLID_HIGH);
	fLeft.x++; fRight.x++;
	StrokeLine(fLeft, fRight, B_SOLID_LOW);

	SetDrawingMode(B_OP_OVER);

	// Draw the icons
	if (fDoubleClickBitmap != NULL 
		&& updateFrame.Intersects(BRect(fDoubleClickBmpPoint,
			fDoubleClickBmpPoint + fDoubleClickBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fDoubleClickBitmap, fDoubleClickBmpPoint);

	if (fSpeedBitmap != NULL 
		&& updateFrame.Intersects(BRect(fSpeedBmpPoint,
			fSpeedBmpPoint + fSpeedBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fSpeedBitmap, fSpeedBmpPoint);
	
	if (fAccelerationBitmap != NULL 
		&& updateFrame.Intersects(BRect(fAccelerationBmpPoint,
			fAccelerationBmpPoint + fAccelerationBitmap->Bounds().RightBottom())))
		DrawBitmapAsync(fAccelerationBitmap, fAccelerationBmpPoint);
	

	SetDrawingMode(B_OP_COPY);
}


void 
SettingsView::SetMouseType(int32 type)
{
	fMouseView->SetMouseType(type);
}


void
SettingsView::MouseMapUpdated()
{
	fMouseView->MouseMapUpdated();
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

