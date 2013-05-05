/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */


#include "PrefsWindow.h"
#include "Common.h"
#include "PulseApp.h"
#include "ConfigView.h"

#include <Catalog.h>
#include <Button.h>
#include <TabView.h>
#include <TextControl.h>

#include <stdlib.h>
#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefsWindow"


PrefsWindow::PrefsWindow(BRect frame, const char *name, 
	BMessenger *messenger, Prefs *prefs)
	: BWindow(frame, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fTarget(*messenger),
	fPrefs(prefs)
{
	// This gives a nice look, and sets the background color correctly
	BRect bounds = Bounds();
	BView* topView = new BView(bounds, "ParentView", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	bounds.top += 10;
	bounds.bottom -= 45;
	fTabView = new BTabView(bounds, "TabView", B_WIDTH_FROM_LABEL);
	fTabView->SetFont(be_plain_font);
	topView->AddChild(fTabView);

	BRect rect = fTabView->ContainerView()->Bounds();
	rect.InsetBy(5, 5);

	ConfigView *normalView = new ConfigView(rect, B_TRANSLATE("Normal mode"),
		PRV_NORMAL_CHANGE_COLOR, fTarget, prefs);
	fTabView->AddTab(normalView);

	ConfigView *miniView = new ConfigView(rect, B_TRANSLATE("Mini mode"), 
		PRV_MINI_CHANGE_COLOR, fTarget, prefs);
	fTabView->AddTab(miniView);

	ConfigView *deskbarView = new ConfigView(rect, B_TRANSLATE("Deskbar mode"),
		PRV_DESKBAR_CHANGE_COLOR, fTarget, prefs);
	fTabView->AddTab(deskbarView);

	float width, height;
	deskbarView->GetPreferredSize(&width, &height);
	normalView->ResizeTo(width, height);
	miniView->ResizeTo(width, height);
	deskbarView->ResizeTo(width, height);

	fTabView->Select(0L);
	fTabView->ResizeTo(deskbarView->Bounds().Width() + 16.0f, 
		deskbarView->Bounds().Height() + 
		fTabView->ContainerView()->Frame().top + 16.0f);

	BButton *okButton = new BButton(rect, "ok", B_TRANSLATE("OK"), 
		new BMessage(PRV_BOTTOM_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okButton->ResizeToPreferred();
	okButton->MoveTo(Bounds().Width() - 8.0f - okButton->Bounds().Width(),
		Bounds().Height() - 8.0f - okButton->Bounds().Height());
	topView->AddChild(okButton);

	BButton *defaultsButton = new BButton(okButton->Frame(), "defaults", 
		B_TRANSLATE("Defaults"), new BMessage(PRV_BOTTOM_DEFAULTS), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	defaultsButton->ResizeToPreferred();
	defaultsButton->MoveBy(-defaultsButton->Bounds().Width() - 10.0f, 0.0f);
	topView->AddChild(defaultsButton);

	okButton->MakeDefault(true);

	fTabView->SetResizingMode(B_FOLLOW_NONE);
	ResizeTo(fTabView->Frame().Width(), fTabView->Frame().bottom + 16.0f
		+ okButton->Bounds().Height());
}


PrefsWindow::~PrefsWindow()
{
	fPrefs->prefs_window_rect = Frame();
	fPrefs->Save();
}


void
PrefsWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case PRV_BOTTOM_OK:
		{
			Hide();

			fTabView->Select(2);
			ConfigView *deskbar = (ConfigView *)FindView(
				B_TRANSLATE("Deskbar mode"));
			deskbar->UpdateDeskbarIconWidth();

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case PRV_BOTTOM_DEFAULTS:
		{
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			BView *view = tab->View();
			view->MessageReceived(message);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
PrefsWindow::QuitRequested()
{
	fTarget.SendMessage(new BMessage(PRV_QUIT));
	return true;
}
