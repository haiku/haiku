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
//	File Name:		Button.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BButton displays and controls a button in a window.
//------------------------------------------------------------------------------

#ifndef _BUTTON_H
#define _BUTTON_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Control.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BButton class ---------------------------------------------------------------
class BButton : public BControl {

public:
					BButton(BRect frame,
							const char *name,
							const char *label,
							BMessage *message,
							uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	
virtual				~BButton();

					BButton(BMessage *archive);

static BArchivable	*Instantiate(BMessage *archive);
virtual	status_t	Archive (BMessage *archive, bool deep = true) const;
	
virtual void		Draw(BRect updateRect);
virtual void		MouseDown(BPoint point);
virtual void		AttachedToWindow();
virtual void		KeyDown(const char *bytes, int32 numBytes);
virtual void		MakeDefault(bool flag);
virtual void		SetLabel(const char *string);
		bool		IsDefault() const;

virtual void		MessageReceived(BMessage *message);
virtual void		WindowActivated(bool active);
virtual void		MouseMoved(BPoint point, uint32 transit, const BMessage *message);
virtual void		MouseUp(BPoint point);
virtual	void		DetachedFromWindow();
virtual	void		SetValue(int32 value);
virtual void		GetPreferredSize (float *width, float *height);
virtual void		ResizeToPreferred();
virtual	status_t	Invoke(BMessage *message = NULL);
virtual	void		FrameMoved(BPoint newLocation);
virtual	void		FrameResized(float width, float height);

virtual void		MakeFocus(bool focused = true);
virtual void		AllAttached();
virtual void		AllDetached();
	
virtual BHandler	*ResolveSpecifier(BMessage *message,
									int32 index,
									BMessage *specifier,
									int32 what,
									const char *property);
virtual	status_t	GetSupportedSuites(BMessage *message);
virtual status_t	Perform(perform_code d, void *arg);

private:
	
virtual	void		_ReservedButton1();
virtual	void		_ReservedButton2();
virtual	void		_ReservedButton3();

		BButton		&operator=(const BButton &);

		BRect		DrawDefault(BRect bounds, bool enabled);
		status_t	Execute ();

		float		fCachedWidth;
		bool		fDrawAsDefault;
		uint32		_reserved[3];
};
//------------------------------------------------------------------------------

#endif // _BUTTON_H

/*
 * $Log $
 *
 * $Id  $
 *
 */
