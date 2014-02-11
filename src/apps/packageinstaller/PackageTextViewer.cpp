/*
 * Copyright (c) 2007-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "PackageTextViewer.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <ScrollView.h>


enum {
	P_MSG_ACCEPT = 'pmac',
	P_MSG_DECLINE
};

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageTextViewer"


PackageTextViewer::PackageTextViewer(const char *text, bool disclaimer)
	:
	BlockingWindow(BRect(125, 125, 675, 475), B_TRANSLATE("Disclaimer"),
		B_AUTO_UPDATE_SIZE_LIMITS)
{
	_InitView(text, disclaimer);
	CenterOnScreen();
}


void
PackageTextViewer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case P_MSG_ACCEPT:
			ReleaseSem(1);
			break;

		case P_MSG_DECLINE:
			ReleaseSem(0);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
PackageTextViewer::_InitView(const char* text, bool disclaimer)
{
	BTextView* textView = new BTextView("text_view");
	textView->MakeEditable(false);
	textView->MakeSelectable(true);
	float margin = ceilf(be_plain_font->Size());
	textView->SetInsets(margin, margin, margin, margin);
	BScrollView* scrollView = new BScrollView("scroll_view", textView, 0, false,
		true);

	BButton* defaultButton;

	if (disclaimer) {
		defaultButton = new BButton("accept", B_TRANSLATE("Accept"),
			new BMessage(P_MSG_ACCEPT));

		BButton* decline = new BButton("decline", B_TRANSLATE("Decline"),
			new BMessage(P_MSG_DECLINE));

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.Add(scrollView)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(defaultButton)
				.Add(decline)
			.End()
			.SetInsets(B_USE_WINDOW_INSETS)
		;
	} else {
		defaultButton = new BButton("accept", B_TRANSLATE("Continue"),
			new BMessage(P_MSG_ACCEPT));

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.Add(scrollView)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(defaultButton)
			.End()
			.SetInsets(B_USE_WINDOW_INSETS)
		;
	}

	defaultButton->MakeDefault(true);

	textView->SetText(text);
}

