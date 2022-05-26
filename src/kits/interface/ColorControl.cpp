/*
 * Copyright 2001-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner, alex@zappotek.com
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 */

/**	BColorControl displays a palette of selectable colors. */

#include <ColorControl.h>

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include <ControlLook.h>
#include <Bitmap.h>
#include <TextControl.h>
#include <Region.h>
#include <Screen.h>
#include <SystemCatalog.h>
#include <Window.h>

using BPrivate::gSystemCatalog;

#include <binary_compatibility/Interface.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ColorControl"

static const uint32 kMsgColorEntered = 'ccol';
static const float kMinCellSize = 6.0f;
static const float kSelectorPenSize = 2.0f;
static const float kSelectorSize = 4.0f;
static const float kSelectorHSpacing = 2.0f;
static const float kTextFieldsHSpacing = 6.0f;
static const float kDefaultFontSize = 12.0f;
static const float kBevelSpacing = 2.0f;
static const uint32 kRampCount = 4;


BColorControl::BColorControl(BPoint leftTop, color_control_layout layout,
	float cellSize, const char* name, BMessage* message, bool useOffscreen)
	:
	BControl(BRect(leftTop, leftTop), name, NULL, message,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE),
	fRedText(NULL),
	fGreenText(NULL),
	fBlueText(NULL),
	fOffscreenBitmap(NULL)
{
	_InitData(layout, cellSize, useOffscreen, NULL);
}


BColorControl::BColorControl(BMessage* data)
	:
	BControl(data),
	fRedText(NULL),
	fGreenText(NULL),
	fBlueText(NULL),
	fOffscreenBitmap(NULL)
{
	int32 layout;
	float cellSize;
	bool useOffscreen;

	data->FindInt32("_layout", &layout);
	data->FindFloat("_csize", &cellSize);
	data->FindBool("_use_off", &useOffscreen);

	_InitData((color_control_layout)layout, cellSize, useOffscreen, data);
}


BColorControl::~BColorControl()
{
	delete fOffscreenBitmap;
}


