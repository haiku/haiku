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
//	File Name:		RadioButton.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BRadioButton represents a single on/off button.  All
//                  sibling BRadioButton objects comprise a single
//                  "multiple choice" control.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <RadioButton.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BRadioButton::BRadioButton(BRect frame, const char *name, const char *label,
						   BMessage *message, uint32 resizMask, uint32 flags)
	:	BControl(frame, name, label, message, resizMask, flags),
		fOutlined(false)
{
	if (Bounds().Height() < 18.0f)
		ResizeTo(Bounds().Width(), 18.0f);
}
//------------------------------------------------------------------------------
BRadioButton::BRadioButton(BMessage *archive)
	:	BControl(archive),
		fOutlined(false)
{
}
//------------------------------------------------------------------------------
BRadioButton::~BRadioButton()
{
}
//------------------------------------------------------------------------------
BArchivable *BRadioButton::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BRadioButton"))
		return new BRadioButton(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BRadioButton::Archive(BMessage *archive, bool deep) const
{
	return BControl::Archive(archive, deep);
}
//------------------------------------------------------------------------------
void BRadioButton::Draw(BRect updateRect)
{
	// Placeholder until sBitmaps is filled in
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
	lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
	darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
	darken3 = tint_color(no_tint, B_DARKEN_3_TINT),
	darken4 = tint_color(no_tint, B_DARKEN_4_TINT),
	darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);

	BRect rect(1.0, 3.0, 13.0, 15.0);

	if (IsEnabled())
	{
		// Dot
		if (Value() == B_CONTROL_ON)
		{
			rgb_color kb_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

			SetHighColor(tint_color(kb_color, B_DARKEN_3_TINT));
			FillEllipse(rect);
			SetHighColor(kb_color);
			FillEllipse(BRect(rect.left + 3, rect.top + 3, rect.right - 4, rect.bottom - 4));
			SetHighColor(tint_color(kb_color, B_DARKEN_3_TINT));
			StrokeLine(BPoint(rect.right - 5, rect.bottom - 4),
				BPoint(rect.right - 4, rect.bottom - 5));
			SetHighColor(tint_color(kb_color, B_LIGHTEN_MAX_TINT));
			StrokeLine(BPoint(rect.left + 4, rect.top + 5),
				BPoint(rect.left + 5, rect.top + 4));

		}
		else
		{
			SetHighColor(lightenmax);
			FillEllipse(rect);
		}

		// Outer circle
		if (fOutlined)
		{
			SetHighColor(darken3);
			StrokeEllipse(rect);
		}
		else
		{
			SetHighColor(darken1);
			StrokeArc(rect, 45.0f, 180.0f);
			SetHighColor(lightenmax);
			StrokeArc(rect, 45.0f, -180.0f);
		}

		rect.InsetBy(1, 1);
		
		// Inner circle
		SetHighColor(darken3);
		StrokeArc(rect, 45.0f, 180.0f);
		StrokeLine(BPoint(rect.left + 1, rect.top + 1),
			BPoint(rect.left + 1, rect.top + 1));
		SetHighColor(no_tint);
		StrokeArc(rect, 45.0f, -180.0f);
		StrokeLine(BPoint(rect.left + 1, rect.bottom - 1),
			BPoint(rect.left + 1, rect.bottom - 1));
		StrokeLine(BPoint(rect.right - 1, rect.bottom - 1),
			BPoint(rect.right - 1, rect.bottom - 1));
		StrokeLine(BPoint(rect.right - 1, rect.top + 1),
			BPoint(rect.right - 1, rect.top + 1));

		// Label
		BFont font;
		GetFont(&font);
		font_height fh;
		font.GetHeight(&fh);

		SetHighColor(darkenmax);
		DrawString(Label(), BPoint(20.0f, 8.0f + (float)ceil(fh.ascent / 2.0f)));

		// Focus
		if (IsFocus())
		{
			float h = 8.0f + (float)ceil(fh.ascent / 2.0f) + 2.0f;

			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine(BPoint(20.0f, h), BPoint(20.0f + StringWidth(Label()), h));
		}
	}
	else
	{
		// Dot
		if (Value() == B_CONTROL_ON)
		{
			rgb_color kb_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

			SetHighColor(tint_color(kb_color, B_LIGHTEN_2_TINT));
			FillEllipse(rect);
			SetHighColor(tint_color(kb_color, B_LIGHTEN_MAX_TINT));
			StrokeLine(BPoint(rect.left + 4, rect.top + 5),
				BPoint(rect.left + 5, rect.top + 4));
			SetHighColor(tint_color(kb_color, B_DARKEN_3_TINT));
			StrokeArc(BRect(rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2),
				45.0f, -180.0f);
		}
		else
		{
			SetHighColor(lighten1);
			FillEllipse(rect);
		}

		// Outer circle
		SetHighColor(no_tint);
		StrokeArc(rect, 45.0f, 180.0f);
		SetHighColor(lighten1);
		StrokeArc(rect, 45.0f, -180.0f);

		rect.InsetBy(1, 1);
		
		// Inner circle
		SetHighColor(darken2);
		StrokeArc(rect, 45.0f, 180.0f);
		StrokeLine(BPoint(rect.left + 1, rect.top + 1),
			BPoint(rect.left + 1, rect.top + 1));
		SetHighColor(no_tint);
		StrokeArc(rect, 45.0f, -180.0f);
		StrokeLine(BPoint(rect.left + 1, rect.bottom - 1),
			BPoint(rect.left + 1, rect.bottom - 1));
		StrokeLine(BPoint(rect.right - 1, rect.bottom - 1),
			BPoint(rect.right - 1, rect.bottom - 1));
		StrokeLine(BPoint(rect.right - 1, rect.top + 1),
			BPoint(rect.right - 1, rect.top + 1));

		// Label
		BFont font;
		GetFont(&font);
		font_height fh;
		font.GetHeight(&fh);

		SetHighColor(tint_color(no_tint, B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(20.0f, 8.0f + (float)ceil(fh.ascent / 2.0f)));
	}
}
//------------------------------------------------------------------------------
void BRadioButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
	{
		BControl::MouseDown(point);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,	B_NO_POINTER_HISTORY |
					  B_SUSPEND_VIEW_FOCUS);

	SetTracking(true);
	fOutlined = true;
	Invalidate();
}
//------------------------------------------------------------------------------
void BRadioButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BRadioButton::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
}
//------------------------------------------------------------------------------
void BRadioButton::SetValue(int32 value)
{
	if (BControl::Value() == value)
		return;

	if (!IsTracking())
	{
		BView *sibling;
		
		for (sibling = PreviousSibling(); sibling != NULL;
			sibling = sibling->PreviousSibling())
		{
			BRadioButton* radio = dynamic_cast<BRadioButton*>(sibling);
				
			if (radio != NULL)
				radio->BControl::SetValue(B_CONTROL_OFF);
		}
			
		for (sibling = NextSibling(); sibling != NULL;
			sibling = sibling->NextSibling())
		{
			BRadioButton* radio = dynamic_cast<BRadioButton*>(sibling);
				
			if (radio != NULL)
				radio->BControl::SetValue(B_CONTROL_OFF);
		}
	}

	BControl::SetValue(B_CONTROL_ON);
}
//------------------------------------------------------------------------------
void BRadioButton::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);

	*height = (float)ceil(fh.ascent + fh.descent + fh.leading) + 6.0f;
	*width = 22.0f + (float)ceil(StringWidth(Label()));
}
//------------------------------------------------------------------------------
void BRadioButton::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	BControl::ResizeTo(width, height);
}
//------------------------------------------------------------------------------
status_t BRadioButton::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}
//------------------------------------------------------------------------------
void BRadioButton::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BRadioButton::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}
//------------------------------------------------------------------------------
void BRadioButton::MouseUp(BPoint point)
{
	if (IsEnabled() && IsTracking())
	{
		fOutlined = false;
		SetTracking(false);

		if (Bounds().Contains(point))
		{
			SetValue(B_CONTROL_ON);
			Invoke();
		}
	}
	else
	{
		BControl::MouseUp(point);
		return;
	}

	Invalidate();
}
//------------------------------------------------------------------------------
void BRadioButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsEnabled() && IsTracking())
	{
		if (transit == B_EXITED_VIEW)
			fOutlined = false;
		else if (transit == B_ENTERED_VIEW)
			fOutlined = true;

		Invalidate();
	}
	else
		BView::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BRadioButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BRadioButton::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}
//------------------------------------------------------------------------------
void BRadioButton::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}
//------------------------------------------------------------------------------
BHandler *BRadioButton::ResolveSpecifier(BMessage *message, int32 index,
										 BMessage *specifier, int32 what,
										 const char *property)
{
	return ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
void BRadioButton::MakeFocus(bool focused)
{
	BControl::MakeFocus();
}
//------------------------------------------------------------------------------
void BRadioButton::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BRadioButton::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
status_t BRadioButton::GetSupportedSuites(BMessage *message)
{
	return GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
status_t BRadioButton::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BRadioButton::_ReservedRadioButton1() {}
void BRadioButton::_ReservedRadioButton2() {}
//------------------------------------------------------------------------------
BRadioButton &BRadioButton::operator=(const BRadioButton &)
{
	return *this;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
