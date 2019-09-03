/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
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
#include "Logger.h"
#include "MarkupTextView.h"
#include "Model.h"
#include "UserUsageConditions.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserUsageConditions"

#define PLACEHOLDER_TEXT "..."

#define INTRODUCTION_TEXT_LATEST "HaikuDepot communicates with a " \
	"sever component called HaikuDepotServer.  These are the latest user " \
	"usage conditions for use of the HaikuDepotServer service."

#define INTRODUCTION_TEXT_USER "HaikuDepot communicates with a " \
	"sever component called HaikuDepotServer.  These are the user usage " \
	"conditions that the user has agreed to in relation to the use of the " \
	"HaikuDepotServer service."

/*!	This is the anticipated number of lines of test that appear in the
	introduction.
*/

#define LINES_INTRODUCTION_TEXT 2

#define WINDOW_FRAME BRect(0, 0, 500, 400)


UserUsageConditionsWindow::UserUsageConditionsWindow(Model& model,
	UserUsageConditions& userUsageConditions)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("User Usage Conditions"),
			B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
				| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fMode(FIXED),
	fModel(model),
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
	_DisplayData(userUsageConditions);
}

UserUsageConditionsWindow::UserUsageConditionsWindow(
	Model& model, UserUsageConditionsSelectionMode mode)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("User Usage Conditions"),
			B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
				| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fMode(mode),
	fModel(model),
	fWorkerThread(-1)
{
	if (mode != LATEST)
		debugger("only the LATEST user usage conditions are handled for now");

	_InitUiControls();

	fWorkerIndicator = new BarberPole("fetch data worker indicator");
	BSize workerIndicatorSize;
	workerIndicatorSize.SetHeight(20);
	fWorkerIndicator->SetExplicitMinSize(workerIndicatorSize);

	BTextView* introductionTextView = new BTextView("introduction text view");
	introductionTextView->AdoptSystemColors();
	introductionTextView->MakeEditable(false);
	introductionTextView->MakeSelectable(false);
	introductionTextView->SetText(B_TRANSLATE(_IntroductionTextForMode(mode)));

	BSize introductionSize;
	introductionSize.SetHeight(
		_ExpectedIntroductionTextHeight(introductionTextView));
	introductionTextView->SetExplicitPreferredSize(introductionSize);

	BScrollView* scrollView = new BScrollView("copy scroll view", fCopyView,
		0, false, true, B_PLAIN_BORDER);
	BButton* okButton = new BButton("ok", B_TRANSLATE("OK"),
		new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(introductionTextView, 1)
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
			UserUsageConditions data(message);
			_DisplayData(data);
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
	if (Logger::IsInfoEnabled()) {
		fprintf(stderr, "unable to quit when the user usage "
			"conditions window is still fetching data\n");
	}
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
		"Fetch user usage conditions data", B_NORMAL_PRIORITY, this);
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
	UserUsageConditions conditions;
	WebAppInterface interface = fModel.GetWebAppInterface();

	if (interface.RetrieveUserUsageConditions(NULL, conditions) == B_OK) {
		BMessage dataMessage(MSG_USER_USAGE_CONDITIONS_DATA);
		conditions.Archive(&dataMessage, true);
		BMessenger(this).SendMessage(&dataMessage);
	} else {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("User Usage Conditions Download Problem"),
			B_TRANSLATE("An error has arisen downloading the user usage "
				"conditions.  Check the log for details and try again."));
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	}

	_SetWorkerThread(-1);
}


void
UserUsageConditionsWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock()) {
		if (Logger::IsInfoEnabled())
			fprintf(stderr, "failed to lock window\n");
	} else {
		fWorkerThread = thread;
		Unlock();
	}
}


void
UserUsageConditionsWindow::_DisplayData(const UserUsageConditions& data)
{
	fCopyView->SetText(data.CopyMarkdown());
	fAgeNoteStringView->SetText(_MinimumAgeText(data.MinimumAge()));
	fVersionStringView->SetText(_VersionText(data.Code()));
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
	UserUsageConditionsSelectionMode mode)
{
	switch (mode) {
		case LATEST:
			return INTRODUCTION_TEXT_LATEST;
		case USER:
			return INTRODUCTION_TEXT_USER;
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