void
BColorControl::_InitData(color_control_layout layout, float size,
	bool useOffscreen, BMessage* data)
{
	fPaletteMode = BScreen(B_MAIN_SCREEN_ID).ColorSpace() == B_CMAP8;
		//TODO: we don't support workspace and colorspace changing for now
		//		so we take the main_screen colorspace at startup
	fColumns = layout;
	fRows = 256 / fColumns;

	_SetCellSize(size);

	fSelectedPaletteColorIndex = -1;
	fPreviousSelectedPaletteColorIndex = -1;
	fFocusedRamp = !fPaletteMode && IsFocus() ? 1 : -1;
	fClickedRamp = -1;

	const char* red = B_TRANSLATE_MARK("Red:");
	const char* green = B_TRANSLATE_MARK("Green:");
	const char* blue = B_TRANSLATE_MARK("Blue:");
	red = gSystemCatalog.GetString(red, "ColorControl");
	green = gSystemCatalog.GetString(green, "ColorControl");
	blue = gSystemCatalog.GetString(blue, "ColorControl");

	if (data != NULL) {
		fRedText = (BTextControl*)FindView("_red");
		fGreenText = (BTextControl*)FindView("_green");
		fBlueText = (BTextControl*)FindView("_blue");

		int32 value = 0;
		data->FindInt32("_val", &value);

		SetValue(value);
	} else {
		BRect textRect(0.0f, 0.0f, 0.0f, 0.0f);
		float labelWidth = std::max(StringWidth(red),
			std::max(StringWidth(green), StringWidth(blue)))
				+ kTextFieldsHSpacing;
		textRect.right = labelWidth + StringWidth("999999");
			// enough room for 3 digits plus 3 digits of padding
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		float labelHeight = fontHeight.ascent + fontHeight.descent;
		textRect.bottom = labelHeight;

		// red

		fRedText = new BTextControl(textRect, "_red", red, "0",
			new BMessage(kMsgColorEntered), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fRedText->SetDivider(labelWidth);

		for (int32 i = 0; i < 256; i++)
			fRedText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fRedText->TextView()->AllowChar(i);
		fRedText->TextView()->SetMaxBytes(3);

		// green

		textRect.OffsetBy(0, _TextRectOffset());
		fGreenText = new BTextControl(textRect, "_green", green, "0",
			new BMessage(kMsgColorEntered), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fGreenText->SetDivider(labelWidth);

		for (int32 i = 0; i < 256; i++)
			fGreenText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fGreenText->TextView()->AllowChar(i);
		fGreenText->TextView()->SetMaxBytes(3);

		// blue

		textRect.OffsetBy(0, _TextRectOffset());
		fBlueText = new BTextControl(textRect, "_blue", blue, "0",
			new BMessage(kMsgColorEntered), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fBlueText->SetDivider(labelWidth);

		for (int32 i = 0; i < 256; i++)
			fBlueText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fBlueText->TextView()->AllowChar(i);
		fBlueText->TextView()->SetMaxBytes(3);

		AddChild(fRedText);
		AddChild(fGreenText);
		AddChild(fBlueText);
	}

	ResizeToPreferred();

	if (useOffscreen) {
		if (fOffscreenBitmap != NULL) {
			BRect bounds = _PaletteFrame();
			fOffscreenBitmap = new BBitmap(bounds, B_RGB32, true, false);
			BView* offscreenView = new BView(bounds, "off_view", 0, 0);

			fOffscreenBitmap->Lock();
			fOffscreenBitmap->AddChild(offscreenView);
			fOffscreenBitmap->Unlock();
		}
	} else {
		delete fOffscreenBitmap;
		fOffscreenBitmap = NULL;
	}
}


void
BColorControl::_LayoutView()
{
	fPaletteFrame.Set(0, 0, fColumns * fCellSize, fRows * fCellSize);
	fPaletteFrame.OffsetBy(kBevelSpacing, kBevelSpacing);
	if (!fPaletteMode) {
		// Reduce the inner space by 1 pixel so that the frame
		// is exactly rows * cellsize pixels in height
		fPaletteFrame.bottom -= 1;
	}

	float rampHeight = (float)(fRows * fCellSize / kRampCount);
	float offset = _TextRectOffset();
	float y = 0;
	if (rampHeight > fRedText->Frame().Height()) {
		// there is enough room to fit kRampCount labels,
		// shift text controls down by one ramp
		offset = rampHeight;
		y = floorf(offset + (offset - fRedText->Frame().Height()) / 2);
	}

	BRect rect = _PaletteFrame();
	fRedText->MoveTo(rect.right + kTextFieldsHSpacing, y);

	y += offset;
	fGreenText->MoveTo(rect.right + kTextFieldsHSpacing, y);

	y += offset;
	fBlueText->MoveTo(rect.right + kTextFieldsHSpacing, y);
}


BArchivable*
BColorControl::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BColorControl"))
		return new BColorControl(data);

	return NULL;
}


status_t
BColorControl::Archive(BMessage* data, bool deep) const
{
	status_t status = BControl::Archive(data, deep);

	if (status == B_OK)
		status = data->AddInt32("_layout", Layout());

	if (status == B_OK)
		status = data->AddFloat("_csize", fCellSize);

	if (status == B_OK)
		status = data->AddBool("_use_off", fOffscreenBitmap != NULL);

	return status;
}


void
BColorControl::SetLayout(BLayout* layout)
{
	// We need to implement this method, since we have another SetLayout()
	// method and C++ has this special method hiding "feature".
	BControl::SetLayout(layout);
}


void
BColorControl::SetValue(int32 value)
{
	rgb_color c1 = ValueAsColor();
	rgb_color c2;
	c2.red = (value & 0xFF000000) >> 24;
	c2.green = (value & 0x00FF0000) >> 16;
	c2.blue = (value & 0x0000FF00) >> 8;
	c2.alpha = 255;

	if (fPaletteMode) {
		//workaround when two indexes have the same color
		rgb_color c
			= BScreen(Window()).ColorForIndex(fSelectedPaletteColorIndex);
		c.alpha = 255;
		if (fSelectedPaletteColorIndex == -1 || c != c2) {
				//here SetValue hasn't been called by mouse tracking
			fSelectedPaletteColorIndex = BScreen(Window()).IndexForColor(c2);
		}

		c2 = BScreen(Window()).ColorForIndex(fSelectedPaletteColorIndex);

		Invalidate(_PaletteSelectorFrame(fPreviousSelectedPaletteColorIndex));
		Invalidate(_PaletteSelectorFrame(fSelectedPaletteColorIndex));

		fPreviousSelectedPaletteColorIndex = fSelectedPaletteColorIndex;
	} else if (c1 != c2)
		Invalidate();

	// Set the value here, since BTextControl will trigger
	// Window()->UpdateIfNeeded() which will cause us to draw the indicators
	// at the old offset.
	if (Value() != value)
		BControl::SetValueNoUpdate(value);

	// the textcontrols have to be updated even when the color
	// hasn't changed since the value is clamped upstream
	// and the textcontrols would still show the unclamped value
	char string[4];
	sprintf(string, "%d", c2.red);
	fRedText->SetText(string);
	sprintf(string, "%d", c2.green);
	fGreenText->SetText(string);
	sprintf(string, "%d", c2.blue);
	fBlueText->SetText(string);
}


rgb_color
BColorControl::ValueAsColor()
{
	int32 value = Value();
	rgb_color color;

	color.red = (value & 0xFF000000) >> 24;
	color.green = (value & 0x00FF0000) >> 16;
	color.blue = (value & 0x0000FF00) >> 8;
	color.alpha = 255;

	return color;
}


void
BColorControl::SetEnabled(bool enabled)
{
	BControl::SetEnabled(enabled);

	fRedText->SetEnabled(enabled);
	fGreenText->SetEnabled(enabled);
	fBlueText->SetEnabled(enabled);
}


void
BColorControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	AdoptParentColors();

	fRedText->SetTarget(this);
	fGreenText->SetTarget(this);
	fBlueText->SetTarget(this);

	if (fOffscreenBitmap != NULL)
		_InitOffscreen();
}


void
BColorControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgColorEntered:
		{
			rgb_color color;
			color.red = min_c(strtol(fRedText->Text(), NULL, 10), 255);
			color.green = min_c(strtol(fGreenText->Text(), NULL, 10), 255);
			color.blue = min_c(strtol(fBlueText->Text(), NULL, 10), 255);
			color.alpha = 255;

			SetValue(color);
			Invoke();
			break;
		}

		case B_SCREEN_CHANGED:
		{
			BRect frame;
			uint32 mode;
			if (message->FindRect("frame", &frame) == B_OK
				&& message->FindInt32("mode", (int32*)&mode) == B_OK) {
				if ((fPaletteMode && mode == B_CMAP8)
					|| (!fPaletteMode && mode != B_CMAP8)) {
					// not switching to or from B_CMAP8, break
					break;
				}

				// fake an archive message (so we don't rebuild views)
				BMessage* data = new BMessage();
				data->AddInt32("_val", Value());

				// reinititialize
				bool useOffscreen = fOffscreenBitmap != NULL;
				_InitData((color_control_layout)fColumns, fCellSize,
					useOffscreen, data);
				if (useOffscreen)
					_InitOffscreen();

				// cleanup
				delete data;
			}
			break;
		}

		default:
			BControl::MessageReceived(message);
	}
}


