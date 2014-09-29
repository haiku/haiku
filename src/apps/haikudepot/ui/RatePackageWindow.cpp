/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RatePackageWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>

#include "MarkupParser.h"
#include "RatingView.h"
#include "TextDocumentView.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RatePackageWindow"


enum {
	MSG_SEND					= 'send',
	MSG_PACKAGE_RATED			= 'rpkg',
	MSG_STABILITY_SELECTED		= 'stbl',
	MSG_LANGUAGE_SELECTED		= 'lngs',
	MSG_RATING_ACTIVE_CHANGED	= 'rtac'
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
		SetPermanentRating(_RatingForMousePos(where));
		BMessage message(MSG_PACKAGE_RATED);
		message.AddFloat("rating", fPermanentRating);
		Window()->PostMessage(&message, Window());
	}
	
	void SetPermanentRating(float rating)
	{
		fPermanentRating = rating;
		SetRating(rating);
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
	Model& model)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Your rating"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fModel(model),
	fRatingText(),
	fTextEditor(new TextEditor(), true),
	fRating(-1.0f),
	fCommentLanguage(fModel.PreferredLanguage()),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	BStringView* ratingLabel = new BStringView("rating label",
		B_TRANSLATE("Your rating:"));

	fSetRatingView = new SetRatingView();	

	fTextView = new TextDocumentView();
	ScrollView* textScrollView = new ScrollView(
		"rating scroll view", fTextView);

	MarkupParser parser;
	fRatingText = parser.CreateDocumentFromMarkup(
		"Here is where you ''could'' type your awesome rating comment, "
		"if only this were already implemented.");

	fTextView->SetInsets(10.0f);
	fTextView->SetTextDocument(fRatingText);
	fTextView->SetTextEditor(fTextEditor);

	// Construct stability rating popup
	BPopUpMenu* stabilityMenu = new BPopUpMenu(B_TRANSLATE("Stability"));
	fStabilityField = new BMenuField("stability",
		B_TRANSLATE("Stability:"), stabilityMenu);

	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Not specified"), "unspecified"));
	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Stable"), "stable"));
	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Mostly stable"), "mostlystable"));
	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Unstable but usable"), "unstablebutusable"));
	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Very unstable"), "veryunstable"));
	fStabilityCodes.Add(StabilityRating(
		B_TRANSLATE("Does not start"), "nostart"));
	
	add_stabilities_to_menu(fStabilityCodes, stabilityMenu);
	stabilityMenu->SetTargetForItems(this);
	
	fStability = fStabilityCodes.ItemAt(0).Name();
	stabilityMenu->ItemAt(0)->SetMarked(true);

	// Construct languages popup
	BPopUpMenu* languagesMenu = new BPopUpMenu(B_TRANSLATE("Language"));
	fCommentLanguageField = new BMenuField("language",
		B_TRANSLATE("Comment language:"), languagesMenu);

	add_languages_to_menu(fModel.SupportedLanguages(), languagesMenu);
	languagesMenu->SetTargetForItems(this);

	BMenuItem* defaultItem = languagesMenu->ItemAt(
		fModel.SupportedLanguages().IndexOf(fCommentLanguage));
	if (defaultItem != NULL)
		defaultItem->SetMarked(true);
	
	fRatingActiveCheckBox = new BCheckBox("rating active",
		B_TRANSLATE("Other users can see this rating"),
		new BMessage(MSG_RATING_ACTIVE_CHANGED));
	// Hide the check mark by default, it will be made visible when
	// the user already made a rating and it is loaded
	fRatingActiveCheckBox->Hide();
	
	// Construct buttons
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	fSendButton = new BButton("send", B_TRANSLATE("Send"),
		new BMessage(MSG_SEND));

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGrid()
			.Add(ratingLabel, 0, 0)
			.Add(fSetRatingView, 1, 0)
			.AddMenuField(fStabilityField, 0, 1)
			.AddMenuField(fCommentLanguageField, 0, 2)
		.End()
		.Add(textScrollView)
		.AddGroup(B_HORIZONTAL)
			.Add(fRatingActiveCheckBox)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fSendButton)
		.End()
		.SetInsets(B_USE_WINDOW_INSETS)
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

		case MSG_LANGUAGE_SELECTED:
			message->FindString("code", &fCommentLanguage);
			break;
			
		case MSG_RATING_ACTIVE_CHANGED:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK)
				fRatingActive = value == B_CONTROL_ON;
			break;
		}

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
	BAutolock locker(this);
	if (!locker.IsLocked() || fWorkerThread >= 0)
		return;

	fPackage = package;

	// See if the user already made a rating for this package,
	// pre-fill the UI with that rating. (When sending the rating, the
	// old one will be replaced.)
	thread_id thread = spawn_thread(&_QueryRatingThreadEntry,
		"Query rating", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		_SetWorkerThread(thread);
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


void
RatePackageWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock())
		return;
	
	bool enabled = thread < 0;

