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
//	File Name:		Button.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BButton displays and controls a button in a window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Button.h>
#include <Window.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BButton::BButton(BRect frame, const char *name, const char *label, BMessage *message,
				  uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags |= B_WILL_DRAW),
		fDrawAsDefault(false)
{
	// Resize to minimum height if needed
	font_height fh;
	GetFontHeight(&fh);
	float minHeight = 12.0f + (float)ceil(fh.ascent + fh.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}
//------------------------------------------------------------------------------
BButton::~BButton()
{
}
//------------------------------------------------------------------------------
BButton::BButton(BMessage *archive)
	:	BControl (archive)
{
	if (archive->FindBool("_default", &fDrawAsDefault) != B_OK)
		fDrawAsDefault = false;
}
//------------------------------------------------------------------------------
BArchivable *BButton::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BButton"))
		return new BButton(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BButton::Archive(BMessage* archive, bool deep) const
{
	status_t err = BControl::Archive(archive, deep);

	if (err != B_OK)
		return err;
	
	if (IsDefault())
		err = archive->AddBool("_default", true);

	return err;
}
//------------------------------------------------------------------------------
void BButton::Draw(BRect updateRect)
{
	font_height fh;
	GetFontHeight(&fh);

	BRect bounds(Bounds());

	// If the focus is changing, just redraw the focus indicator
	if (IsFocusChanging())
	{
		float x = (bounds.right - StringWidth(Label())) / 2.0f;
		float y = bounds.bottom - fh.descent - (IsDefault() ? 6.0f : 3.0f);

		if (IsFocus())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_LIGHTEN_1_TINT));

		StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));

		return;
	}

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
		darken4 = tint_color(no_tint, B_DARKEN_4_TINT),
		darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);

	BRect rect(bounds);
	
	if (IsDefault())
		rect = DrawDefault(rect, IsEnabled());
	else
		rect.InsetBy(1,1);
	
	if (IsEnabled())
	{
		// This can be set outside draw
		SetHighColor(darken4);

		// Dark border
		StrokeRect(rect);

		BeginLineArray(8);

		// Corners
		AddLine(rect.LeftBottom(), rect.LeftBottom(), no_tint);
		AddLine(rect.LeftTop(), rect.LeftTop(), no_tint);
		AddLine(rect.RightTop(), rect.RightTop(), no_tint);
		AddLine(rect.RightBottom(), rect.RightBottom(), no_tint);

		rect.InsetBy(1, 1);

		// First bevel
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
			BPoint(rect.right, rect.bottom), darken2);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
			BPoint(rect.right, rect.top + 1.0f), darken2);

		AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom), lighten1);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
			BPoint(rect.right, rect.top), lighten1);

		EndLineArray();

		rect.InsetBy(1, 1);

		// Second bevel
		SetHighColor(lightenmax);
		FillRect(rect);

		SetHighColor(no_tint);
		StrokeLine(BPoint(rect.right, rect.top + 1.0f),
			BPoint(rect.right, rect.bottom));
		StrokeLine(BPoint(rect.left + 1.0f, rect.bottom));
		
		rect.InsetBy(1, 1);

		// Filling
		rect.left += 1.0f;
		rect.top += 1.0f;
		SetHighColor(lighten1);
		FillRect(rect);

		if (Value())
		{
			// Invert
			rect.left -= 3;
			rect.top -= 3;
			rect.right += 2;
			rect.bottom += 2;
			InvertRect(rect);
		}

		// Label
		float x = (bounds.right - StringWidth(Label())) / 2.0f;
		float y = bounds.bottom - fh.descent - (IsDefault() ? 8.0f : 5.0f);

		if (Value())
		{
			SetHighColor(lightenmax);
			SetLowColor(darkenmax);
		}
		else
		{
			SetHighColor(darkenmax);
			SetLowColor(lighten2);
		}

		DrawString(Label(), BPoint(x, y));
		
		// Focus
		if (IsFocus())
		{
			y += 2.0f;
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));
		}
	}
	else
	{
		// This can be set outside draw
		SetHighColor(darken2);

		// Dark border
		StrokeRect(rect);

		BeginLineArray(8);

		// Corners
		AddLine(rect.LeftBottom(), rect.LeftBottom(), no_tint);
		AddLine(rect.LeftTop(), rect.LeftTop(), no_tint);
		AddLine(rect.RightTop(), rect.RightTop(), no_tint);
		AddLine(rect.RightBottom(), rect.RightBottom(), no_tint);

		rect.InsetBy(1, 1);

		// First bevel
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
			BPoint(rect.right, rect.bottom), no_tint);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
			BPoint(rect.right, rect.top + 1.0f), no_tint);

		AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom), lighten1);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
			BPoint(rect.right, rect.top), lighten1);

		EndLineArray();

		rect.InsetBy(1, 1);

		// Second bevel
		SetHighColor(lightenmax);
		FillRect(rect);

		SetHighColor(no_tint);
		StrokeLine(BPoint(rect.right, rect.top + 1.0f),
			BPoint(rect.right, rect.bottom));
		StrokeLine(BPoint(rect.left + 1.0f, rect.bottom));
		
		rect.InsetBy(1, 1);

		// Filling
		rect.left += 1.0f;
		rect.top += 1.0f;
		SetHighColor(lighten1);
		FillRect(rect);

		// Label
		float x = (bounds.right - StringWidth(Label())) / 2.0f;
		float y = bounds.bottom - fh.descent - 5.0f;

		SetHighColor(tint_color(no_tint, B_DISABLED_LABEL_TINT));
		SetLowColor(lighten2);
		DrawString(Label(), BPoint(x, y));
	}
}
//------------------------------------------------------------------------------
void BButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	SetValue(B_CONTROL_ON);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS)
	{
 		SetTracking(true);
 		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
 	}
 	else
 	{
		BRect bounds = Bounds();
		uint32 buttons;

		do
		{
			Window()->UpdateIfNeeded();
			
			snooze(40000);

			GetMouse(&point, &buttons, true);

 			bool inside = bounds.Contains(ConvertFromScreen(point));

			if ((Value() == B_CONTROL_ON) != inside)
				SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
		} while (buttons != 0);

		if (Value() == B_CONTROL_ON)
			Invoke();
	}
}
//------------------------------------------------------------------------------
void BButton::AttachedToWindow()
{
	BControl::AttachedToWindow();

	if (IsDefault())
		Window()->SetDefaultButton(this);
}
//------------------------------------------------------------------------------
void BButton::KeyDown(const char *bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE)
	{
		if (!IsEnabled())
			return;

		SetValue(B_CONTROL_ON);
		Invoke();
	}
	else
		BControl::KeyDown(bytes, numBytes);
}
//------------------------------------------------------------------------------
void BButton::MakeDefault(bool flag)
{
	BButton *oldDefault = NULL;
	BWindow *window = Window();
	
	if (window)
		oldDefault = window->DefaultButton();

	if (flag)
	{
		if (fDrawAsDefault && oldDefault == this)
			return;

		fDrawAsDefault = true;

		ResizeBy(6.0f, 6.0f);
		MoveBy(-3.0f, -3.0f);

		if (window && oldDefault != this)
			window->SetDefaultButton(this);
	}
	else
	{
		if (!fDrawAsDefault)
			return;

		fDrawAsDefault = false;

		ResizeBy(-6.0f, -6.0f);
		MoveBy(3.0f, 3.0f);

		if (window && oldDefault == this)
			window->SetDefaultButton(NULL);
	}
}
//------------------------------------------------------------------------------
void BButton::SetLabel(const char *string)
{
	BControl::SetLabel(string);
}
//------------------------------------------------------------------------------
bool BButton::IsDefault() const
{
	return fDrawAsDefault;
}
//------------------------------------------------------------------------------
void BButton::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BButton::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}
//------------------------------------------------------------------------------
void BButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if ((Value() == B_CONTROL_ON) != inside)
		SetValue(inside ? B_CONTROL_ON : B_CONTROL_OFF);
}
//------------------------------------------------------------------------------
void BButton::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	if (Bounds().Contains(point))
		Invoke();
	
	SetTracking(false);
}
//------------------------------------------------------------------------------
void BButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BButton::SetValue(int32 value)
{
	BControl::SetValue(value);
}
//------------------------------------------------------------------------------
void BButton::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);

	*height = 12.0f + (float)ceil(fh.ascent + fh.descent);
	*width = 20.0f + (float)ceil(StringWidth(Label()));
	
	if (*width < 75.0f)
		*width = 75.0f;

	if (fDrawAsDefault)
	{
		*width += 6.0f;
		*height += 6.0f;
	}
}
//------------------------------------------------------------------------------
void BButton::ResizeToPreferred()
{
	 BControl::ResizeToPreferred();
}
//------------------------------------------------------------------------------
status_t BButton::Invoke(BMessage *message)
{
	Sync();
	snooze(50000);
	
	status_t err = BControl::Invoke(message);

	SetValue(B_CONTROL_OFF);

	return err;
}
//------------------------------------------------------------------------------
void BButton::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}
//------------------------------------------------------------------------------
void BButton::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}
//------------------------------------------------------------------------------
void BButton::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
}
//------------------------------------------------------------------------------
void BButton::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BButton::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
BHandler *BButton::ResolveSpecifier(BMessage *message, int32 index,
									BMessage *specifier, int32 what,
									const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what, property);
}
//------------------------------------------------------------------------------
status_t BButton::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
status_t BButton::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BButton::_ReservedButton1() {}
void BButton::_ReservedButton2() {}
void BButton::_ReservedButton3() {}
//------------------------------------------------------------------------------
BButton &BButton::operator=(const BButton &)
{
	return *this;
}
//------------------------------------------------------------------------------
BRect BButton::DrawDefault(BRect bounds, bool enabled)
{
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
	darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
	darken4 = tint_color(no_tint, B_DARKEN_4_TINT);

	if (enabled)
	{
		// Dark border
		BeginLineArray(4);
		AddLine(BPoint(bounds.left, bounds.bottom - 1.0f),
			BPoint(bounds.left, bounds.top + 1.0f), darken4);
		AddLine(BPoint(bounds.left + 1.0f, bounds.top),
			BPoint(bounds.right - 1.0f, bounds.top), darken4);
		AddLine(BPoint(bounds.right, bounds.top + 1.0f),
			BPoint(bounds.right, bounds.bottom - 1.0f), darken4);
		AddLine(BPoint(bounds.left + 1.0f, bounds.bottom),
			BPoint(bounds.right - 1.0f, bounds.bottom), darken4);
		EndLineArray();

		bounds.InsetBy(1.0f, 1.0f);

		// Bevel
		SetHighColor(darken1);
		StrokeRect(bounds);

		bounds.InsetBy(1.0f, 1.0f);

		// Filling
		SetHighColor(lighten1);
		FillRect(bounds);

		bounds.InsetBy(2.0f, 2.0f);
	}
	else
	{
		// Dark border
		BeginLineArray(4);
		AddLine(BPoint(bounds.left, bounds.bottom - 1.0f),
			BPoint(bounds.left, bounds.top + 1.0f), darken1);
		AddLine(BPoint(bounds.left + 1.0f, bounds.top),
			BPoint(bounds.right - 1.0f, bounds.top), darken1);
		AddLine(BPoint(bounds.right, bounds.top + 1.0f),
			BPoint(bounds.right, bounds.bottom - 1.0f), darken1);
		AddLine(BPoint(bounds.left + 1.0f, bounds.bottom),
			BPoint(bounds.right - 1.0f, bounds.bottom), darken1);
		EndLineArray();

		bounds.InsetBy(1.0f, 1.0f);

		// Filling
		SetHighColor(lighten1);
		FillRect(bounds);

		bounds.InsetBy(3.0f, 3.0f);
	}

	return bounds;
}
//------------------------------------------------------------------------------
status_t BButton::Execute()
{
	if (!IsEnabled())
		return B_ERROR;

	SetValue(B_CONTROL_ON);
	return Invoke();
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
