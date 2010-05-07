/*
 * Copyright 2007-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProgressWindow.h"

#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>
#include <MessageRunner.h>
#include <Screen.h>
#include <StatusBar.h>

#include "ShowImageConstants.h"


static const uint32 kMsgShow = 'show';

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ProgressWindow"


ProgressWindow::ProgressWindow(BWindow* referenceWindow, bool center)
	:
	BWindow(BRect(0, 0, 250, 100), B_TRANSLATE("Progress monitor"),
		B_MODAL_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fRunner(NULL)
{
	BRect rect = Bounds();

	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds().InsetByCopy(5, 5);
	fStatusBar = new BStatusBar(rect, "status", NULL, NULL);
	float width, height;
	fStatusBar->GetPreferredSize(&width, &height);
	fStatusBar->ResizeTo(rect.Width(), height);
	fStatusBar->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fStatusBar);

	BScreen screen(referenceWindow);
	if (!center) {
		ResizeTo(Bounds().Width(), height + 9);
		// TODO: frame width!
		MoveTo(screen.Frame().left + 5,
			screen.Frame().bottom - Bounds().Height() - 5);
	} else
		CenterIn(screen.Frame());

	Run();
}


ProgressWindow::~ProgressWindow()
{
	delete fRunner;
}


void
ProgressWindow::Start()
{
	BAutolock _(this);

	fRetrievedUpdate = false;
	fRetrievedShow = false;
	delete fRunner;

	BMessage show(kMsgShow);
	fRunner = new BMessageRunner(this, &show, 1000000, 1);
}


void
ProgressWindow::Stop()
{
	BAutolock _(this);

	delete fRunner;
	fRunner = NULL;

	if (!IsHidden())
		Hide();
}


void
ProgressWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgShow:
			if (fRetrievedUpdate && IsHidden()) {
				Show();
				Minimize(false);
			}

			fRetrievedShow = true;
			break;

		case kMsgProgressStatusUpdate:
			float percent;
			if (message->FindFloat("percent", &percent) == B_OK)
				fStatusBar->Update(percent - fStatusBar->CurrentValue());

			const char *text;
			if (message->FindString("message", &text) == B_OK)
				fStatusBar->SetText(text);

			fRetrievedUpdate = true;

			if (fRetrievedShow && IsHidden()) {
				Show();
				Minimize(false);
			}
			break;

		default:
			BWindow::MessageReceived(message);
	}
}

