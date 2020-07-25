/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserUsageConditionsWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>

#include "AppUtils.h"
#include "BarberPole.h"
#include "HaikuDepotConstants.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "MarkupTextView.h"
#include "Model.h"
#include "UserUsageConditions.h"
#include "ServerHelper.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserUsageConditions"

#define PLACEHOLDER_TEXT "..."

#define INTRODUCTION_TEXT_LATEST "HaikuDepot communicates with a " \
	"server component called HaikuDepotServer. These are the latest " \
	"usage conditions for use of the HaikuDepotServer service."

#define INTRODUCTION_TEXT_USER "HaikuDepot communicates with a " \
	"server component called HaikuDepotServer. These are the usage " \
	"conditions that the user '%Nickname%' agreed to at %AgreedToTimestamp% "\
	"in relation to the use of the HaikuDepotServer service."

#define KEY_USER_USAGE_CONDITIONS	"userUsageConditions"
#define KEY_USER_DETAIL				"userDetail"

/*!	This is the anticipated number of lines of test that appear in the
	introduction.
*/

#define LINES_INTRODUCTION_TEXT 2

#define WINDOW_FRAME BRect(0, 0, 500, 400)


UserUsageConditionsWindow::UserUsageConditionsWindow(Model& model,
	UserUsageConditions& userUsageConditions)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Usage conditions"),
			B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
				| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fMode(FIXED),
	fModel(model),
	fIntroductionTextView(NULL),
	fWorkerThread(-1)
{
	_InitUiControls();

	BScrollView* scrollView = new BScrollView("copy scroll view", fCopyView,
		0, false, true, B_PLAIN_BORDER);
	BButton* okButton = new BButton("ok", B_TRANSLATE("OK"),
		new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fVersionStringView, 1)
		.Add(scrollView, 97)
		.Add(fAgeNoteStringView, 1)
		.AddGroup(B_HORIZONTAL, 1)
			.AddGlue()
			.Add(okButton)
			.End()
		.End();

	CenterOnScreen();

	UserDetail userDetail;
		// invalid user detail
	_DisplayData(userDetail, userUsageConditions);
}

UserUsageConditionsWindow::UserUsageConditionsWindow(
	Model& model, UserUsageConditionsSelectionMode mode)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Usage conditions"),
			B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
				| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fMode(mode),
	fModel(model),
	fWorkerThread(-1)
{
	_InitUiControls();

	fWorkerIndicator = new BarberPole("fetch data worker indicator");
	BSize workerIndicatorSize;
	workerIndicatorSize.SetHeight(20);
	fWorkerIndicator->SetExplicitSize(workerIndicatorSize);

	fIntroductionTextView = new BTextView("introduction text view");
	fIntroductionTextView->AdoptSystemColors();
	fIntroductionTextView->MakeEditable(false);
	fIntroductionTextView->MakeSelectable(false);
	UserDetail userDetail;
	fIntroductionTextView->SetText(_IntroductionTextForMode(mode, userDetail));

	BSize introductionSize;
	introductionSize.SetHeight(
		_ExpectedIntroductionTextHeight(fIntroductionTextView));
	fIntroductionTextView->SetExplicitPreferredSize(introductionSize);

	BScrollView* scrollView = new BScrollView("copy scroll view", fCopyView,
		0, false, true, B_PLAIN_BORDER);
	BButton* okButton = new BButton("ok", B_TRANSLATE("OK"),
		new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fIntroductionTextView, 1)
		.AddGlue()
		.Add(fVersionStringView, 1)
		.Add(scrollView, 95)
		.Add(fAgeNoteStringView, 1)
		.AddGroup(B_HORIZONTAL, 1)
			.AddGlue()
			.Add(okButton)
			.End()
		.Add(fWorkerIndicator, 1)
		.End();

	CenterOnScreen();

	_FetchData();
		// start a new thread to pull down the user usage conditions data.
}


UserUsageConditionsWindow::~UserUsageConditionsWindow()
{
}


/*! This sets up the UI controls / interface elements that are not specific to
    a given mode of viewing.
*/

