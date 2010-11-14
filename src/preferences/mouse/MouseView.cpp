/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Philippe Saint-Pierre stpere@gmail.com
 */


#include "MouseView.h"

#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Slider.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Window.h>

#include "MouseConstants.h"
#include "MouseSettings.h"
#include "MouseWindow.h"

static const int32 kButtonTop = 6;
static const int32 kMouseDownWidth = 72;
static const int32 kMouseDownHeight = 30;

static const int32 kOneButtonOffsets[4]
	= { 0, kMouseDownWidth };
static const int32 kTwoButtonOffsets[4]
	= { 0, kMouseDownWidth / 2, kMouseDownWidth };
static const int32 kThreeButtonOffsets[4]
	= { 0, kMouseDownWidth / 3, kMouseDownWidth / 3 * 2, kMouseDownWidth };

static const rgb_color kButtonPressedColor = { 120, 120, 120 };
static const rgb_color kButtonReleasedLeftColor = { 255, 255, 255 };
static const rgb_color kButtonReleasedRightColor = { 184, 184, 184 };
static const rgb_color kButtonPressedSeparatorColor = { 48, 48, 48 };
static const rgb_color kButtonReleasedSeparatorColor = { 88, 88, 88 };
static const rgb_color kButtonTextColor = { 32, 32, 32 };
static const rgb_color kMouseShadowColor = { 150, 150, 150 };
static const rgb_color kMouseBodyTopColor = { 210, 210, 210 };
static const rgb_color kMouseBodyBottomColor = { 140, 140, 140 };
static const rgb_color kMouseOutlineColor = { 0, 0, 0 };

static const int32*
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


MouseView::MouseView(const MouseSettings &settings)
	:
	BView("mouse_view", B_PULSE_NEEDED | B_WILL_DRAW),
	fSettings(settings),
	fType(-1),
	fButtons(0),
	fOldButtons(0)
{
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


MouseView::~MouseView()
{
}


void
MouseView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = kMouseDownWidth + 2;
	if (_height)
		*_height = 100;
}


void
MouseView::MouseUp(BPoint)
{
	fButtons = 0;
	Invalidate(BRect(0, kButtonTop, kMouseDownWidth,
		kButtonTop + kMouseDownHeight));
	fOldButtons = fButtons;
}


void
MouseView::MouseDown(BPoint where)
{
	BMessage *mouseMsg = Window()->CurrentMessage();
	fButtons = mouseMsg->FindInt32("buttons");
	int32 modifiers = mouseMsg->FindInt32("modifiers");
	if (modifiers & B_CONTROL_KEY) {
		if (modifiers & B_COMMAND_KEY)
			fButtons = B_TERTIARY_MOUSE_BUTTON;
		else
			fButtons = B_SECONDARY_MOUSE_BUTTON;
	}

	// Get the current clipping region before requesting any updates.
	// Otherwise those parts would be excluded from the region.
	BRegion clipping;
	GetClippingRegion(&clipping);

	if (fOldButtons != fButtons) {
		Invalidate(BRect(0, kButtonTop, kMouseDownWidth, kButtonTop
			+ kMouseDownHeight));
		fOldButtons = fButtons;
	}

	const int32* offset = getButtonOffsets(fType);
	int32 button = -1;
	for (int32 i = 0; i <= fType; i++) {
		BRect frame(offset[i], kButtonTop, offset[i + 1] - 1,
			kButtonTop + kMouseDownHeight);
		if (frame.Contains(where)) {
			button = i;
			break;
		}
	}
	if (button < 0)
		return;

	// We are setup to receive all mouse events, even if our window
	// is not active, so make sure that we don't display the menu when
	// the user clicked inside our view, but another window is on top.
	if (clipping.Contains(where)) {
		button = _ConvertFromVisualOrder(button);

		BPopUpMenu menu("Mouse Map Menu");
		BMessage message(kMsgMouseMap);
		message.AddInt32("button", button);

		menu.AddItem(new BMenuItem("1", new BMessage(message)));
		menu.AddItem(new BMenuItem("2", new BMessage(message)));
		menu.AddItem(new BMenuItem("3", new BMessage(message)));

		menu.ItemAt(getMappingNumber(fSettings.Mapping(button)))
			->SetMarked(true);
		menu.SetTargetForItems(Window());

		ConvertToScreen(&where);
		menu.Go(where, true);
	}
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
MouseView::Draw(BRect updateFrame)
{
	BRect mouseBody(Bounds());
	mouseBody.right -= 2;
	mouseBody.bottom -= 2;
	float radius = 10;

	// Draw the shadow
	SetHighColor(kMouseShadowColor);
	FillRoundRect(mouseBody.OffsetByCopy(2, 2), radius, radius);

	// Draw the body
	BGradientLinear gradient(mouseBody.LeftTop(), mouseBody.LeftBottom());
	gradient.AddColor(kMouseBodyTopColor, 0);
	gradient.AddColor(kMouseBodyBottomColor, 255);
	FillRoundRect(mouseBody, radius, radius, gradient);

	// Draw the outline
	SetHighColor(kMouseOutlineColor);
	StrokeRoundRect(mouseBody, radius, radius);

	mouse_map map;
	fSettings.Mapping(map);

	const int32* offset = getButtonOffsets(fType);
	bool middlePressed = fType == 3 && (map.button[2] & fButtons) != 0;

	for (int32 i = 0; i < fType; i++) {
		BRect border(offset[i] + 1, kButtonTop + 2, offset[i + 1] - 1,
			kButtonTop + kMouseDownHeight - 4);
		bool pressed = (fButtons & map.button[_ConvertFromVisualOrder(i)]) != 0;
			// is button currently pressed?

		if (pressed) {
			BRect frame(offset[i], 0, offset[i + 1], kMouseDownHeight - 1);
			frame.InsetBy(1, 1);
			SetHighColor(kButtonPressedColor);
			FillRect(frame.OffsetByCopy(0, kButtonTop));
		}

		SetDrawingMode(B_OP_OVER);

		if (i > 0 && fType > i) {
			// left border
			SetHighColor(pressed ?
				kButtonPressedColor : kButtonReleasedLeftColor);
			StrokeLine(BPoint(border.LeftTop()), BPoint(border.LeftBottom()));

			// draw separator
			BRect separator = border.OffsetByCopy(-1, -1);
			separator.bottom += 2;
			if (!middlePressed)
				border.top++;

			SetHighColor(middlePressed ?
				kButtonPressedSeparatorColor : kButtonReleasedSeparatorColor);
			StrokeLine(BPoint(separator.LeftTop()),
				BPoint(separator.LeftBottom()));
		}

		if (fType > 1 && i + 1 < fType) {
			// right border
			SetHighColor(pressed ?
				kButtonPressedColor : kButtonReleasedRightColor);
			StrokeLine(BPoint(border.RightTop()),
				BPoint(border.RightBottom()));
		}

		// draw mapping number centered over the button

		SetHighColor(kButtonTextColor);

		char number[2] = {0};
		number[0] = getMappingNumber(map.button[_ConvertFromVisualOrder(i)])
			+ '1';

		DrawString(number, BPoint(border.left +
			(border.Width() - StringWidth(number)) / 2, kButtonTop + 18));
	}
}


int32
MouseView::_ConvertFromVisualOrder(int32 i)
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
MouseView::MouseMapUpdated()
{
	Invalidate();
}


void
MouseView::UpdateFromSettings()
{
	SetMouseType(fSettings.MouseType());
}

