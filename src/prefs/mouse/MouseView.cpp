// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseView.cpp
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

#include "MouseView.h"
#include "MouseWindow.h"
#include "MouseConstants.h"
#include "MouseBitmap.h"
#include "MouseSettings.h"


static const int32 kOneButtonOffsets[4] = {0, 55};
static const int32 kTwoButtonOffsets[4] = {0, 27, 55};
static const int32 kThreeButtonOffsets[4] = {0, 18, 36, 55};

static const int32 kButtonTop = 6;

static const rgb_color kButtonPressedColor = {120, 120, 120};
static const rgb_color kButtonReleasedLeftColor = {255, 255, 255};
static const rgb_color kButtonReleasedRightColor = {184, 184, 184};
static const rgb_color kButtonPressedSeparatorColor = {48, 48, 48};
static const rgb_color kButtonReleasedSeparatorColor = {88, 88, 88};
static const rgb_color kButtonTextColor = {32, 32, 32};


static const int32 *
getButtonOffsets(int32 type)
{
	switch (type) {
		case 1:
			return kOneButtonOffsets;
		case 2:
			return kTwoButtonOffsets;
		case 3:
		default:
			return kThreeButtonOffsets;
	}
}


static uint32
getMappingNumber(int32 mapping)
{
	switch (mapping) {
		case B_PRIMARY_MOUSE_BUTTON:
			return 0;
		case B_SECONDARY_MOUSE_BUTTON:
			return 1;
		case B_TERTIARY_MOUSE_BUTTON:
			return 2;
	}
	return 0;
}


//	#pragma mark -


MouseView::MouseView(BRect rect, const MouseSettings &settings)
	: BView(rect, "mouse_view", B_FOLLOW_ALL, B_PULSE_NEEDED | B_WILL_DRAW),
	fSettings(settings),
	fType(-1),
	fButtons(0),
	fOldButtons(0)
{
	fMouseBitmap = new BBitmap(BRect(0, 0, kMouseWidth - 1, kMouseHeight - 1), B_CMAP8);
	fMouseBitmap->SetBits(kMouseBits, sizeof(kMouseBits), 0, kMouseColorSpace);

	fMouseDownBitmap = new BBitmap(BRect(0, 0, kMouseDownWidth - 1, kMouseDownHeight - 1), B_CMAP8);
	fMouseDownBitmap->SetBits(kMouseDownBits, sizeof(kMouseDownBits), 0, kMouseDownColorSpace);

	// ToDo: move these images to the resources as well
	//fMouseBitmap = BTranslationUtils::GetBitmap('PNG ', "mouse_bmap");
	//fMouseDownBitmap = BTranslationUtils::GetBitmap('PNG ', "pressed_mouse_bmap");

  	if (fMouseDownBitmap != NULL)
  		fMouseDownBounds = fMouseDownBitmap->Bounds();
  	else
  		fMouseDownBounds.Set(0, 0, 55, 29);
}


MouseView::~MouseView()
{
	delete fMouseBitmap;
	delete fMouseDownBitmap;
}


void 
MouseView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = fMouseBitmap != NULL ? fMouseBitmap->Bounds().Width() : 57;
	if (_height)
		*_height = fMouseBitmap != NULL ? fMouseBitmap->Bounds().Height() : 82;
}


void
MouseView::MouseDown(BPoint where)
{
	const int32 *offset = getButtonOffsets(fType);
	int32 button = -1;
	for (int32 i = 0; i <= fType; i++) {
		BRect frame(offset[i], kButtonTop, offset[i + 1] - 1,
			kButtonTop + fMouseDownBounds.Height());
		if (frame.Contains(where)) {
			button = i;
			break;
		}
	}
	if (button < 0)
		return;

	button = ConvertFromVisualOrder(button);

	BPopUpMenu menu("Mouse Map Menu");
	BMessage message(kMsgMouseMap);
	message.AddInt32("button", button);

	menu.AddItem(new BMenuItem("1", new BMessage(message)));
	menu.AddItem(new BMenuItem("2", new BMessage(message)));
	menu.AddItem(new BMenuItem("3", new BMessage(message)));

	menu.ItemAt(getMappingNumber(fSettings.Mapping(button)))->SetMarked(true);
	menu.SetTargetForItems(Window());

	ConvertToScreen(&where);
	menu.Go(where, true);

	Invalidate();
}