void
UserUsageConditionsWindow::_InitUiControls()
{
	fCopyView = new MarkupTextView("copy view");
	fCopyView->SetViewUIColor(B_NO_COLOR);
	fCopyView->SetLowColor(RGB_COLOR_WHITE);
	fCopyView->SetInsets(8.0f);

	fAgeNoteStringView = new BStringView("age note string view",
		PLACEHOLDER_TEXT);
	fAgeNoteStringView->AdoptSystemColors();
	fAgeNoteStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BFont versionFont(be_plain_font);
	versionFont.SetSize(9.0);

	fVersionStringView = new BStringView("version string view",
		PLACEHOLDER_TEXT);
	fVersionStringView->AdoptSystemColors();
	fVersionStringView->SetFont(&versionFont);
	fVersionStringView->SetAlignment(B_ALIGN_RIGHT);
	fVersionStringView->SetHighUIColor(B_PANEL_TEXT_COLOR, B_DARKEN_3_TINT);
	fVersionStringView->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


void
UserUsageConditionsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_USER_USAGE_CONDITIONS_DATA:
		{
			BMessage userDetailMessage;
			BMessage userUsageConditionsMessage;
			message->FindMessage(KEY_USER_DETAIL, &userDetailMessage);
			message->FindMessage(KEY_USER_USAGE_CONDITIONS,
				&userUsageConditionsMessage);
			UserDetail userDetail(&userDetailMessage);
			UserUsageConditions userUsageConditions(&userUsageConditionsMessage);
			_DisplayData(userDetail, userUsageConditions);
			fWorkerIndicator->Stop();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
UserUsageConditionsWindow::QuitRequested()
{
	// for now we just don't allow the quit when the background thread
	// is processing.  In the future it would be good if the HTTP
	// requests were re-organized such that cancellations were easier to
	// implement.

	if (fWorkerThread == -1)
		return true;
	HDINFO("unable to quit when the user usage "
		"conditions window is still fetching data");
	return false;
}


/*!	This method is called on the main thread in order to initiate the background
	processing to obtain the user usage conditions data.  It will take
	responsibility for coordinating the creation of the thread and starting the
	thread etc...
*/

void
UserUsageConditionsWindow::_FetchData()
{
	if (-1 != fWorkerThread)
		debugger("illegal state - attempt to fetch, but fetch in progress");
	thread_id thread = spawn_thread(&_FetchDataThreadEntry,
		"Fetch usage conditions data", B_NORMAL_PRIORITY, this);
	if (thread >= 0) {
		fWorkerIndicator->Start();
		_SetWorkerThread(thread);
		resume_thread(fWorkerThread);
	} else {
		debugger("unable to start a thread to fetch the user usage "
			"conditions.");
	}
}


/*!	This method is called from the thread in order to start the thread; it is
	the entry-point for the background processing to obtain the user usage
	conditions.
*/

/*static*/ int32
UserUsageConditionsWindow::_FetchDataThreadEntry(void* data)
{
	UserUsageConditionsWindow* win
		= reinterpret_cast<UserUsageConditionsWindow*>(data);
	win->_FetchDataPerform();
	return 0;
}


/*!	This method will perform the task of obtaining data about the user usage
	conditions.
*/

void
UserUsageConditionsWindow::_FetchDataPerform()
{
	UserDetail userDetail;
	UserUsageConditions conditions;
	WebAppInterface interface = fModel.GetWebAppInterface();
	BString code;
	status_t status = _FetchUserUsageConditionsCodePerform(userDetail, code);

	if (status == B_OK) {
		if (fMode == USER && code.IsEmpty()) {
			BString message = B_TRANSLATE(
				"The user '%Nickname%' has not agreed to any usage "
				"conditions.");
			message.ReplaceAll("%Nickname%", userDetail.Nickname());
			AppUtils::NotifySimpleError(B_TRANSLATE("No usage conditions"),
				message);
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
			status = B_BAD_DATA;
		}
	} else {
		_NotifyFetchProblem();
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	}

	if (status == B_OK) {
		if (interface.RetrieveUserUsageConditions(code, conditions) == B_OK) {
			BMessage userUsageConditionsMessage;
			BMessage userDetailMessage;
			conditions.Archive(&userUsageConditionsMessage, true);
			userDetail.Archive(&userDetailMessage, true);
			BMessage dataMessage(MSG_USER_USAGE_CONDITIONS_DATA);
			dataMessage.AddMessage(KEY_USER_USAGE_CONDITIONS,
				&userUsageConditionsMessage);
			dataMessage.AddMessage(KEY_USER_DETAIL, &userDetailMessage);
			BMessenger(this).SendMessage(&dataMessage);
		} else {
			_NotifyFetchProblem();
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
		}
	}

	_SetWorkerThread(-1);
}


status_t
UserUsageConditionsWindow::_FetchUserUsageConditionsCodePerform(
	UserDetail& userDetail, BString& code)
{
	switch (fMode) {
		case LATEST:
			code.SetTo("");
				// no code in order to get the latest
			return B_OK;
		case USER:
			return _FetchUserUsageConditionsCodeForUserPerform(
				userDetail, code);
		default:
			debugger("unhanded mode");
			return B_ERROR;
	}
}


status_t
UserUsageConditionsWindow::_FetchUserUsageConditionsCodeForUserPerform(
	UserDetail& userDetail, BString& code)
{
	WebAppInterface interface = fModel.GetWebAppInterface();

	if (interface.Nickname().IsEmpty())
		debugger("attempt to get user details for the current user, but"
			" there is no current user");

	BMessage responseEnvelopeMessage;
	status_t result = interface.RetrieveCurrentUserDetail(
		responseEnvelopeMessage);

	if (result == B_OK) {
		// could be an error or could be a valid response envelope
		// containing data.
		switch (interface.ErrorCodeFromResponse(responseEnvelopeMessage)) {
			case ERROR_CODE_NONE:
				result = WebAppInterface::UnpackUserDetail(
					responseEnvelopeMessage, userDetail);
				break;
			default:
				ServerHelper::NotifyServerJsonRpcError(responseEnvelopeMessage);
				result = B_ERROR;
					// just any old error to stop
				break;
		}
	} else {
		HDERROR("an error has arisen communicating with the"
			" server to obtain data for a user's user usage conditions"
			" [%s]", strerror(result));
		ServerHelper::NotifyTransportError(result);
	}

	if (result == B_OK) {
		BString userUsageConditionsCode = userDetail.Agreement().Code();
		HDDEBUG("the user [%s] has agreed to uuc [%s]",
			interface.Nickname().String(),
			userUsageConditionsCode.String());
		code.SetTo(userUsageConditionsCode);
	} else {
		HDDEBUG("unable to get details of the user [%s]",
			interface.Nickname().String());
	}

	return result;
}


void
UserUsageConditionsWindow::_NotifyFetchProblem()
{
	AppUtils::NotifySimpleError(
		B_TRANSLATE("Usage conditions download problem"),
		B_TRANSLATE("An error has arisen downloading the usage "
			"conditions. Check the log for details and try again. "
			ALERT_MSG_LOGS_USER_GUIDE));
}


void
UserUsageConditionsWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock())
		HDERROR("failed to lock window");
	else {
		fWorkerThread = thread;
		Unlock();
	}
}


