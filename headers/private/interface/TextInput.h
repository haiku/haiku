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
							_BTextInput_(BTextControl* parent, BRect frame,
										 const char* name, BRect rect,
										 uint32 mask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
										 uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
							~_BTextInput_();

	virtual	void			AttachedToWindow();
	virtual	void			MakeFocus(bool state = true);
	virtual	void			SetViewColor(rgb_color);
			rgb_color		ViewColor();
	virtual bool			CanEndLine(int32);
	virtual void			KeyDown(const char* bytes, int32 numBytes);
	virtual void			Draw(BRect);
	virtual	void			SetEnabled(bool state);
	virtual	void			MouseDown(BPoint where);

private:
		  	rgb_color		fViewColor;
		  	bool			fEnabled;
		  	bool			fChanged;
			BTextControl*	fParent;
};
//------------------------------------------------------------------------------

#endif	// _TEXT_CONTROLI_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

