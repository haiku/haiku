/*
 * Copyright 2007-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProgressWindow.h"

#include <Autolock.h>
#include <Button.h>
#include <Catalog.h>
#include <MessageRunner.h>
#include <Screen.h>
#include <StatusBar.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProgressWindow"


static const uint32 kMsgShow = 'show';


ProgressWindow::ProgressWindow(BWindow* referenceWindow,
		BMessage* abortMessage)
	: BWindow(BRect(0, 0, 250, 100), B_TRANSLATE("Progress monitor"),
		B_MODAL_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
			| B_NO_WORKSPACE_ACTIVATION),
	fRunner(NULL)
{
	BRect rect = Bounds();

	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	AddChild(view);

	rect = view->Bounds().InsetByCopy(8, 8);
	fStatusBar = new BStatusBar(rect, "status", NULL, NULL);
	float width, height;
	fStatusBar->GetPreferredSize(&width, &height);
	fStatusBar->ResizeTo(rect.Width(), height);
	fStatusBar->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fStatusBar);

	if (abortMessage != NULL && referenceWindow) {
		rect.top += height + 8;
		BButton* button = new BButton(rect, "abort", B_TRANSLATE("Abort"),
			abortMessage);
		button->ResizeToPreferred();
		button->MoveBy((rect.Width() - button->Bounds().Width()) / 2, 0);
		view->AddChild(button);
		button->SetTarget(referenceWindow);
		height = button->Frame().bottom;
	}

	ResizeTo(Bounds().Width(), height + 8);
	_Center(referenceWindow);
	Run();
}


ProgressWindow::~ProgressWindow()
{
	delete fRunner;
}


void
ProgressWindow::_Center(BWindow* referenceWindow)
{
	BRect frame;
	if (referenceWindow != NULL)
		frame = referenceWindow->Frame();
	else
		frame = BScreen().Frame();

	MoveTo(frame.left + (frame.Width() - Bounds().Width()) / 2,
		frame.top + (frame.Height() - Bounds().Height()) / 2);
}

void
ProgressWindow::Start(BWindow* referenceWindow)
{
	BAutolock _(this);

	_Center(referenceWindow);

	if (referenceWindow != NULL)
		SetWorkspaces(referenceWindow->Workspaces());

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
			break;
	}
}
