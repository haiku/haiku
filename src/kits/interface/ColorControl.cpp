/*
 * Copyright 2001-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	BColorControl displays a palette of selectable colors. */


#include <stdio.h>
#include <stdlib.h>

#include <ColorControl.h>
#include <Bitmap.h>
#include <TextControl.h>
#include <Screen.h>
#include <Window.h>
#include <Errors.h>
#include <Region.h>


const uint32 U_COLOR_CONTROL_RED_CHANGED_MSG = 'CCRC';
const uint32 U_COLOR_CONTROL_GREEN_CHANGED_MSG = 'CCGC';
const uint32 U_COLOR_CONTROL_BLUE_CHANGED_MSG = 'CCBC';


BColorControl::BColorControl(BPoint leftTop, color_control_layout matrix,
	float cellSize, const char *name, BMessage *message,
	bool bufferedDrawing)
	: BControl(BRect(leftTop, leftTop), name, NULL, message,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
{
	InitData(matrix, cellSize, bufferedDrawing, NULL);
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
	if (!fRetainCache)
		fCachedIndex = -1;

	fRetainCache = false;

	if (fBitmap)
	{
		if (fBitmap->Lock())
		{
			if (!fOffscreenView)
				UpdateOffscreen();
			fBitmap->Unlock();
		}
	}

	rgb_color c1 = ValueAsColor();
	rgb_color c2;
	c2.red = (color & 0xFF000000) >> 24;
	c2.green = (color & 0x00FF0000) >> 16;
	c2.blue = (color & 0x0000FF00) >> 8;
	char string[4];

	if (c1.red != c2.red)
	{
		sprintf(string, "%d", c2.red);
		fRedText->SetText(string);
	}

	if (c1.green != c2.green)
	{
		sprintf(string, "%d", c2.green);
		fGreenText->SetText(string);
	}

	if (c1.blue != c2.blue)
	{
		sprintf(string, "%d", c2.blue);
		fBlueText->SetText(string);
	}

	if (LockLooper())
	{
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
	
	BControl::SetValue(color);
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
		case 'ccol':
		{
			rgb_color color;
			
			color.red = strtol(fRedText->Text(), NULL, 10);
			color.green = strtol(fGreenText->Text(), NULL, 10);
			color.blue = strtol(fBlueText->Text(), NULL, 10);

			SetValue(color);
			Invoke();

			break;
		}
		default:
			BControl::MessageReceived(message);
	}
}
//------------------------------------------------------------------------------
void BColorControl::Draw(BRect updateRect)
{
	if (fTState != NULL)
		return;

	if (fBitmap)
	{
		if (!fBitmap->Lock())
			return;

		if (fOffscreenView->Bounds().Intersects(updateRect))
		{
			UpdateOffscreen(updateRect);
			DrawBitmap(fBitmap, updateRect.LeftTop());
		}

		fBitmap->Unlock();
	}
	else if (Bounds().Intersects(updateRect))
		DrawColorArea(this, updateRect);
}
//------------------------------------------------------------------------------
void BColorControl::MouseDown(BPoint point)
{
	rgb_color color = ValueAsColor();

	BRect rect(0.0f, 0.0f, fColumns * fCellSize, fRows * fCellSize);
	float rampsize = rect.bottom / 4;

	uint8 shade = (unsigned char)max_c(0,
		min_c((point.x - 2) * 255 / (rect.Width() - 4.0f), 255));

	if (point.y - 2 < rampsize)
	{
		color.red = color.green = color.blue = shade;
		fFocusedComponent = 1;
	}
	else if (point.y - 2 < (rampsize * 2))
	{
		color.red = shade;
		fFocusedComponent = 2;
	}
	else if (point.y - 2 < (rampsize * 3))
	{
		color.green = shade;
		fFocusedComponent = 3;
	}
//	else if (point.y - 2 < rect.bottom)
	else
	{
		color.blue = shade;
		fFocusedComponent = 4;
	}

	SetValue(color);
	Invoke();

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

		BRect rect(0.0f, 0.0f, fColumns * fCellSize, fRows * fCellSize);

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
		Invoke();
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


void
BColorControl::LayoutView(bool calcFrame)
{
	BRect rect(0.0f, 0.0f, ceil(fColumns * fCellSize), ceil(fRows * fCellSize + 2));

	if (rect.Height() < fBlueText->Frame().bottom) {
		// adjust the height to fit
		rect.bottom = fBlueText->Frame().bottom;
	}

	ResizeTo(rect.Width() + fRedText->Bounds().Width(), rect.Height());

	float offset = floor(rect.bottom / 4);
	float y = offset;
	if (offset < fRedText->Bounds().Height() + 2) {
		offset = fRedText->Bounds().Height() + 2;
		y = 0;
	}

	fRedText->MoveTo(rect.right + 1, y);

	y += offset;
	fGreenText->MoveTo(rect.right + 1, y);

	y += offset;
	fBlueText->MoveTo(rect.right + 1, y);
}


void
BColorControl::UpdateOffscreen()
{
}


void
BColorControl::UpdateOffscreen(BRect update)
{
	if (fBitmap->Lock())
	{
		update = update & fOffscreenView->Bounds();
		fOffscreenView->FillRect(update);
		DrawColorArea(fOffscreenView, update);
		fOffscreenView->Sync();
		fBitmap->Unlock();
	}
}


void
BColorControl::DrawColorArea(BView *target, BRect update)
{
	BRegion region(update);
	target->ConstrainClippingRegion(&region);

	BRect rect(0.0f, 0.0f, ceil(fColumns * fCellSize), Bounds().bottom);

	rgb_color noTint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lightenmax = tint_color(noTint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(noTint, B_DARKEN_1_TINT),
	darken4 = tint_color(noTint, B_DARKEN_4_TINT);

	// First bevel
	target->SetHighColor(darken1);
	target->StrokeLine(rect.LeftBottom(), rect.LeftTop());
	target->StrokeLine(rect.LeftTop(), rect.RightTop());
	target->SetHighColor(lightenmax);
	target->StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	target->StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f));

	rect.InsetBy(1.0f, 1.0f);

	// Second bevel
	target->SetHighColor(darken4);
	target->StrokeLine(rect.LeftBottom(), rect.LeftTop());
	target->StrokeLine(rect.LeftTop(), rect.RightTop());
	target->SetHighColor(noTint);
	target->StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	target->StrokeLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1.0f));

	// Ramps
	rgb_color white = {255, 255, 255, 255};
	rgb_color red = {255, 0, 0, 255};
	rgb_color green = {0, 255, 0, 255};
	rgb_color blue = {0, 0, 255, 255};

	rect.InsetBy(1.0f, 1.0f);
	
	BRect ramprect(rect);
	float rampsize = ramprect.Height() / 4.0;
	
	ramprect.bottom = ramprect.top + rampsize;
	
	ColorRamp(BRect(), ramprect, target, white, 0, false);
	
	ramprect.OffsetBy(0,rampsize);
	ColorRamp(BRect(), ramprect, target, red, 0, false);

	ramprect.OffsetBy(0,rampsize);
	ColorRamp(BRect(), ramprect, target, green, 0, false);

	ramprect.OffsetBy(0,rampsize);
	ColorRamp(BRect(), ramprect, target, blue, 0, false);

	// Selectors
	rgb_color color = ValueAsColor();
	float x, y = rampsize * 1.5;

	target->SetDrawingMode(B_OP_OVER);
	target->SetPenSize(2.0f);

	x = rect.left + color.red * (rect.Width() - 7) / 255;
	target->SetHighColor(255, 255, 255);
	target->StrokeEllipse(BRect(x, y, x + 4.0f, y + 4.0f));

	y += rampsize;

	x = rect.left + color.green * (rect.Width() - 7) / 255;
	target->SetHighColor(255, 255, 255);
	target->StrokeEllipse(BRect(x, y, x + 4.0f, y + 4.0f));

	y += rampsize;

	x = rect.left + color.blue * (rect.Width() - 7) / 255;
	target->SetHighColor(255, 255, 255);
	target->StrokeEllipse(BRect ( x, y, x + 4.0f, y + 4.0f));

	target->SetPenSize(1.0f);
	target->SetDrawingMode(B_OP_COPY);

	target->ConstrainClippingRegion(NULL);
}
//------------------------------------------------------------------------------
void BColorControl::ColorRamp(BRect r, BRect where, BView *target, rgb_color c,
							  int16 flag, bool focused)
{
	rgb_color color = {255, 255, 255, 255};
	float width = where.Width()+1;

	target->BeginLineArray(width);

	for (float i = 0; i <= width; i++)
	{
		color.red = (uint8)(i * c.red / width);
		color.green = (uint8)(i * c.green / width);
		color.blue = (uint8)(i * c.blue / width);

		target->AddLine(BPoint(where.left + i, where.top),
			BPoint(where.left + i, where.bottom),
			color);
	}

	target->EndLineArray();
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
void
BColorControl::InitData(color_control_layout layout, float size,
	bool useOffscreen, BMessage *data)
{
	BRect bounds(Bounds());

	fColumns = layout;
	fRows = 256 / fColumns;

	fTState = NULL;

	fCellSize = size;

	fRound = 1.0f;
	fFastSet = false;

	BScreen screen(Window());
	fLastMode = screen.ColorSpace();

	if (useOffscreen) {
		fBitmap = new BBitmap(bounds, B_RGB32, true, false);
		fOffscreenView = new BView(bounds, "off_view", 0, 0);

		fBitmap->Lock();
		fBitmap->AddChild(fOffscreenView);
		fBitmap->Unlock();
	} else {
		fBitmap = NULL;
		fOffscreenView = NULL;
	}

	fFocused = false;
	fCachedIndex = -1;
	fRetainCache = false;

	if (data) {
		int32 _val = 0;
		data->FindInt32("_val", &_val);

		SetValue(_val);

		fRedText = (BTextControl*)FindView("_red");
		fGreenText = (BTextControl*)FindView("_green");
		fBlueText = (BTextControl*)FindView("_blue");
	} else {
		BRect rect(0.0f, 0.0f, 70.0f, 15.0f);
		float labelWidth = StringWidth("Green:") + 5;
		rect.right = labelWidth + StringWidth("999") + 20;

		// red

		fRedText = new BTextControl(rect, "_red", "Red:", "0",
			new BMessage('ccol'), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fRedText->SetDivider(labelWidth);

		float offset = fRedText->Bounds().Height() + 2;

		for (int32 i = 0; i < 256; i++)
			fRedText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fRedText->TextView()->AllowChar(i);
		fRedText->TextView()->SetMaxBytes(3);

		// green

		rect.OffsetBy(0.0f, offset);
		fGreenText = new BTextControl(rect, "_green", "Green:", "0",
			new BMessage('ccol'), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fGreenText->SetDivider(labelWidth);

		for (int32 i = 0; i < 256; i++)
			fGreenText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fGreenText->TextView()->AllowChar(i);
		fGreenText->TextView()->SetMaxBytes(3);

		// blue

		rect.OffsetBy(0.0f, offset);
		fBlueText = new BTextControl(rect, "_blue", "Blue:", "0",
			new BMessage('ccol'), B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_NAVIGABLE);
		fBlueText->SetDivider(labelWidth);

		for (int32 i = 0; i < 256; i++)
			fBlueText->TextView()->DisallowChar(i);
		for (int32 i = '0'; i <= '9'; i++)
			fBlueText->TextView()->AllowChar(i);
		fBlueText->TextView()->SetMaxBytes(3);

		LayoutView(false);
		AddChild(fRedText);
		AddChild(fGreenText);
		AddChild(fBlueText);
	}
}


void
BColorControl::DoMouseMoved(BPoint pt)
{
}


void
BColorControl::DoMouseUp(BPoint pt)
{
}

