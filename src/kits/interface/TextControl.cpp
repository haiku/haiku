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
BTextControl::BTextControl(BRect frame, const char* name, const char* label,
						   const char* text, BMessage* message, uint32 mask,
						   uint32 flags)
	:	BControl(frame, name, label, message, mask, flags)
{
	if (label)
	{
		fDivider = frame.Width() / 2.0f;
	}
	else
	{
		// no label
		fDivider = 0.0f;
	}

	fModificationMessage = NULL;

	frame = Bounds();
	if (fDivider)
	{
		frame.left = fDivider + 5.0f;
	}

	BRect rect(frame);
	rect.OffsetTo(0,0);

	frame.OffsetBy(-2,1);

	rect.InsetBy(2,2);
	fText = new _BTextInput_(this, frame, "text", rect);
	fText->SetText(text);
	AddChild(fText);
}
//------------------------------------------------------------------------------
BTextControl::~BTextControl()
{
	if (fText)
	{
		fText->RemoveSelf();
		delete fText;
	}
}
//------------------------------------------------------------------------------
BTextControl::BTextControl(BMessage* data)
	:	BControl(data)
{
	if (data->FindInt32("_a_label", (int32*)&fLabelAlign) != B_OK)
	{
		fLabelAlign = B_ALIGN_LEFT;
	}

	alignment textAlign;
	if (data->FindInt32("_a_text", (int32*)&textAlign) != B_OK)
	{
		fText->SetAlignment(B_ALIGN_LEFT);
	}
	else
	{
		fText->SetAlignment(textAlign);
	}

	if (data->FindFloat("_divide", &fDivider) != B_OK)
	{
		if (Label())
			fDivider = Frame().Width()/2.0f;
		else
			fDivider = 0.0f;
	}

	if (data->FindMessage("_mod_msg", fModificationMessage) != B_OK)
	{
		fModificationMessage = NULL;	// Is this really necessary?
	}

	// TODO: Recover additional info as per final implementation of Archive()
}
//------------------------------------------------------------------------------
BArchivable* BTextControl::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data,"BTextControl"))
	{
		return NULL;
	}

	return new BTextControl(data);
}
//------------------------------------------------------------------------------
status_t BTextControl::Archive(BMessage* data, bool deep = true) const
{
	// TODO: compare against original version and finish
	status_t err = BView::Archive(data, deep);

	if (!err)
		err = data->AddInt32("_a_label", fLabelAlign);
	if (!err)
		err = data->AddInt32("_a_text", fText->Alignment());
	if (!err)
		err = data->AddFloat("_divide", fDivider);
	if (!err && fModificationMessage)
		err = data->AddMessage("_mod_msg", fModificationMessage);

	return err;
}
//------------------------------------------------------------------------------
void BTextControl::SetText(const char* text)
{
	fText->SetText(text);
}
//------------------------------------------------------------------------------
const char* BTextControl::Text() const
{
	return fText->Text();
}
//------------------------------------------------------------------------------
void BTextControl::SetValue(int32 value)
{
}
//------------------------------------------------------------------------------
status_t BTextControl::Invoke(BMessage* msg = NULL)
{
	return BControl::Invoke(msg);
}
//------------------------------------------------------------------------------
BTextView* BTextControl::TextView() const
{
	return (BTextView*)fText;
}
//------------------------------------------------------------------------------
void BTextControl::SetModificationMessage(BMessage* message)
{
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
	fLabelAlign = label;
	fText->SetAlignment(text);
}
//------------------------------------------------------------------------------
void BTextControl::GetAlignment(alignment* label, alignment* text) const
{
	*label = fLabelAlign;
	*text = fText->Alignment();
}
//------------------------------------------------------------------------------
void BTextControl::SetDivider(float dividing_line)
{
	fDivider = dividing_line;
// here I need some code to resize and invalidate the textView

	BRect frame = fText->Frame();
	if (fDivider)
	{
		frame.left = fDivider + 5.0f;
	}
	else
	{
		frame.left = 0.0f;
	}
	
	if (!frame.IsValid())
	{
		frame.left = frame.right - 6.0f;
	}

	fText->ResizeTo( frame.Width(), frame.Height());
//	fText->FrameResized( frame.Width(), frame.Height());
	fText->MoveTo( frame.left, frame.top);
	fText->Invalidate();

	Invalidate();
}
//------------------------------------------------------------------------------
float BTextControl::Divider() const
{
	return fDivider;
}
//------------------------------------------------------------------------------
void BTextControl::Draw(BRect rect)
{
	SetLowColor(ViewColor());
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	if (Label())
	{
		float y = ceil(fh.ascent + fh.descent + fh.leading) + 2.0f;
		float x;
		switch (fLabelAlign)
		{
			case B_ALIGN_RIGHT:
				x = fDivider - font.StringWidth(Label()) - 3.0f;
				break;

			case B_ALIGN_CENTER:
				x = fDivider - font.StringWidth(Label())/2.0f;
				break;

			default:
				x = 3.0f;
				break;
		}
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
					 IsEnabled() ? B_DARKEN_MAX_TINT : B_DISABLED_LABEL_TINT));
		DrawString(Label(), BPoint(x, y));
	}

	rect = fText->Frame();

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	rect.InsetBy(-1,-1);
	SetHighColor(tint_color(base, IsEnabled() ?
							B_DARKEN_1_TINT : B_DARKEN_2_TINT));
	StrokeLine(BPoint(rect.left,rect.bottom), BPoint(rect.left, rect.top));
	StrokeLine(BPoint(rect.left+1.0f,rect.top), BPoint(rect.right, rect.top));
	SetHighColor(tint_color(base, IsEnabled() ?
							B_LIGHTEN_MAX_TINT : B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(rect.left+1.0f,rect.bottom),
			   BPoint(rect.right, rect.bottom));
	StrokeLine(BPoint(rect.right,rect.bottom),
			   BPoint(rect.right, rect.top+1.0f));
}
//------------------------------------------------------------------------------
void BTextControl::MouseDown(BPoint where)
{
	if (IsEnabled())
	{
		MakeFocus(true);
	}
}
void BTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();
	if (Parent())
	{
		SetViewColor(Parent()->ViewColor());
	}

	float w;
	float h;
	GetPreferredSize(&w, &h);
	ResizeTo(Bounds().Width(), h);
	fText->ResizeTo(fText->Bounds().Width(), h);
}
void BTextControl::MakeFocus(bool state = true)
{
	if (IsEnabled())
	{
		fText->MakeFocus(state);
		Invalidate();
	}
}
//------------------------------------------------------------------------------
void BTextControl::SetEnabled(bool state)
{
	fText->SetEnabled(state);
	BControl::SetEnabled(state);
}
//------------------------------------------------------------------------------
void BTextControl::GetPreferredSize(float* width, float* height)
{
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	*height = ceil(fh.ascent + fh.descent + fh.leading) + 7.0f;

	// TODO: this one I need to find out
	*width = 4.0f + ceil(font.StringWidth(Label()))*2.0f;
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
	BView::SetFlags(flags);
}
//------------------------------------------------------------------------------
void BTextControl::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_CONTROL_MODIFIED:
			if (fModificationMessage)
			{
				BControl::Invoke(fModificationMessage);
			}
			break;
	
		default:
			BControl::MessageReceived(msg);
			break;
	}
}
//------------------------------------------------------------------------------
BHandler* BTextControl::ResolveSpecifier(BMessage* msg, int32 index,
										 BMessage* specifier, int32 form,
										 const char* property)
{
	return NULL;
}
//------------------------------------------------------------------------------
status_t BTextControl::GetSupportedSuites(BMessage* data)
{
	return B_OK;
}
//------------------------------------------------------------------------------
void BTextControl::MouseUp(BPoint pt)
{
	BControl::MouseUp(pt);
}
//------------------------------------------------------------------------------
void BTextControl::MouseMoved(BPoint pt, uint32 code, const BMessage* msg)
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
	BControl::WindowActivated(active);
}
//------------------------------------------------------------------------------
status_t BTextControl::Perform(perform_code d, void* arg)
{
	return BControl::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BTextControl::_ReservedTextControl1()
{
}
//------------------------------------------------------------------------------
void BTextControl::_ReservedTextControl2()
{
}
//------------------------------------------------------------------------------
void BTextControl::_ReservedTextControl3()
{
}
//------------------------------------------------------------------------------
void BTextControl::_ReservedTextControl4()
{
}
//------------------------------------------------------------------------------
BTextControl& BTextControl::operator=(const BTextControl&)
{
	// Assignment not allowed
	return *this;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

