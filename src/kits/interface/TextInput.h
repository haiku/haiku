/*
 * Copyright 2001-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 */

//! The BTextView derivative owned by an instance of BTextControl.

#ifndef	_TEXT_CONTROLI_H
#define	_TEXT_CONTROLI_H


#include <TextView.h>


class BTextControl;

namespace BPrivate {

class _BTextInput_ : public BTextView {
public:
						_BTextInput_(BRect frame, BRect textRect,
							uint32 resizeMask,
							uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED);
						_BTextInput_(BMessage *data);
virtual					~_BTextInput_();

static	BArchivable*	Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			MouseDown(BPoint where);
virtual	void			FrameResized(float width, float height);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			MakeFocus(bool focusState = true);

virtual	BSize			MinSize();

		void			SetInitialText();

virtual	void			Paste(BClipboard *clipboard);

protected:

virtual	void			InsertText(const char *inText, int32 inLength,
								   int32 inOffset, const text_run_array *inRuns);
virtual	void			DeleteText(int32 fromOffset, int32 toOffset);

private:

		BTextControl	*TextControl();

		char			*fPreviousText;
		bool			fInMouseDown;
};

}	// namespace BPrivate

using namespace BPrivate;


#endif	// _TEXT_CONTROLI_H

