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
#include <StringView.h>

#include "MarkupParser.h"
#include "RatingView.h"
#include "TextDocumentView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RatePackageWindow"


enum {
	MSG_SEND				= 'send',
	MSG_PACKAGE_RATED		= 'rpkg',
	MSG_STABILITY_SELECTED	= 'stbl',
	MSG_LANGUAGE_SELECTED	= 'lngs'
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


class SetRatingView : public RatingView {
public:
	SetRatingView()
		:
		RatingView("rate package view"),
		fPermanentRating(0.0f)
	{
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		SetRating(fPermanentRating);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (dragMessage != NULL)
			return;
	
		if ((transit != B_INSIDE_VIEW && transit != B_ENTERED_VIEW)
			|| where.x > MinSize().width) {
			SetRating(fPermanentRating);
			return;
		}
	
		float hoverRating = _RatingForMousePos(where);
		SetRating(hoverRating);
	}

	virtual void MouseDown(BPoint where)
	{
		fPermanentRating = _RatingForMousePos(where);
		BMessage message(MSG_PACKAGE_RATED);
		message.AddFloat("rating", fPermanentRating);
		Window()->PostMessage(&message, Window());
	}

private:
	float _RatingForMousePos(BPoint where)
	{
		return std::min(5.0f, ceilf(5.0f * where.x / MinSize().width));
	}

	float		fPermanentRating;
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


static void
add_languages_to_menu(const StringList& languages, BMenu* menu)
{
	for (int i = 0; i < languages.CountItems(); i++) {
		const BString& language = languages.ItemAtFast(i);
		BMessage* message = new BMessage(MSG_LANGUAGE_SELECTED);
		message->AddString("code", language);
		BMenuItem* item = new BMenuItem(language, message);
		menu->AddItem(item);
	}
}


RatePackageWindow::RatePackageWindow(BWindow* parent, BRect frame,
	const BString& preferredLanguage, const StringList& supportedLanguages)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Your rating"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fRatingText(),
	fRating(-1.0f),
	fCommentLanguage(preferredLanguage)
{
	AddToSubset(parent);

	BStringView* ratingLabel = new BStringView("rating label",
		B_TRANSLATE("Your rating:"));

	SetRatingView* setRatingView = new SetRatingView();	

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

	// Construct languages popup
	BPopUpMenu* languagesMenu = new BPopUpMenu(B_TRANSLATE("Language"));
	BMenuField* languageField = new BMenuField("language",
		B_TRANSLATE("Comment language:"), languagesMenu);

	add_languages_to_menu(supportedLanguages, languagesMenu);
	languagesMenu->SetTargetForItems(this);

	BMenuItem* defaultItem = languagesMenu->ItemAt(
		supportedLanguages.IndexOf(fCommentLanguage));
	if (defaultItem != NULL)
		defaultItem->SetMarked(true);
	
	// Construct buttons
	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	BButton* sendButton = new BButton("send", B_TRANSLATE("Send"),
		new BMessage(MSG_SEND));

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGrid()
			.Add(ratingLabel, 0, 0)
			.Add(setRatingView, 1, 0)
			.AddMenuField(stabilityRatingField, 0, 1)
			.AddMenuField(languageField, 0, 2)
		.End()
		.Add(textScrollView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(sendButton)
		.End()
		.SetInsets(B_USE_DEFAULT_SPACING)
	;

	// NOTE: Do not make Send the default button. The user might want
	// to type line-breaks instead of sending when hitting RETURN.

	CenterIn(parent->Frame());
}


RatePackageWindow::~RatePackageWindow()
{
}


void
RatePackageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PACKAGE_RATED:
			message->FindFloat("rating", &fRating);
			break;

		case MSG_STABILITY_SELECTED:
			message->FindString("name", &fStability);
			break;

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
