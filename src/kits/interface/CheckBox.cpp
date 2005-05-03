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
#include <CheckBox.h>
#include <Window.h>


BCheckBox::BCheckBox(BRect frame, const char *name, const char *label,
					 BMessage *message, uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, message, resizingMode, flags),
		fOutlined(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = (float)ceil(6.0f + fontHeight.ascent + fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


BCheckBox::~BCheckBox()
{

}


BCheckBox::BCheckBox(BMessage *archive)
	:	BControl(archive),
		fOutlined(false)
{
}


BArchivable *
BCheckBox::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BCheckBox"))
		return new BCheckBox(archive);
	else
		return NULL;
}


status_t
BCheckBox::Archive(BMessage *archive, bool deep) const
{
	return BControl::Archive(archive,deep);
}


void
BCheckBox::Draw(BRect updateRect)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// If the focus is changing, just redraw the focus indicator
	if (IsFocusChanging()) {
		float x = (float)ceil(10.0f + fontHeight.ascent);
		float y = 5.0f + (float)ceil(fontHeight.ascent);

		if (IsFocus())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));

		return;
	}

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
	lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
	darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
	darken3 = tint_color(no_tint, B_DARKEN_3_TINT),
	darken4 = tint_color(no_tint, B_DARKEN_4_TINT),
	darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT);

	BRect rect(1.0f, 3.0f, (float)ceil(3.0f + fontHeight.ascent),
		(float)ceil(5.0f + fontHeight.ascent));

	if (IsEnabled()) {
		// Filling
		SetHighColor(lightenmax);
		FillRect(rect);

		// Box
		if (fOutlined) {
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
		} else {
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
		if (Value() == B_CONTROL_ON) {
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
		SetHighColor(darkenmax);
		DrawString(Label(), BPoint((float)ceil(10.0f + fontHeight.ascent),
			3.0f + (float)ceil(fontHeight.ascent)));

		// Focus
		if (IsFocus()) {
			float x = (float)ceil(10.0f + fontHeight.ascent);
			float y = 5.0f + (float)ceil(fontHeight.ascent);

			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine(BPoint(x, y), BPoint(x + StringWidth(Label()), y));
		}
	} else {
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
		if (Value() == B_CONTROL_ON) {
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
		SetHighColor(tint_color(no_tint, B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint((float)ceil(10.0f + fontHeight.ascent),
			3.0f + (float)ceil(fontHeight.ascent)));
	}
}


void
BCheckBox::AttachedToWindow()
{
	BControl::AttachedToWindow();
}


void
BCheckBox::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	fOutlined = true;

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		Invalidate();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		BRect bounds = Bounds();
		uint32 buttons;

		Draw(Bounds());
		Flush();

		do {
			snooze(40000);

			GetMouse(&point, &buttons, true);

			bool inside = bounds.Contains(point);

			if (fOutlined != inside) {
				fOutlined = inside;
				Draw(Bounds());
				Flush();
			}
		} while (buttons != 0);

		if (fOutlined) {
			fOutlined = false;
			SetValue(!Value());
			Invoke();
		} else {
			Draw(Bounds());
			Flush();
		}
	}	
}


void
BCheckBox::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}


void
BCheckBox::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
}


void
BCheckBox::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
}


void
BCheckBox::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}

	if (fOutlined) {
		fOutlined = false;
		SetValue(!Value());
		Invoke();
	} else {
		Invalidate();
	}

	SetTracking(false);
}


void
BCheckBox::MouseMoved(BPoint point, uint32 transit,
						   const BMessage *message)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(point);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}
}


void
BCheckBox::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BCheckBox::SetValue(int32 value)
{
	BControl::SetValue(value);
}


void
BCheckBox::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);

	*height = (float)ceil(6.0f + fh.ascent + fh.descent);
	*width = 12.0f + fh.ascent;
	
	if (Label())
		*width += StringWidth(Label());

	*width = (float)ceil(*width);
}


void
BCheckBox::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


status_t
BCheckBox::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}


void
BCheckBox::FrameMoved(BPoint newLocation)
{
	BControl::FrameMoved(newLocation);
}


void
BCheckBox::FrameResized(float width, float height)
{
	BControl::FrameResized(width, height);
}


BHandler *
BCheckBox::ResolveSpecifier(BMessage *message, int32 index,
									  BMessage *specifier, int32 what,
									  const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
BCheckBox::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}


void
BCheckBox::MakeFocus(bool focused)
{
	BControl::MakeFocus(focused);
}


void
BCheckBox::AllAttached()
{
	BControl::AllAttached();
}


void
BCheckBox::AllDetached()
{
	BControl::AllDetached();
}


status_t
BCheckBox::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}


void BCheckBox::_ReservedCheckBox1() {}
void BCheckBox::_ReservedCheckBox2() {}
void BCheckBox::_ReservedCheckBox3() {}


BCheckBox &
BCheckBox::operator=(const BCheckBox &)
{
	return *this;
}
