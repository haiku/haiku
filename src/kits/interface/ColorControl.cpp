//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		BColorControl.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BColorControl displays a palette of selectable colors.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include "ColorControl.h"
#include "TextControl.h"
#include <Support/Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

const uint32 U_COLOR_CONTROL_RED_CHANGED_MSG = 'CCRC';
const uint32 U_COLOR_CONTROL_GREEN_CHANGED_MSG = 'CCGC';
const uint32 U_COLOR_CONTROL_BLUE_CHANGED_MSG = 'CCBC';

//------------------------------------------------------------------------------
BColorControl::BColorControl(BPoint leftTop, color_control_layout matrix,
							 float cellSize, const char *name,
							 BMessage *message, bool bufferedDrawing)
	:	BControl(BRect(leftTop, leftTop), name, NULL, message,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
{
	switch (matrix)
	{
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

	fCellSize = cellSize;

	BRect rect(0.0f, 0.0f, 196, 52);

	ResizeTo(rect.Width() + 70, rect.Height());

	fRedText = new BTextControl(BRect(rect.right + 1, 0.0f,
		rect.right + 70, 15.0f), "_red", "Red:", NULL,
		new BMessage(U_COLOR_CONTROL_RED_CHANGED_MSG));
	AddChild(fRedText);

	fGreenText = new BTextControl(BRect(rect.right + 1, 16.0f,
		rect.right + 70, 30.0f), "_green", "Green:", NULL,
		new BMessage(U_COLOR_CONTROL_GREEN_CHANGED_MSG));
	AddChild(fGreenText);

	fBlueText = new BTextControl(BRect(rect.right + 1, 31.0f,
		rect.right + 70, 45), "_blue", "Blue:", NULL,
		new BMessage(U_COLOR_CONTROL_BLUE_CHANGED_MSG));
	AddChild(fBlueText);

	fFocusedComponent = 0;
}
//------------------------------------------------------------------------------
BColorControl::BColorControl(BMessage *archive)
	:	BControl(archive)
{
	int32 layout;
	bool use_offscreen;

	archive->FindInt32("_layout", &layout);
	SetLayout((color_control_layout)layout);

	archive->FindFloat("_csize", &fCellSize);

	archive->FindBool("_use_off", &use_offscreen);
}
//------------------------------------------------------------------------------
BColorControl::~BColorControl()
{
}
//------------------------------------------------------------------------------
BArchivable *BColorControl::Instantiate(BMessage *archive)
{
	if ( validate_instantiation(archive, "BColorControl"))
		return new BColorControl(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BColorControl::Archive(BMessage *archive, bool deep) const
{
	BControl::Archive(archive, deep);

	archive->AddInt32("_layout", Layout());
	archive->AddFloat("_csize", fCellSize);
	archive->AddBool("_use_off", fOffscreenView != NULL);

	return B_OK;
}
//------------------------------------------------------------------------------
void BColorControl::SetValue(int32 color)
{
	BControl::SetValue(color);

	rgb_color c = ValueAsColor();
	char string[4];

	sprintf(string, "%d", c.red);
	fRedText->SetText( string);
	sprintf(string, "%d", c.green);
	fGreenText->SetText( string);
	sprintf(string, "%d", c.blue);
	fBlueText->SetText(string);

	Invoke();
}
//------------------------------------------------------------------------------
rgb_color BColorControl::ValueAsColor()
{
	int32 value = Value();
	rgb_color color;

	color.red = (value & 0xFF000000) >> 24;
	color.green = (value & 0x00FF0000) >> 16;
	color.blue = (value & 0x0000FF00) >> 8;
	color.alpha = 255;

	return color;
}
//------------------------------------------------------------------------------
void BColorControl::SetEnabled(bool enabled)
{
	BControl::SetEnabled(enabled);

	fRedText->SetEnabled(enabled);
	fGreenText->SetEnabled(enabled);
	fBlueText->SetEnabled(enabled);
}
//------------------------------------------------------------------------------
void BColorControl::AttachedToWindow()
{
	if (Parent())
      SetViewColor(Parent()->ViewColor());

	BControl::AttachedToWindow();

	fRedText->SetTarget(this);
	fGreenText->SetTarget(this);
	fBlueText->SetTarget(this);
}
//------------------------------------------------------------------------------
void BColorControl::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case U_COLOR_CONTROL_RED_CHANGED_MSG:
		{
			unsigned char shade;
			rgb_color color = ValueAsColor();

			sscanf(fRedText->Text(), "%d", &shade);

			color.red = max_c(0, min_c(shade, 255));

			SetValue(color);

			break;
		}
		case U_COLOR_CONTROL_GREEN_CHANGED_MSG:
		{
			unsigned char shade;
			rgb_color color = ValueAsColor();

			sscanf(fGreenText->Text(), "%d", &shade);

			color.green = max_c(0, min_c(shade, 255));

			SetValue(color);

			break;
		}
		case U_COLOR_CONTROL_BLUE_CHANGED_MSG:
		{
			unsigned char shade;
			rgb_color color = ValueAsColor();

			sscanf(fBlueText->Text(), "%d", &shade);

			color.blue = max_c(0, min_c(shade, 255));

			SetValue(color);

			break;
		}
		default:
			BControl::MessageReceived(message);
	}
}
//------------------------------------------------------------------------------
void BColorControl::Draw(BRect updateRect)
{
	BRect rect(0.0f, 0.0f, 196, 52);

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
	darken4 = tint_color(no_tint, B_DARKEN_4_TINT);

	// First bevel
	SetHighColor(darken1);
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.LeftTop(), rect.RightTop());
	SetHighColor(lightenmax);
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f));

	rect.InsetBy(1.0f, 1.0f);

	// Second bevel
	SetHighColor(darken4);
	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.LeftTop(), rect.RightTop());
	SetHighColor(no_tint);
	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f));

	// Ramps
	rgb_color white = {255, 255, 255, 255};
	rgb_color red = {255, 0, 0, 255};
	rgb_color green = {0, 255, 0, 255};
	rgb_color blue = {0, 0, 255, 255};

	rect.InsetBy(1.0f, 1.0f);

	ColorRamp(BRect(), BRect(rect.left, rect.top,
		rect.right, rect.top + 12.0f), this, white, 0, false);

	ColorRamp(BRect(), BRect(rect.left, rect.top + 13.0f,
		rect.right, rect.top + 24.0f), this, red, 0, false);

	ColorRamp(BRect(), BRect(rect.left, rect.top + 25.0f,
		rect.right, rect.top + 36.0f), this, green, 0, false);

	ColorRamp(BRect(), BRect(rect.left, rect.top + 37.0f,
		rect.right, rect.top + 48.0f), this, blue, 0, false);

	// Selectors
	rgb_color color = ValueAsColor();
	float x, y = rect.top + 16.0f;

	SetPenSize(2.0f);

	x = rect.left + color.red * (rect.Width() - 7) / 255;
	SetHighColor(255, 255, 255);
	StrokeEllipse(BRect(x, y, x + 4.0f, y + 4.0f));

	y += 11;

	x = rect.left + color.green * (rect.Width() - 7) / 255;
	SetHighColor(255, 255, 255);
	StrokeEllipse(BRect(x, y, x + 4.0f, y + 4.0f));

	y += 11;

	x = rect.left + color.blue * (rect.Width() - 7) / 255;
	SetHighColor(255, 255, 255);
	StrokeEllipse(BRect ( x, y, x + 4.0f, y + 4.0f));

	SetPenSize(1.0f);
}
//------------------------------------------------------------------------------
void BColorControl::MouseDown(BPoint point)
{
	rgb_color color = ValueAsColor();

	BRect rect(0.0f, 0.0f, 196, 52);

	uint8 shade = (unsigned char)max_c(0,
		min_c((point.x - 2) * 255 / (rect.Width() - 4.0f), 255));

	if (point.y - 2 < 12)
	{
		color.red = color.green = color.blue = shade;
		fFocusedComponent = 1;
	}
	else if (point.y - 2 < 24)
	{
		color.red = shade;
		fFocusedComponent = 2;
	}
	else if (point.y - 2 < 36)
	{
		color.green = shade;
		fFocusedComponent = 3;
	}
	else if (point.y - 2 < 48)
	{
		color.blue = shade;
		fFocusedComponent = 4;
	}

	SetValue(color);

	MakeFocus();
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}
//------------------------------------------------------------------------------
void BColorControl::KeyDown(const char *bytes, int32 numBytes)
{
}
//------------------------------------------------------------------------------
void BColorControl::SetCellSize(float cellSide)
{
	fCellSize = cellSide;
}
//------------------------------------------------------------------------------
float BColorControl::CellSize() const
{
	return fCellSize;
}
//------------------------------------------------------------------------------
void BColorControl::SetLayout(color_control_layout layout)
{
	switch (layout)
	{
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
}
//------------------------------------------------------------------------------
color_control_layout BColorControl::Layout() const
{
	if (fColumns == 4 &&fRows == 64)
		return B_CELLS_4x64;
	if (fColumns == 8 &&fRows == 32)
		return B_CELLS_8x32;
	if (fColumns == 16 &&fRows == 16)
		return B_CELLS_16x16;
	if (fColumns == 32 &&fRows == 8)
		return B_CELLS_32x8;
	if (fColumns == 64 &&fRows == 4)
		return B_CELLS_64x4;

	return B_CELLS_32x8;
}
//------------------------------------------------------------------------------
void BColorControl::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}
//------------------------------------------------------------------------------
void BColorControl::MouseUp(BPoint point)
{
	fFocusedComponent = 0;
}
//------------------------------------------------------------------------------
void BColorControl::MouseMoved(BPoint point, uint32 transit,
							   const BMessage *message)
{
	if (fFocusedComponent != 0)
	{
		rgb_color color = ValueAsColor();

		BRect rect(0.0f, 0.0f, 196, 52);

		uint8 shade = (unsigned char)max_c(0,
		min_c((point.x - 2) * 255 / (rect.Width() - 4.0f), 255));

		switch (fFocusedComponent)
		{
			case 1:
				color.red = color.green = color.blue = shade;
				break;
			case 2:
				color.red = shade;
				break;
			case 3:
				color.green = shade;
				break;
			case 4:
				color.blue = shade;
				break;
		}

		SetValue(color);
	}
}
//------------------------------------------------------------------------------
void BColorControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BColorControl::GetPreferredSize(float *width, float *height)
{
	*width = fColumns * fCellSize + 4.0f + 88.0f;
	*height = fRows * fCellSize + 4.0f;
}
//------------------------------------------------------------------------------
void BColorControl::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}
//------------------------------------------------------------------------------
status_t BColorControl::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}
//------------------------------------------------------------------------------
void BColorControl::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
void BColorControl::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}
//------------------------------------------------------------------------------
BHandler *BColorControl::ResolveSpecifier(BMessage *msg, int32 index,
										  BMessage *specifier, int32 form,
										  const char *property)
{
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}
//------------------------------------------------------------------------------
status_t BColorControl::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
void BColorControl::MakeFocus(bool state)
{
	BControl::MakeFocus(state);
}
//------------------------------------------------------------------------------
void BColorControl::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BColorControl::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
status_t BColorControl::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BColorControl::_ReservedColorControl1() {}
void BColorControl::_ReservedColorControl2() {}
void BColorControl::_ReservedColorControl3() {}
void BColorControl::_ReservedColorControl4() {}
//------------------------------------------------------------------------------
BColorControl &BColorControl::operator=(const BColorControl &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BColorControl::LayoutView(bool calc_frame)
{
}
//------------------------------------------------------------------------------
void BColorControl::UpdateOffscreen()
{
}
//------------------------------------------------------------------------------
void BColorControl::UpdateOffscreen(BRect update)
{
}
//------------------------------------------------------------------------------
void BColorControl::DrawColorArea(BView *target, BRect update)
{
}
//------------------------------------------------------------------------------
void BColorControl::ColorRamp(BRect r, BRect where, BView *target, rgb_color c,
							  int16 flag, bool focused)
{
	rgb_color color = {255, 255, 255, 255};
	float width = where.Width();

	for (float i = 0; i <= width; i++)
	{
		color.red = (uint8)(i * c.red / width);
		color.green = (uint8)(i * c.green / width);
		color.blue = (uint8)(i * c.blue / width);

		target->SetHighColor(color);
		target->StrokeLine(BPoint(where.left + i, where.top),
			BPoint(where.left + i, where.bottom));
	}
}
//------------------------------------------------------------------------------
void BColorControl::KbAdjustColor(uint32 key)
{
}
//------------------------------------------------------------------------------
bool BColorControl::key_down32(uint32 key)
{
	return false;
}
//------------------------------------------------------------------------------
bool BColorControl::key_down8(uint32 key)
{
	return false;
}
//------------------------------------------------------------------------------
BRect BColorControl::CalcFrame(BPoint start, color_control_layout layout,
							   int32 size)
{
	BRect rect;

	switch (layout)
	{
		case B_CELLS_4x64:
			rect.Set(0.0f, 0.0f, 4 * size + 4.0f,
				64 * size + 4.0f);
			break;
		case B_CELLS_8x32:
			rect.Set(0.0f, 0.0f, 8 * size + 4.0f,
				32 * size + 4.0f);
			break;
		case B_CELLS_16x16:
			rect.Set(0.0f, 0.0f, 16 * size + 4.0f,
				16 * size + 4.0f);
			break;
		case B_CELLS_32x8:
			rect.Set(0.0f, 0.0f, 32 * size + 4.0f,
				8 * size + 4.0f);
			break;
		case B_CELLS_64x4:
			rect.Set(0.0f, 0.0f, 64 * size + 4.0f,
				4 * size + 4.0f);
			break;
	}

	return rect;
}
//------------------------------------------------------------------------------
void BColorControl::InitData(color_control_layout layout, float size,
							 bool use_offscreen, BMessage *data)
{
}
//------------------------------------------------------------------------------
void BColorControl::DoMouseMoved(BPoint pt)
{
}
//------------------------------------------------------------------------------
void BColorControl::DoMouseUp(BPoint pt)
{
}
//------------------------------------------------------------------------------
