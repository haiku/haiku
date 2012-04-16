/*
 * Copyright (c) 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageTextViewer.h"

#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <ScrollView.h>

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>


enum {
	P_MSG_ACCEPT = 'pmac',
	P_MSG_DECLINE
};

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageTextViewer"


PackageTextViewer::PackageTextViewer(const char *text, bool disclaimer)
	:
	BWindow(BRect(125, 125, 675, 475), B_TRANSLATE("Disclaimer"),
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE),
	fValue(0)
{
	_InitView(text, disclaimer);
}


PackageTextViewer::~PackageTextViewer()
{
}


int32
PackageTextViewer::Go()
{
	// Since this class can be thought of as a modified BAlert window, no use
	// to reinvent a well fledged wheel. This concept has been borrowed from
	// the current BAlert implementation
	fSemaphore = create_sem(0, "TextViewer");
	if (fSemaphore < B_OK) {
		Quit();
		return B_ERROR;
	}

	BWindow *parent =
		dynamic_cast<BWindow *>(BLooper::LooperForThread(find_thread(NULL)));
	Show();

	if (parent) {
		status_t ret;
		for (;;) {
			do {
				ret = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, 50000);
			} while (ret == B_INTERRUPTED);

			if (ret == B_BAD_SEM_ID)
				break;
			parent->UpdateIfNeeded();
		}
	}
	else {
		// Since there are no spinlocks, wait until the semaphore is free
		while (acquire_sem(fSemaphore) == B_INTERRUPTED) {
		}
	}

	int32 value = fValue;
	if (Lock())
		Quit();

	return value;
}


void
PackageTextViewer::MessageReceived(BMessage *msg)
{
	if (msg->what == P_MSG_ACCEPT) {
		if (fSemaphore >= B_OK) {
			fValue = 1;
			delete_sem(fSemaphore);
			fSemaphore = -1;
		}
	} else if (msg->what == P_MSG_DECLINE) {
		if (fSemaphore >= B_OK) {
			fValue = 0;
			delete_sem(fSemaphore);
			fSemaphore = -1;
		}
	} else
		BWindow::MessageReceived(msg);
}


// #pragma mark -


void
PackageTextViewer::_InitView(const char *text, bool disclaimer)
{
	fBackground = new BView(Bounds(), "background_view", 0, 0);
	fBackground->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect bounds;
	BRect rect = Bounds();
	if (disclaimer) {
		BButton *button = new BButton(BRect(0, 0, 1, 1), "accept",
			B_TRANSLATE("Accept"), new BMessage(P_MSG_ACCEPT));
		button->ResizeToPreferred();

		bounds = button->Bounds();
		rect.top = rect.bottom - bounds.bottom - 5.0f;
		rect.left = rect.right - bounds.right - 5.0f;
		rect.bottom = bounds.bottom;
		rect.right = bounds.right;
		button->MoveTo(rect.LeftTop());
		button->MakeDefault(true);
		fBackground->AddChild(button);

		button = new BButton(BRect(0, 0, 1, 1), "decline",
			B_TRANSLATE("Decline"),	new BMessage(P_MSG_DECLINE));
		button->ResizeToPreferred();

		bounds = button->Bounds();
		rect.left -= bounds.right + 7.0f;
		button->MoveTo(rect.LeftTop());
		fBackground->AddChild(button);
	} else {
		BButton *button = new BButton(BRect(0, 0, 1, 1), "accept",
			B_TRANSLATE("Continue"), new BMessage(P_MSG_ACCEPT));
		button->ResizeToPreferred();

		bounds = button->Bounds();
		rect.top = rect.bottom - bounds.bottom - 5.0f;
		rect.left = rect.right - bounds.right - 5.0f;
		rect.bottom = bounds.bottom;
		rect.right = bounds.right;
		button->MoveTo(rect.LeftTop());
		button->MakeDefault(true);
		fBackground->AddChild(button);
	}

	bounds = Bounds().InsetBySelf(5.0f, 5.0f);
	bounds.bottom = rect.top - 6.0f;
	bounds.right -= B_V_SCROLL_BAR_WIDTH;

	fText = new BTextView(bounds, "text_view", BRect(0, 0, bounds.Width(),
		bounds.Height()), B_FOLLOW_NONE, B_WILL_DRAW);
	fText->MakeEditable(false);
	fText->MakeSelectable(true);
	fText->SetText(text);

	BScrollView *scroll = new BScrollView("scroll_view", fText,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);

	fBackground->AddChild(scroll);

	AddChild(fBackground);
}


/*void
PackageTextViewer::_InitView(const char *text, bool disclaimer)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fText = new BTextView(BRect(0, 0, 1, 1), "text_view", BRect(0, 0, 1, 1),
			B_FOLLOW_NONE, B_WILL_DRAW | B_SUPPORTS_LAYOUT);
	fText->MakeEditable(false);
	fText->MakeSelectable(true);
	BScrollView *scroll = new BScrollView("scroll_view", fText,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);

	if (disclaimer) {
		BButton *accept = new BButton("accept", B_TRANSLATE("Accept"),
				new BMessage(P_MSG_ACCEPT));

		BButton *decline = new BButton("decline", B_TRANSLATE("Decline"),
				new BMessage(P_MSG_DECLINE));

		fBackground = BGroupLayoutBuilder(B_VERTICAL)
			.Add(scroll)
			.AddGroup(B_HORIZONTAL, 5.0f)
				.AddGlue()
				.Add(accept)
				.Add(decline)
			.End();
	}
	else {
		BButton *button = new BButton("accept", B_TRANSLATE("Continue"),
				new BMessage(P_MSG_ACCEPT));

		fBackground = BGroupLayoutBuilder(B_VERTICAL)
			.Add(scroll)
			.AddGroup(B_HORIZONTAL, 5.0f)
				.AddGlue()
				.Add(button)
			.End();
	}

	AddChild(fBackground);

	fBackground->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fText->SetText(text);
}*/