void
BColorControl::Draw(BRect updateRect)
{
	if (fOffscreenBitmap != NULL)
		DrawBitmap(fOffscreenBitmap, B_ORIGIN);
	else
		_DrawColorArea(this, updateRect);

	_DrawSelectors(this);
}


void
BColorControl::_DrawColorArea(BView* target, BRect updateRect)
{
	BRect rect = _PaletteFrame();
	bool enabled = IsEnabled();

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);

	uint32 flags = be_control_look->Flags(this);
	be_control_look->DrawTextControlBorder(target, rect, updateRect,
		base, flags);

	if (fPaletteMode) {
		int colBegin = max_c(0, -1 + int(updateRect.left) / int(fCellSize));
		int colEnd = min_c(fColumns,
			2 + int(updateRect.right) / int(fCellSize));
		int rowBegin = max_c(0, -1 + int(updateRect.top) / int(fCellSize));
		int rowEnd = min_c(fRows, 2 + int(updateRect.bottom)
			/ int(fCellSize));

		// grid
		target->SetHighColor(enabled ? darken1 : base);

		for (int xi = 0; xi < fColumns + 1; xi++) {
			float x = fPaletteFrame.left + float(xi) * fCellSize;
			target->StrokeLine(BPoint(x, fPaletteFrame.top),
				BPoint(x, fPaletteFrame.bottom));
		}
		for (int yi = 0; yi < fRows + 1; yi++) {
			float y = fPaletteFrame.top + float(yi) * fCellSize;
			target->StrokeLine(BPoint(fPaletteFrame.left, y),
				BPoint(fPaletteFrame.right, y));
		}

		// colors
		for (int col = colBegin; col < colEnd; col++) {
			for (int row = rowBegin; row < rowEnd; row++) {
				uint8 colorIndex = row * fColumns + col;
				float x = fPaletteFrame.left + col * fCellSize;
				float y = fPaletteFrame.top + row * fCellSize;

				target->SetHighColor(system_colors()->color_list[colorIndex]);
				target->FillRect(BRect(x + 1, y + 1,
					x + fCellSize - 1, y + fCellSize - 1));
			}
		}
	} else {
		rgb_color white = { 255, 255, 255, 255 };
		rgb_color red   = { 255, 0, 0, 255 };
		rgb_color green = { 0, 255, 0, 255 };
		rgb_color blue  = { 0, 0, 255, 255 };

		rgb_color compColor = { 0, 0, 0, 255 };
		if (!enabled) {
			compColor.red = compColor.green = compColor.blue = 156;
			red.red = green.green = blue.blue = 70;
			white.red = white.green = white.blue = 70;
		}
		_DrawColorRamp(_RampFrame(0), target, white, compColor, 0, false,
			updateRect);
		_DrawColorRamp(_RampFrame(1), target, red, compColor, 0, false,
			updateRect);
		_DrawColorRamp(_RampFrame(2), target, green, compColor, 0, false,
			updateRect);
		_DrawColorRamp(_RampFrame(3), target, blue, compColor, 0, false,
			updateRect);
	}
}


