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
//	File Name:		StringView.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BStringView draw a non-editable text string
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <posix/string.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
BStringView::BStringView(BRect frame, const char* name, const char* text,
						 uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags)
{
	fText = strdup(text);
	fAlign = B_ALIGN_LEFT;
}
//------------------------------------------------------------------------------
BStringView::BStringView(BMessage* data)
	:	BView(data)
{
	const char* text;

	if (data->FindInt32("_aligne",(int32&)fAlign) != B_OK)
	{
		fAlign = B_ALIGN_LEFT;
	}

	if (data->FindString("_text",&text) != B_OK)
	{
		text = NULL;
	}

	SetText(text);
}
//------------------------------------------------------------------------------
BArchivable* BStringView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data,"BStringView"))
	{
		return NULL;
	}

	return new BStringView(data);
}
//------------------------------------------------------------------------------
status_t BStringView::Archive(BMessage* data, bool deep) const
{
	BView::Archive(data, deep);
	
	if (fText)
	{
		data->AddString("_text",fText);
	}

	data->AddInt32("_align", fAlign);

	return B_OK;
}
//------------------------------------------------------------------------------
BStringView::~BStringView()
{
	if (fText)
	{
		delete[] fText;
	}
}
//------------------------------------------------------------------------------
void BStringView::SetText(const char* text)
{
	if (fText)
	{
		delete[] fText;
	}
	fText = strdup(text);
	Invalidate();
}
//------------------------------------------------------------------------------
const char* BStringView::Text() const
{
	return fText;
}
//------------------------------------------------------------------------------
void BStringView::SetAlignment(alignment flag)
{
	fAlign = flag;
	Invalidate();
}
//------------------------------------------------------------------------------
alignment BStringView::Alignment() const
{
	return fAlign;
}
//------------------------------------------------------------------------------
void BStringView::AttachedToWindow()
{
	if (Parent())
	{
		SetViewColor(Parent()->ViewColor());
	}
}
//------------------------------------------------------------------------------
void BStringView::Draw(BRect bounds)
{
	SetLowColor(ViewColor());
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);
	
	float y = Bounds().bottom - ceil(fh.descent);
	float x;
	switch (fAlign)
	{
		case B_ALIGN_RIGHT:
			x = Bounds().Width() - font.StringWidth(fText) - 2.0f;
			break;

		case B_ALIGN_CENTER:
			x = (Bounds().Width() - font.StringWidth(fText))/2.0f;
			break;

		default:
			x = 2.0f;
			break;
	}

	DrawString( fText, BPoint(x,y) );
}
//------------------------------------------------------------------------------
void BStringView::ResizeToPreferred()
{
	float w, h;
	GetPreferredSize(&w, &h);
	BView::ResizeTo(w, h);
}
//------------------------------------------------------------------------------
void BStringView::GetPreferredSize(float* width, float* height)
{
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);

	*height = ceil(fh.ascent + fh.descent + fh.leading) + 2.0f;
	*width = 4.0f + ceil(font.StringWidth(fText));
}
//------------------------------------------------------------------------------
void BStringView::MessageReceived(BMessage* msg)
{
	BView::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BStringView::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}
//------------------------------------------------------------------------------
void BStringView::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}
//------------------------------------------------------------------------------
void BStringView::MouseMoved(BPoint pt, uint32 code, const BMessage* msg)
{
	BView::MouseMoved(pt, code, msg);
}
//------------------------------------------------------------------------------
void BStringView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BStringView::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}
//------------------------------------------------------------------------------
void BStringView::FrameResized(float newWidth, float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
}
//------------------------------------------------------------------------------
BHandler* BStringView::ResolveSpecifier(BMessage* msg, int32 index,
										BMessage* specifier, int32 form,
										const char* property)
{
	return NULL;
}
//------------------------------------------------------------------------------
void BStringView::MakeFocus(bool state = true)
{
	BView::MakeFocus(state);
}
//------------------------------------------------------------------------------
void BStringView::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BStringView::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
status_t BStringView::GetSupportedSuites(BMessage* data)
{
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BStringView::Perform(perform_code d, void* arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BStringView::_ReservedStringView1()
{
}
//------------------------------------------------------------------------------
void BStringView::_ReservedStringView2()
{
}
//------------------------------------------------------------------------------
void BStringView::_ReservedStringView3()
{
}
//------------------------------------------------------------------------------
BStringView& BStringView::operator=(const BStringView&)
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

