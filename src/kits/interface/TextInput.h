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
//	File Name:		TextInput.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	The BTextView derivative owned by an instance of
//					BTextControl.
//------------------------------------------------------------------------------

#ifndef	_TEXT_CONTROLI_H
#define	_TEXT_CONTROLI_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <TextView.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BTextControl;

// _BTextInput_ class ----------------------------------------------------------
class _BTextInput_ : public BTextView {
public:
						_BTextInput_(BRect frame, BRect textRect,
							uint32 resizeMask,
							uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED);
						_BTextInput_(BMessage *data);
						~_BTextInput_();

static	BArchivable*	Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			FrameResized(float width, float height);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			MakeFocus(bool focusState = true);

		void			AlignTextRect();
		void			SetInitialText();

virtual	void			Paste(BClipboard *clipboard);

protected:

virtual	void			InsertText(const char *inText, int32 inLength,
								   int32 inOffset, const text_run_array *inRuns);
virtual	void			DeleteText(int32 fromOffset, int32 toOffset);

private:

		BTextControl	*TextControl();

		char			*fPreviousText;
		bool			fBool;
};
//------------------------------------------------------------------------------

#endif	// _TEXT_CONTROLI_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