void
BColorControl::_DrawSelectors(BView* target)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax = tint_color(base, B_LIGHTEN_MAX_TINT);

	if (fPaletteMode) {
		if (fSelectedPaletteColorIndex != -1) {
			target->SetHighColor(lightenmax);
			target->StrokeRect(
				_PaletteSelectorFrame(fSelectedPaletteColorIndex));
		}
	} else {
		rgb_color color = ValueAsColor();
		target->SetHighColor(255, 255, 255);
		target->SetLowColor(0, 0, 0);

		int components[4] = { color.alpha, color.red, color.green, color.blue };

		for (int i = 1; i < 4; i++) {
			BPoint center = _SelectorPosition(_RampFrame(i), components[i]);

			target->SetPenSize(kSelectorPenSize);
			target->StrokeEllipse(center, kSelectorSize / 2, kSelectorSize / 2);
			target->SetPenSize(kSelectorPenSize / 2);
			target->StrokeEllipse(center, kSelectorSize, kSelectorSize,
				B_SOLID_LOW);
			if (i == fFocusedRamp) {
				target->StrokeEllipse(center,
					kSelectorSize / 2, kSelectorSize / 2, B_SOLID_LOW);
			}
		}

		target->SetPenSize(1.0f);
	}
}


void
BColorControl::_DrawColorRamp(BRect rect, BView* target,
	rgb_color baseColor, rgb_color compColor, int16 flag, bool focused,
	BRect updateRect)
{
	float width = rect.Width() + 1;
	rgb_color color = ValueAsColor();
	color.alpha = 255;

	updateRect = updateRect & rect;

	if (updateRect.IsValid() && updateRect.Width() >= 0) {
		target->BeginLineArray((int32)updateRect.Width() + 1);

		for (float i = (updateRect.left - rect.left);
				i <= (updateRect.right - rect.left) + 1; i++) {
			if (baseColor.red == 255)
				color.red = (uint8)(i * 255 / width) + compColor.red;
			if (baseColor.green == 255)
				color.green = (uint8)(i * 255 / width) + compColor.green;
			if (baseColor.blue == 255)
				color.blue = (uint8)(i * 255 / width) + compColor.blue;

			target->AddLine(BPoint(rect.left + i, rect.top),
				BPoint(rect.left + i, rect.bottom - 1), color);
		}

		target->EndLineArray();
	}
}


