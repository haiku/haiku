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
//	File Name:		PictureButton.cpp
//	Author:			Graham MacDonald (macdonag@btopenworld.com)
//	Description:	BPictureButton displays and controls a picture button in a
//					window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <PictureButton.h>


// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BPictureButton::BPictureButton (BRect frame, 
								const char* name, 
								BPicture *off,
								BPicture *on,
								BMessage *message, 
								uint32 behavior,
								uint32 resizeMask, 
								uint32 flgs)
	: BControl (frame, name, "", message, resizeMask, flgs), fOutlined (false),
	  fBehavior (behavior)
{
	fEnabledOff = new BPicture(*off);
	fEnabledOn = new BPicture(*on);
	fDisabledOff = NULL;
	fDisabledOn = NULL;
}


/***********************************************************************
 *  Method: BPictureButton::BPictureButton
 *  Params: BMessage *data
 * Effects: 
 ***********************************************************************/
BPictureButton::BPictureButton(BMessage *data) : BControl (data)
{
	BMessage pictureArchive;
	
	// Default to 1 state button if not here - is this valid?
	if (data->FindInt32 ("_behave", (int32 *)&fBehavior ) != B_OK )
	{
		fBehavior = B_ONE_STATE_BUTTON;
	}
	
	// Now expand the pictures:
	if (data->FindMessage("_e_on", &pictureArchive) == B_OK)
	{
		fEnabledOn = new BPicture(&pictureArchive);
	}
	if (data->FindMessage("_e_off", &pictureArchive) == B_OK)
	{
		fEnabledOff = new BPicture(&pictureArchive);
	}

	if (fBehavior == B_TWO_STATE_BUTTON)
	{
		if (data->FindMessage("_d_on", &pictureArchive) == B_OK)
		{
			fDisabledOn = new BPicture(&pictureArchive);
		}
		if (data->FindMessage("_d_off", &pictureArchive) == B_OK)
		{
			fDisabledOff = new BPicture(&pictureArchive);
		}
	}
	else
	{
		fDisabledOn = fDisabledOff = NULL;
	}
}


/***********************************************************************
 *  Method: BPictureButton::~BPictureButton
 *  Params: 
 * Effects: 
 ***********************************************************************/
BPictureButton::~BPictureButton()
{
	delete fEnabledOn;
	delete fEnabledOff;

	if (fBehavior == B_TWO_STATE_BUTTON)
	{
		delete fDisabledOn;
		delete fDisabledOff;
	}
}


/***********************************************************************
 *  Method: BPictureButton::Instantiate
 *  Params: BMessage *data
 * Returns: BArchivable *
 * Effects: 
 ***********************************************************************/
BArchivable *
BPictureButton::Instantiate (BMessage *data)
{
	// Do we ned to take into account deep copy of images?  I guess so!
	if ( validate_instantiation(data, "BPictureButton"))
	{
		return new BPictureButton(data);
	}
	else
	{
		return NULL;
	}
}


/***********************************************************************
 *  Method: BPictureButton::Archive
 *  Params: BMessage *data, bool deep
 * Returns: status_t
 * Effects: 
 ***********************************************************************/
status_t
BPictureButton::Archive (BMessage *data, bool deep) const
{
	status_t err = BControl::Archive(data, deep);

	if (err != B_OK) return err;

	// Do we need this?  Good practice!
	err = data->AddString("class", "BPictureButton");

	if (err != B_OK) return err;

	// Fill out message, depending on whether a deep copy is required or not.
	if (deep)
	{
		BMessage pictureArchive;

		if (fEnabledOn->Archive(&pictureArchive, deep) == B_OK)
		{
			err = data->AddMessage("_e_on", &pictureArchive);
			if (err != B_OK) return err;
		}

		if (fEnabledOff->Archive(&pictureArchive, deep) == B_OK)
		{
			err = data->AddMessage("_e_off", &pictureArchive);
			if (err != B_OK) return err;
		}

		// Do we add messages for pictures that don't exist?
		if (fBehavior == B_TWO_STATE_BUTTON)
		{
			if (fDisabledOn->Archive(&pictureArchive, deep) == B_OK)
			{
				err = data->AddMessage("_d_on", &pictureArchive);
				if (err != B_OK) return err;
			}
			
			if (fDisabledOff->Archive(&pictureArchive, deep) == B_OK)
			{
				err = data->AddMessage("_d_off", &pictureArchive);
				if (err != B_OK) return err;
			}
		}
	}

	err = data->AddInt32("_behave", fBehavior);

	return err;
}


