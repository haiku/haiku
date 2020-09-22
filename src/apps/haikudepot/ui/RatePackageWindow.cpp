/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RatePackageWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <StringView.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "LanguageMenuUtils.h"
#include "Logger.h"
#include "MarkupParser.h"
#include "RatingView.h"
#include "ServerHelper.h"
#include "TextDocumentView.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RatePackageWindow"


enum {
	MSG_SEND						= 'send',
	MSG_PACKAGE_RATED				= 'rpkg',
	MSG_STABILITY_SELECTED			= 'stbl',
	MSG_RATING_ACTIVE_CHANGED		= 'rtac',
	MSG_RATING_DETERMINATE_CHANGED	= 'rdch'
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
		fPermanentRating(0.0f),
		fRatingDeterminate(true)
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

/*! By setting this to false, this indicates that there is no rating for the
    set; ie NULL.  The indeterminate rating is indicated by a pale grey
    colored star.
*/

	void SetRatingDeterminate(bool value) {
		fRatingDeterminate = value;
		Invalidate();
	}

protected:
	virtual const BBitmap* StarBitmap()
	{
		if (fRatingDeterminate)
			return fStarBlueBitmap->Bitmap(BITMAP_SIZE_16);
		return fStarGrayBitmap->Bitmap(BITMAP_SIZE_16);
	}

private:
	float _RatingForMousePos(BPoint where)
	{
		return std::min(5.0f, ceilf(5.0f * where.x / MinSize().width));
	}

	float		fPermanentRating;
	bool		fRatingDeterminate;
};


