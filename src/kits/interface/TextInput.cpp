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
//	File Name:		TextInput.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	The BTextView derivative owned by an instance of
//					BTextControl.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <InterfaceDefs.h>
#include <TextControl.h>
#include <Window.h>
#include <Message.h>
#include <TextView.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextInput.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
_BTextInput_::_BTextInput_(BTextControl* parent, BRect frame, const char* name,
						   BRect rect, uint32 mask, uint32 flags)
	:	BTextView(frame, name, rect, mask, flags),
		fEnabled(true),
		fChanged(false),
		fParent(parent)
{
	SetMaxBytes(255);
	SetWordWrap(false);
}
//------------------------------------------------------------------------------
_BTextInput_::~_BTextInput_()
{
}
//------------------------------------------------------------------------------
void _BTextInput_::SetEnabled(bool state)
{
	fEnabled = state;
	if (fEnabled)
	{
		BView::SetFlags(Flags() | B_NAVIGABLE);
	}
	else
	{
		BView::SetFlags(Flags() & (0xffffffff - B_NAVIGABLE));
	}

	Invalidate();
}
//------------------------------------------------------------------------------
void _BTextInput_::AttachedToWindow()
{
	BFont font;
	GetFont(&font);
	float h = font.Size() + 6.0f;
	ResizeTo(Bounds().Width(), h);

	fViewColor = BTextView::ViewColor();
}
//------------------------------------------------------------------------------
rgb_color _BTextInput_::ViewColor()
{
	return fViewColor;
}
//------------------------------------------------------------------------------
void _BTextInput_::SetViewColor(rgb_color color)
{
	fViewColor = color;
}
//------------------------------------------------------------------------------
void _BTextInput_::MakeFocus(bool state)
{
	BTextView::MakeFocus(state);
	if (state)
	{
		BTextView::SetViewColor(255, 255, 255, 255);
		fChanged = false;
		SelectAll();
	}
	else
	{
		BTextView::SetViewColor(fViewColor);
	}

	Invalidate();
}
//------------------------------------------------------------------------------
bool _BTextInput_::CanEndLine(int32 end)
{
	return false;
}
//------------------------------------------------------------------------------
void _BTextInput_::KeyDown(const char* bytes, int32 numBytes)
{
	bool send = false;

	BMessage* msg = Window()->CurrentMessage();
	if (numBytes == 1)
	{
		switch (bytes[0])
		{
			case B_UP_ARROW:
				msg->ReplaceInt64("when", (int64)system_time());
				msg->ReplaceInt32("key", 38);
				msg->ReplaceInt32("raw_char", B_TAB);
				msg->ReplaceInt32("modifiers", B_SCROLL_LOCK | B_SHIFT_KEY);
				msg->ReplaceInt8("byte", B_TAB);
				msg->ReplaceString("bytes", "");
				fParent->BView::MakeFocus(true);
				Looper()->PostMessage(msg);
				send = true;
				break;

			case B_ENTER:
			{
				SelectAll();
				send = true;
				fChanged = true;
				break;
			}		

			case B_DOWN_ARROW:
				msg->ReplaceInt64("when", (int64)system_time());
				msg->ReplaceInt32("key", 38);
				msg->ReplaceInt32("raw_char", B_TAB);
				msg->ReplaceInt8("byte", B_TAB);
				msg->ReplaceString("bytes", "");
				Looper()->PostMessage(msg);
				send = true;
				break;

			case B_TAB:
				// This will make sure that it will skip to the next object
				BView::KeyDown(bytes, numBytes);
				send = true;
				break;
		
			default:
				BTextView::KeyDown(bytes, numBytes);
		}
	}
	else
	{
		BTextView::KeyDown(bytes, numBytes);
	}

	if (send)
	{
		if (fChanged)
		{
			Window()->PostMessage(new BMessage(B_CONTROL_INVOKED), fParent);
		}
	}
	else
	{
		Window()->PostMessage(new BMessage(B_CONTROL_MODIFIED), fParent);
	}

	fChanged = true;
}
//------------------------------------------------------------------------------
void _BTextInput_::MouseDown(BPoint where)
{
	if (fEnabled)
	{
		BTextView::MouseDown(where);
	}
}
//------------------------------------------------------------------------------
void _BTextInput_::Draw(BRect rect)
{

	BTextView::Draw(rect);
	
	rect = Bounds();

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(tint_color(base, fEnabled ?
							B_DARKEN_4_TINT : B_LIGHTEN_2_TINT));
	StrokeLine(BPoint(rect.left,rect.bottom), BPoint(rect.left, rect.top));
	StrokeLine(BPoint(rect.left+1.0f,rect.top), BPoint(rect.right, rect.top));
	SetHighColor(tint_color(base, B_NO_TINT));
	StrokeLine(BPoint(rect.left+1.0f,rect.bottom),
			   BPoint(rect.right, rect.bottom));
	StrokeLine(BPoint(rect.right,rect.bottom),
			   BPoint(rect.right, rect.top+1.0f));

	if (IsFocus())
	{
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(rect);
	}

	if (!fEnabled)
	{
		rect.InsetBy(1,1);
		SetDrawingMode(B_OP_ALPHA);
		rgb_color color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
									 B_LIGHTEN_1_TINT);
		color.alpha = 140;
		SetHighColor(color);
		FillRect(rect);
		SetDrawingMode(B_OP_COPY);
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

