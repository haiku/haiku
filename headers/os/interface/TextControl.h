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
//	File Name:		TextControl.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BTextControl displays text that can act like a control.
//------------------------------------------------------------------------------
#ifndef	_TEXT_CONTROL_H
#define	_TEXT_CONTROL_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Control.h>
#include <InterfaceDefs.h>
#include <TextView.h>			// For convenience

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


class _BTextInput_;

// BTextControl class -----------------------------------------------------------
class BTextControl : public BControl {
public:
							BTextControl(BRect frame, const char* name,
										 const char* label,
										 const char* initial_text, BMessage* message,
										 uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
										 uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual					~BTextControl();
							BTextControl(BMessage* data);
	static	BArchivable		*Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	virtual	void			SetText(const char* text);
			const char*		Text() const;

	virtual	void			SetValue(int32 value);
	virtual	status_t		Invoke(BMessage* msg = NULL);

			BTextView*		TextView() const;

	virtual	void			SetModificationMessage(BMessage* message);
			BMessage*		ModificationMessage() const;

	virtual	void			SetAlignment(alignment label, alignment text);
			void			GetAlignment(alignment* label, alignment* text) const;
	virtual	void			SetDivider(float dividing_line);
			float			Divider() const;

	virtual	void			Draw(BRect updateRect);
	virtual	void			MouseDown(BPoint where);
	virtual	void			AttachedToWindow();
	virtual	void			MakeFocus(bool focusState = true);
	virtual	void			SetEnabled(bool state);
	virtual	void			FrameMoved(BPoint new_position);
	virtual	void			FrameResized(float new_width, float new_height);
	virtual	void			WindowActivated(bool active);
	
	virtual	void			GetPreferredSize(float* width, float* height);
	virtual	void			ResizeToPreferred();

	virtual void			MessageReceived(BMessage* msg);
	virtual BHandler		*ResolveSpecifier(BMessage* msg, int32 index,
											  BMessage* specifier, int32 form,
											  const char* property);

	virtual	void			MouseUp(BPoint pt);
	virtual	void			MouseMoved(BPoint pt, uint32 code,
									   const BMessage* msg);
	virtual	void			DetachedFromWindow();

	virtual void			AllAttached();
	virtual void			AllDetached();
	virtual status_t		GetSupportedSuites(BMessage* data);
	virtual void			SetFlags(uint32 flags);

// Private or reserved ---------------------------------------------------------
	virtual status_t		Perform(perform_code d, void* arg);

private:
	friend	class 			_BTextInput_;
	virtual	void			_ReservedTextControl1();
	virtual	void			_ReservedTextControl2();
	virtual	void			_ReservedTextControl3();
	virtual	void			_ReservedTextControl4();

			BTextControl&	operator=(const BTextControl&);

			// this one is not defined, so I guess I am on my own here
			_BTextInput_*	fText;

			char*			fUnusedCharP;	//fLabel;	// this is in BControl
			BMessage*		fModificationMessage;
			alignment		fLabelAlign;
			float			fDivider;
//			uint16			fPrevWidth;
//			uint16			fPrevHeight;
			uint32			_reserved[4];
#if !_PR3_COMPATIBLE_
			uint32			_more_reserved[4];
#endif

//			bool			fClean;
			bool			fSkipSetFlags;		// will use this for SHIFT-TAB
			bool			fUnusedBool1;
			bool			fUnusedBool2;
};
//------------------------------------------------------------------------------

#endif	// _TEXTCONTROL_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

