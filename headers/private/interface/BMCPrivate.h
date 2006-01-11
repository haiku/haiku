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
//	File Name:		BMCPrivate.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	The BMCPrivate classes are used by BMenuField.
//------------------------------------------------------------------------------

#ifndef _BMC_PRIVATE_H
#define _BMC_PRIVATE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <MenuBar.h>
#include <MenuItem.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
class _BMCItem_ : public BMenuItem {
	
public:
					_BMCItem_(BMenu *menu);
					_BMCItem_(BMessage *message);
virtual				~_BMCItem_();

static	BArchivable	*Instantiate(BMessage *data);

		void		Draw();
		void		GetContentSize(float *width, float *height);

private:
friend class BMenuField;

			bool	fShowPopUpMarker;
};
//------------------------------------------------------------------------------

/*//------------------------------------------------------------------------------
_BMCFilter_::_BMCFilter_(BMenuField *menuField, uint32)
{
}
//------------------------------------------------------------------------------
_BMCFilter_::~_BMCFilter_()
{
}
//------------------------------------------------------------------------------
filter_result _BMCFilter_::Filter(BMessage *message, BHandler **handler)
{
}
//------------------------------------------------------------------------------
_BMCFilter_ &_BMCFilter_::operator=(const _BMCFilter_ &)
{
	return *this;
}
//------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
class _BMCMenuBar_ : public BMenuBar {

public:
					_BMCMenuBar_(BRect frame, bool fixed_size,
						BMenuField *menuField);
					_BMCMenuBar_(BMessage *data);
virtual				~_BMCMenuBar_();

static	BArchivable	*Instantiate(BMessage *data);

virtual	void		AttachedToWindow();
virtual	void		Draw(BRect updateRect);
virtual	void		FrameResized(float width, float height);
virtual	void		MessageReceived(BMessage* msg);
virtual	void		MakeFocus(bool focused = true);
virtual void		MouseDown(BPoint where);

private:
		_BMCMenuBar_&operator=(const _BMCMenuBar_ &);

		BMenuField		*fMenuField;
		bool			fFixedSize;	
		BMessageRunner	*fRunner;
};
//------------------------------------------------------------------------------

#endif // _BMC_PRIVATE_H