/***********************************************************************
 *  Method: BPictureButton::Draw
 *  Params: BRect updateRect
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::Draw (BRect updateRect)
{
	BRect rect = Bounds();

	// Need to check if TWO_STATE, if setEnabled=false, and if diabled picture is null
	// If so, and in debug, bring up an Alert, if so, and not in debug, output to stdout

	// We should request the view's base color which normaly is (216,216,216)
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (fBehavior == B_ONE_STATE_BUTTON)
	{
		if (BControl::Value() == B_CONTROL_ON)
		{
			BView::DrawPicture(fEnabledOn);
		}
		else
		{
			BView::DrawPicture(fEnabledOff);
		}
	}
	else		// Must be B_TWO_STATE_BUTTON:
	{
		if (BControl::IsEnabled())
		{
			if (BControl::Value() == B_CONTROL_ON)
			{
				BView::DrawPicture(fEnabledOn);
			}
			else
			{
				BView::DrawPicture(fEnabledOff);
			}
		}
		else		// Must be Disabled:
		{
			if (BControl::Value() == B_CONTROL_ON)
			{
				if (fDisabledOn == NULL)
				{
					debugger("Need to set the 'disabled' pictures for this BPictureButton ");
				}
				BView::DrawPicture(fDisabledOn);
			}
			else
			{
				if (fDisabledOn == NULL)
				{
					debugger("Need to set the 'disabled' pictures for this BPictureButton ");
				}
				BView::DrawPicture(fDisabledOff);
			}
		}
	}
	
	if (IsFocus())
	{
		SetHighColor( ui_color(B_KEYBOARD_NAVIGATION_COLOR) );
		StrokeRect(rect, B_SOLID_HIGH);
	}
}


/***********************************************************************
 *  Method: BPictureButton::MouseDown
 *  Params: BPoint where
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::MouseDown (BPoint point)
{
	if (!IsEnabled())
	{
		BControl::MouseDown(point);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);

	if (fBehavior == B_ONE_STATE_BUTTON)
	{
		SetValue(B_CONTROL_ON);
	}
	else
	{
		if (Value() == B_CONTROL_ON)
		{
			SetValue(B_CONTROL_OFF);
		}
		else
		{
			SetValue(B_CONTROL_ON);
		}
	}
	SetTracking(true);
}


/***********************************************************************
 *  Method: BPictureButton::MouseUp
 *  Params: BPoint pt
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::MouseUp (BPoint point)
{
	if (IsEnabled() && IsTracking())
	{
		if (Bounds().Contains(point))
		{
			if (fBehavior == B_ONE_STATE_BUTTON)
			{
				if (Value() == B_CONTROL_ON)
				{
					snooze(75000);
					SetValue(B_CONTROL_OFF);
				}
			}
			BControl::Invoke();
		}
		
		SetTracking(false);
	}
}


/***********************************************************************
 *  Method: BPictureButton::MouseMoved
 *  Params: BPoint pt, uint32 code, const BMessage *msg
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::MouseMoved (BPoint pt, uint32 code, const BMessage *msg)
{
	if (IsEnabled() && IsTracking())
	{
		if (code == B_EXITED_VIEW)
			SetValue(B_CONTROL_OFF);
		else if (code == B_ENTERED_VIEW)
			SetValue(B_CONTROL_ON);
	}
	else
	{
		BControl::MouseMoved(pt, code, msg);
	}
}


/***********************************************************************
 *  Method: BPictureButton::KeyDown
 *  Params: const char *bytes, int32 numBytes
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::KeyDown (const char *bytes, int32 numBytes)
{
	if (numBytes == 1)
	{
		switch (bytes[0])
		{
			case B_ENTER:
			case B_SPACE:
				if (fBehavior == B_ONE_STATE_BUTTON)
				{
					SetValue(B_CONTROL_ON);
					snooze(50000);
					SetValue(B_CONTROL_OFF);
				}
				else
				{
					if (Value() == B_CONTROL_ON)
					{
						SetValue(B_CONTROL_OFF);
					}
					else
					{
						SetValue(B_CONTROL_ON);
					}
				}
				Invoke();
				break;

			default:
				BControl::KeyDown(bytes, numBytes);
		}
	}
	else
	{
		BControl::KeyDown(bytes, numBytes);
	}
}


/***********************************************************************
 *  Method: BPictureButton::SetEnabledOn
 *  Params: BPicture *on
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetEnabledOn (BPicture *on)
{
	if (!fEnabledOn)
	{
		delete fEnabledOn;
	}
	fEnabledOn = new BPicture (*on);
}


/***********************************************************************
 *  Method: BPictureButton::SetEnabledOff
 *  Params: BPicture *off
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetEnabledOff (BPicture *off)
{
	if (!fEnabledOff)
	{
		delete fEnabledOff;
	}
	fEnabledOff = new BPicture (*off);
}


/***********************************************************************
 *  Method: BPictureButton::SetDisabledOn
 *  Params: BPicture *on
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetDisabledOn (BPicture *on)
{
	if (!fDisabledOn)
	{
		delete fDisabledOn;
	}
	fDisabledOn = new BPicture (*on);
}


/***********************************************************************
 *  Method: BPictureButton::SetDisabledOff
 *  Params: BPicture *off
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetDisabledOff (BPicture *off)
{
	if (!fDisabledOff)
	{
		delete fDisabledOff;
	}
	fDisabledOff = new BPicture (*off);
}


/***********************************************************************
 *  Method: BPictureButton::EnabledOn
 *  Params: 
 * Returns: BPicture *
 * Effects: 
 ***********************************************************************/
