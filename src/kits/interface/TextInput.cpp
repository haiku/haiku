//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku, Inc.
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
//	Authors:		Frans van Nispen (xlr8@tref.nl)
//					Marc Flerackers (mflerackers@androme.be)
//	Description:	The BTextView derivative owned by an instance of
//					BTextControl.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <InterfaceDefs.h>
#include <Message.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include "TextInput.h"


_BTextInput_::_BTextInput_(BRect frame, BRect textRect, uint32 resizeMask,
						   uint32 flags)
	:	BTextView(frame, "_input_", textRect, resizeMask, flags),
		fPreviousText(NULL),
		fBool(false)
{
	MakeResizable(true);
}


_BTextInput_::_BTextInput_(BMessage *archive)
	:	BTextView(archive),
		fPreviousText(NULL),
		fBool(false)
{
	MakeResizable(true);
}


_BTextInput_::~_BTextInput_()
{
	free(fPreviousText);
}


BArchivable *
_BTextInput_::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "_BTextInput_"))
		return new _BTextInput_(archive);
	else
		return NULL;
}


status_t
_BTextInput_::Archive(BMessage *data, bool deep) const
{
	return BTextView::Archive(data, true);
}


void
_BTextInput_::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
	AlignTextRect();
}


void
_BTextInput_::KeyDown(const char* bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_ENTER: 
		{
			if (!TextControl()->IsEnabled())
				break;
			
			if(strcmp(Text(), fPreviousText) != 0) {
				TextControl()->Invoke();
				free(fPreviousText);
				fPreviousText = strdup(Text());
			}

			SelectAll();
			break;
		}		

		case B_TAB:
			BView::KeyDown(bytes, numBytes);
			break;
	
		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}


void
_BTextInput_::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	BTextView::MakeFocus(state);

	if (state) {
		SetInitialText();

		fBool = true;

		if (Window()) {
			BMessage *message = Window()->CurrentMessage();

			if (message && message->what == B_KEY_DOWN)
				SelectAll();
		}
	} else {
		if (strcmp(Text(), fPreviousText) != 0)
			TextControl()->Invoke();

		free(fPreviousText);
		fPreviousText = NULL;
		fBool = false;

		if (Window()) {
			BMessage *message = Window()->CurrentMessage();

			if (message && message->what == B_MOUSE_DOWN)
				Select(0, 0);
		}
	}

	if (Window()) {
		Draw(Bounds());
		Flush();
	}
}


void
_BTextInput_::AlignTextRect()
{
	// TODO
}


void
_BTextInput_::SetInitialText()
{
	if (fPreviousText) {
		free(fPreviousText);
		fPreviousText = NULL;
	}

	if (Text())
		fPreviousText = strdup(Text());
}


void
_BTextInput_::Paste(BClipboard *clipboard)
{
	BTextView::Paste(clipboard);
	Invalidate();
}


void
_BTextInput_::InsertText(const char *inText, int32 inLength,
							  int32 inOffset, const text_run_array *inRuns)
{
	char *buffer = NULL;

	if (strpbrk(inText, "\r\n") && inLength <= 1024) {
		buffer = (char *)malloc(inLength);

		if (buffer) {
			strcpy(buffer, inText);

			for (int32 i = 0; i < inLength; i++)
				if (buffer[i] == '\r' || buffer[i] == '\n')
					buffer[i] = ' ';
		}
	}

	BTextView::InsertText(buffer ? buffer : inText, inLength, inOffset,
		inRuns);

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);

	free(buffer);
}


void
_BTextInput_::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);
}


BTextControl *
_BTextInput_::TextControl()
{
	BTextControl *textControl = NULL;

	if (Parent())
		textControl = dynamic_cast<BTextControl*>(Parent());

	if (!textControl)
		debugger("_BTextInput_ should have a BTextControl as parent");

	return textControl;
}