void
UserUsageConditionsWindow::_DisplayData(
	const UserDetail& userDetail,
	const UserUsageConditions& userUsageConditions)
{
	fCopyView->SetText(userUsageConditions.CopyMarkdown());
	fAgeNoteStringView->SetText(_MinimumAgeText(
		userUsageConditions.MinimumAge()));
	fVersionStringView->SetText(_VersionText(userUsageConditions.Code()));
	if (fIntroductionTextView != NULL) {
		fIntroductionTextView->SetText(
			_IntroductionTextForMode(fMode, userDetail));
	}
}


/*static*/ const BString
UserUsageConditionsWindow::_VersionText(const BString& code)
{
	BString versionText(
		B_TRANSLATE("Version %Code%"));
	versionText.ReplaceAll("%Code%", code);
	return versionText;
}


/*static*/ const BString
UserUsageConditionsWindow::_MinimumAgeText(uint8 minimumAge)
{
	BString minimumAgeString;
	minimumAgeString.SetToFormat("%" B_PRId8, minimumAge);
	BString ageNoteText(
		B_TRANSLATE("Users are required to be %AgeYears% years of age or "
			"older."));
	ageNoteText.ReplaceAll("%AgeYears%", minimumAgeString);
	return ageNoteText;
}


/*static*/ const BString
UserUsageConditionsWindow::_IntroductionTextForMode(
	UserUsageConditionsSelectionMode mode,
	const UserDetail& userDetail)
{
	switch (mode) {
		case LATEST:
			return B_TRANSLATE(INTRODUCTION_TEXT_LATEST);
		case USER:
		{
			BString nicknamePresentation = PLACEHOLDER_TEXT;
			BString agreedToTimestampPresentation = PLACEHOLDER_TEXT;

			if (!userDetail.Nickname().IsEmpty())
				nicknamePresentation = userDetail.Nickname();

			uint64 timestampAgreed = userDetail.Agreement().TimestampAgreed();

			if (timestampAgreed > 0) {
				agreedToTimestampPresentation =
					LocaleUtils::TimestampToDateTimeString(timestampAgreed);
			}

			BString text = B_TRANSLATE(INTRODUCTION_TEXT_USER);
			text.ReplaceAll("%Nickname%", nicknamePresentation);
			text.ReplaceAll("%AgreedToTimestamp%",
				agreedToTimestampPresentation);
			return text;
		}
		default:
			return "???";
	}
}


/*static*/ float
UserUsageConditionsWindow::_ExpectedIntroductionTextHeight(
	BTextView* introductionTextView)
{
	float insetTop;
	float insetBottom;
	introductionTextView->GetInsets(NULL, &insetTop, NULL, &insetBottom);

	BSize introductionSize;
	font_height fh;
	be_plain_font->GetHeight(&fh);
	return ((fh.ascent + fh.descent + fh.leading) * LINES_INTRODUCTION_TEXT)
		+ insetTop + insetBottom;
}
