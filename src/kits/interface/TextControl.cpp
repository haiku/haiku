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
//	File Name:		TextControl.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BTextControl displays text that can act like a control.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <TextControl.h>
#include <Window.h>
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextInput.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BTextControl::BTextControl(BRect frame, const char *name, const char *label,
						   const char *text, BMessage *message, uint32 mask,
						   uint32 flags)
	:	BControl(frame, name, label, message, mask, flags | B_FRAME_EVENTS)
{
	InitData(label, text);

	BRect bounds(Bounds());
	
	font_height fh;
	GetFontHeight(&fh);

	float height = (float)ceil(fh.ascent + fh.descent + fh.leading);
	float lineHeight = fText->LineHeight(0);

	ResizeTo(bounds.Width(), height + 8);

	BRect textBounds(fText->Bounds());

	fText->ResizeTo(textBounds.Width(), lineHeight + 4);
	fText->MoveBy(0, (bounds.Height() - height) / 2.0f);
}
//------------------------------------------------------------------------------
BTextControl::~BTextControl()
{
	SetModificationMessage(NULL);
}
//------------------------------------------------------------------------------
BTextControl::BTextControl(BMessage *data)
	:	BControl(data)
{
	InitData(Label(), NULL, data);

	int32 _a_label = B_ALIGN_LEFT;
	int32 _a_text = B_ALIGN_LEFT;

	if (data->HasInt32("_a_label"))
		data->FindInt32("_a_label", &_a_label);

	if (data->HasInt32("_a_text"))
		data->FindInt32("_a_text", &_a_text);
	
	SetAlignment((alignment)_a_label, (alignment)_a_text);

	if (data->HasFloat("_divide"))
		data->FindFloat("_a_text", &fDivider);

	if (data->HasMessage("_mod_msg"))
	{
		BMessage *_mod_msg = new BMessage;
		data->FindMessage("_mod_msg", _mod_msg);
		SetModificationMessage(_mod_msg);
	}
}
//------------------------------------------------------------------------------
BArchivable* BTextControl::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BTextControl"))
		return new BTextControl(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BTextControl::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	alignment _a_label, _a_text;

	GetAlignment(&_a_label, &_a_text);

	data->AddInt32("_a_label", _a_label);
	data->AddInt32("_a_text", _a_text);

	data->AddFloat("_divide", Divider());

	if (ModificationMessage())
		data->AddMessage("_mod_msg", ModificationMessage());

	return B_OK;
}
//------------------------------------------------------------------------------
void BTextControl::SetText(const char *text)
{
	if (InvokeKind() != B_CONTROL_INVOKED)
		return;

	fText->SetText(text);

	if (IsFocus())
		fText->SetInitialText();

	fText->Invalidate();
}
//------------------------------------------------------------------------------
const char *BTextControl::Text() const
{
	return fText->Text();
}
//------------------------------------------------------------------------------
void BTextControl::SetValue(int32 value)
{
	BControl::SetValue(value);
}
//------------------------------------------------------------------------------
status_t BTextControl::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}
//------------------------------------------------------------------------------
BTextView* BTextControl::TextView() const
{
	return fText;
}
//------------------------------------------------------------------------------
void BTextControl::SetModificationMessage(BMessage *message)
{
	if (fModificationMessage)
		delete fModificationMessage;

	fModificationMessage = message;
}
//------------------------------------------------------------------------------
BMessage* BTextControl::ModificationMessage() const
{
	return fModificationMessage;
}
//------------------------------------------------------------------------------
void BTextControl::SetAlignment(alignment label, alignment text)
{
	fText->SetAlignment(text);
	fText->AlignTextRect();

	if (fLabelAlign != label)
	{
		fLabelAlign = label;
		Invalidate();
	}
}
//------------------------------------------------------------------------------
void BTextControl::GetAlignment(alignment *label, alignment *text) const
{
	*label = fLabelAlign;
	*text = fText->Alignment();
}
//------------------------------------------------------------------------------
void BTextControl::SetDivider(float dividing_line)
{
	float dx = fDivider - dividing_line;

	fDivider = dividing_line;

	fText->MoveBy(-dx, 0.0f);
	fText->ResizeBy(dx, 0.0f);

	if (Window())
	{
		fText->Invalidate();
		Invalidate();
	}
}
//------------------------------------------------------------------------------
float BTextControl::Divider() const
{
	return fDivider;
}
//------------------------------------------------------------------------------
void BTextControl::Draw(BRect updateRect)
{
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
	lighten1 = tint_color(no_tint, B_LIGHTEN_1_TINT),
	lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
	lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
	darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
	darken2 = tint_color(no_tint, B_DARKEN_2_TINT),
	darken4 = tint_color(no_tint, B_DARKEN_4_TINT),
	darkenmax = tint_color(no_tint, B_DARKEN_MAX_TINT),
	nav = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	bool enabled = IsEnabled();
	bool active = false;

	if (fText->IsFocus() && Window()->IsActive())
		active = true;

	BRect rect(fText->Frame());
	rect.InsetBy(-1.0f, -1.0f);

	if (active)
	{
		SetHighColor(nav);
		StrokeRect(rect);
	}
	else
	{
		if (enabled)
			SetHighColor(darken4);
		else
			SetHighColor(darken2);

		StrokeLine(rect.LeftTop(), rect.LeftBottom());
		StrokeLine(rect.LeftTop(), rect.RightTop());

		SetHighColor(no_tint);
		StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
		StrokeLine(BPoint(rect.right, rect.top + 1.0f));
	}

	rect.InsetBy(-1.0f, -1.0f);

	if (enabled)
		SetHighColor(darken1);
	else
		SetHighColor(no_tint);

	StrokeLine(rect.LeftBottom(), rect.LeftTop());
	StrokeLine(rect.RightTop());

	if (enabled)
		SetHighColor(lighten2);
	else
		SetHighColor(lighten1);

	StrokeLine(BPoint(rect.left + 1.0f, rect.bottom), rect.RightBottom());
	StrokeLine(BPoint(rect.right, rect.top + 1.0f), rect.RightBottom());

	if (Label())
	{
		font_height fh;
		GetFontHeight(&fh);

		float y = (float)ceil(fh.ascent + fh.descent + fh.leading) + 2.0f;
		float x;

		switch (fLabelAlign)
		{
			case B_ALIGN_RIGHT:
				x = fDivider - StringWidth(Label()) - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - StringWidth(Label()) / 2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
					 IsEnabled() ? B_DARKEN_MAX_TINT : B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(x, y));
	}
}
//------------------------------------------------------------------------------
void BTextControl::MouseDown(BPoint where)
{
	if (!fText->IsFocus())
	{
		fText->MakeFocus(true);
		fText->SelectAll();
	}
}
//------------------------------------------------------------------------------
void BTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	bool enabled = IsEnabled();
	rgb_color textColor;
	rgb_color color = HighColor();
	BFont font;

	fText->GetFontAndColor(0, &font, &color);

	if (enabled)
		textColor = color;
	else
		textColor = tint_color(color, B_LIGHTEN_2_TINT);

	fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	if (enabled)
	{
		color.red = 255;
		color.green = 255;
		color.blue = 255;
	}
	else
		color = tint_color(color, B_LIGHTEN_2_TINT);

	fText->SetViewColor(color);
	fText->SetLowColor(color);

	fText->MakeEditable(enabled);
}
//------------------------------------------------------------------------------
void BTextControl::MakeFocus(bool state)
{
	fText->MakeFocus(state);

	if (state)
		fText->SelectAll();
}
//------------------------------------------------------------------------------
void BTextControl::SetEnabled(bool state)
{
	if (IsEnabled() == state)
		return;

	if (Window())
	{
		fText->MakeEditable(state);

		rgb_color textColor;
		rgb_color color = {0, 0, 0, 255};
		BFont font;

		fText->GetFontAndColor(0, &font, &color);

		if (state)
			textColor = color;
		else
			textColor = tint_color(color, B_DISABLED_LABEL_TINT);

		fText->SetFontAndColor(&font, B_FONT_ALL, &textColor);

		if (state)
		{
			color.red = 255;
			color.green = 255;
			color.blue = 255;
		}
		else
			color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_LIGHTEN_2_TINT);

		fText->SetViewColor(color);
		fText->SetLowColor(color);

		fText->Invalidate();
		Window()->UpdateIfNeeded();
	}

	BControl::SetEnabled(state);
}
//------------------------------------------------------------------------------
void BTextControl::GetPreferredSize(float *width, float *height)
{
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	*height = (float)ceil(fh.ascent + fh.descent + fh.leading) + 7.0f;

	// TODO: this one I need to find out
	*width = 4.0f + (float)ceil(font.StringWidth(Label()))*2.0f;
}
//------------------------------------------------------------------------------
void BTextControl::ResizeToPreferred()
{
	float w;
	float h;
	GetPreferredSize(&w, &h);
	BView::ResizeTo(w,h);
}
//------------------------------------------------------------------------------
void BTextControl::SetFlags(uint32 flags)
{
	if (!fSkipSetFlags)
	{
		// If the textview is navigable, set it to not navigable if needed
		// Else if it is not navigable, set it to navigable if needed
		if (fText->Flags() & B_NAVIGABLE)
		{
			if (!(flags & B_NAVIGABLE))
				fText->SetFlags(fText->Flags() & ~B_NAVIGABLE);
		}
		else
		{
			if (flags & B_NAVIGABLE)
				fText->SetFlags(fText->Flags() | B_NAVIGABLE);
		}

		// Don't make this one navigable
		flags &= ~B_NAVIGABLE;
	}

	BView::SetFlags(flags);
}
//------------------------------------------------------------------------------
void BTextControl::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_SET_PROPERTY:
		case B_GET_PROPERTY:
			// TODO
			break;
		default:
			BControl::MessageReceived(msg);
			break;
	}
}
//------------------------------------------------------------------------------
BHandler *BTextControl::ResolveSpecifier(BMessage *msg, int32 index,
										 BMessage *specifier, int32 form,
										 const char *property)
{
	/*
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	if (propInfo.FindMatch(message, 0, specifier, what, property) < B_OK)
		return BControl::ResolveSpecifier(message, index, specifier, what,
			property);
	else
		return this;
	*/
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}
//------------------------------------------------------------------------------
status_t BTextControl::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
void BTextControl::MouseUp(BPoint pt)
{
	BControl::MouseUp(pt);
}
//------------------------------------------------------------------------------
void BTextControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BControl::MouseMoved(pt, code, msg);
}
//------------------------------------------------------------------------------
void BTextControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BTextControl::AllAttached()
{
	BControl::AllAttached();
}
//------------------------------------------------------------------------------
void BTextControl::AllDetached()
{
	BControl::AllDetached();
}
//------------------------------------------------------------------------------
void BTextControl::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}
//------------------------------------------------------------------------------
void BTextControl::FrameResized(float newWidth, float newHeight)
{
	BControl::FrameResized(newWidth, newHeight);
}
//------------------------------------------------------------------------------
void BTextControl::WindowActivated(bool active)
{
	if (fText->IsFocus())
		Draw(Bounds());
}
//------------------------------------------------------------------------------
status_t BTextControl::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BTextControl::_ReservedTextControl1() {}
void BTextControl::_ReservedTextControl2() {}
void BTextControl::_ReservedTextControl3() {}
void BTextControl::_ReservedTextControl4() {}
//------------------------------------------------------------------------------
BTextControl &BTextControl::operator=(const BTextControl&)
{
	return *this;
}
//------------------------------------------------------------------------------
void BTextControl::CommitValue()
{
}
//------------------------------------------------------------------------------
void BTextControl::InitData(const char *label, const char *initial_text,
							BMessage *data)
{
	BRect bounds(Bounds());

	fText = NULL;
	//fLabel = NULL;
	fModificationMessage = NULL;
	fLabelAlign = B_ALIGN_LEFT;
	fDivider = 0.0f;
	fPrevWidth = 0;
	fPrevHeight = 0;
	//fClean = true;
	fSkipSetFlags = false;

	int32 flags = 0;

	BFont font(be_bold_font);

	if (!data || !data->HasString("_fname"))
		flags = 1;

	if (!data || !data->HasFloat("_fflt"))
		flags |= 2;

	if (flags != 0)
		SetFont(&font, flags);

	if (label)
		fDivider = bounds.Width() / 2.0f;

	if (Flags() & B_NAVIGABLE)
	{
		fSkipSetFlags = true;
		SetFlags(Flags() & ~B_NAVIGABLE);
		fSkipSetFlags = false;
	}

	if (data)
		fText = (_BTextInput_*)FindView("_input_");
	else
	{
		BRect frame(fDivider, bounds.top + 2.0f, bounds.right - 2.0f,
			bounds.bottom - 2.0f);
		BRect textRect(frame.OffsetToCopy(0.0f, 0.0f));
	
		fText = new _BTextInput_(frame, textRect,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS |
			B_NAVIGABLE);

		AddChild(fText);
		
		SetText(initial_text);
		fText->SetAlignment(B_ALIGN_LEFT);
		fText->AlignTextRect();
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