BPicture *
BPictureButton::EnabledOn () const
{
	return fEnabledOn;
}


/***********************************************************************
 *  Method: BPictureButton::EnabledOff
 *  Params: 
 * Returns: BPicture *
 * Effects: 
 ***********************************************************************/
BPicture *
BPictureButton::EnabledOff () const
{
	return fEnabledOff;
}


/***********************************************************************
 *  Method: BPictureButton::DisabledOn
 *  Params: 
 * Returns: BPicture *
 * Effects: 
 ***********************************************************************/
BPicture *
BPictureButton::DisabledOn () const
{
	return fDisabledOn;
}


/***********************************************************************
 *  Method: BPictureButton::DisabledOff
 *  Params: 
 * Returns: BPicture *
 * Effects: 
 ***********************************************************************/
BPicture *
BPictureButton::DisabledOff() const
{
	return fDisabledOff;
}


/***********************************************************************
 *  Method: BPictureButton::SetBehavior
 *  Params: uint32 behavior
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetBehavior (uint32 behavior)
{
	fBehavior = behavior;
}


/***********************************************************************
 *  Method: BPictureButton::Behavior
 *  Params: 
 * Returns: uint32
 * Effects: 
 ***********************************************************************/
uint32
BPictureButton::Behavior () const
{
	return fBehavior;
}


