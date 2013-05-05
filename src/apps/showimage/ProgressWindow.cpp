/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProgressWindow.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>
#include <MessageRunner.h>
#include <Screen.h>
#include <StatusBar.h>

#include "ShowImageConstants.h"


static const uint32 kMsgShow = 'show';

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProgressWindow"


ProgressWindow::ProgressWindow()
	:
	BWindow(BRect(0, 0, 250, 100), B_TRANSLATE("Progress monitor"),
		B_MODAL_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,// B_FLOATING_APP_WINDOW_FEEL,
		// TODO: a bug in the app_server prevents an initial floating-app feel
		// to work correctly; the window will then not be visible for the first
		// image, even though it's later set to normal feel in that case.
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fRunner(NULL)
{
	BRect rect = Bounds();

	BView* view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds().InsetByCopy(5, 5);
	fStatusBar = new BStatusBar(rect, "status", NULL, NULL);
	float width, height;
	fStatusBar->GetPreferredSize(&width, &height);
	fStatusBar->ResizeTo(rect.Width(), height);
	fStatusBar->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fStatusBar);

	ResizeTo(Bounds().Width(), height + 9);

	Run();
}


ProgressWindow::~ProgressWindow()
{
	delete fRunner;
}


void
ProgressWindow::Start(BWindow* referenceWindow, bool center)
{
	BAutolock _(this);

	BScreen screen(referenceWindow);
	if (!center) {
		BMessage settings;
		GetDecoratorSettings(&settings);

		int32 borderWidth;
		if (settings.FindInt32("border width", &borderWidth) != B_OK)
			borderWidth = 5;

		MoveTo(screen.Frame().left + borderWidth,
			screen.Frame().bottom - Bounds().Height() - borderWidth);
	} else
		CenterIn(screen.Frame());

	SetFeel(referenceWindow->IsHidden()
		? B_NORMAL_WINDOW_FEEL : B_FLOATING_APP_WINDOW_FEEL);

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
ProgressWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgShow:
			if (fRetrievedUpdate && IsHidden()) {
				Show();
				Minimize(false);
			}

			fRetrievedShow = true;
			break;

		case kMsgProgressUpdate:
			float percent;
			if (message->FindFloat("percent", &percent) == B_OK)
				fStatusBar->Update(percent - fStatusBar->CurrentValue());

			const char* text;
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

