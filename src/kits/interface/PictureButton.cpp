/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Graham MacDonald (macdonag@btopenworld.com)
 */

/**	BPictureButton displays and controls a picture button in a window. */


#include <PictureButton.h>


BPictureButton::BPictureButton(BRect frame, const char* name, 
		BPicture *off, BPicture *on, BMessage *message, 
		uint32 behavior, uint32 resizeMask, uint32 flags)
	: BControl(frame, name, "", message, resizeMask, flags),
	fOutlined(false),
	fBehavior(behavior)
{
	fEnabledOff = new BPicture(*off);
	fEnabledOn = new BPicture(*on);
	fDisabledOff = NULL;
	fDisabledOn = NULL;
}


BPictureButton::BPictureButton(BMessage *data)
	: BControl(data)
{
	BMessage pictureArchive;

	// Default to 1 state button if not here - is this valid?
	if (data->FindInt32 ("_behave", (int32 *)&fBehavior) != B_OK)
		fBehavior = B_ONE_STATE_BUTTON;

	// Now expand the pictures:
	if (data->FindMessage("_e_on", &pictureArchive) == B_OK)
		fEnabledOn = new BPicture(&pictureArchive);

	if (data->FindMessage("_e_off", &pictureArchive) == B_OK)
		fEnabledOff = new BPicture(&pictureArchive);

	if (fBehavior == B_TWO_STATE_BUTTON) {
		if (data->FindMessage("_d_on", &pictureArchive) == B_OK)
			fDisabledOn = new BPicture(&pictureArchive);

		if (data->FindMessage("_d_off", &pictureArchive) == B_OK)
			fDisabledOff = new BPicture(&pictureArchive);
	} else
		fDisabledOn = fDisabledOff = NULL;
}


BPictureButton::~BPictureButton()
{
	delete fEnabledOn;
	delete fEnabledOff;
	delete fDisabledOn;
	delete fDisabledOff;
}


BArchivable *
BPictureButton::Instantiate(BMessage *data)
{
	if ( validate_instantiation(data, "BPictureButton"))
		return new BPictureButton(data);

	return NULL;
}


status_t
BPictureButton::Archive(BMessage *data, bool deep) const
{
	status_t err = BControl::Archive(data, deep);
	if (err != B_OK)
		return err;

	// Fill out message, depending on whether a deep copy is required or not.
	if (deep) {
		BMessage pictureArchive;

		if (fEnabledOn->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_e_on", &pictureArchive);
			if (err != B_OK)
				return err;
		}

		if (fEnabledOff->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_e_off", &pictureArchive);
			if (err != B_OK)
				return err;
		}

		// Do we add messages for pictures that don't exist?
		if (fBehavior == B_TWO_STATE_BUTTON) {
			if (fDisabledOn->Archive(&pictureArchive, deep) == B_OK) {
				err = data->AddMessage("_d_on", &pictureArchive);
				if (err != B_OK)
					return err;
			}

			if (fDisabledOff->Archive(&pictureArchive, deep) == B_OK) {
				err = data->AddMessage("_d_off", &pictureArchive);
				if (err != B_OK)
					return err;
			}
		}
	}

	return data->AddInt32("_behave", fBehavior);
}


void
BPictureButton::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	// Need to check if TWO_STATE, if setEnabled=false, and if diabled picture is null
	// If so, and in debug, bring up an Alert, if so, and not in debug, output to stdout

	// We should request the view's base color which normaly is (216,216,216)
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);

	if (fBehavior == B_ONE_STATE_BUTTON) {
		if (Value() == B_CONTROL_ON)
			DrawPicture(fEnabledOn);
		else
			DrawPicture(fEnabledOff);
	} else {
		// B_TWO_STATE_BUTTON

		if (IsEnabled()) {
			if (Value() == B_CONTROL_ON)
				DrawPicture(fEnabledOn);
			else
				DrawPicture(fEnabledOff);
		} else {
			// disabled
			if (Value() == B_CONTROL_ON) {
				if (fDisabledOn == NULL)
					debugger("Need to set the 'disabled' pictures for this BPictureButton ");

				DrawPicture(fDisabledOn);
			} else {
				if (fDisabledOn == NULL)
					debugger("Need to set the 'disabled' pictures for this BPictureButton ");

				DrawPicture(fDisabledOff);
			}
		}
	}

	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(rect, B_SOLID_HIGH);
	}
}