BPoint
BColorControl::_SelectorPosition(const BRect& rampRect, uint8 shade) const
{
	float radius = kSelectorSize / 2 + kSelectorPenSize / 2;

	return BPoint(rampRect.left + kSelectorHSpacing + radius +
		shade * (rampRect.Width() - 2 * (kSelectorHSpacing + radius)) / 255,
		rampRect.top + rampRect.Height() / 2);
}


BRect
BColorControl::_PaletteFrame() const
{
	return fPaletteFrame.InsetByCopy(-kBevelSpacing, -kBevelSpacing);
}


BRect
BColorControl::_RampFrame(uint8 rampIndex) const
{
	float rampHeight = (float)(fRows * fCellSize / kRampCount);

	return BRect(fPaletteFrame.left,
		fPaletteFrame.top + float(rampIndex) * rampHeight,
		fPaletteFrame.right,
		fPaletteFrame.top + float(rampIndex + 1) * rampHeight);
}


void
BColorControl::_SetCellSize(float size)
{
	BFont font;
	GetFont(&font);
	fCellSize = std::max(kMinCellSize,
		ceilf(size * font.Size() / kDefaultFontSize));
}


float
BColorControl::_TextRectOffset()
{
	return std::max(fRedText->Bounds().Height(),
		ceilf(_PaletteFrame().Height() / 3));
}


BRect
BColorControl::_PaletteSelectorFrame(uint8 colorIndex) const
{
	uint32 row = colorIndex / fColumns;
	uint32 column = colorIndex % fColumns;
	float x = fPaletteFrame.left + column * fCellSize;
	float y = fPaletteFrame.top + row * fCellSize;
	return BRect(x, y, x + fCellSize, y + fCellSize);
}


void
BColorControl::_InitOffscreen()
{
	if (fOffscreenBitmap->Lock()) {
		BView* offscreenView = fOffscreenBitmap->ChildAt((int32)0);
		if (offscreenView != NULL) {
			_DrawColorArea(offscreenView, _PaletteFrame());
			offscreenView->Sync();
		}
		fOffscreenBitmap->Unlock();
	}
}


void
BColorControl::_InvalidateSelector(int16 ramp, rgb_color color, bool focused)
{
	if (fPaletteMode)
		return;

	if (ramp < 1 || ramp > 3)
		return;

	float invalidateRadius = focused
		? kSelectorSize + kSelectorPenSize / 2
		: kSelectorSize / 2 + kSelectorPenSize;

	uint8 colorValue = ramp == 1 ? color.red : ramp == 2 ? color.green
		: color.blue;

	BPoint pos = _SelectorPosition(_RampFrame(ramp), colorValue);
	Invalidate(BRect(pos.x - invalidateRadius, pos.y - invalidateRadius,
		pos.x + invalidateRadius, pos.y + invalidateRadius));
}


void
BColorControl::SetCellSize(float size)
{
	_SetCellSize(size);
	ResizeToPreferred();
}


float
BColorControl::CellSize() const
{
	return fCellSize;
}


void
BColorControl::SetLayout(color_control_layout layout)
{
	switch (layout) {
		case B_CELLS_4x64:
			fColumns = 4;
			fRows = 64;
			break;

		case B_CELLS_8x32:
			fColumns = 8;
			fRows = 32;
			break;

		case B_CELLS_16x16:
			fColumns = 16;
			fRows = 16;
			break;

		case B_CELLS_32x8:
			fColumns = 32;
			fRows = 8;
			break;

		case B_CELLS_64x4:
			fColumns = 64;
			fRows = 4;
			break;
	}

	ResizeToPreferred();
	Invalidate();
}


