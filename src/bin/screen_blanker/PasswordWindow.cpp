/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "PasswordWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Screen.h>
#include <StringView.h>


PasswordWindow::PasswordWindow()
	: BWindow(BRect(100, 100, 400, 230), "Enter Password", B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_MOVABLE | B_NOT_CLOSABLE |B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS ,
		B_ALL_WORKSPACES)
{
	BScreen screen(this);
	MoveTo(screen.Frame().left + (screen.Frame().Width() - Bounds().Width()) / 2,
		screen.Frame().top + (screen.Frame().Height() - Bounds().Height()) / 2);

	BView* topView = new BView(Bounds(), "top", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(topView);

	BRect bounds(Bounds());	
	bounds.InsetBy(5, 5);

	BBox* box = new BBox(bounds, NULL, B_FOLLOW_NONE);
	box->SetLabel("Unlock screen saver");
	topView->AddChild(box);

	fPassword = new BTextControl(BRect(10,28,260,47), NULL, "Enter password:",
		NULL, B_FOLLOW_NONE);
	fPassword->TextView()->HideTyping(true);
	fPassword->SetDivider(100);
	box->AddChild(fPassword);
	fPassword->MakeFocus(true);

	BButton* button = new BButton(BRect(160,70,255,85), "fUnlock", "Unlock",
		new BMessage(kMsgUnlock), B_FOLLOW_NONE);
	button->SetTarget(NULL, be_app);
	box->AddChild(button);
	button->MakeDefault(true);
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
