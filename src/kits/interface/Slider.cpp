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
#include <malloc.h>
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

//------------------------------------------------------------------------------
BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message, 
				 int32 minValue, int32 maxValue, thumb_style thumbType, 
				 uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags),
		fModificationMessage(NULL),
		fSnoozeAmount(20000),
		fUseFillColor(false),
		fMinLimitStr(NULL),
		fMaxLimitStr(NULL),
		fMinValue(minValue),
		fMaxValue(maxValue),
		fKeyIncrementValue(1),
		fHashMarkCount(0),
		fHashMarks(B_HASH_MARKS_NONE),
		fOffScreenBits(NULL),
		fOffScreenView(NULL),
		fStyle(thumbType),
		fOrientation(B_HORIZONTAL),
		fBarThickness(6.0f)
{
	fBarColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT);
	fFillColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_MAX_TINT);
}
//------------------------------------------------------------------------------
BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message, 
				 int32 minValue, int32 maxValue, orientation posture,
				 thumb_style thumbType, uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags),
		fModificationMessage(NULL),
		fSnoozeAmount(20000),
		fUseFillColor(false),
		fMinLimitStr(NULL),
		fMaxLimitStr(NULL),
		fMinValue(minValue),
		fMaxValue(maxValue),
		fKeyIncrementValue(1),
		fHashMarkCount(0),
		fHashMarks(B_HASH_MARKS_NONE),
		fOffScreenBits(NULL),
		fOffScreenView(NULL),
		fStyle(thumbType),
		fOrientation(posture),
		fBarThickness(6.0f)
{
	fBarColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT);
	fFillColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_MAX_TINT);
}
//------------------------------------------------------------------------------
BSlider::~BSlider()
{
	if (fMinLimitStr)
		free(fMinLimitStr);

	if (fMaxLimitStr)
		free(fMaxLimitStr);
}
//------------------------------------------------------------------------------
BSlider::BSlider(BMessage *archive)
	:	BControl (archive),
		fModificationMessage(NULL),
		fSnoozeAmount(20000),
		fUseFillColor(false),
		fMinLimitStr(NULL),
		fMaxLimitStr(NULL),
		fMinValue(0),
		fMaxValue(100),
		fKeyIncrementValue(1),
		fHashMarkCount(0),
		fHashMarks(B_HASH_MARKS_NONE),
		fOffScreenBits(NULL),
		fOffScreenView(NULL),
		fStyle(B_BLOCK_THUMB),
		fOrientation(B_HORIZONTAL),
		fBarThickness(6.0f)
{
	BMessage modMessage;
	int16 anInt16;
	int32 anInt32;
	const char *aString1 = NULL, *aString2 = NULL;
	const rgb_color *aColorPtr;

	if (archive->FindMessage("_mod_msg", &modMessage))
		SetModificationMessage(new BMessage(modMessage));

	if (archive->FindInt32("_sdelay", &anInt32))
		SetSnoozeAmount(anInt32);

	if(archive->FindData("_bcolor", B_INT32_TYPE, (const void**)&aColorPtr, NULL))
		SetBarColor(*aColorPtr);

	if(archive->FindData("_fcolor", B_INT32_TYPE, (const void**)&aColorPtr, NULL))
		UseFillColor(true, aColorPtr);

	if (archive->FindString("_minlbl", &aString1) ||
		archive->FindString("_maxlbl", &aString2))
		SetLimitLabels(aString1, aString2);

	archive->FindInt32("_min", &fMinValue);
	archive->FindInt32("_max", &fMaxValue);

	archive->FindInt32("_incrementvalue", &fKeyIncrementValue);

	archive->FindInt32("_hashcount", &fHashMarkCount);

	if(archive->FindInt16("_hashloc", &anInt16))
		SetHashMarks((hash_mark_location)anInt16);

	if(archive->FindInt32("_sstyle", &anInt32))
		SetStyle((thumb_style)anInt32);
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
	status_t err = BControl::Archive(archive, deep);

	if (err != B_OK)
		return err;

	if (fModificationMessage)
		err = archive->AddMessage("_mod_msg", fModificationMessage);

	if (err != B_OK)
		return err;

	if (fSnoozeAmount != 20000)
		err = archive->AddInt32("_sdelay", fSnoozeAmount);

	if (err != B_OK)
		return err;

	err = archive->AddData("_bcolor", B_INT32_TYPE, &fBarColor, sizeof(int32));

	if (err != B_OK)
		return err;

	err = archive->AddData("_fcolor", B_INT32_TYPE,  &fFillColor, sizeof(int32));

	if (err != B_OK)
		return err;

	if (fMinLimitStr)
		err = archive->AddString("_minlbl", fMinLimitStr);

	if (err != B_OK)
		return err;

	if (fMaxLimitStr)
		err = archive->AddString("_maxlbl", fMaxLimitStr);

	if (err != B_OK)
		return err;

	err = archive->AddInt32("_min", fMinValue);

	if (err != B_OK)
		return err;

	err = archive->AddInt32("_max", fMaxValue);

	if (err != B_OK)
		return err;

	if (fKeyIncrementValue != 1)
		err = archive->AddInt32("_incrementvalue", fKeyIncrementValue);

	if (err != B_OK)
		return err;

	if (fHashMarkCount != 0)
		err = archive->AddInt32("_hashcount", fHashMarkCount);

	if (err != B_OK)
		return err;

	if (fHashMarks != B_HASH_MARKS_NONE)
		err = archive->AddInt16("_hashloc", fHashMarks);

	if (err != B_OK)
		return err;

	if (fStyle != B_BLOCK_THUMB)
		err = archive->AddInt32("_sstyle", fStyle);

	return err;
}
//------------------------------------------------------------------------------
status_t BSlider::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BSlider::WindowActivated(bool state)
{

}
//------------------------------------------------------------------------------
void BSlider::AttachedToWindow()
{
	BControl::AttachedToWindow();

	ResizeToPreferred();

	fLocation.Set(9.0f, 0.0f);

	fOffScreenBits = new BBitmap(Bounds(), B_CMAP8, true, false);
	fOffScreenView = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
	fOffScreenBits->AddChild(fOffScreenView);
}
//------------------------------------------------------------------------------
void BSlider::AllAttached()
{
}
//------------------------------------------------------------------------------
void BSlider::AllDetached()
{
}
//------------------------------------------------------------------------------
void BSlider::DetachedFromWindow()
{
	// TODO rewrite this
	if (fOffScreenView)
	{
		fOffScreenBits->Lock();
		fOffScreenView->RemoveSelf();
		fOffScreenBits->Unlock();
		delete fOffScreenView;
		fOffScreenView = NULL;
	}

	if (fOffScreenBits)
	{
		delete fOffScreenBits;
		fOffScreenBits = NULL;
	}
}
//------------------------------------------------------------------------------
void BSlider::MessageReceived(BMessage *msg)
{
}
//------------------------------------------------------------------------------
void BSlider::FrameMoved(BPoint new_position)
{
}
//------------------------------------------------------------------------------
void BSlider::FrameResized(float w,float h)
{
	if (fOffScreenView)
	{
		fOffScreenBits->Lock();
		fOffScreenView->RemoveSelf();
		fOffScreenView->ResizeTo(Bounds().Width(), Bounds().Height());
		fOffScreenBits->Unlock();
	}
	else
		fOffScreenView = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);

	if (fOffScreenBits)
		delete fOffScreenBits;
	
	fOffScreenBits = new BBitmap(Bounds(), B_CMAP8, true, false);
	fOffScreenBits->AddChild(fOffScreenView);
}
//------------------------------------------------------------------------------
void BSlider::KeyDown(const char *bytes, int32 numBytes)
{
	if (!IsEnabled())
		return;

	int32 value = Value();

	if (bytes[0] == B_LEFT_ARROW || bytes[0] == B_DOWN_ARROW)
		SetValue ( value - 1 );
	else if (bytes[0] == B_RIGHT_ARROW || bytes[0] == B_UP_ARROW)
		SetValue(value + 1);
	else
		BControl::KeyDown(bytes, numBytes);

	if (Value() != value)
		Invoke();
}
//------------------------------------------------------------------------------
void BSlider::MouseDown(BPoint point)
{
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);

	SetValue(ValueForPoint(point));
	SetTracking(true);
}
//------------------------------------------------------------------------------
void BSlider::MouseUp(BPoint point)
{
	Invoke();

	SetTracking(false);
}
//------------------------------------------------------------------------------
void BSlider::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsTracking())
	{
		SetValue(ValueForPoint(point));

		if (fModificationMessage)
			Invoke(fModificationMessage);
	}
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
	value = max_c(fMinValue, min_c(fMaxValue, value));
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
		BControl::SetValue((int32)(position *(fMaxValue - fMinValue) + fMinValue));
}
//------------------------------------------------------------------------------
float BSlider::Position() const
{
	return ((float)(BControl::Value() - fMinValue) / (float)(fMaxValue - fMinValue));
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
	fOffScreenBits->Lock();
	DrawSlider();
	fOffScreenView->Sync();
	fOffScreenBits->Unlock();

	if (fOffScreenBits)
		DrawBitmap(fOffScreenBits, B_ORIGIN);
}
//------------------------------------------------------------------------------
void BSlider::DrawSlider()
{
	OffscreenView()->SetHighColor(ViewColor());
	OffscreenView()->FillRect(Bounds());
	
	DrawBar();
	DrawHashMarks();
	DrawThumb();

	if (IsFocus())
		DrawFocusMark();

	DrawText();
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
	if (fStyle == B_BLOCK_THUMB)
		_DrawBlockThumb();
	else
		_DrawTriangleThumb();
}
//------------------------------------------------------------------------------
void BSlider::DrawFocusMark()
{
	OffscreenView()->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	OffscreenView()->StrokeRect(Bounds());
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
	BRect frame = Bounds();
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
	// TODO: Check if we have to filter this
	BControl::SetFlags(flags);
}
//------------------------------------------------------------------------------
void BSlider::SetResizingMode(uint32 mode)
{
	// TODO: Check if we have to filter this
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
	BControl::GetSupportedSuites(message);

	message->AddString("suites", "suite/vnd.Be-slider");
	
	return B_OK;
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
	// TODO: Do the necessary to change the style
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

	if (bar_color)
		fFillColor = *bar_color;
}
//------------------------------------------------------------------------------
bool BSlider::FillColor(rgb_color *bar_color) const
{
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
	// TODO: Do the necessary to change the orientation
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
	// TODO: Resize the control
	fBarThickness = thickness;
}
//------------------------------------------------------------------------------
void BSlider::SetFont(const BFont *font, uint32 properties)
{
	// TODO: Resize the control
	BControl::SetFont(font, properties);
}
//------------------------------------------------------------------------------
void BSlider::SetLimits(int32 minimum, int32 maximum)
{
	// TODO: Redraw
	fMinValue = minimum;
	fMaxValue = maximum;
}
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
	if (Orientation() == B_HORIZONTAL)
		return BarFrame().left + 1.0f;
	else
		return BarFrame().bottom - 1.0f;
}
//------------------------------------------------------------------------------
float BSlider::_MaxPosition() const
{
	if (Orientation() == B_HORIZONTAL)
		return BarFrame().right - 1.0f;
	else
		return BarFrame().top + 1.0f;
}
//------------------------------------------------------------------------------			
//void BSlider::_ReservedSlider4() {}
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
	// TODO: Move common init code here
}
//------------------------------------------------------------------------------



























