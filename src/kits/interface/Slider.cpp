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
//	File Name:		Slider.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BSlider creates and displays a sliding thumb control.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Slider.h>
#include <Message.h>
#include <Window.h>
#include <Bitmap.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
rgb_color _long_to_color_(int32 color)
{
	return *((rgb_color*)&color);
}

int32 _color_to_long_(rgb_color color)
{
	return *((int32*)&color);
}

//------------------------------------------------------------------------------
BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message, 
				 int32 minValue, int32 maxValue, thumb_style thumbType, 
				 uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags)
{
	fModificationMessage = NULL;
	fSnoozeAmount = 20000;
	fOrientation = B_HORIZONTAL;
	fBarThickness = 6.0f;
	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;
	fMinValue = minValue;
	fMaxValue = maxValue;

	SetValue(0);

	fKeyIncrementValue = 1;
	fHashMarkCount = 0;
	fHashMarks = B_HASH_MARKS_NONE;
	fStyle = thumbType;

	if (Style() == B_BLOCK_THUMB)
		SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));
	else
		SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));

	UseFillColor(false, NULL);

	_InitObject();
}
//------------------------------------------------------------------------------
BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message, 
				 int32 minValue, int32 maxValue, orientation posture,
				 thumb_style thumbType, uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags)
{
	fModificationMessage = NULL;
	fSnoozeAmount = 20000;
	fOrientation = posture;
	fBarThickness = 6.0f;
	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;
	fMinValue = minValue;
	fMaxValue = maxValue;

	SetValue(0);

	fKeyIncrementValue = 1;
	fHashMarkCount = 0;
	fHashMarks = B_HASH_MARKS_NONE;
	fStyle = thumbType;

	if (Style() == B_BLOCK_THUMB)
		SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));
	else
		SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_4_TINT));

	UseFillColor(false, NULL);

	_InitObject();
}
//------------------------------------------------------------------------------
BSlider::~BSlider()
{
	if (fOffScreenBits)
		delete fOffScreenBits;

	if (fModificationMessage)
		delete fModificationMessage;

	if (fMinLimitStr)
		free(fMinLimitStr);

	if (fMaxLimitStr)
		free(fMaxLimitStr);
}
//------------------------------------------------------------------------------
BSlider::BSlider(BMessage *archive)
	:	BControl (archive)
{
	fModificationMessage = NULL;
		 
	if (archive->HasMessage("_mod_msg"))
	{
		BMessage *message = new BMessage;

		archive->FindMessage("_mod_msg", message);
		
		SetModificationMessage(message);
	}

	if (archive->FindInt32("_sdelay", &fSnoozeAmount) != B_OK)
		SetSnoozeAmount(20000);

	int32 color;

	if(archive->FindInt32("_fcolor", &color) == B_OK)
	{
		rgb_color fillColor = _long_to_color_(color);
		UseFillColor(true, &fillColor);
	}
	else
		UseFillColor(false);

	int32 orient;

	if(archive->FindInt32("_orient", &orient) == B_OK)
		fOrientation = (orientation)orient;
	else
		fOrientation = B_HORIZONTAL;

	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;

	const char *minlbl = NULL, *maxlbl = NULL;

	archive->FindString("_minlbl", &minlbl);
	archive->FindString("_maxlbl", &maxlbl);

	SetLimitLabels(minlbl, maxlbl);

	if (archive->FindInt32("_min", &fMinValue) != B_OK)
		fMinValue = 0;

	if (archive->FindInt32("_max", &fMaxValue) != B_OK)
		fMaxValue = 100;

	if (archive->FindInt32("_incrementvalue", &fKeyIncrementValue) != B_OK)
		fKeyIncrementValue = 1;

	if (archive->FindInt32("_hashcount", &fHashMarkCount) != B_OK)
		fHashMarkCount = 11;

	int16 hashloc;

	if(archive->FindInt16("_hashloc", &hashloc) == B_OK)
		fHashMarks = (hash_mark_location)hashloc;
	else
		fHashMarks = B_HASH_MARKS_NONE;

	int16 sstyle;

	if(archive->FindInt16("_sstyle", &sstyle) == B_OK)
		fStyle = (thumb_style)sstyle;
	else
		fStyle = B_BLOCK_THUMB;

	if(archive->FindInt32("_bcolor", &color) == B_OK)
		SetBarColor(_long_to_color_(color));
	else
	{
		if (Style() == B_BLOCK_THUMB)
			SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_DARKEN_4_TINT));
		else
			SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_DARKEN_4_TINT));
	}

	float bthickness;

	if (archive->FindFloat("_bthickness", &bthickness) == B_OK)
		fBarThickness = bthickness;
	else
		fBarThickness = 6.0f;

	_InitObject();
}
//------------------------------------------------------------------------------
BArchivable *BSlider::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BSlider"))
		return new BSlider(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BSlider::Archive(BMessage *archive, bool deep) const
{
	BControl::Archive(archive, deep);

	if (ModificationMessage())
		archive->AddMessage("_mod_msg", ModificationMessage());

	archive->AddInt32("_sdelay", fSnoozeAmount);

	archive->AddInt32("_bcolor", _color_to_long_(fBarColor));

	if (FillColor(NULL))
		archive->AddInt32("_fcolor", _color_to_long_(fFillColor));

	if (fMinLimitStr)
		archive->AddString("_minlbl", fMinLimitStr);

	if (fMaxLimitStr)
		archive->AddString("_maxlbl", fMaxLimitStr);

	archive->AddInt32("_min", fMinValue);
	archive->AddInt32("_max", fMaxValue);

	archive->AddInt32("_incrementvalue", fKeyIncrementValue);

	archive->AddInt32("_hashcount", fHashMarkCount);

	archive->AddInt16("_hashloc", fHashMarks);
	
	archive->AddInt16("_sstyle", fStyle);

	archive->AddInt32("_orient", fOrientation);

	archive->AddFloat("_bthickness", fBarThickness);

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BSlider::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BSlider::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}
//------------------------------------------------------------------------------
void BSlider::AttachedToWindow()
{
	ResizeToPreferred();

	fLocation.Set(9.0f, 0.0f);

	BRect bounds(Bounds());

	if (!fOffScreenView)
	{
		fOffScreenView = new BView(bounds, "", B_FOLLOW_ALL, B_WILL_DRAW);

		BFont font;
		GetFont(&font);
		fOffScreenView->SetFont(&font);
	}

	if (!fOffScreenBits)
	{
		// TODO: should use B_CMAP8
		fOffScreenBits = new BBitmap(bounds, B_CMAP8, true, false);

		if (fOffScreenBits && fOffScreenView)
			fOffScreenBits->AddChild(fOffScreenView);
	}
	else if (fOffScreenView)
		fOffScreenBits->AddChild(fOffScreenView);

	SetValue(Value());

	if (fOffScreenView && Parent())
	{
		rgb_color color = Parent()->ViewColor();

		fOffScreenBits->Lock();
		fOffScreenView->SetViewColor(color);
		fOffScreenView->SetLowColor(color);
		fOffScreenBits->Unlock();
	}

	BControl::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BSlider::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BSlider::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
void BSlider::DetachedFromWindow()
{
	BControl::DetachedFromWindow();

	if (fOffScreenBits)
	{
		delete fOffScreenBits;
		fOffScreenBits = NULL;
		fOffScreenView = NULL;
	}
}
//------------------------------------------------------------------------------
void BSlider::MessageReceived(BMessage *msg)
{
	BSlider::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BSlider::FrameMoved(BPoint new_position)
{
	BSlider::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
void BSlider::FrameResized(float w,float h)
{
	BControl::FrameResized(w, h);

	BRect bounds(Bounds());

	if (bounds.right <= 0.0f || bounds.bottom <= 0.0f)
		return;

	if (fOffScreenBits)
	{
		fOffScreenBits->RemoveChild(fOffScreenView);
		delete fOffScreenBits;

		fOffScreenView->ResizeTo(bounds.Width(), bounds.Height());
	
		fOffScreenBits = new BBitmap(Bounds(), B_CMAP8, true, false);
		fOffScreenBits->AddChild(fOffScreenView);
	}

	SetValue(Value());
	// virtual
}
//------------------------------------------------------------------------------
void BSlider::KeyDown(const char *bytes, int32 numBytes)
{
	if (!IsEnabled() || IsHidden())
		return;

	switch (bytes[0])
	{
		case B_LEFT_ARROW:
		case B_DOWN_ARROW:
		{
			SetValue(Value() - KeyIncrementValue());
			Invoke();
			break;
		}
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		{
			SetValue(Value() + KeyIncrementValue());
			Invoke();
			break;
		}
		default:
			BControl::KeyDown(bytes, numBytes);
	}
}
//------------------------------------------------------------------------------
void BSlider::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	if (BarFrame().Contains(point) || ThumbFrame().Contains(point))
		fInitialLocation = _Location();

	BPoint pt;
	uint32 buttons;

	GetMouse(&pt, &buttons, true);

	if (fOrientation == B_HORIZONTAL)
	{
		if (pt.x < _MinPosition())
			pt.x = _MinPosition();
		else if (pt.x > _MaxPosition())
			pt.x = _MaxPosition();
	}
	else
	{
		if (pt.y > _MinPosition())
			pt.y = _MinPosition();
		else if (pt.y < _MaxPosition())
			pt.y = _MaxPosition();
	}

	SetValue(ValueForPoint(pt));
	//virtual
	InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS)
	{
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
	else
	{
		BPoint prevPt;
		bool update;

		while (buttons)
		{
			prevPt = pt;
			update = false;

			snooze(SnoozeAmount());
			GetMouse(&pt, &buttons, true);

			if (fOrientation == B_HORIZONTAL)
			{
				if (pt.x != prevPt.x)
				{
					update = true;

					if (pt.x < _MinPosition())
						pt.x = _MinPosition();
					else if (pt.x > _MaxPosition())
						pt.x = _MaxPosition();
				}
			}
			else
			{
				if (pt.y != prevPt.y)
				{
					update = true;

					if (pt.y > _MinPosition())
						pt.y = _MinPosition();
					else if (pt.y < _MaxPosition())
						pt.y = _MaxPosition();
				}
			}

			if (update)
			{
				SetValue(ValueForPoint(pt));
				//virtual
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
		}
	}

	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) == 0)
	{
		if (_Location() != fInitialLocation)
			Invoke();
	}
}
//------------------------------------------------------------------------------
void BSlider::MouseUp(BPoint point)
{
	if (IsTracking())
	{
		if (_Location() != fInitialLocation)
			Invoke();

		SetTracking(false);
	}
	else
		BControl::MouseUp(point);
}
//------------------------------------------------------------------------------
void BSlider::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsTracking())
	{
		BPoint loc = _Location();
		bool update = false;

		if (fOrientation == B_HORIZONTAL)
		{
			if (point.x != loc.x)
			{
				update = true;

				if (point.x < _MinPosition())
					point.x = _MinPosition();
				else if (point.x > _MaxPosition())
					point.x = _MaxPosition();
			}
		}
		else
		{
			if (point.y != loc.y)
			{
				update = true;

				if (point.y < _MinPosition())
					point.y = _MinPosition();
				else if (point.y > _MaxPosition())
					point.y = _MaxPosition();
			}
		}

		if (update)
		{
			SetValue(ValueForPoint(point));
			//virtual
		}

		if (_Location() != loc)
			InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
	}
	else
		BControl::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BSlider::Pulse()
{
	BControl::Pulse();
}
//------------------------------------------------------------------------------
void BSlider::SetLabel(const char *label)
{
	BControl::SetLabel(label);
}
//------------------------------------------------------------------------------
void BSlider::SetLimitLabels(const char *minLabel, const char *maxLabel)
{
	if (minLabel)
	{
		if (fMinLimitStr)
			free(fMinLimitStr);
		fMinLimitStr = strdup(minLabel);
	}

	if (maxLabel)
	{
		if (fMaxLimitStr)
			free(fMaxLimitStr);
		fMaxLimitStr = strdup(maxLabel);
	}

	ResizeToPreferred();
	Invalidate();
}
//------------------------------------------------------------------------------
const char *BSlider::MinLimitLabel() const
{
	return fMinLimitStr;
}
//------------------------------------------------------------------------------
const char *BSlider::MaxLimitLabel() const
{
	return fMaxLimitStr;
}
//------------------------------------------------------------------------------
void BSlider::SetValue(int32 value)
{
	if (value < fMinValue)
		value = fMinValue;
	if (value > fMaxValue)
		value = fMaxValue;

	BPoint loc;
	float pos = (float)(value - fMinValue) / (float)(fMaxValue - fMinValue) *
		_MaxPosition() - _MinPosition();

	if (fOrientation == B_HORIZONTAL)
	{
		loc.x = ceil(_MinPosition() + pos);
		loc.y = 0;
	}
	else
	{
		loc.x = 0;
		loc.y = floor(_MaxPosition() - pos);
	}

	_SetLocation(loc);

	BControl::SetValue(value);
}
//------------------------------------------------------------------------------
int32 BSlider::ValueForPoint(BPoint location) const
{
	if (fOrientation == B_HORIZONTAL)
		return (int32)((location.x - _MinPosition()) * (fMaxValue - fMinValue) /
			(_MaxPosition() - _MinPosition())) + fMinValue;
	else
		return (int32)((location.y - _MinPosition()) * (fMaxValue - fMinValue) /
			(_MaxPosition() - _MinPosition())) + fMinValue;
}
//------------------------------------------------------------------------------
void BSlider::SetPosition(float position)
{
	if (position <= 0.0f)
		BControl::SetValue(fMinValue);
	else if (position >= 1.0f)
		BControl::SetValue(fMaxValue);
	else
		BControl::SetValue((int32)(position * (fMaxValue - fMinValue) + fMinValue));
}
//------------------------------------------------------------------------------
float BSlider::Position() const
{
	return ((float)(Value() - fMinValue) / (float)(fMaxValue - fMinValue));
}
//------------------------------------------------------------------------------
void BSlider::SetEnabled(bool on)
{
	BControl::SetEnabled(on);
}
//------------------------------------------------------------------------------
void BSlider::GetLimits(int32 *minimum, int32 *maximum)
{
	*minimum = fMinValue;
	*maximum = fMaxValue;
}
//------------------------------------------------------------------------------
void BSlider::Draw(BRect updateRect)
{
	DrawSlider();
}
//------------------------------------------------------------------------------
void BSlider::DrawSlider()
{
	if (!fOffScreenBits)
		return;

	if (fOffScreenBits->Lock())
	{
		OffscreenView()->SetHighColor(ViewColor());
		OffscreenView()->FillRect(Bounds());

		DrawBar();
		DrawHashMarks();
		DrawThumb();
		DrawFocusMark();
		DrawText();
		OffscreenView()->Sync();
		DrawBitmap(fOffScreenBits, B_ORIGIN);

		fOffScreenBits->Unlock();
	}
}
//------------------------------------------------------------------------------
void BSlider::DrawBar()
{
	BRect frame = BarFrame();
	BView *view = OffscreenView();

	if (fUseFillColor)
	{
		if (fOrientation == B_HORIZONTAL)
		{
			view->SetHighColor(fBarColor);
			view->FillRect(BRect((float)floor(frame.left + 1 + Position() *
				(frame.Width() - 2)), frame.top, frame.right, frame.bottom));

			view->SetHighColor(fFillColor);
			view->FillRect(BRect(frame.left, frame.top,
				(float)floor(frame.left + 1 + Position() * (frame.Width() - 2)),
				frame.bottom));
		}
		else
		{
			view->SetHighColor(fBarColor);
			view->FillRect(BRect(frame.left, frame.top, frame.right,
				(float)floor(frame.bottom - 1 - Position() * (frame.Height() - 2))));

			view->SetHighColor(fFillColor);
			view->FillRect(BRect(frame.left,
				(float)floor(frame.bottom - 1 - Position() *
				(frame.Height() - 2)), frame.right, frame.bottom));

		}
	}
	else
	{
		view->SetHighColor(fBarColor);
		view->FillRect(frame);
	}

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
		darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
		darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);
	
	view->SetHighColor(darken1);
	view->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.left + 1.0f, frame.top));
	view->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.left + 1.0f, frame.bottom));
	view->StrokeLine(BPoint(frame.right - 1.0f, frame.top),
		BPoint(frame.right, frame.top));

	view->SetHighColor(darken2);
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
		BPoint(frame.right - 1.0f, frame.top));
	view->StrokeLine(BPoint(frame.left, frame.bottom - 1.0f),
		BPoint(frame.left, frame.top + 1.0f));

	view->SetHighColor(lightenmax);
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.bottom),
		BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.top + 1.0f));

	frame.InsetBy(1.0f, 1.0f);

	view->SetHighColor(darkenmax);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.left, frame.top));
	view->StrokeLine(BPoint(frame.right, frame.top));
}
//------------------------------------------------------------------------------
void BSlider::DrawHashMarks()
{
	BRect frame = HashMarksFrame();
	BView *view = OffscreenView();
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT);

	float pos = _MinPosition();
	float factor = (_MaxPosition() - pos) / (fHashMarkCount - 1);

	if (fHashMarks & B_HASH_MARKS_TOP)
	{
		if (fOrientation == B_HORIZONTAL)
		{
			for (int32 i = 0; i < fHashMarkCount; i++)
			{
				view->SetHighColor(darken2);
				view->StrokeLine(BPoint(pos, frame.top),
					BPoint(pos, frame.top + 5));
				view->SetHighColor(lightenmax);
				view->StrokeLine(BPoint(pos + 1, frame.top),
					BPoint(pos + 1, frame.top + 5));

				pos += factor;
			}
		}
		else
		{
			for (int32 i = 0; i < fHashMarkCount; i++)
			{
				view->SetHighColor(darken2);
				view->StrokeLine(BPoint(frame.left, pos),
					BPoint(frame.left + 5, pos));
				view->SetHighColor(lightenmax);
				view->StrokeLine(BPoint(frame.left, pos + 1),
					BPoint(frame.left + 5, pos + 1));

				pos += factor;
			}
		}
	}

	pos = _MinPosition();

	if (fHashMarks & B_HASH_MARKS_BOTTOM)
	{
		if (fOrientation == B_HORIZONTAL)
		{
			for (int32 i = 0; i < fHashMarkCount; i++)
			{
				view->SetHighColor(darken2);
				view->StrokeLine(BPoint(pos, frame.bottom - 5),
					BPoint(pos, frame.bottom));
				view->SetHighColor(lightenmax);
				view->StrokeLine(BPoint(pos + 1, frame.bottom - 5),
					BPoint(pos + 1, frame.bottom));

				pos += factor;
			}
		}
		else
		{
			for (int32 i = 0; i < fHashMarkCount; i++)
			{
				view->SetHighColor(darken2);
				view->StrokeLine(BPoint(frame.right - 5, pos),
					BPoint(frame.right, pos));
				view->SetHighColor(lightenmax);
				view->StrokeLine(BPoint(frame.right - 5, pos + 1),
					BPoint(frame.right, pos + 1));

				pos += factor;
			}
		}
	}
}
//------------------------------------------------------------------------------
void BSlider::DrawThumb()
{
	if (Style() == B_BLOCK_THUMB)
		_DrawBlockThumb();
	else
		_DrawTriangleThumb();
}
//------------------------------------------------------------------------------
void BSlider::DrawFocusMark()
{
	if (!IsFocus())
		return;

	OffscreenView()->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

	BRect frame = ThumbFrame();

	if (fStyle == B_BLOCK_THUMB)
	{
		frame.left += 2.0f;
		frame.top += 2.0f;
		frame.right -= 3.0f;
		frame.bottom -= 3.0f;
		OffscreenView()->StrokeRect(frame);
	}
	else
	{
		if (fOrientation == B_HORIZONTAL)
			OffscreenView()->StrokeLine(BPoint(frame.left, frame.bottom + 3.0f),
				BPoint(frame.right, frame.bottom + 3.0f));
		else
			OffscreenView()->StrokeLine(BPoint(frame.left - 2.0f, frame.top),
				BPoint(frame.left - 2.0f, frame.bottom));
	}
	
}
//------------------------------------------------------------------------------
void BSlider::DrawText()
{
	BRect bounds(Bounds());
	BView *view = OffscreenView();

	view->SetHighColor(0, 0, 0);

	font_height fheight;

	GetFontHeight(&fheight);

	if (Orientation() == B_HORIZONTAL)
	{
		if (Label())
			view->DrawString(Label(), BPoint(2.0f, (float)ceil(fheight.ascent)));

		if (fMinLimitStr)
			view->DrawString(fMinLimitStr, BPoint(2.0f, bounds.bottom - 4.0f));

		if (fMaxLimitStr)
			view->DrawString(fMaxLimitStr, BPoint(bounds.right -
				StringWidth(fMaxLimitStr) - 2.0f, bounds.bottom - 4.0f));
	}
	else
	{
		float ascent = (float)ceil(fheight.ascent);

		if (Label())
			view->DrawString(Label(), BPoint(bounds.Width() / 2.0f -
				StringWidth(Label()) / 2.0f, ascent));

		if (fMaxLimitStr)
			view->DrawString(fMaxLimitStr, BPoint(bounds.Width() / 2.0f -
				StringWidth(fMaxLimitStr) / 2.0f, ascent +
				(Label() ? (float)ceil(ascent + fheight.descent + 2.0f) : 0.0f)));

		if (fMinLimitStr)
			view->DrawString(fMinLimitStr, BPoint(bounds.Width() / 2.0f -
				StringWidth(fMinLimitStr) / 2.0f,
				bounds.bottom - 2.0f));
	}
}
//------------------------------------------------------------------------------
char *BSlider::UpdateText() const
{
	return NULL;
}
//------------------------------------------------------------------------------
BRect BSlider::BarFrame() const
{
	BRect frame(Bounds());
	font_height fheight;

	GetFontHeight(&fheight);

	float textHeight = (float)ceil(fheight.ascent + fheight.descent);
	
	if (fStyle == B_BLOCK_THUMB)
	{
		if (Orientation() == B_HORIZONTAL)
		{
			frame.left = 8.0f;
			frame.top = 6.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right -= 8.0f;
			frame.bottom = frame.bottom - 6.0f -
				(fMinLimitStr || fMaxLimitStr ? textHeight + 4.0f : 0.0f);
		}
		else
		{
			frame.left = frame.Width() / 2.0f - 3;
			frame.top = 12.0f + (Label() ? textHeight : 0.0f) +
				(fMaxLimitStr ? textHeight : 0.0f);
			frame.right = frame.left + 6;
			frame.bottom = frame.bottom - 8.0f -
				(fMinLimitStr ? textHeight + 4 : 0.0f);
		}
	}
	else
	{
		if (Orientation() == B_HORIZONTAL)
		{
			frame.left = 7.0f;
			frame.top = 6.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right -= 7.0f;
			frame.bottom = frame.bottom - 6.0f -
				(fMinLimitStr || fMaxLimitStr ? textHeight + 4.0f : 0.0f);
		}
		else
		{
			frame.left = frame.Width() / 2.0f - 3;
			frame.top = 11.0f + (Label() ? textHeight : 0.0f) +
				(fMaxLimitStr ? textHeight : 0.0f);
			frame.right = frame.left + 6;
			frame.bottom = frame.bottom - 7.0f -
				(fMinLimitStr ? textHeight + 4 : 0.0f);
		}
	}

	return frame;
}
//------------------------------------------------------------------------------
BRect BSlider::HashMarksFrame() const
{
	BRect frame(BarFrame());

	if (fOrientation == B_HORIZONTAL)
	{
		frame.top -= 6.0f;
		frame.bottom += 6.0f;
	}
	else
	{
		frame.left -= 6.0f;
		frame.right += 6.0f;
	}

	return frame;
}
//------------------------------------------------------------------------------
BRect BSlider::ThumbFrame() const
{
	BRect frame = Bounds();
	font_height fheight;

	GetFontHeight(&fheight);

	float textHeight = (float)ceil(fheight.ascent + fheight.descent);

	if (fStyle == B_BLOCK_THUMB)
	{
		if (Orientation() == B_HORIZONTAL)
		{
			frame.left = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 8.0f;
			frame.top = 2.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right = frame.left + 17.0f;
			frame.bottom = frame.bottom - 3.0f -
				(MinLimitLabel() || MaxLimitLabel() ? textHeight + 4.0f : 0.0f);
		}
		else
		{
			frame.left = frame.Width() / 2.0f - 7;
			frame.top = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 8.0f;
			frame.right = frame.left + 13;
			frame.bottom = frame.top + 17;
		}
	}
	else
	{
		if (Orientation() == B_HORIZONTAL)
		{
			frame.left = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 6;
			frame.top = 9.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right = frame.left + 12.0f;
			frame.bottom = frame.bottom - 3.0f -
				(MinLimitLabel() || MaxLimitLabel() ? textHeight + 4.0f : 0.0f);
		}
		else
		{
			frame.left = frame.Width() / 2.0f - 6;
			frame.top = (float)floor(Position() * (_MaxPosition() - _MinPosition())) +
				_MinPosition() - 6.0f;
			frame.right = frame.left + 7;
			frame.bottom = frame.top + 12;
		}
	}

	return frame;
}
//------------------------------------------------------------------------------
void BSlider::SetFlags(uint32 flags)
{
	BControl::SetFlags(flags);
}
//------------------------------------------------------------------------------
void BSlider::SetResizingMode(uint32 mode)
{
	BControl::SetResizingMode(mode);
}
//------------------------------------------------------------------------------
void BSlider::GetPreferredSize(float *width, float *height)
{
	font_height fheight;

	GetFontHeight(&fheight);

	if (Orientation() == B_HORIZONTAL)
	{
		*width = (Frame().Width() < 32.0f) ? 32.0f : Frame().Width();
		*height = 18.0f;
		
		if (Label())
			*height += (float)ceil(fheight.ascent + fheight.descent) + 4.0f;

		if (MinLimitLabel() || MaxLimitLabel())
			*height += (float)ceil(fheight.ascent + fheight.descent) + 4.0f;
	}
	else // B_VERTICAL
	{
		*width = (Frame().Width() < 18.0f) ? 18.0f : Frame().Width();
		*height = 32.0f;
		
		if (Label())
			*height += (float)ceil(fheight.ascent + fheight.descent) + 4.0f;

		if (MaxLimitLabel())
			*height += (float)ceil(fheight.ascent + fheight.descent) + 4.0f;

		if (MinLimitLabel())
			*height += (float)ceil(fheight.ascent + fheight.descent) + 4.0f;

		if (Label() && (MaxLimitLabel()))
			*height -= 4.0f;

		*height = (Frame().Height() < *height) ? *height : Frame().Height();
	}
}
//------------------------------------------------------------------------------
void BSlider::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}
//------------------------------------------------------------------------------
status_t BSlider::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}
//------------------------------------------------------------------------------
BHandler *BSlider::ResolveSpecifier(BMessage *message, int32 index,
									BMessage *specifier, int32 command,
									const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, command,
		property);
}
//------------------------------------------------------------------------------
status_t BSlider::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
void BSlider::SetModificationMessage(BMessage *message)
{
	if (fModificationMessage)
		delete fModificationMessage;

	fModificationMessage = message;
}
//------------------------------------------------------------------------------
BMessage *BSlider::ModificationMessage() const
{
	return fModificationMessage;
}
//------------------------------------------------------------------------------
void BSlider::SetSnoozeAmount(int32 snooze_time)
{
	if (snooze_time < 5000)
		snooze_time = 5000;
	if (snooze_time > 1000000)
		snooze_time = 1000000;

	fSnoozeAmount = snooze_time;
}
//------------------------------------------------------------------------------
int32 BSlider::SnoozeAmount() const
{
	return fSnoozeAmount;
}
//------------------------------------------------------------------------------
void BSlider::SetKeyIncrementValue(int32 increment_value)
{
	fKeyIncrementValue = increment_value;
}
//------------------------------------------------------------------------------
int32 BSlider::KeyIncrementValue() const
{
	return fKeyIncrementValue;
}
//------------------------------------------------------------------------------
void BSlider::SetHashMarkCount(int32 hash_mark_count)
{
	fHashMarkCount = hash_mark_count;
	Invalidate();
}
//------------------------------------------------------------------------------
int32 BSlider::HashMarkCount() const
{
	return fHashMarkCount;
}
//------------------------------------------------------------------------------
void BSlider::SetHashMarks(hash_mark_location where)
{
	fHashMarks = where;
	Invalidate();
}
//------------------------------------------------------------------------------
hash_mark_location BSlider::HashMarks() const
{
	return fHashMarks;
}
//------------------------------------------------------------------------------
void BSlider::SetStyle(thumb_style style)
{
	fStyle = style;
}
//------------------------------------------------------------------------------
thumb_style BSlider::Style() const
{
	return fStyle;
}
//------------------------------------------------------------------------------
void BSlider::SetBarColor(rgb_color bar_color)
{
	fBarColor = bar_color;
}
//------------------------------------------------------------------------------
rgb_color BSlider::BarColor() const
{
	return fBarColor;
}
//------------------------------------------------------------------------------
void BSlider::UseFillColor(bool use_fill, const rgb_color *bar_color)
{
	fUseFillColor = use_fill;

	if (use_fill && bar_color)
		fFillColor = *bar_color;
}
//------------------------------------------------------------------------------
bool BSlider::FillColor(rgb_color *bar_color) const
{
	if (bar_color && fUseFillColor)
		*bar_color = fFillColor;

	return fUseFillColor;
}
//------------------------------------------------------------------------------
BView *BSlider::OffscreenView() const
{
	return fOffScreenView;
}
//------------------------------------------------------------------------------
orientation BSlider::Orientation() const
{
	return fOrientation;
}
//------------------------------------------------------------------------------
void BSlider::SetOrientation(orientation posture)
{
	fOrientation = posture;
}
//------------------------------------------------------------------------------
float BSlider::BarThickness() const
{
	return fBarThickness;
}
//------------------------------------------------------------------------------
void BSlider::SetBarThickness(float thickness)
{
	fBarThickness = thickness;
}
//------------------------------------------------------------------------------
void BSlider::SetFont(const BFont *font, uint32 properties)
{
	BControl::SetFont(font, properties);

	if (fOffScreenView && fOffScreenBits)
	{
		if (fOffScreenBits->Lock())
		{
			fOffScreenView->SetFont(font, properties);
			fOffScreenBits->Unlock();
		}
	}
}
//------------------------------------------------------------------------------
#ifdef __HAIKU__
void BSlider::SetLimits(int32 minimum, int32 maximum)
{
	// TODO: Redraw
	fMinValue = minimum;
	fMaxValue = maximum;
}
#endif
//------------------------------------------------------------------------------
void BSlider::_DrawBlockThumb()
{
	BRect frame = ThumbFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
		darken3 = tint_color(no_tint, B_DARKEN_3_TINT),
		darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);

	// Outline
	view->SetHighColor(darken3);
	view->StrokeLine(BPoint(frame.left, frame.bottom - 2.0f),
		BPoint(frame.left, frame.top + 1.0f));
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
		BPoint(frame.right - 2.0f, frame.top));
	view->StrokeLine(BPoint(frame.right, frame.top + 2.0f),
		BPoint(frame.right, frame.bottom - 1.0f));
	view->StrokeLine(BPoint(frame.left + 2.0f, frame.bottom),
		BPoint(frame.right - 1.0f, frame.bottom));

	// First bevel
	frame.InsetBy(1.0f, 1.0f);

	view->SetHighColor(lighten2);
	view->FillRect(frame);

	view->SetHighColor(darkenmax);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right - 1.0f, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.bottom - 1),
		BPoint(frame.right, frame.top));

	frame.InsetBy(1.0f, 1.0f);

	// Second bevel and center dots
	view->SetHighColor(darken2);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.top));

	if (Orientation() == B_HORIZONTAL)
	{
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 2.0f),
			BPoint(frame.left + 6.0f, frame.top + 2.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 4.0f),
			BPoint(frame.left + 6.0f, frame.top + 4.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 6.0f),
			BPoint(frame.left + 6.0f, frame.top + 6.0f));
	}
	else
	{
		view->StrokeLine(BPoint(frame.left + 2.0f, frame.top + 6.0f),
			BPoint(frame.left + 2.0f, frame.top + 6.0f));
		view->StrokeLine(BPoint(frame.left + 4.0f, frame.top + 6.0f),
			BPoint(frame.left + 4.0f, frame.top + 6.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 6.0f),
			BPoint(frame.left + 6.0f, frame.top + 6.0f));
	}

	view->StrokeLine(BPoint(frame.right + 1.0f, frame.bottom + 1.0f),
		BPoint(frame.right + 1.0f, frame.bottom + 1.0f));

	frame.InsetBy(1.0f, 1.0f);

	// Third bevel
	view->SetHighColor(no_tint);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.top));
}
//------------------------------------------------------------------------------
void BSlider::_DrawTriangleThumb()
{
	BRect frame = ThumbFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
//		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
//		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
//		darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
//		darken3 = tint_color(no_tint, B_DARKEN_3_TINT),
		darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);
	
	if (Orientation() == B_HORIZONTAL)
	{
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(frame.left, frame.bottom - 1.0f),
			BPoint(frame.left + 6.0f, frame.top),
			BPoint(frame.right, frame.bottom - 1.0f));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(frame.right, frame.bottom + 1),
			BPoint(frame.left, frame.bottom + 1));
		view->StrokeLine(BPoint(frame.right, frame.bottom),
			BPoint(frame.left + 6.0f, frame.top));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(frame.right - 1, frame.bottom),
			BPoint(frame.left, frame.bottom));
		view->StrokeLine(BPoint(frame.left, frame.bottom),
			BPoint(frame.left + 5.0f, frame.top + 1));

		view->SetHighColor(no_tint);
		view->StrokeLine(BPoint(frame.right - 2, frame.bottom - 1.0f),
			BPoint(frame.left + 3.0f, frame.bottom - 1.0f));
		view->StrokeLine(BPoint(frame.right - 3, frame.bottom - 2.0f),
			BPoint(frame.left + 6.0f, frame.top + 1));
	}
	else
	{
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(frame.left + 1.0f, frame.top),
			BPoint(frame.left + 7.0f, frame.top + 6.0f),
			BPoint(frame.left + 1.0f, frame.bottom));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(frame.left, frame.top + 1),
			BPoint(frame.left, frame.bottom));
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.bottom),
			BPoint(frame.left + 7.0f, frame.top + 6.0f));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(frame.left, frame.top),
			BPoint(frame.left, frame.bottom - 1));
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
			BPoint(frame.left + 6.0f, frame.top + 5.0f));

		view->SetHighColor(no_tint);
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.top + 2.0f),
			BPoint(frame.left + 1.0f, frame.bottom - 1.0f));
		view->StrokeLine(BPoint(frame.left + 2.0f, frame.bottom - 2.0f),
			BPoint(frame.left + 6.0f, frame.top + 6.0f));
	}
}
//------------------------------------------------------------------------------
BPoint BSlider::_Location() const
{
	return fLocation;
}
//------------------------------------------------------------------------------
void BSlider::_SetLocation(BPoint p)
{
	fLocation = p;
}
//------------------------------------------------------------------------------
float BSlider::_MinPosition() const
{
	if (fOrientation == B_HORIZONTAL)
		return BarFrame().left + 1.0f;
	else
		return BarFrame().bottom - 1.0f;
}
//------------------------------------------------------------------------------
float BSlider::_MaxPosition() const
{
	if (fOrientation == B_HORIZONTAL)
		return BarFrame().right - 1.0f;
	else
		return BarFrame().top + 1.0f;
}
//------------------------------------------------------------------------------
extern "C"
void _ReservedSlider4__7BSlider(BSlider *slider, int32 minimum, int32 maximum)
{
#ifdef __HAIKU__
	slider->SetLimits(minimum, maximum);
#endif
}
//------------------------------------------------------------------------------
void BSlider::_ReservedSlider5() {}
void BSlider::_ReservedSlider6() {}
void BSlider::_ReservedSlider7() {}
void BSlider::_ReservedSlider8() {}
void BSlider::_ReservedSlider9() {}
void BSlider::_ReservedSlider10() {}
void BSlider::_ReservedSlider11() {}
void BSlider::_ReservedSlider12() {}
//------------------------------------------------------------------------------
BSlider &BSlider::operator=(const BSlider &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BSlider::_InitObject()
{
	fLocation.x = 0;
	fLocation.y = 0;
	fInitialLocation.x = 0;
	fInitialLocation.y = 0;

	fOffScreenBits = NULL;
	fOffScreenView = NULL;
}
//------------------------------------------------------------------------------



























