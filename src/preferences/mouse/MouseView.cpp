/*
 * Copyright 2003-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Philippe Saint-Pierre stpere@gmail.com
 */


#include "MouseView.h"

#include <algorithm>

#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Shape.h>
#include <Slider.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <Window.h>

#include "MouseConstants.h"
#include "MouseSettings.h"
#include "MouseWindow.h"


static const int32 kButtonTop = 3;
static const int32 kMouseDownWidth = 72;
static const int32 kMouseDownHeight = 35;

static const int32 kOneButtonOffsets[4]
	= { 0, kMouseDownWidth };
static const int32 kTwoButtonOffsets[4]
	= { 0, kMouseDownWidth / 2, kMouseDownWidth };
static const int32 kThreeButtonOffsets[4]
	= { 0, kMouseDownWidth / 3 + 1, kMouseDownWidth / 3 * 2, kMouseDownWidth };

static const rgb_color kButtonTextColor = { 0, 0, 0, 255 };
static const rgb_color kMouseShadowColor = { 100, 100, 100, 128 };
static const rgb_color kMouseBodyTopColor = { 0xed, 0xed, 0xed, 255 };
static const rgb_color kMouseBodyBottomColor = { 0x85, 0x85, 0x85, 255 };
static const rgb_color kMouseOutlineColor = { 0x51, 0x51, 0x51, 255 };
static const rgb_color kMouseButtonOutlineColor = { 0xa0, 0xa0, 0xa0, 255 };
static const rgb_color kButtonPressedColor = { 110, 110, 110, 110 };


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
	fScaling = std::max(1.0f, be_plain_font->Size() / 12.0f);
}