color_control_layout
BColorControl::Layout() const
{
	if (fColumns == 4 && fRows == 64)
		return B_CELLS_4x64;

	if (fColumns == 8 && fRows == 32)
		return B_CELLS_8x32;

	if (fColumns == 16 && fRows == 16)
		return B_CELLS_16x16;

	if (fColumns == 32 && fRows == 8)
		return B_CELLS_32x8;

	if (fColumns == 64 && fRows == 4)
		return B_CELLS_64x4;

	return B_CELLS_32x8;
}


void
BColorControl::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}


void
BColorControl::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsFocus() && !fPaletteMode && numBytes == 1) {
		rgb_color color = ValueAsColor();

		switch (bytes[0]) {
			case B_UP_ARROW:
			{
				int16 oldFocus = fFocusedRamp;
				fFocusedRamp--;
				if (fFocusedRamp < 1)
					fFocusedRamp = 3;

				_InvalidateSelector(oldFocus, color, true);
				_InvalidateSelector(fFocusedRamp, color, true);
				break;
			}

			case B_DOWN_ARROW:
			{
				int16 oldFocus = fFocusedRamp;
				fFocusedRamp++;
				if (fFocusedRamp > 3)
					fFocusedRamp = 1;

				_InvalidateSelector(oldFocus, color, true);
				_InvalidateSelector(fFocusedRamp, color, true);
				break;
			}

			case B_LEFT_ARROW:
			{
				bool goFaster = false;
				if (Window() != NULL) {
					BMessage* message = Window()->CurrentMessage();
					if (message != NULL && message->what == B_KEY_DOWN) {
						int32 repeats = 0;
						if (message->FindInt32("be:key_repeat", &repeats)
								== B_OK && repeats > 4) {
							goFaster = true;
						}
					}
				}

				if (fFocusedRamp == 1) {
					if (goFaster && color.red >= 5)
						color.red -= 5;
					else if (color.red > 0)
						color.red--;
				} else if (fFocusedRamp == 2) {
					if (goFaster && color.green >= 5)
						color.green -= 5;
					else if (color.green > 0)
						color.green--;
				} else if (fFocusedRamp == 3) {
				 	if (goFaster && color.blue >= 5)
						color.blue -= 5;
					else if (color.blue > 0)
						color.blue--;
				}

				SetValue(color);
				Invoke();
				break;
			}

			case B_RIGHT_ARROW:
			{
				bool goFaster = false;
				if (Window() != NULL) {
					BMessage* message = Window()->CurrentMessage();
					if (message != NULL && message->what == B_KEY_DOWN) {
						int32 repeats = 0;
						if (message->FindInt32("be:key_repeat", &repeats)
								== B_OK && repeats > 4) {
							goFaster = true;
						}
					}
				}

				if (fFocusedRamp == 1) {
					if (goFaster && color.red <= 250)
						color.red += 5;
					else if (color.red < 255)
						color.red++;
				} else if (fFocusedRamp == 2) {
					if (goFaster && color.green <= 250)
						color.green += 5;
					else if (color.green < 255)
						color.green++;
				} else if (fFocusedRamp == 3) {
				 	if (goFaster && color.blue <= 250)
						color.blue += 5;
					else if (color.blue < 255)
						color.blue++;
				}

				SetValue(color);
				Invoke();
				break;
			}
		}
	}

	BControl::KeyDown(bytes, numBytes);
}


void
BColorControl::MouseUp(BPoint point)
{
	fClickedRamp = -1;
	SetTracking(false);
}


void
BColorControl::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;
	if (!fPaletteFrame.Contains(point))
		return;

	if (fPaletteMode) {
		int col = (int)((point.x - fPaletteFrame.left) / fCellSize);
		int row = (int)((point.y - fPaletteFrame.top) / fCellSize);
		int colorIndex = row * fColumns + col;
		if (colorIndex >= 0 && colorIndex < 256) {
			fSelectedPaletteColorIndex = colorIndex;
			SetValue(system_colors()->color_list[colorIndex]);
		}
	} else {
		rgb_color color = ValueAsColor();

		uint8 shade = (unsigned char)max_c(0,
			min_c((point.x - _RampFrame(0).left) * 255
				/ _RampFrame(0).Width(), 255));

		if (_RampFrame(0).Contains(point)) {
			color.red = color.green = color.blue = shade;
			fClickedRamp = 0;
		} else if (_RampFrame(1).Contains(point)) {
			color.red = shade;
			fClickedRamp = 1;
		} else if (_RampFrame(2).Contains(point)) {
			color.green = shade;
			fClickedRamp = 2;
		} else if (_RampFrame(3).Contains(point)) {
			color.blue = shade;
			fClickedRamp = 3;
		}

		SetValue(color);
	}

	Invoke();

	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS,
		B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
}


