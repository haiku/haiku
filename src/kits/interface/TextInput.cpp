/*
 * Copyright 2001-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Marc Flerackers (mflerackers@androme.be)
 *		John Scipione (jscipione@gmail.com)
 */


#include "TextInput.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <InterfaceDefs.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>


namespace BPrivate {


_BTextInput_::_BTextInput_(BRect frame, BRect textRect, uint32 resizeMask,
	uint32 flags)
	:
	BTextView(frame, "_input_", textRect, resizeMask, flags),
	fPreviousText(NULL),
	fInMouseDown(false)
{
	MakeResizable(true);
}


_BTextInput_::_BTextInput_(BMessage* archive)
	:
	BTextView(archive),
	fPreviousText(NULL),
	fInMouseDown(false)
{
	MakeResizable(true);
}


_BTextInput_::~_BTextInput_()
{
	free(fPreviousText);
}


BArchivable*
_BTextInput_::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "_BTextInput_"))
		return new _BTextInput_(archive);

	return NULL;
}


status_t
_BTextInput_::Archive(BMessage* data, bool deep) const
{
	return BTextView::Archive(data, true);
}


void
_BTextInput_::MouseDown(BPoint where)
{
	fInMouseDown = true;
	BTextView::MouseDown(where);
	fInMouseDown = false;
}


void
_BTextInput_::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
}


void
_BTextInput_::KeyDown(const char* bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_ENTER:
		{
			if (!TextControl()->IsEnabled())
				break;

			if (fPreviousText == NULL || strcmp(Text(), fPreviousText) != 0) {
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
		if (!fInMouseDown)
			SelectAll();
	} else {
		if (strcmp(Text(), fPreviousText) != 0)
			TextControl()->Invoke();

		free(fPreviousText);
		fPreviousText = NULL;
	}

	if (Window() != NULL) {
		// Invalidate parent to draw or remove the focus mark
		if (BTextControl* parent = dynamic_cast<BTextControl*>(Parent())) {
			BRect frame = Frame();
			frame.InsetBy(-1.0, -1.0);
			parent->Invalidate(frame);
		}
	}
}


BSize
_BTextInput_::MinSize()
{
	BSize min;
	min.height = ceilf(LineHeight(0) + 2.0);
	// we always add at least one pixel vertical inset top/bottom for
	// the text rect.
	min.width = min.height * 3;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), min);
}


void
_BTextInput_::SetInitialText()
{
	free(fPreviousText);
	fPreviousText = NULL;

	if (Text() != NULL)
		fPreviousText = strdup(Text());
}


void
_BTextInput_::Paste(BClipboard* clipboard)
{
	BTextView::Paste(clipboard);
	Invalidate();
}


void
_BTextInput_::InsertText(const char* inText, int32 inLength,
	int32 inOffset, const text_run_array* inRuns)
{
	// Filter all line breaks, note that inText is not terminated.
	if (inLength == 1) {
		if (*inText == '\n' || *inText == '\r')
			BTextView::InsertText(" ", 1, inOffset, inRuns);
		else
			BTextView::InsertText(inText, 1, inOffset, inRuns);
	} else {
		BString filteredText(inText, inLength);
		filteredText.ReplaceAll('\n', ' ');
		filteredText.ReplaceAll('\r', ' ');
		BTextView::InsertText(filteredText.String(), inLength, inOffset,
			inRuns);
	}

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);
}


void
_BTextInput_::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);
}


BTextControl*
_BTextInput_::TextControl()
{
	BTextControl* textControl = NULL;
	if (Parent() != NULL)
		textControl = dynamic_cast<BTextControl*>(Parent());

	if (textControl == NULL)
		debugger("_BTextInput_ should have a BTextControl as parent");

	return textControl;
}


}	// namespace BPrivate