RatePackageWindow::RatePackageWindow(BWindow* parent, BRect frame,
	Model& model)
	:
	BWindow(frame, B_TRANSLATE("Rate package"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fModel(model),
	fRatingText(),
	fTextEditor(new TextEditor(), true),
	fRating(RATING_NONE),
	fRatingDeterminate(false),
	fCommentLanguageCode(LANGUAGE_DEFAULT_CODE),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	BStringView* ratingLabel = new BStringView("rating label",
		B_TRANSLATE("Your rating:"));

	fSetRatingView = new SetRatingView();
	fSetRatingView->SetRatingDeterminate(false);
	fRatingDeterminateCheckBox = new BCheckBox("has rating", NULL,
		new BMessage(MSG_RATING_DETERMINATE_CHANGED));
	fRatingDeterminateCheckBox->SetValue(B_CONTROL_OFF);

	fTextView = new TextDocumentView();
	ScrollView* textScrollView = new ScrollView(
		"rating scroll view", fTextView);

	// Get a TextDocument with default paragraph and character style
	MarkupParser parser;
	fRatingText = parser.CreateDocumentFromMarkup("");

	fTextView->SetInsets(10.0f);
	fTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fTextView->SetTextDocument(fRatingText);
	fTextView->SetTextEditor(fTextEditor);

	// Construct stability rating popup
	BPopUpMenu* stabilityMenu = new BPopUpMenu(B_TRANSLATE("Stability"));
	fStabilityField = new BMenuField("stability",
		B_TRANSLATE("Stability:"), stabilityMenu);
	_InitStabilitiesMenu(stabilityMenu);

	// Construct languages popup
	BPopUpMenu* languagesMenu = new BPopUpMenu(B_TRANSLATE("Language"));
	fCommentLanguageField = new BMenuField("language",
		B_TRANSLATE("Comment language:"), languagesMenu);
	_InitLanguagesMenu(languagesMenu);

	fRatingActiveCheckBox = new BCheckBox("rating active",
		B_TRANSLATE("This rating is visible to other users"),
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
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 1, 0)
				.Add(fRatingDeterminateCheckBox)
				.Add(fSetRatingView)
			.End()
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
RatePackageWindow::_InitLanguagesMenu(BPopUpMenu* menu)
{
	AutoLocker<BLocker> locker(fModel.Lock());
	fCommentLanguageCode = fModel.Language()->PreferredLanguage()->Code();

	LanguageMenuUtils::AddLanguagesToMenu(fModel.Language(), menu);
	menu->SetTargetForItems(this);
	LanguageMenuUtils::MarkLanguageInMenu(fCommentLanguageCode, menu);
}


void
RatePackageWindow::_InitStabilitiesMenu(BPopUpMenu* menu)
{
	AutoLocker<BLocker> locker(fModel.Lock());
	int32 countStabilities = fModel.CountRatingStabilities();

	menu->SetTargetForItems(this);

	if (0 == countStabilities) {
		menu->SetEnabled(false);
		return;
	}

	for (int32 i = 0; i < countStabilities; i++) {
		const RatingStabilityRef stability = fModel.RatingStabilityAtIndex(i);
		BMessage* message = new BMessage(MSG_STABILITY_SELECTED);
		message->AddString("code", stability->Code());
		BMenuItem* item = new BMenuItem(stability->Name(), message);
		menu->AddItem(item);

		if (i == 0) {
			fStabilityCode = stability->Code();
			item->SetMarked(true);
		}
	}
}


void
RatePackageWindow::DispatchMessage(BMessage* message, BHandler *handler)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
			// if the user presses escape, close the window.
		if ((message->FindInt8("byte", &key) == B_OK)
			&& key == B_ESCAPE) {
			Quit();
			return;
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
RatePackageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PACKAGE_RATED:
			message->FindFloat("rating", &fRating);
			fRatingDeterminate = true;
			fSetRatingView->SetRatingDeterminate(true);
			fRatingDeterminateCheckBox->SetValue(B_CONTROL_ON);
			break;

		case MSG_STABILITY_SELECTED:
			message->FindString("code", &fStabilityCode);
			break;

		case MSG_LANGUAGE_SELECTED:
			message->FindString("code", &fCommentLanguageCode);
			break;

		case MSG_RATING_DETERMINATE_CHANGED:
			fRatingDeterminate = fRatingDeterminateCheckBox->Value()
				== B_CONTROL_ON;
			fSetRatingView->SetRatingDeterminate(fRatingDeterminate);
			break;

		case MSG_RATING_ACTIVE_CHANGED:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK)
				fRatingActive = value == B_CONTROL_ON;
			break;
		}

		case MSG_DID_ADD_USER_RATING:
		{
			BAlert* alert = new(std::nothrow) BAlert(
				B_TRANSLATE("User rating"),
				B_TRANSLATE("Your rating was uploaded successfully. "
					"You can update or remove it at the HaikuDepot Server "
					"website."),
				B_TRANSLATE("Close"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
			_RefreshPackageData();
			break;
		}

		case MSG_DID_UPDATE_USER_RATING:
		{
			BAlert* alert = new(std::nothrow) BAlert(
				B_TRANSLATE("User rating"),
				B_TRANSLATE("Your rating was updated."),
				B_TRANSLATE("Close"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
			_RefreshPackageData();
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

/*! Refresh the data shown about the current page.  This may be useful, for
    example when somebody adds a rating and that changes the rating of the
    package or they add a rating and want to see that immediately.  The logic
    should round-trip to the server so that actual data is shown.
*/

void
RatePackageWindow::_RefreshPackageData()
{
	BMessage message(MSG_SERVER_DATA_CHANGED);
	message.AddString("name", fPackage->Name());
	be_app->PostMessage(&message);
}


void
RatePackageWindow::SetPackage(const PackageInfoRef& package)
{
	BAutolock locker(this);
	if (!locker.IsLocked() || fWorkerThread >= 0)
		return;

	fPackage = package;

	BString windowTitle(B_TRANSLATE("Rate %Package%"));
	windowTitle.ReplaceAll("%Package%", package->Title());
	SetTitle(windowTitle);

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
	thread_id thread = spawn_thread(&_SendRatingThreadEntry,
		"Send rating", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		_SetWorkerThread(thread);
}


void
RatePackageWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock())
		return;

	bool enabled = thread < 0;

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


/*static*/ int32
RatePackageWindow::_QueryRatingThreadEntry(void* data)
{
	RatePackageWindow* window = reinterpret_cast<RatePackageWindow*>(data);
	window->_QueryRatingThread();
	return 0;
}


/*! A server request has been made to the server and the server has responded
    with some data.  The data is known not to be an error and now the data can
    be extracted into the user interface elements.
*/

void
RatePackageWindow::_RelayServerDataToUI(BMessage& response)
{
	if (Lock()) {
		response.FindString("code", &fRatingID);
		response.FindBool("active", &fRatingActive);
		BString comment;
		if (response.FindString("comment", &comment) == B_OK) {
			MarkupParser parser;
			fRatingText = parser.CreateDocumentFromMarkup(comment);
			fTextView->SetTextDocument(fRatingText);
		}
		if (response.FindString("userRatingStabilityCode",
				&fStabilityCode) == B_OK) {
			BMenu* menu = fStabilityField->Menu();
			AppUtils::MarkItemWithCodeInMenu(fStabilityCode, menu);
		}
		if (response.FindString("naturalLanguageCode",
			&fCommentLanguageCode) == B_OK) {
			LanguageMenuUtils::MarkLanguageInMenu(
				fCommentLanguageCode, fCommentLanguageField->Menu());
		}
		double rating;
		if (response.FindDouble("rating", &rating) == B_OK) {
			fRating = (float)rating;
			fRatingDeterminate = fRating >= 0.0f;
			fSetRatingView->SetPermanentRating(fRating);
		} else {
			fRatingDeterminate = false;
		}

		fSetRatingView->SetRatingDeterminate(fRatingDeterminate);
		fRatingDeterminateCheckBox->SetValue(
			fRatingDeterminate ? B_CONTROL_ON : B_CONTROL_OFF);
		fRatingActiveCheckBox->SetValue(fRatingActive);
		fRatingActiveCheckBox->Show();

		fSendButton->SetLabel(B_TRANSLATE("Update"));

		Unlock();
	} else
		HDERROR("unable to acquire lock to update the ui");
}


void
RatePackageWindow::_QueryRatingThread()
{
	if (!Lock()) {
		HDERROR("rating query: Failed to lock window");
		return;
	}

	PackageInfoRef package(fPackage);

	Unlock();

	BAutolock locker(fModel.Lock());
	BString nickname = fModel.Nickname();
	locker.Unlock();

	if (package.Get() == NULL) {
		HDERROR("rating query: No package");
		_SetWorkerThread(-1);
		return;
	}

	WebAppInterface interface;
	BMessage info;
	const DepotInfo* depot = fModel.DepotForName(package->DepotName());
	BString repositoryCode;

	if (depot != NULL)
		repositoryCode = depot->WebAppRepositoryCode();

	if (repositoryCode.IsEmpty()) {
		HDERROR("unable to obtain the repository code for depot; %s",
			package->DepotName().String());
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	} else {
		status_t status = interface
			.RetreiveUserRatingForPackageAndVersionByUser(package->Name(),
				package->Version(), package->Architecture(), repositoryCode,
				nickname, info);

		if (status == B_OK) {
				// could be an error or could be a valid response envelope
				// containing data.
			switch (interface.ErrorCodeFromResponse(info)) {
				case ERROR_CODE_NONE:
				{
					//info.PrintToStream();
					BMessage result;
					if (info.FindMessage("result", &result) == B_OK) {
						_RelayServerDataToUI(result);
					} else {
						HDERROR("bad response envelope missing 'result' entry");
						ServerHelper::NotifyTransportError(B_BAD_VALUE);
						BMessenger(this).SendMessage(B_QUIT_REQUESTED);
					}
					break;
				}
				case ERROR_CODE_OBJECTNOTFOUND:
						// an expected response
					HDINFO("there was no previous rating for this"
						" user on this version of this package so a new rating"
						" will be added.");
					break;
				default:
					ServerHelper::NotifyServerJsonRpcError(info);
					BMessenger(this).SendMessage(B_QUIT_REQUESTED);
					break;
			}
		} else {
			HDERROR("an error has arisen communicating with the"
				" server to obtain data for an existing rating [%s]",
				strerror(status));
			ServerHelper::NotifyTransportError(status);
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
		}
	}

	_SetWorkerThread(-1);
}


int32
RatePackageWindow::_SendRatingThreadEntry(void* data)
{
	RatePackageWindow* window = reinterpret_cast<RatePackageWindow*>(data);
	window->_SendRatingThread();
	return 0;
}


void
RatePackageWindow::_SendRatingThread()
{
	if (!Lock()) {
		HDERROR("upload rating: Failed to lock window");
		return;
	}

	BMessenger messenger = BMessenger(this);
	BString package = fPackage->Name();
	BString architecture = fPackage->Architecture();
	BString repositoryCode;
	int rating = (int)fRating;
	BString stability = fStabilityCode;
	BString comment = fRatingText->Text();
	BString languageCode = fCommentLanguageCode;
	BString ratingID = fRatingID;
	bool active = fRatingActive;

	if (!fRatingDeterminate)
		rating = RATING_NONE;

	const DepotInfo* depot = fModel.DepotForName(fPackage->DepotName());

	if (depot != NULL)
		repositoryCode = depot->WebAppRepositoryCode();

	WebAppInterface interface = fModel.GetWebAppInterface();

	Unlock();

	if (repositoryCode.Length() == 0) {
		HDERROR("unable to find the web app repository code for the local "
			"depot %s",
			fPackage->DepotName().String());
		return;
	}

	if (stability == "unspecified")
		stability = "";

	status_t status;
	BMessage info;
	if (ratingID.Length() > 0) {
		HDINFO("will update the existing user rating [%s]", ratingID.String());
		status = interface.UpdateUserRating(ratingID,
			languageCode, comment, stability, rating, active, info);
	} else {
		HDINFO("will create a new user rating for pkg [%s]", package.String());
		status = interface.CreateUserRating(package, fPackage->Version(),
			architecture, repositoryCode, languageCode, comment, stability,
			rating, info);
	}

	if (status == B_OK) {
			// could be an error or could be a valid response envelope
			// containing data.
		switch (interface.ErrorCodeFromResponse(info)) {
			case ERROR_CODE_NONE:
			{
				if (ratingID.Length() > 0)
					messenger.SendMessage(MSG_DID_UPDATE_USER_RATING);
				else
					messenger.SendMessage(MSG_DID_ADD_USER_RATING);
				break;
			}
			default:
				ServerHelper::NotifyServerJsonRpcError(info);
				break;
		}
	} else {
		HDERROR("an error has arisen communicating with the"
			" server to obtain data for an existing rating [%s]",
			strerror(status));
		ServerHelper::NotifyTransportError(status);
	}

	messenger.SendMessage(B_QUIT_REQUESTED);
	_SetWorkerThread(-1);
}