void
BColorControl::MouseMoved(BPoint point, uint32 transit,
	const BMessage* message)
{
	if (!IsTracking())
		return;

	if (fPaletteMode && fPaletteFrame.Contains(point)) {
		int col = (int)((point.x - fPaletteFrame.left) / fCellSize);
		int row = (int)((point.y - fPaletteFrame.top) / fCellSize);
		int colorIndex = row * fColumns + col;
		if (colorIndex >= 0 && colorIndex < 256) {
			fSelectedPaletteColorIndex = colorIndex;
			SetValue(system_colors()->color_list[colorIndex]);
		}
	} else {
		if (fClickedRamp < 0 || fClickedRamp > 3)
			return;

		rgb_color color = ValueAsColor();

		uint8 shade = (unsigned char)max_c(0,
			min_c((point.x - _RampFrame(0).left) * 255
				/ _RampFrame(0).Width(), 255));

		if (fClickedRamp == 0)
			color.red = color.green = color.blue = shade;
		else if (fClickedRamp == 1)
			color.red = shade;
		else if (fClickedRamp == 2)
			color.green = shade;
		else if (fClickedRamp == 3)
			color.blue = shade;

		SetValue(color);
	}

	Invoke();
}


void
BColorControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BColorControl::GetPreferredSize(float* _width, float* _height)
{
	BRect rect = _PaletteFrame();

	if (rect.Height() < fBlueText->Frame().bottom) {
		// adjust the height to fit
		rect.bottom = fBlueText->Frame().bottom;
	}

	if (_width) {
		*_width = rect.Width() + kTextFieldsHSpacing
			+ fRedText->Bounds().Width();
	}

	if (_height)
		*_height = rect.Height();
}


void
BColorControl::ResizeToPreferred()
{
	_LayoutView();
	BControl::ResizeToPreferred();
}


status_t
BColorControl::Invoke(BMessage* message)
{
	return BControl::Invoke(message);
}


void
BColorControl::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}


void
BColorControl::FrameResized(float newWidth, float newHeight)
{
	BControl::FrameResized(newWidth, newHeight);
}


BHandler*
BColorControl::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return BControl::ResolveSpecifier(message, index, specifier, form,
		property);
}


status_t
BColorControl::GetSupportedSuites(BMessage* data)
{
	return BControl::GetSupportedSuites(data);
}


void
BColorControl::MakeFocus(bool focused)
{
	fFocusedRamp = !fPaletteMode && focused ? 1 : -1;
	BControl::MakeFocus(focused);
}


void
BColorControl::AllAttached()
{
	BControl::AllAttached();
}


void
BColorControl::AllDetached()
{
	BControl::AllDetached();
}


status_t
BColorControl::SetIcon(const BBitmap* icon, uint32 flags)
{
	return BControl::SetIcon(icon, flags);
}


status_t
BColorControl::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BColorControl::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BColorControl::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BColorControl::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BColorControl::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BColorControl::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BColorControl::GetHeightForWidth(data->width, &data->min,
				&data->max, &data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BColorControl::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BColorControl::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			BColorControl::DoLayout();
			return B_OK;
		}

		case PERFORM_CODE_SET_ICON:
		{
			perform_data_set_icon* data = (perform_data_set_icon*)_data;
			return BColorControl::SetIcon(data->icon, data->flags);
		}
	}

	return BControl::Perform(code, _data);
}


void BColorControl::_ReservedColorControl1() {}
void BColorControl::_ReservedColorControl2() {}
void BColorControl::_ReservedColorControl3() {}
void BColorControl::_ReservedColorControl4() {}


BColorControl &
BColorControl::operator=(const BColorControl &)
{
	return *this;
}
