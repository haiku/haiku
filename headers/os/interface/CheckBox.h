/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHECK_BOX_H
#define _CHECK_BOX_H

#include <Control.h>


class BCheckBox : public BControl {
public:
								BCheckBox(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BCheckBox(const char* name, const char* label,
									BMessage* message, uint32 flags
										= B_WILL_DRAW | B_NAVIGABLE); 
								BCheckBox(const char* label,
									BMessage* message = NULL); 
								BCheckBox(BMessage* archive);

	virtual						~BCheckBox();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				Draw(BRect updateRect);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				FrameMoved(BPoint newLocation);
	virtual	void				FrameResized(float width, float height);
	virtual	void				WindowActivated(bool active);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				MakeFocus(bool focused = true);

	virtual	void				SetValue(int32 value);
	virtual	status_t			Invoke(BMessage* message = NULL);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code code, void* data);

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);

private:
	// FBC padding
	virtual	void				_ReservedCheckBox1();
	virtual	void				_ReservedCheckBox2();
	virtual	void				_ReservedCheckBox3();

private:
			BRect				_CheckBoxFrame() const;
			BSize				_ValidatePreferredSize();

private:
	// Forbidden
			BCheckBox&			operator=(const BCheckBox&);

private:
			BSize				fPreferredSize;
			bool				fOutlined;
};

#endif // _CHECK_BOX_H

