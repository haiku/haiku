/*
 * Copyright 2001-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_RADIO_BUTTON_H
#define	_RADIO_BUTTON_H


#include <Bitmap.h>
#include <Control.h>


class BRadioButton : public BControl {
public:
								BRadioButton(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizMask
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BRadioButton(const char* name,
									const char* label, BMessage* message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BRadioButton(const char* label,
									BMessage* message);

								BRadioButton(BMessage* archive);
	virtual						~BRadioButton();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				AttachedToWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				SetValue(int32 value);
	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();
	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				WindowActivated(bool active);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				DetachedFromWindow();
	virtual	void				FrameMoved(BPoint newLocation);
	virtual	void				FrameResized(float width, float height);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);

	virtual	void				MakeFocus(bool focused = true);
	virtual	void				AllAttached();
	virtual	void				AllDetached();
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code d, void* argument);

	virtual	BSize				MaxSize();


private:
	friend	status_t			_init_interface_kit_();

	virtual	void				_ReservedRadioButton1();
	virtual	void				_ReservedRadioButton2();

			BRadioButton&		operator=(const BRadioButton& other);

			BRect				_KnobFrame() const;
			BRect				_KnobFrame(
									const font_height& fontHeight) const;
			void				_Redraw();
				// for use in "synchronous" BWindows

private:
	static	BBitmap*			sBitmaps[2][3];

			bool				fOutlined;

			uint32				_reserved[2];
};

#endif // _RADIO_BUTTON_H
