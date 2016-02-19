/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHECK_BOX_H
#define _CHECK_BOX_H


#include <Control.h>


class BCheckBox : public BControl {
public:
								BCheckBox(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BCheckBox(const char* name, const char* label,
									BMessage* message, uint32 flags
										= B_WILL_DRAW | B_NAVIGABLE);
								BCheckBox(const char* label,
									BMessage* message = NULL);
								BCheckBox(BMessage* data);

	virtual						~BCheckBox();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	void				Draw(BRect updateRect);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				WindowActivated(bool active);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			LayoutAlignment();

	virtual	void				MakeFocus(bool focused = true);

	virtual	void				SetValue(int32 value);
	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code code, void* data);

	virtual	status_t			SetIcon(const BBitmap* icon, uint32 flags = 0);

			bool				IsPartialStateToOff() const;
			void				SetPartialStateToOff(bool partialToOff);

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);

private:
	// FBC padding
	virtual	void				_ReservedCheckBox1();
	virtual	void				_ReservedCheckBox2();
	virtual	void				_ReservedCheckBox3();

private:
	inline	BRect				_CheckBoxFrame(const font_height& fontHeight)
									const;
			BRect				_CheckBoxFrame() const;
			BSize				_ValidatePreferredSize();
			int32				_NextState() const;

private:
	// Forbidden
			BCheckBox&			operator=(const BCheckBox&);

private:
			BSize				fPreferredSize;
			bool				fOutlined;
			bool				fPartialToOff;
};


#endif // _CHECK_BOX_H
