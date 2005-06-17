/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "AlertWindow.h"
#include "AlertView.h"
#include "Constants.h"

#include <Window.h>


AlertWindow::AlertWindow(BRect frame, BMessenger target)
	: BWindow(frame, "Revert", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	fTarget(target)
{
	fAlertView = new AlertView(Bounds(), "AlertView");
	AddChild(fAlertView);

	// the view displays a decrementing counter (until the user must take action)
	SetPulseRate(1000000);	// every second
}


void
AlertWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BUTTON_KEEP_MSG:
			fTarget.SendMessage(MAKE_INITIAL_MSG);
			PostMessage(B_QUIT_REQUESTED);
			break;

		case BUTTON_REVERT_MSG:
			fTarget.SendMessage(SET_INITIAL_MODE_MSG);
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
