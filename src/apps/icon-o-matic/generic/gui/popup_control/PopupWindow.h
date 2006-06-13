/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef POPUP_WINDOW_H
#define POPUP_WINDOW_H

#include <MWindow.h>

enum {
	MSG_POPUP_SHOWN		= 'push',
	MSG_POPUP_HIDDEN	= 'puhi',
};

class PopupControl;
class PopupView;

class PopupWindow : public MWindow {
 public:
								PopupWindow(PopupView* child,
											PopupControl* control);
	virtual						~PopupWindow();

	virtual	void				MessageReceived(BMessage* message);

								// MWindow
	virtual	void				Show();
	virtual	void				Hide();
	virtual	bool				QuitRequested();

	virtual	void				PopupDone(bool canceled = true);

 private:
			bool				fCanceled;
			PopupControl*		fControl;
};


#endif	// POPUP_CONTROL_H
