/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RatePackageWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>

#include "MarkupParser.h"
#include "TextDocumentView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RatePackageWindow"


enum {
	MSG_SEND			= 'send'
};


RatePackageWindow::RatePackageWindow(BWindow* parent, BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Your rating"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fRatingText()
{
	AddToSubset(parent);
	CenterIn(parent->Frame());

	TextDocumentView* textView = new TextDocumentView();
	BScrollView* textScrollView = new BScrollView("rating scroll view",
		textView);

	MarkupParser parser;
	fRatingText = parser.CreateDocumentFromMarkup(
		"Here is where you ''could'' type your awesome rating comment, "
		"if only this were already implemented.");

	textView->SetInsets(10.0f);
	textView->SetTextDocument(fRatingText);
	textView->SetTextEditor(TextEditorRef(new TextEditor(), true));

	fSendButton = new BButton("send", B_TRANSLATE("Send"),
		new BMessage(MSG_SEND));

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(textScrollView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fSendButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING)
	;
}


RatePackageWindow::~RatePackageWindow()
{
}


void
RatePackageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SEND:
			_SendRating();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
RatePackageWindow::SetPackage(const PackageInfoRef& package)
{
	// TODO: Just remember which package the rating is for.
}


void
RatePackageWindow::_SendRating()
{
	// TODO: Implement...
	BAlert* alert = new BAlert("Not implemented",
		"Sorry, the web application is not yet finished and "
		"this functionality is not implemented.",
		"Thanks for telling me after I typed all this!");
	alert->Go(NULL);

	PostMessage(B_QUIT_REQUESTED);
}