/***********************************************************************
 *  Method: BPictureButton::MessageReceived
 *  Params: BMessage *msg
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::MessageReceived (BMessage *msg)
{
	BControl::MessageReceived(msg);
}


/***********************************************************************
 *  Method: BPictureButton::WindowActivated
 *  Params: bool state
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::WindowActivated (bool state)
{
	BView::WindowActivated (state);
}


/***********************************************************************
 *  Method: BPictureButton::AttachedToWindow
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::AttachedToWindow ()
{
	BControl::AttachedToWindow();
}


/***********************************************************************
 *  Method: BPictureButton::DetachedFromWindow
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::DetachedFromWindow ()
{
	BView::DetachedFromWindow();
}


/***********************************************************************
 *  Method: BPictureButton::SetValue
 *  Params: int32 value
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::SetValue (int32 value)
{
	BControl::SetValue(value);
}


/***********************************************************************
 *  Method: BPictureButton::Invoke
 *  Params: BMessage *msg
 * Returns: status_t
 * Effects: 
 ***********************************************************************/
status_t
BPictureButton::Invoke (BMessage *msg)
{
	return BControl::Invoke(msg);
}


/***********************************************************************
 *  Method: BPictureButton::FrameMoved
 *  Params: BPoint new_position
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::FrameMoved (BPoint new_position)
{
	BView::FrameMoved (new_position);
}


/***********************************************************************
 *  Method: BPictureButton::FrameResized
 *  Params: float new_width, float new_height
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::FrameResized (float new_width, float new_height)
{
	BView::FrameResized (new_width, new_height);
}


/***********************************************************************
 *  Method: BPictureButton::ResolveSpecifier
 *  Params: BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property
 * Returns: BHandler *
 * Effects: 
 ***********************************************************************/
BHandler *
BPictureButton::ResolveSpecifier (BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property)
{
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}


/***********************************************************************
 *  Method: BPictureButton::GetSupportedSuites
 *  Params: BMessage *data
 * Returns: status_t
 * Effects: 
 ***********************************************************************/
status_t
BPictureButton::GetSupportedSuites (BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}


/***********************************************************************
 *  Method: BPictureButton::ResizeToPreferred
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::ResizeToPreferred ()
{
	float w, h;
	GetPreferredSize (&w, &h);
	BView::ResizeTo (w,h);
}


/***********************************************************************
 *  Method: BPictureButton::GetPreferredSize
 *  Params: float *width, float *height
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::GetPreferredSize (float *width, float *height)
{
	// Need to do some test to see what the Be method returns...
}


/***********************************************************************
 *  Method: BPictureButton::MakeFocus
 *  Params: bool state
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::MakeFocus (bool state)
{
	BControl::MakeFocus(state);
}


/***********************************************************************
 *  Method: BPictureButton::AllAttached
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::AllAttached ()
{
	BView::AllAttached();
}


/***********************************************************************
 *  Method: BPictureButton::AllDetached
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::AllDetached ()
{
	BView::AllDetached();
}


/***********************************************************************
 *  Method: BPictureButton::Perform
 *  Params: perform_code d, void *arg
 * Returns: status_t
 * Effects: 
 ***********************************************************************/
status_t
BPictureButton::Perform (perform_code d, void *arg)
{
	// Really clutching at straws here....
	return B_ERROR;
}


/***********************************************************************
 *  Method: BPictureButton::_ReservedPictureButton1
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::_ReservedPictureButton1 ()
{
}


/***********************************************************************
 *  Method: BPictureButton::_ReservedPictureButton2
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::_ReservedPictureButton2()
{
}


/***********************************************************************
 *  Method: BPictureButton::_ReservedPictureButton3
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::_ReservedPictureButton3()
{
}


/***********************************************************************
 *  Method: BPictureButton::operator=
 *  Params: const BPictureButton &
 * Returns: BPictureButton &
 * Effects: 
 ***********************************************************************/
BPictureButton &
BPictureButton::operator=(const BPictureButton &button)
{
	return *this;
}


/***********************************************************************
 *  Method: BPictureButton::Redraw
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::Redraw()
{
}


/***********************************************************************
 *  Method: BPictureButton::InitData
 *  Params: 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
BPictureButton::InitData()
{
}