MouseView::~MouseView()
{
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


void
MouseView::GetPreferredSize(float* _width, float* _height)
{
	if (_width != NULL)
		*_width = fScaling * (kMouseDownWidth + 2);
	if (_height != NULL)
		*_height = fScaling * 104;
}


void
MouseView::AttachedToWindow()
{
	AdoptParentColors();

	UpdateFromSettings();
	_CreateButtonsPicture();

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	fDigitHeight = int32(ceilf(fontHeight.ascent) + ceilf(fontHeight.descent));
	fDigitBaseline = int32(ceilf(fontHeight.ascent));
}


void
MouseView::MouseUp(BPoint)
{
	fButtons = 0;
	Invalidate(_ButtonsRect());
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
		Invalidate(_ButtonsRect());
		fOldButtons = fButtons;
	}

	const int32* offset = getButtonOffsets(fType);
	int32 button = -1;
	for (int32 i = 0; i <= fType; i++) {
		if (_ButtonRect(offset, i).Contains(where)) {
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
MouseView::Draw(BRect updateFrame)
{
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	SetScale(fScaling * 1.8);

	BShape mouseShape;
	mouseShape.MoveTo(BPoint(16, 12));
	// left
	BPoint control[3] = { BPoint(12, 16), BPoint(8, 64), BPoint(32, 64) };
	mouseShape.BezierTo(control);
	// right
	BPoint control2[3] = { BPoint(56, 64), BPoint(52, 16), BPoint(48, 12) };
	mouseShape.BezierTo(control2);
	// top
	BPoint control3[3] = { BPoint(44, 8), BPoint(20, 8), BPoint(16, 12) };
	mouseShape.BezierTo(control3);
	mouseShape.Close();

	// Draw the shadow
	SetOrigin(-17 * fScaling, -11 * fScaling);
	SetHighColor(kMouseShadowColor);
	FillShape(&mouseShape, B_SOLID_HIGH);

	// Draw the body
	SetOrigin(-21 * fScaling, -14 * fScaling);
	BGradientRadial bodyGradient(28, 24, 128);
	bodyGradient.AddColor(kMouseBodyTopColor, 0);
	bodyGradient.AddColor(kMouseBodyBottomColor, 255);

	FillShape(&mouseShape, bodyGradient);

	// Draw the outline
	SetPenSize(1 / 1.8 / fScaling);
	SetDrawingMode(B_OP_OVER);
	SetHighColor(kMouseOutlineColor);

	StrokeShape(&mouseShape, B_SOLID_HIGH);

	// bottom button border
	BShape buttonsOutline;
	buttonsOutline.MoveTo(BPoint(13, 27));
	BPoint control4[] = { BPoint(18, 30), BPoint(46, 30), BPoint(51, 27) };
	buttonsOutline.BezierTo(control4);

	SetHighColor(kMouseButtonOutlineColor);
	StrokeShape(&buttonsOutline, B_SOLID_HIGH);

	SetScale(1);
	SetOrigin(0, 0);

	// Separator between the buttons
	const int32* offset = getButtonOffsets(fType);
	for (int32 i = 1; i < fType; i++) {
		BRect buttonRect = _ButtonRect(offset, i);
		StrokeLine(buttonRect.LeftTop(), buttonRect.LeftBottom());
	}

	mouse_map map;
	fSettings.Mapping(map);

	SetDrawingMode(B_OP_OVER);

	if (fButtons != 0)
		ClipToPicture(&fButtonsPicture, B_ORIGIN, false);

	for (int32 i = 0; i < fType; i++) {
		// draw mapping number centered over the button

		bool pressed = (fButtons & map.button[_ConvertFromVisualOrder(i)]) != 0;
			// is button currently pressed?
		if (pressed) {
			SetDrawingMode(B_OP_ALPHA);
			SetHighColor(kButtonPressedColor);
			FillRect(_ButtonRect(offset, i));
		}

		BRect border(fScaling * (offset[i] + 1), fScaling * (kButtonTop + 5),
			fScaling * offset[i + 1] - 1,
			fScaling * (kButtonTop + kMouseDownHeight - 4));
		if (i == 0)
			border.left += fScaling * 5;
		if (i == fType - 1)
			border.right -= fScaling * 4;

		char number[2] = {0};
		number[0] = getMappingNumber(map.button[_ConvertFromVisualOrder(i)])
			+ '1';

		SetDrawingMode(B_OP_OVER);
		SetHighColor(kButtonTextColor);
		DrawString(number, BPoint(
			border.left + (border.Width() - StringWidth(number)) / 2,
			border.top + fDigitBaseline
				+ (border.IntegerHeight() - fDigitHeight) / 2));
	}

	if (fButtons != 0)
		ClipToPicture(NULL);
}


BRect
MouseView::_ButtonsRect() const
{
	return BRect(0, fScaling * kButtonTop, fScaling * kMouseDownWidth,
			fScaling * (kButtonTop + kMouseDownHeight));
}


BRect
MouseView::_ButtonRect(const int32* offsets, int index) const
{
	return BRect(fScaling * offsets[index], fScaling * kButtonTop,
		fScaling * offsets[index + 1] - 1,
		fScaling * (kButtonTop + kMouseDownHeight));
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
MouseView::_CreateButtonsPicture()
{
	BeginPicture(&fButtonsPicture);
	SetScale(1.8 * fScaling);
	SetOrigin(-21 * fScaling, -14 * fScaling);

	BShape mouseShape;
	mouseShape.MoveTo(BPoint(48, 12));
	// top
	BPoint control3[3] = { BPoint(44, 8), BPoint(20, 8), BPoint(16, 12) };
	mouseShape.BezierTo(control3);
	// left
	BPoint control[3] = { BPoint(12, 16), BPoint(13, 27), BPoint(13, 27) };
	mouseShape.BezierTo(control);
	// bottom
	BPoint control4[] = { BPoint(18, 30), BPoint(46, 30), BPoint(51, 27) };
	mouseShape.BezierTo(control4);
	// right
	BPoint control2[3] = { BPoint(51, 27), BPoint(50, 14), BPoint(48, 12) };
	mouseShape.BezierTo(control2);

	mouseShape.Close();

	SetHighColor(255, 0, 0, 255);
	FillShape(&mouseShape, B_SOLID_HIGH);

	EndPicture();
	SetScale(1);
}
