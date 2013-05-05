/*
 * Copyright 2001-2006, Haiku.
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

#include <Catalog.h>
#include <Window.h>
#include <Screen.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screen"


AlertWindow::AlertWindow(BMessenger target)
	: BWindow(BRect(100.0, 100.0, 400.0, 193.0), B_TRANSLATE("Undo"),
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	fTarget(target)
{
	fAlertView = new AlertView(Bounds(), "AlertView");

	ResizeTo(fAlertView->Bounds().Width(), fAlertView->Bounds().Height());
	AddChild(fAlertView);

	// center window on screen
	BScreen screen(this);
	MoveTo(screen.Frame().left + (screen.Frame().Width() - Frame().Width()) / 2,
		screen.Frame().top + (screen.Frame().Height() - Frame().Height()) / 2);
}


void
AlertWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BUTTON_KEEP_MSG:
			fTarget.SendMessage(MAKE_INITIAL_MSG);
			PostMessage(B_QUIT_REQUESTED);
			break;

		case BUTTON_UNDO_MSG:
			fTarget.SendMessage(BUTTON_UNDO_MSG);
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
