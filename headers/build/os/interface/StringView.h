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
//	File Name:		StringView.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BStringView draw a non-editable text string
//------------------------------------------------------------------------------

#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <View.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BStringView class -----------------------------------------------------------
class BStringView : public BView{
public:
						BStringView(BRect bounds, const char* name, const char* text,
									uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
						BStringView(BMessage* data);
	virtual 			~BStringView();
	static	BArchivable	*Instantiate(BMessage* data);
	virtual	status_t	Archive(BMessage* data, bool deep = true) const;

			void		SetText(const char* text);
			const char	*Text() const;
			void		SetAlignment(alignment flag);
			alignment	Alignment() const;

	virtual	void		AttachedToWindow();
	virtual	void		Draw(BRect bounds);

	virtual void		MessageReceived(BMessage* msg);
	virtual	void		MouseDown(BPoint pt);
	virtual	void		MouseUp(BPoint pt);
	virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage* msg);
	virtual	void		DetachedFromWindow();
	virtual	void		FrameMoved(BPoint new_position);
	virtual	void		FrameResized(float new_width, float new_height);

	virtual BHandler	*ResolveSpecifier(	BMessage* msg, int32 index, BMessage* specifier,
											int32 form, const char* property);

	virtual void		ResizeToPreferred();
	virtual void		GetPreferredSize(float* width, float* height);
	virtual void		MakeFocus(bool state = true);
	virtual void		AllAttached();
	virtual void		AllDetached();
	virtual status_t	GetSupportedSuites(BMessage* data);


// Private or reserved ---------------------------------------------------------
	virtual status_t	Perform(perform_code d, void* arg);

private:
	virtual	void		_ReservedStringView1();
	virtual	void		_ReservedStringView2();
	virtual	void		_ReservedStringView3();

			BStringView	&operator=(const BStringView&);

			char*		fText;
			alignment	fAlign;
			uint32		_reserved[3];
};
//------------------------------------------------------------------------------

#endif	// _STRINGVIEW_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

