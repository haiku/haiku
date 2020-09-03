/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
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

#include "InputConstants.h"
#include "MouseSettings.h"


static const int32 kButtonTop = 3;
static const int32 kMouseDownWidth = 72;
static const int32 kMouseDownHeight = 35;

#define W kMouseDownWidth / 100
static const int32 kButtonOffsets[][7] = {
	{ 0, 100 * W },
	{ 0, 50 * W, 100 * W },
	{ 0, 35 * W, 65 * W, 100 * W },
	{ 0, 25 * W, 50 * W, 75 * W, 100 * W },
	{ 0, 20 * W, 40 * W, 60 * W, 80 * W, 100 * W },
	{ 0, 19 * W, 34 * W, 50 * W, 66 * W, 82 * W, 100 * W }
};
#undef W

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
	if ((type - 1) >= (int32)B_COUNT_OF(kButtonOffsets))
		return kButtonOffsets[2];
	return kButtonOffsets[type - 1];
}


static uint32
getMappingNumber(uint32 mapping)
{
	if (mapping == 0)
		return 0;

	int i;
	for (i = 0; mapping != 1; i++)
		mapping >>= 1;

	return i;
}

MouseView::MouseView(const MouseSettings &settings)
	:
	BView("Mouse", B_PULSE_NEEDED | B_WILL_DRAW),
	fSettings(settings),
	fType(-1),
	fButtons(0),
	fOldButtons(0)
{
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	fScaling = std::max(1.0f, be_plain_font->Size() / 7.0f);
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
	if (fSettings.MouseType() > 6)
		debugger("Mouse type is invalid");
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

	if (clipping.Contains(where)) {
		button = _ConvertFromVisualOrder(button);

		BPopUpMenu menu("Mouse Map Menu");
		BMessage message(kMsgMouseMap);
		message.AddInt32("button", button);

		for (int i = 1; i < 7; i++) {
			char tmp[2];
			sprintf(tmp, "%d", i);
			menu.AddItem(new BMenuItem(tmp, new BMessage(message)));
		}

		int32 mapping = fSettings.Mapping(button);
		BMenuItem* item = menu.ItemAt(getMappingNumber(mapping));
		if (item)
			item->SetMarked(true);
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

	mouse_map map;
	fSettings.Mapping(map);

	SetDrawingMode(B_OP_OVER);

	// All button drawing is clipped to the outline of the buttons area,
	// simplifying the code below as it can overdraw things.
	ClipToPicture(&fButtonsPicture, B_ORIGIN, false);

	// Separator between the buttons
	const int32* offset = getButtonOffsets(fType);
	for (int32 i = 1; i < fType; i++) {
		BRect buttonRect = _ButtonRect(offset, i);
		StrokeLine(buttonRect.LeftTop(), buttonRect.LeftBottom());
	}

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

		char label[2] = {0};
		int32 number = getMappingNumber(map.button[_ConvertFromVisualOrder(i)]);
		label[0] = number + '1';

		SetDrawingMode(B_OP_OVER);
		SetHighColor(kButtonTextColor);
		DrawString(label, BPoint(
			border.left + (border.Width() - StringWidth(label)) / 2,
			border.top + fDigitBaseline
				+ (border.IntegerHeight() - fDigitHeight) / 2));
	}

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


/** The buttons on a mouse are normally 1 (left), 2 (right), 3 (middle) so
 * we need to reorder them */
int32
MouseView::_ConvertFromVisualOrder(int32 i)
{
	if (fType < 3)
		return i;

	switch (i) {
		case 0:
			return 0;
		case 1:
			return 2;
		case 2:
			return 1;
		default:
			return i;
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
	BPoint control4[3] = { BPoint(18, 30), BPoint(46, 30), BPoint(51, 27) };
	mouseShape.BezierTo(control4);
	// right
	BPoint control2[3] = { BPoint(51, 27), BPoint(50, 14), BPoint(48, 12) };
	mouseShape.BezierTo(control2);

	mouseShape.Close();

	SetHighColor(255, 0, 0, 255);
	FillShape(&mouseShape, B_SOLID_HIGH);

	EndPicture();
	SetOrigin(0, 0);
	SetScale(1);
}
