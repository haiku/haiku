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
//	File Name:		CheckBox.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BCheckBox displays an on/off control.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <CheckBox.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BCheckBox::BCheckBox(BRect frame, const char *name, const char *label,
					 BMessage *message, uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags)
{
	fOutlined = false;

	if (Bounds().Height() < 18.0f)
		ResizeTo(Bounds().Width(), 18.0f);
}
//------------------------------------------------------------------------------
BCheckBox::~BCheckBox()
{

}
//------------------------------------------------------------------------------
BCheckBox::BCheckBox(BMessage *archive)
	:	BControl(archive)
{
	fOutlined = false;
}
//------------------------------------------------------------------------------
BArchivable *BCheckBox::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCheckBox"))
		return new BCheckBox(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BCheckBox::Archive(BMessage *archive, bool deep) const
{
	return BControl::Archive(archive,deep);
}
//------------------------------------------------------------------------------
void BCheckBox::Draw(BRect updateRect)
{
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
		// Filling
		SetHighColor(lightenmax);
		FillRect(rect);

		// Box
		if (fOutlined)
		{
			SetHighColor(darken3);
			StrokeRect(rect);

			rect.InsetBy(1,1);

			BeginLineArray(6);

			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken2);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken2);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.right, rect.bottom), darken4);
			AddLine(BPoint(rect.right, rect.bottom),
					BPoint(rect.right, rect.top), darken4);

			EndLineArray();
		}
		else
		{
			BeginLineArray(6);

			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken1);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken1);
			rect.InsetBy(1,1);
			AddLine(BPoint(rect.left, rect.bottom),
					BPoint(rect.left, rect.top), darken4);
			AddLine(BPoint(rect.left, rect.top),
					BPoint(rect.right, rect.top), darken4);
			AddLine(BPoint(rect.left + 1.0f, rect.bottom),
					BPoint(rect.right, rect.bottom), no_tint);
			AddLine(BPoint(rect.right, rect.bottom),
					BPoint(rect.right, rect.top + 1.0f), no_tint);

			EndLineArray();
		}

		// Checkmark
		if (Value() == B_CONTROL_ON)
		{
			rect.InsetBy(2,2);

			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			SetPenSize(2);
			StrokeLine(BPoint(rect.left, rect.top),
					   BPoint(rect.right, rect.bottom));
			StrokeLine(BPoint(rect.left, rect.bottom),
					   BPoint(rect.right, rect.top));
			SetPenSize(1);
		}

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
		// Filling
		SetHighColor(lighten1);
		FillRect(rect);

		// Box
		BeginLineArray(6);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), no_tint);
		AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), no_tint);
		rect.InsetBy(1,1);
		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), darken2);
		AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.right, rect.top), darken2);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), darken1);
		AddLine(BPoint(rect.right, rect.bottom),
				BPoint(rect.right, rect.top + 1.0f), darken1);

		EndLineArray();

		// Checkmark
		if (Value() == B_CONTROL_ON)
		{
			rect.InsetBy(2, 2);

			SetHighColor(tint_color(ui_color(B_KEYBOARD_NAVIGATION_COLOR),
				B_DISABLED_MARK_TINT));
			SetPenSize(2);
			StrokeLine(BPoint(rect.left, rect.top),
					   BPoint(rect.right, rect.bottom));
			StrokeLine(BPoint(rect.left, rect.bottom),
					   BPoint(rect.right, rect.top));
			SetPenSize(1);
		}

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
void BCheckBox::AttachedToWindow()
{
	BControl::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BCheckBox::MouseDown(BPoint point)
{
	if (!IsEnabled())
	{
		BControl::MouseDown(point);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,	B_NO_POINTER_HISTORY |
					  B_SUSPEND_VIEW_FOCUS);

	fOutlined = true;
	SetTracking(true);
	Invalidate();
}
//------------------------------------------------------------------------------
void BCheckBox::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BCheckBox::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}
//------------------------------------------------------------------------------
void BCheckBox::KeyDown(const char *bytes, int32 numBytes)
{
//	if (*((int*)bytes) == VK_RETURN || *((int*)bytes) == VK_SPACE)
//		SetValue(Value() ? 0 : 1);
//	else
//	{
//		SetValue(0);
		BControl::KeyDown(bytes, numBytes);
//	}
}
//------------------------------------------------------------------------------
void BCheckBox::MouseUp(BPoint point)
{
	if (IsEnabled() && IsTracking())
	{
		fOutlined = false;

		if (Bounds().Contains(point))
		{
			if (Value() == B_CONTROL_OFF)
				SetValue(B_CONTROL_ON);
			else
				SetValue(B_CONTROL_OFF);
	
			BControl::Invoke();
		}
		SetTracking(false);
	}
	else
	{
		BControl::MouseUp(point);
		return;
	}
}
//------------------------------------------------------------------------------
void BCheckBox::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
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
		BControl::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BCheckBox::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BCheckBox::SetValue(int32 value)
{
	BControl::SetValue(value);
}
//------------------------------------------------------------------------------
void BCheckBox::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);

	*height = (float)ceil(fh.ascent + fh.descent + fh.leading) + 6.0f;
	*width = 22.0f + (float)ceil(StringWidth(Label()));
}
//------------------------------------------------------------------------------
void BCheckBox::ResizeToPreferred()
{
	float widht, height;
	GetPreferredSize(&widht, &height);
	BView::ResizeTo(widht,height);
}
//------------------------------------------------------------------------------
status_t BCheckBox::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}
//------------------------------------------------------------------------------
void BCheckBox::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}
//------------------------------------------------------------------------------
void BCheckBox::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}
//------------------------------------------------------------------------------
BHandler *BCheckBox::ResolveSpecifier(BMessage *message, int32 index,
									  BMessage *specifier, int32 what,
									  const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
status_t BCheckBox::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
void BCheckBox::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
}
//------------------------------------------------------------------------------
void BCheckBox::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BCheckBox::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
status_t BCheckBox::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BCheckBox::_ReservedCheckBox1() {}
void BCheckBox::_ReservedCheckBox2() {}
void BCheckBox::_ReservedCheckBox3() {}
//------------------------------------------------------------------------------
BCheckBox &BCheckBox::operator=(const BCheckBox &)
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
