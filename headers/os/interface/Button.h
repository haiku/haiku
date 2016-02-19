/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUTTON_H
#define _BUTTON_H


#include <Control.h>


class BButton : public BControl {
public:
			enum BBehavior {
				B_BUTTON_BEHAVIOR,
				B_TOGGLE_BEHAVIOR,
				B_POP_UP_BEHAVIOR,
			};

public:
								BButton(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE
										| B_FULL_UPDATE_ON_RESIZE); 
								BButton(const char* name, const char* label,
									BMessage* message,
										uint32 flags = B_WILL_DRAW | B_NAVIGABLE
											| B_FULL_UPDATE_ON_RESIZE);
								BButton(const char* label,
									BMessage* message = NULL);
								BButton(BMessage* data);

	virtual						~BButton();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;
	
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				AttachedToWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeDefault(bool flag);
	virtual	void				SetLabel(const char* string);
			bool				IsDefault() const;

			bool				IsFlat() const;
			void				SetFlat(bool flat);

			BBehavior			Behavior() const;
			void				SetBehavior(BBehavior behavior);

			BMessage*			PopUpMessage() const;
			void				SetPopUpMessage(BMessage* message);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				WindowActivated(bool active);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				DetachedFromWindow();
	virtual	void				SetValue(int32 value);
	virtual	void				GetPreferredSize (float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();
	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				MakeFocus(bool focus = true);
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

	virtual	status_t			SetIcon(const BBitmap* icon, uint32 flags = 0);

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);

private:
	virtual	void				_ReservedButton1();
	virtual	void				_ReservedButton2();
	virtual	void				_ReservedButton3();

			BButton&			operator=(const BButton &);

			BSize				_ValidatePreferredSize();

			BRect				_PopUpRect() const;

	inline	bool				_Flag(uint32 flag) const;
	inline	bool				_SetFlag(uint32 flag, bool set);
			 
private:
			BSize				fPreferredSize;
			uint32				fFlags;
			BBehavior			fBehavior;
			BMessage*			fPopUpMessage;
};


#endif	// _BUTTON_H