void
MouseView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	UpdateFromSettings();
}


void
MouseView::Pulse()
{
	BPoint point;
	GetMouse(&point, &fButtons, true);

	if (fOldButtons != fButtons) {
		Invalidate(BRect(0, kButtonTop,
			fMouseDownBounds.Width(), kButtonTop + fMouseDownBounds.Height()));
		fOldButtons = fButtons;
	}
}


void 
MouseView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

	// Draw the mouse top
	SetDrawingMode(B_OP_ALPHA);
	if (fMouseBitmap != NULL)
		DrawBitmapAsync(fMouseBitmap, updateFrame, updateFrame);

	mouse_map map;
	fSettings.Mapping(map);

	const int32 *offset = getButtonOffsets(fType);
	bool middlePressed = fType == 3 && (map.button[2] & fButtons) != 0;

	for (int32 i = 0; i < fType; i++) {
		BRect border(offset[i] + 1, kButtonTop + 2, offset[i + 1] - 1,
			kButtonTop + fMouseDownBounds.Height() - 4);
		bool pressed = (fButtons & map.button[ConvertFromVisualOrder(i)]) != 0;
			// is button currently pressed?

		if (pressed) {
			BRect frame(offset[i], 0, offset[i + 1], fMouseDownBounds.Height() + 1);
			if (fMouseDownBitmap != NULL)
				DrawBitmapAsync(fMouseDownBitmap, frame, frame.OffsetByCopy(0, kButtonTop));
			else if (fMouseBitmap == NULL) {
				SetHighColor(kButtonPressedColor);
				SetDrawingMode(B_OP_OVER);
				FillRect(frame.OffsetByCopy(0, kButtonTop));
			}
		}
		SetDrawingMode(B_OP_OVER);

		if (i > 0 && fType > i) {
			// left border
			SetHighColor(pressed ? kButtonPressedColor : kButtonReleasedLeftColor);
			StrokeLine(BPoint(border.LeftTop()), BPoint(border.LeftBottom()));

			// draw separator
			BRect separator = border.OffsetByCopy(-1, -1);
			separator.bottom += 2;
			if (!middlePressed)
				border.top++;

			SetHighColor(middlePressed ?
				kButtonPressedSeparatorColor : kButtonReleasedSeparatorColor);
			StrokeLine(BPoint(separator.LeftTop()), BPoint(separator.LeftBottom()));			
		}

		if (fType > 1 && i + 1 < fType) {
			// right border
			SetHighColor(pressed ? kButtonPressedColor : kButtonReleasedRightColor);
			StrokeLine(BPoint(border.RightTop()), BPoint(border.RightBottom()));
		}

		// draw mapping number centered over the button

		SetHighColor(kButtonTextColor);

		char number[2] = {0};
		number[0] = getMappingNumber(map.button[ConvertFromVisualOrder(i)]) + '1';

		DrawString(number,
			BPoint(border.left + (border.Width() - StringWidth(number)) / 2, kButtonTop + 18));
	}

	Sync();
	SetDrawingMode(B_OP_COPY);
}


int32
MouseView::ConvertFromVisualOrder(int32 i)
{
	if (fType < 3)
		return i;

	switch (i) {
		case 0:
		default:
			return 0;
		case 1:
			return 2;
		case 2:
			return 1;
	}
}


void 
MouseView::SetMouseType(int32 type)
{
	fType = type;
	Invalidate();
}


void 
MouseView::UpdateFromSettings()
{
	SetMouseType(fSettings.MouseType());
}

