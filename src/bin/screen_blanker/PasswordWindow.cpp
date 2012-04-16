/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Julun <host.haiku@gmx.de>
 */


#include "PasswordWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Screen.h>

#include <WindowPrivate.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver password dialog"


PasswordWindow::PasswordWindow()
	:
	BWindow(BRect(100, 100, 400, 230), "Enter password",
		B_NO_BORDER_WINDOW_LOOK, kPasswordWindowFeel
			/* TODO: B_MODAL_APP_WINDOW_FEEL should also behave correctly */,
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS, B_ALL_WORKSPACES)
{
	BView* topView = new BView(Bounds(), "topView", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BRect bounds(Bounds());
	bounds.InsetBy(10.0, 10.0);

	BBox *customBox = new BBox(bounds, "customBox", B_FOLLOW_NONE);
	topView->AddChild(customBox);
	customBox->SetLabel(B_TRANSLATE("Unlock screen saver"));

	bounds.top += 10.0;
	fPassword = new BTextControl(bounds, "password",
		B_TRANSLATE("Enter password:"), "VeryLongPasswordPossible",
		B_FOLLOW_NONE);
	customBox->AddChild(fPassword);
	fPassword->MakeFocus(true);
	fPassword->ResizeToPreferred();
	fPassword->TextView()->HideTyping(true);
	fPassword->SetDivider(be_plain_font->StringWidth(
		B_TRANSLATE_NOCOLLECT("Enter password:")) + 5.0);

	BButton* button = new BButton(BRect(), "unlock", B_TRANSLATE("Unlock"),
		new BMessage(kMsgUnlock), B_FOLLOW_NONE);
	customBox->AddChild(button);
	button->MakeDefault(true);
	button->ResizeToPreferred();
	button->SetTarget(NULL, be_app);

	BRect frame = fPassword->Frame();
	button->MoveTo(frame.right - button->Bounds().Width(), frame.bottom + 10.0);
	customBox->ResizeTo(frame.right + 10.0,	button->Frame().bottom + 10.0);

	frame = customBox->Frame();
	ResizeTo(frame.right + 10.0, frame.bottom + 10.0);

	BScreen screen(this);
	MoveTo(screen.Frame().left + (screen.Frame().Width() - Bounds().Width()) / 2,
		screen.Frame().top + (screen.Frame().Height() - Bounds().Height()) / 2);
}


void
PasswordWindow::SetPassword(const char* text)
{
	if (Lock()) {
		fPassword->SetText(text);
		fPassword->MakeFocus(true);
		Unlock();
	}
}
