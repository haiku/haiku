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
//	File Name:		Box.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BBox objects group views together and draw a border
//                  around them.
//------------------------------------------------------------------------------

#ifndef _BOX_H
#define _BOX_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <View.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BBox class ------------------------------------------------------------------
class BBox : public BView {

public:
						BBox(BRect frame,
							const char *name = NULL,
							uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS |
											B_NAVIGABLE_JUMP,
							border_style border = B_FANCY_BORDER);
virtual					~BBox();

/* Archiving */
						BBox(BMessage *archive);
static	BArchivable		*Instantiate(BMessage *archive);
virtual	status_t		Archive(BMessage *archive, bool deep = true) const;

virtual	void			SetBorder(border_style border);
		border_style	Border() const;

		void			SetLabel(const char *string);
		status_t		SetLabel(BView *viewLabel);

		const char		*Label() const;
		BView			*LabelView() const;

virtual	void			Draw(BRect updateRect);
virtual	void			AttachedToWindow();
virtual	void			DetachedFromWindow();
virtual	void			AllAttached();
virtual	void			AllDetached();
virtual void			FrameResized(float width, float height);
virtual void			MessageReceived(BMessage *message);
virtual	void			MouseDown(BPoint point);
virtual	void			MouseUp(BPoint point);
virtual	void			WindowActivated(bool active);
virtual	void			MouseMoved(BPoint point, uint32 transit,
								   const BMessage *message);
virtual	void			FrameMoved(BPoint newLocation);

virtual BHandler		*ResolveSpecifier(BMessage *message,
										int32 index,
										BMessage *specifier,
										int32 what,
										const char *property);

virtual void			ResizeToPreferred();
virtual void			GetPreferredSize(float *width, float *height);
virtual void			MakeFocus(bool focused = true);
virtual status_t		GetSupportedSuites(BMessage *message);

virtual status_t		Perform(perform_code d, void *arg);

private:

virtual	void			_ReservedBox1();
virtual	void			_ReservedBox2();

		BBox			&operator=(const BBox &);

		void			InitObject(BMessage *data = NULL);
		void			DrawPlain();
		void			DrawFancy();
		void			ClearAnyLabel();

		char			*fLabel;
		BRect			fBounds;
		border_style	fStyle;
		BView			*fLabelView;
		uint32			_reserved[1];
};
//------------------------------------------------------------------------------

#endif // _BOX_H

/*
 * $Log $
 *
 * $Id  $
 *
 */
