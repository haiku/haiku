/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef POPUP_CONTROL_H
#define POPUP_CONTROL_H

#include <View.h>

#include <layout.h>

class PopupView;
class PopupWindow;

class PopupControl : public MView, public BView {
 public:
								PopupControl(const char* name,
											 PopupView* child);
	virtual						~PopupControl();

								// MView
	virtual	minimax				layoutprefs() = 0;
	virtual	BRect				layout(BRect frame) = 0;

								// BHandler
	virtual	void				MessageReceived(BMessage* message);

								// PopupControl
			void				SetPopupAlignment(float hPopupAlignment,
												  float vPopupAlignment);
			void				SetPopupLocation(BPoint leftTop);

								// "offset" will be modified if the popup
								// window had to be moved in order to
								// show entirely on screen
	virtual	void				ShowPopup(BPoint* offset = NULL);
	virtual	void				HidePopup();

	virtual	void				PopupShown();
	virtual	void				PopupHidden(bool canceled);

 private:
			PopupWindow*		fPopupWindow;
			PopupView*			fPopupChild;
			float				fHPopupAlignment;
			float				fVPopupAlignment;
			BPoint				fPopupLeftTop;
};


#endif	// POPUP_CONTROL_H
