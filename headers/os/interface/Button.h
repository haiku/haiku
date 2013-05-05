/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUTTON_H
#define _BUTTON_H

#include <Control.h>


class BButton : public BControl {
public:
								BButton(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE
										| B_FULL_UPDATE_ON_RESIZE); 
								BButton(const char* name, const char* label,
									BMessage *message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE
										| B_FULL_UPDATE_ON_RESIZE);
								BButton(const char* label,
									BMessage* message = NULL);
	
								BButton(BMessage *archive);

	virtual						~BButton();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				AttachedToWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeDefault(bool flag);
	virtual	void				SetLabel(const char* string);
			bool				IsDefault() const;

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				WindowActivated(bool active);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* message);
	virtual	void				MouseUp(BPoint point);
	virtual	void				DetachedFromWindow();
	virtual	void				SetValue(int32 value);
	virtual	void				GetPreferredSize (float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();
	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	void				FrameMoved(BPoint newLocation);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused = true);
	virtual	void				AllAttached();
	virtual	void				AllDetached();
	
	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);
	virtual	status_t			Perform(perform_code d, void* arg);


	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();


protected:
	virtual	void				LayoutInvalidated(bool descendants = false);

private:
	virtual	void				_ReservedButton1();
	virtual	void				_ReservedButton2();
	virtual	void				_ReservedButton3();

private:
			BButton&			operator=(const BButton &);

			BSize				_ValidatePreferredSize();
	
			BRect				_DrawDefault(BRect bounds, bool enabled);
			void 				_DrawFocusLine(float x, float y, float width, 
									bool bVisible);
			 
			BSize				fPreferredSize;
			bool				fDrawAsDefault;

			uint32				_reserved[2];
};

#endif // _BUTTON_H