void
BPictureButton::MouseDown(BPoint point)
{
	if (!IsEnabled()) {
		BControl::MouseDown(point);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,
		B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);

	if (fBehavior == B_ONE_STATE_BUTTON) {
		SetValue(B_CONTROL_ON);
	} else {
		if (Value() == B_CONTROL_ON)
			SetValue(B_CONTROL_OFF);
		else
			SetValue(B_CONTROL_ON);
	}
	SetTracking(true);
}


void
BPictureButton::MouseUp(BPoint point)
{
	if (IsEnabled() && IsTracking()) {
		if (Bounds().Contains(point)) {
			if (fBehavior == B_ONE_STATE_BUTTON) {
				if (Value() == B_CONTROL_ON) {
					snooze(75000);
					SetValue(B_CONTROL_OFF);
				}
			}
			Invoke();
		}

		SetTracking(false);
	}
}


void
BPictureButton::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (IsEnabled() && IsTracking()) {
		if (transit == B_EXITED_VIEW)
			SetValue(B_CONTROL_OFF);
		else if (transit == B_ENTERED_VIEW)
			SetValue(B_CONTROL_ON);
	} else
		BControl::MouseMoved(point, transit, msg);
}


void
BPictureButton::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_ENTER:
			case B_SPACE:
				if (fBehavior == B_ONE_STATE_BUTTON) {
					SetValue(B_CONTROL_ON);
					snooze(50000);
					SetValue(B_CONTROL_OFF);
				} else {
					if (Value() == B_CONTROL_ON)
						SetValue(B_CONTROL_OFF);
					else
						SetValue(B_CONTROL_ON);
				}
				Invoke();
				return;
		}
	}

	BControl::KeyDown(bytes, numBytes);
}


void
BPictureButton::SetEnabledOn(BPicture *on)
{
	delete fEnabledOn;
	fEnabledOn = new BPicture(*on);
}


void
BPictureButton::SetEnabledOff(BPicture *off)
{
	delete fEnabledOff;
	fEnabledOff = new BPicture(*off);
}


void
BPictureButton::SetDisabledOn(BPicture *on)
{
	delete fDisabledOn;
	fDisabledOn = new BPicture(*on);
}


void
BPictureButton::SetDisabledOff(BPicture *off)
{
	delete fDisabledOff;
	fDisabledOff = new BPicture(*off);
}


BPicture *
BPictureButton::EnabledOn() const
{
	return fEnabledOn;
}


BPicture *
BPictureButton::EnabledOff() const
{
	return fEnabledOff;
}


BPicture *
BPictureButton::DisabledOn() const
{
	return fDisabledOn;
}


BPicture *
BPictureButton::DisabledOff() const
{
	return fDisabledOff;
}


void
BPictureButton::SetBehavior(uint32 behavior)
{
	fBehavior = behavior;
}


uint32
BPictureButton::Behavior() const
{
	return fBehavior;
}


void
BPictureButton::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}


void
BPictureButton::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}


void
BPictureButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
}


void
BPictureButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}


void
BPictureButton::SetValue(int32 value)
{
	BControl::SetValue(value);
}


status_t
BPictureButton::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}


void
BPictureButton::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}


void
BPictureButton::FrameResized(float newWidth, float newHeight)
{
	BControl::FrameResized(newWidth, newHeight);
}


BHandler *
BPictureButton::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *specifier, int32 form, const char *property)
{
	return BControl::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BPictureButton::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}


void
BPictureButton::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	BControl::ResizeTo(width, height);
}


void
BPictureButton::GetPreferredSize(float* _width, float* _height)
{
	// Need to do some test to see what the Be method returns...
	BControl::GetPreferredSize(_width, _height);
}


void
BPictureButton::MakeFocus(bool state)
{
	BControl::MakeFocus(state);
}


void
BPictureButton::AllAttached()
{
	BControl::AllAttached();
}


void
BPictureButton::AllDetached()
{
	BControl::AllDetached();
}


status_t
BPictureButton::Perform (perform_code d, void *arg)
{
	// Really clutching at straws here....
	return B_ERROR;
}


void BPictureButton::_ReservedPictureButton1() {}
void BPictureButton::_ReservedPictureButton2() {}
void BPictureButton::_ReservedPictureButton3() {}


BPictureButton &
BPictureButton::operator=(const BPictureButton &button)
{
	return *this;
}


void
BPictureButton::Redraw()
{
}


void
BPictureButton::InitData()
{
}

