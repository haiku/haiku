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
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>

#include "MarkupParser.h"
#include "TextDocumentView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RatePackageWindow"


enum {
	MSG_SEND				= 'send',
	MSG_STABILITY_SELECTED	= 'stbl'
};

//! Layouts the scrollbar so it looks nice with no border and the document
// window look.
class ScrollView : public BScrollView {
public:
	ScrollView(const char* name, BView* target)
		:
		BScrollView(name, target, 0, false, true, B_FANCY_BORDER)
	{
	}

	virtual void DoLayout()
	{
		BRect innerFrame = Bounds();
		innerFrame.InsetBy(2, 2);

		BScrollBar* vScrollBar = ScrollBar(B_VERTICAL);
		BScrollBar* hScrollBar = ScrollBar(B_HORIZONTAL);

		if (vScrollBar != NULL)
			innerFrame.right -= vScrollBar->Bounds().Width() - 1;
		if (hScrollBar != NULL)
			innerFrame.bottom -= hScrollBar->Bounds().Height() - 1;

		BView* target = Target();
		if (target != NULL) {
			Target()->MoveTo(innerFrame.left, innerFrame.top);
			Target()->ResizeTo(innerFrame.Width(), innerFrame.Height());
		}

		if (vScrollBar != NULL) {
			BRect rect = innerFrame;
			rect.left = rect.right + 1;
			rect.right = rect.left + vScrollBar->Bounds().Width();
			rect.top -= 1;
			rect.bottom += 1;

			vScrollBar->MoveTo(rect.left, rect.top);
			vScrollBar->ResizeTo(rect.Width(), rect.Height());
		}

		if (hScrollBar != NULL) {
			BRect rect = innerFrame;
			rect.top = rect.bottom + 1;
			rect.bottom = rect.top + hScrollBar->Bounds().Height();
			rect.left -= 1;
			rect.right += 1;

			hScrollBar->MoveTo(rect.left, rect.top);
			hScrollBar->ResizeTo(rect.Width(), rect.Height());
		}
	}
};


static void
add_stabilities_to_menu(const StabilityRatingList& stabilities, BMenu* menu)
{
	for (int i = 0; i < stabilities.CountItems(); i++) {
		const StabilityRating& stability = stabilities.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_STABILITY_SELECTED);
		message->AddString("name", stability.Name());
		BMenuItem* item = new BMenuItem(stability.Label(), message);
		menu->AddItem(item);
	}
}


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
	ScrollView* textScrollView = new ScrollView(
		"rating scroll view", textView);

	MarkupParser parser;
	fRatingText = parser.CreateDocumentFromMarkup(
		"Here is where you ''could'' type your awesome rating comment, "
		"if only this were already implemented.");

	textView->SetInsets(10.0f);
	textView->SetTextDocument(fRatingText);
	textView->SetTextEditor(TextEditorRef(new TextEditor(), true));

	// Construct stability rating popup
	// Construct repository popup
	BPopUpMenu* stabilityMenu = new BPopUpMenu(B_TRANSLATE("Stability"));
	BMenuField* stabilityRatingField = new BMenuField("stability",
		B_TRANSLATE("Stability:"), stabilityMenu);
	
	StabilityRatingList stabilities;
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Not specified"), "UNSPECIFIED"));
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Mostly stable"), "MOSTLY_STABLE"));
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Stable"), "STABLE"));
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Not stable, but usable"), "USABLE"));
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Unstable"), "UNSTABLE"));
	stabilities.Add(StabilityRating(
		B_TRANSLATE("Does not start"), "DOES_NOT_START"));
	
	add_stabilities_to_menu(stabilities, stabilityMenu);
	stabilityMenu->SetTargetForItems(this);
	
	fStability = stabilities.ItemAt(0).Name();
	stabilityMenu->ItemAt(0)->SetMarked(true);

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	BButton* sendButton = new BButton("send", B_TRANSLATE("Send"),
		new BMessage(MSG_SEND));

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(textScrollView)
		.Add(stabilityRatingField)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(sendButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING)
	;

	// NOTE: Do not make Send the default button. The user might want
	// to type line-breaks instead of sending when hitting RETURN.
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
		case MSG_STABILITY_SELECTED:
			message->FindString("name", &fStability);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
RatePackageWindow::SetPackage(const PackageInfoRef& package)
{
	fPackage = package;
	// TODO: See if the user already made a rating for this package,
	// pre-fill the UI with that rating. When sending the rating, replace
	// the old one.
}


void
RatePackageWindow::_SendRating()
{
	// TODO: Implement...
	BAlert* alert = new BAlert(B_TRANSLATE("Not implemented"),
		B_TRANSLATE("Sorry, while the web application would already support "
		"storing your rating, HaikuDepot was not yet updated to use "
		"this functionality."),
		B_TRANSLATE("Thanks for telling me after I typed all this!"));
	alert->Go(NULL);

	PostMessage(B_QUIT_REQUESTED);
}
