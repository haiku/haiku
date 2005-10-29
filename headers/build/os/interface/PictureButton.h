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
//	File Name:		PictureButton.h
//	Author:			Graham MacDonald (macdonag@btopenworld.com)
//	Description:	BPictureButton displays and controls a picture button in a
//					window.
//------------------------------------------------------------------------------

#ifndef _PICTURE_BUTTON_H
#define _PICTURE_BUTTON_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Control.h>
#include <Picture.h>	/* For convenience */

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
enum {
	B_ONE_STATE_BUTTON,
	B_TWO_STATE_BUTTON
};


// BPictureButton class --------------------------------------------------------
class BPictureButton : public BControl
{
public:
	BPictureButton (BRect frame, 
					const char* name, 
					BPicture *off, 			   
					BPicture *on,
					BMessage *message, 
					uint32 behavior = B_ONE_STATE_BUTTON,
					uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
					uint32 flgs = B_WILL_DRAW | B_NAVIGABLE);
	BPictureButton (BMessage *data);
					
	virtual ~BPictureButton ();

	
	static	BArchivable	*Instantiate (BMessage *data);

	virtual status_t Archive (BMessage *data, bool deep = true) const;

	virtual void Draw (BRect updateRect);

	virtual void MouseDown (BPoint point);
	virtual void KeyDown (const char *bytes, int32 numBytes);

	virtual void SetEnabledOn (BPicture *on);
	virtual void SetEnabledOff (BPicture *off);
	virtual void SetDisabledOn (BPicture *on);
	virtual void SetDisabledOff (BPicture *off);
		
	BPicture *EnabledOn () const;
	BPicture *EnabledOff () const;
	BPicture *DisabledOn () const;
	BPicture *DisabledOff () const;

	virtual void SetBehavior (uint32 behavior);
	uint32 Behavior () const;

	virtual void MessageReceived (BMessage *msg);
	virtual void MouseUp (BPoint point);
	virtual void WindowActivated (bool state);
	virtual void MouseMoved (BPoint pt, uint32 code, const BMessage *msg);
	virtual void AttachedToWindow ();
	virtual void DetachedFromWindow ();
	virtual void SetValue (int32 value);
	virtual status_t Invoke (BMessage *msg = NULL);
	virtual void FrameMoved (BPoint new_position);
	virtual void FrameResized (float new_width, float new_height);

	virtual BHandler *ResolveSpecifier (BMessage *msg,
										 int32 index,
										 BMessage *specifier,
										 int32 form,
										 const char *property);
	virtual status_t GetSupportedSuites (BMessage *data);

	virtual void ResizeToPreferred ();
	virtual void GetPreferredSize (float *width, float *height);
	virtual void MakeFocus (bool state = true);
	virtual void AllAttached ();
	virtual void AllDetached ();


	// Private or reserved -----------------------------------------------------
	virtual status_t Perform (perform_code d, void *arg);


private:

	virtual void _ReservedPictureButton1 ();
	virtual void _ReservedPictureButton2 ();
	virtual void _ReservedPictureButton3 ();

	BPictureButton &operator= (const BPictureButton &);

	void Redraw ();
	void InitData ();

	BPicture *fEnabledOff;
	BPicture *fEnabledOn;
	BPicture *fDisabledOff;
	BPicture *fDisabledOn;

	bool fOutlined;

	uint32 fBehavior;
	uint32 _reserved[4];
};

//------------------------------------------------------------------------------

#endif /* _PICTURE_BUTTON_H */

/*
 * $Log $
 *
 * $Id  $
 *
 */

