/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */

#ifndef PASSWORDWINDOW_H
#define PASSWORDWINDOW_H

#include <Window.h>
#include <Box.h>
#include <TextControl.h>
#include <Button.h>

const static int32 UNLOCK_MESSAGE = 'ULMS';

class PasswordWindow : public BWindow
{
	public:
		PasswordWindow() : BWindow(BRect(100,100,400,230),"pwView",B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL  ,
				B_NOT_MOVABLE | B_NOT_CLOSABLE |B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS ,
				B_ALL_WORKSPACES), fDie(false) { Setup(); }

		void Setup(void);
		const char *GetPassword(void) {return fPassword->Text();}
		void SetPassword(const char* text) { if (Lock()) { fPassword->SetText(text); fPassword->MakeFocus(true); Unlock();} }

		bool fDie;
	private:
		BView *fBgd;
		BBox *fCustomBox;
		BTextControl *fPassword;
		BButton *fUnlock;
};

#endif // PASSWORDWINDOW_H