//	fTextEditor->SetEnabled(enabled);
//	fSetRatingView->SetEnabled(enabled);
	fStabilityField->SetEnabled(enabled);
	fCommentLanguageField->SetEnabled(enabled);
	fSendButton->SetEnabled(enabled);
	
	if (thread >= 0) {
		fWorkerThread = thread;
		resume_thread(fWorkerThread);
	} else {
		fWorkerThread = -1;
	}

	Unlock();
}


int32
RatePackageWindow::_QueryRatingThreadEntry(void* data)
{
	RatePackageWindow* window = reinterpret_cast<RatePackageWindow*>(data);
	window->_QueryRatingThread();
	return 0;
}


void
RatePackageWindow::_QueryRatingThread()
{
	if (!Lock())
		return;

	PackageInfoRef package(fPackage);

	Unlock();

	BAutolock locker(fModel.Lock());
	BString username = fModel.Username();
	locker.Unlock();

	if (package.Get() == NULL) {
		_SetWorkerThread(-1);
		return;
	}

	WebAppInterface interface;
	BMessage info;

	status_t status = interface.RetrieveUserRating(
		package->Title(), package->Version(), package->Architecture(),
		username, info);

//	info.PrintToStream();

	BMessage result;
	if (status == B_OK && info.FindMessage("result", &result) == B_OK
		&& Lock()) {
		
		result.FindString("code", &fRatingID);
		result.FindBool("active", &fRatingActive);
		BString comment;
		if (result.FindString("comment", &comment) == B_OK) {
			MarkupParser parser;
			fRatingText = parser.CreateDocumentFromMarkup(comment);
			fTextView->SetTextDocument(fRatingText);
		}
		if (result.FindString("userRatingStabilityCode",
			&fStability) == B_OK) {
			int32 index = -1;
			for (int32 i = fStabilityCodes.CountItems() - 1; i >= 0; i--) {
				const StabilityRating& stability
					= fStabilityCodes.ItemAtFast(i);
				if (stability.Name() == fStability) {
					index = i;
					break;
				}
			}
			BMenuItem* item = fStabilityField->Menu()->ItemAt(index);
			if (item != NULL)
				item->SetMarked(true);
		}
		if (result.FindString("naturalLanguageCode",
			&fCommentLanguage) == B_OK) {
			BMenuItem* item = fCommentLanguageField->Menu()->ItemAt(
				fModel.SupportedLanguages().IndexOf(fCommentLanguage));
			if (item != NULL)
				item->SetMarked(true);
		}
		double rating;
		if (result.FindDouble("rating", &rating) == B_OK) {
			fRating = (float)rating;
			fSetRatingView->SetPermanentRating(fRating);
		}
		
		fRatingActiveCheckBox->SetValue(fRatingActive);
		fRatingActiveCheckBox->Show();

		Unlock();
	}

	_SetWorkerThread(-1);
}


