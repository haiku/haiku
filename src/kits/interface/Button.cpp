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
	:	BControl(frame, name, label, message, resizingMode, flags),
		fDrawAsDefault(false)
{
	if (Bounds().Height() < 24.0f)
		ResizeTo(Bounds().Width(), 24.0f);
}
//------------------------------------------------------------------------------
BButton::~BButton()
{
}
//------------------------------------------------------------------------------
BButton::BButton(BMessage *archive)
	:	BControl (archive)
{
	if ( archive->FindBool ( "_default", &fDrawAsDefault ) != B_OK )
		fDrawAsDefault = false;
}
//------------------------------------------------------------------------------
BArchivable *BButton::Instantiate ( BMessage *archive )
{
	if ( validate_instantiation(archive, "BButton"))
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
	
	if (fDrawAsDefault)
		err = archive->AddBool("_default", fDrawAsDefault);

	return err;
}
//------------------------------------------------------------------------------
void BButton::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
		darken4 = tint_color(no_tint, B_DARKEN_4_TINT),
		darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);
	
	if (!IsDefault())
	{
		SetHighColor(no_tint);
		StrokeRect(rect);
		rect.InsetBy(1,1);
	}
	
	if (IsEnabled())
	{
		if (Value())
		{
			BeginLineArray(8);

			// Dark border
			AddLine(BPoint(rect.left, rect.bottom-1.0f),
				BPoint(rect.left, rect.top+1.0f), darken4);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right-1.0f, rect.top), darken4);
			AddLine(BPoint(rect.left+1.0f, rect.bottom),
				BPoint(rect.right-1.0f, rect.bottom), darken4);
			AddLine(BPoint(rect.right, rect.bottom-1.0f),
				BPoint(rect.right, rect.top+1.0f), darken4);

			rect.InsetBy(1,1);

			// First bevel
			AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), darkenmax);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right, rect.top), darkenmax);
			AddLine(BPoint(rect.left+1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), darken4);
			AddLine(BPoint(rect.right, rect.bottom-1.0f),
				BPoint(rect.right, rect.top+1.0f), darken4);

			EndLineArray();

			rect.InsetBy(1,1);

			// Filling
			SetHighColor(darkenmax);
			FillRect(rect);

			// Label
			BFont font;
			GetFont(&font);

			float x = Bounds().Width() / 2 - StringWidth(Label()) / 2.0f;
			float y = Bounds().Height() / 2.0f + (float)ceil(font.Size() / 2.0f);

			SetHighColor(lightenmax);
			SetLowColor(darkenmax);
			DrawString(Label(), BPoint(x, y));
		}
		else
		{
			BeginLineArray(14);

			// Dark border
			AddLine(BPoint(rect.left, rect.bottom-1.0f),
				BPoint(rect.left, rect.top+1.0f), darken4);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right-1.0f, rect.top), darken4);
			AddLine(BPoint(rect.left+1.0f, rect.bottom),
				BPoint(rect.right-1.0f, rect.bottom), darken4);
			AddLine(BPoint(rect.right, rect.bottom-1.0f),
				BPoint(rect.right, rect.top+1.0f), darken4);

			rect.InsetBy(1,1);

			// First bevel
			AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), lighten1);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right, rect.top), lighten1);
			AddLine(BPoint(rect.left+1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), darken2);
			AddLine(BPoint(rect.right, rect.bottom-1.0f),
				BPoint(rect.right, rect.top+1.0f), darken2);

			rect.InsetBy(1,1);

			// Second bevel
			AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), lightenmax);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right, rect.top), lightenmax);
			AddLine(BPoint(rect.left+1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), no_tint);
			AddLine(BPoint(rect.right, rect.bottom-1.0f),
				BPoint(rect.right, rect.top+1.0f), no_tint);

			rect.InsetBy(1,1);

			// Third bevel
			AddLine(BPoint(rect.left, rect.top),
				BPoint(rect.left, rect.bottom), lightenmax);
			AddLine(BPoint(rect.left+1.0f, rect.top),
				BPoint(rect.right, rect.top), lightenmax);

			EndLineArray();

			rect.left +=1;
			rect.top += 1;

			// Filling
			SetHighColor(lighten2);
			FillRect(rect);

			// Label
			BFont font;
			GetFont(&font);

			float x = Bounds().Width() / 2 - StringWidth(Label()) / 2.0f;
			float y = Bounds().Height() / 2.0f + (float)ceil(font.Size() / 2.0f);

			SetHighColor(darkenmax);
			DrawString(Label(), BPoint(x, y));
			
			// Focus
			if (IsFocus())
			{
				font_height fh;
				font.GetHeight(&fh);

				y += 2.0f;
				SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
				StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));
			}
		}
	}
	else
	{
		BeginLineArray(14);

		// Dark border
		AddLine(BPoint(rect.left, rect.bottom-1.0f),
			BPoint(rect.left, rect.top+1.0f), darken2);
		AddLine(BPoint(rect.left+1.0f, rect.top),
			BPoint(rect.right-1.0f, rect.top), darken2);
		AddLine(BPoint(rect.left+1.0f, rect.bottom),
			BPoint(rect.right-1.0f, rect.bottom), darken2);
		AddLine(BPoint(rect.right, rect.bottom-1.0f),
			BPoint(rect.right, rect.top+1.0f), darken2);

		rect.InsetBy(1,1);

		// First bevel
		AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom), lighten1);
		AddLine(BPoint(rect.left+1.0f, rect.top),
			BPoint(rect.right, rect.top), lighten1);
		AddLine(BPoint(rect.left+1.0f, rect.bottom),
			BPoint(rect.right, rect.bottom), no_tint);
		AddLine(BPoint(rect.right, rect.bottom-1.0f),
			BPoint(rect.right, rect.top+1.0f), no_tint);

		rect.InsetBy(1,1);

		// Second bevel
		AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom), lightenmax);
		AddLine(BPoint(rect.left+1.0f, rect.top),
			BPoint(rect.right, rect.top), lightenmax);
		AddLine(BPoint(rect.left+1.0f, rect.bottom),
			BPoint(rect.right, rect.bottom), no_tint);
		AddLine(BPoint(rect.right, rect.bottom-1.0f),
			BPoint(rect.right, rect.top+1.0f), no_tint);

		rect.InsetBy(1,1);

		// Third bevel
		AddLine(BPoint(rect.left, rect.top),
			BPoint(rect.left, rect.bottom), lightenmax);
		AddLine(BPoint(rect.left+1.0f, rect.top),
			BPoint(rect.right, rect.top), lightenmax);

		EndLineArray();

		rect.left +=1;
		rect.top += 1;

		// Filling
		SetHighColor(lighten2);
		FillRect(rect);

		// Label
		BFont font;
		GetFont(&font);

		float x = Bounds().Width() / 2 - StringWidth(Label()) / 2.0f;
		float y = Bounds().Height() / 2.0f + (float)ceil(font.Size() / 2.0f);

		SetHighColor(tint_color(no_tint, B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(x, y));
	}
}
//------------------------------------------------------------------------------
void BButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
	{
		BControl::MouseDown(point);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);

	SetValue(B_CONTROL_ON);
	SetTracking(true);
}
//------------------------------------------------------------------------------
void BButton::AttachedToWindow()
{
	BControl::AttachedToWindow();

	if (IsDefault())
		Window()->SetDefaultButton((BButton*)this);
}
//------------------------------------------------------------------------------
void BButton::KeyDown ( const char *bytes, int32 numBytes )
{
	if (numBytes == 1)
	{
		switch (bytes[0])
		{
			case B_ENTER:
			case B_SPACE:
				SetValue(B_CONTROL_ON);
				snooze(50000);
				SetValue(B_CONTROL_OFF);
				Invoke();
				break;

			default:
				BControl::KeyDown(bytes, numBytes);
		}
	}
	else
		BControl::KeyDown(bytes, numBytes);
}
//------------------------------------------------------------------------------
void BButton::MakeDefault(bool flag)
{
	if (flag == IsDefault())
		return;

	fDrawAsDefault = flag;
	
	if (Window())
	{
		BButton *button = (BButton*)(Window()->DefaultButton());
		
		if (fDrawAsDefault)
		{
			if(button)
				button->MakeDefault(false);

			Window()->SetDefaultButton((BButton*)this);
		}
		else
		{
			if (button == this)
				Window()->SetDefaultButton(NULL);
		}
	}

	Invalidate();
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
	if (IsEnabled() && IsTracking())
	{
		if (transit == B_EXITED_VIEW)
			SetValue(B_CONTROL_OFF);
		else if (transit == B_ENTERED_VIEW)
			SetValue(B_CONTROL_ON);
	}
	else
		BControl::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BButton::MouseUp(BPoint point)
{
	if (IsEnabled() && IsTracking())
	{
		if (Bounds().Contains(point))
		{
			if ( Value() == B_CONTROL_ON)
			{
				SetValue(B_CONTROL_OFF);
				BControl::Invoke();
			}
		}
		
		SetTracking(false);
	}
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
void BButton::GetPreferredSize (float *width, float *height)
{
	font_height fh;

	GetFontHeight(&fh);

	*height = (float)ceil(fh.ascent + fh.descent + fh.leading) + 12.0f;
	*width = 20.0f + (float)ceil(StringWidth(Label()));
	
	if (*width < 75.0f)
		*width = 75.0f;
}
//------------------------------------------------------------------------------
void BButton::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width,height);
}
//------------------------------------------------------------------------------
status_t BButton::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
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
	return B_ERROR;
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
	// TODO:
	return BRect();
}
//------------------------------------------------------------------------------
status_t Execute()
{
	// TODO:
	return B_ERROR;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
