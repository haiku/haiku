/*
 * Copyright 2001-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PICTURE_BUTTON_H
#define _PICTURE_BUTTON_H


#include <Control.h>
#include <Picture.h>


enum {
	B_ONE_STATE_BUTTON,
	B_TWO_STATE_BUTTON
};


class BPictureButton : public BControl {
public:
								BPictureButton(BRect frame, const char* name,
									BPicture* off, BPicture* on,
									BMessage* message,
									uint32 behavior = B_ONE_STATE_BUTTON,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flgs = B_WILL_DRAW | B_NAVIGABLE);
								BPictureButton(BMessage* archive);

	virtual						~BPictureButton();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focus = true);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);

	virtual	void				SetEnabledOn(BPicture* picture);
	virtual	void				SetEnabledOff(BPicture* picture);
	virtual	void				SetDisabledOn(BPicture* picture);
	virtual	void				SetDisabledOff(BPicture* picture);

			BPicture*			EnabledOn() const;
			BPicture*			EnabledOff() const;
			BPicture*			DisabledOn() const;
			BPicture*			DisabledOff() const;

	virtual	void				SetBehavior(uint32 behavior);
	uint32						Behavior() const;

	virtual	void				SetValue(int32 value);
	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	status_t			Perform(perform_code code, void* data);

	virtual	status_t			SetIcon(const BBitmap* icon, uint32 flags = 0);

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedPictureButton1();
	virtual	void				_ReservedPictureButton2();
	virtual	void				_ReservedPictureButton3();

			BPictureButton&		operator=(const BPictureButton& other);

private:
			BPicture*			fEnabledOff;
			BPicture*			fEnabledOn;
			BPicture*			fDisabledOff;
			BPicture*			fDisabledOn;

			bool				unused;

			uint32				fBehavior;

			uint32				_reserved[4];
};

#endif // _PICTURE_BUTTON_H
