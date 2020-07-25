/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ToLatestUserUsageConditionsWindow.h"

#include <Alert.h>
#include <Autolock.h>
#include <AutoLocker.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Locker.h>
#include <SeparatorView.h>
#include <TextView.h>

#include "AppUtils.h"
#include "LinkView.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "Model.h"
#include "UserUsageConditionsWindow.h"
#include "ServerHelper.h"
#include "WebAppInterface.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ToLatestUserUsageConditionsWindow"

#define PLACEHOLDER_TEXT B_UTF8_ELLIPSIS

#define WINDOW_FRAME BRect(0, 0, 500, 280)

#define KEY_USER_USAGE_CONDITIONS	"userUsageConditions"

#define NO_PRIOR_MESSAGE_TEXT "The user [%Nickname%] has authenticated, but " \
	"before proceeding, you are required to agree to the most recent usage " \
	"conditions."

#define PRIOR_MESSAGE_TEXT "The user \"%Nickname%\" has previously agreed to " \
	"usage conditions, but the usage conditions have been updated since. " \
	"The updated usage conditions now need to be agreed to."

enum {
	MSG_AGREE =									'agre',
	MSG_AGREE_FAILED =							'agfa',
	MSG_AGREE_MINIMUM_AGE_TOGGLE =				'amat',
	MSG_AGREE_USER_USAGE_CONDITIONS_TOGGLE =	'auct'
};


ToLatestUserUsageConditionsWindow::ToLatestUserUsageConditionsWindow(
	BWindow* parent,
	Model& model, const UserDetail& userDetail)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Update usage conditions"),
		B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE ),
	fModel(model),
	fUserDetail(userDetail),
	fWorkerThread(-1),
	fQuitRequestedDuringWorkerThread(false),
	fMutableControlsEnabled(false)
{
	AddToSubset(parent);
	_InitUiControls();

	// some layout magic happening here.  If the checkboxes are put directly
	// into the main vertical-group then the window tries to shrink to the
	// preferred size of the checkboxes.  To avoid this, a grid is used to house
	// the checkboxes and a fake extra grid column is added to the right of the
	// checkboxes which prevents the window from reducing in size to meet the
	// checkboxes.

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGrid()
			.Add(fMessageTextView, 0, 0, 2)
			.Add(fConfirmMinimumAgeCheckBox, 0, 1, 1)
			.Add(fConfirmUserUsageConditionsCheckBox, 0, 2, 1)
			.Add(fUserUsageConditionsLink, 0, 3, 2)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
			// rule off
		.AddGroup(B_HORIZONTAL, 1)
			.AddGlue()
			.Add(fLogoutButton)
			.Add(fAgreeButton)
		.End()
		.Add(fWorkerIndicator, 1)
		.SetInsets(B_USE_WINDOW_INSETS);

	CenterOnScreen();

	_FetchData();
		// start a new thread to pull down the user usage conditions data.
}


ToLatestUserUsageConditionsWindow::~ToLatestUserUsageConditionsWindow()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		wait_for_thread(fWorkerThread, NULL);
}


void
ToLatestUserUsageConditionsWindow::_InitUiControls()
{
	fMessageTextView = new BTextView("message text view");
	fMessageTextView->AdoptSystemColors();
	fMessageTextView->MakeEditable(false);
	fMessageTextView->MakeSelectable(false);
	BString message;
	if (fUserDetail.Agreement().Code().IsEmpty())
		message = B_TRANSLATE(NO_PRIOR_MESSAGE_TEXT);
	else
		message = B_TRANSLATE(PRIOR_MESSAGE_TEXT);
	message.ReplaceAll("%Nickname%", fUserDetail.Nickname());
	fMessageTextView->SetText(message);

	fConfirmMinimumAgeCheckBox = new BCheckBox("confirm minimum age",
		PLACEHOLDER_TEXT,
			// is filled in when the user usage conditions data is available
		new BMessage(MSG_AGREE_MINIMUM_AGE_TOGGLE));

	fConfirmUserUsageConditionsCheckBox = new BCheckBox(
		"confirm usage conditions",
		B_TRANSLATE("I agree to the usage conditions"),
		new BMessage(MSG_AGREE_USER_USAGE_CONDITIONS_TOGGLE));

	fUserUsageConditionsLink = new LinkView("usage conditions view",
		B_TRANSLATE("View the usage conditions"),
		new BMessage(MSG_VIEW_LATEST_USER_USAGE_CONDITIONS));
	fUserUsageConditionsLink->SetTarget(this);

	fLogoutButton = new BButton("logout", B_TRANSLATE("Logout"),
		new BMessage(MSG_LOG_OUT));
	fAgreeButton = new BButton("agree", B_TRANSLATE("Agree"),
		new BMessage(MSG_AGREE));

	fWorkerIndicator = new BarberPole("fetch data worker indicator");
	BSize workerIndicatorSize;
	workerIndicatorSize.SetHeight(20);
	fWorkerIndicator->SetExplicitSize(workerIndicatorSize);

	fMutableControlsEnabled = false;
	_EnableMutableControls();
}


void
ToLatestUserUsageConditionsWindow::_EnableMutableControls()
{
	bool ageChecked = fConfirmMinimumAgeCheckBox->Value() == 1;
	bool conditionsChecked = fConfirmUserUsageConditionsCheckBox->Value() == 1;
	fUserUsageConditionsLink->SetEnabled(fMutableControlsEnabled);
	fAgreeButton->SetEnabled(fMutableControlsEnabled && ageChecked
		&& conditionsChecked);
	fLogoutButton->SetEnabled(fMutableControlsEnabled);
	fConfirmUserUsageConditionsCheckBox->SetEnabled(fMutableControlsEnabled);
	fConfirmMinimumAgeCheckBox->SetEnabled(fMutableControlsEnabled);
}


void
ToLatestUserUsageConditionsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_USER_USAGE_CONDITIONS_DATA:
		{
			BMessage userUsageConditionsMessage;
			message->FindMessage(KEY_USER_USAGE_CONDITIONS,
				&userUsageConditionsMessage);
			UserUsageConditions userUsageConditions(
				&userUsageConditionsMessage);
			_DisplayData(userUsageConditions);
			fWorkerIndicator->Stop();
			break;
		}
		case MSG_LOG_OUT:
			_HandleLogout();
			break;
		case MSG_AGREE:
			_HandleAgree();
			break;
		case MSG_AGREE_FAILED:
			_HandleAgreeFailed();
			break;
		case MSG_VIEW_LATEST_USER_USAGE_CONDITIONS:
			_HandleViewUserUsageConditions();
			break;
		case MSG_AGREE_MINIMUM_AGE_TOGGLE:
		case MSG_AGREE_USER_USAGE_CONDITIONS_TOGGLE:
			_EnableMutableControls();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ToLatestUserUsageConditionsWindow::QuitRequested()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0) {
		if (Logger::IsDebugEnabled())
			HDINFO("quit requested while worker thread is operating -- will "
				"try again once the worker thread has completed");
		fQuitRequestedDuringWorkerThread = true;
		return false;
	}

	return true;
}


void
ToLatestUserUsageConditionsWindow::_SetWorkerThread(thread_id thread)
{
	if (thread >= 0) {
		fWorkerThread = thread;
		resume_thread(fWorkerThread);
	} else {
		fWorkerThread = -1;
		if (fQuitRequestedDuringWorkerThread)
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
		fQuitRequestedDuringWorkerThread = false;
	}
}


void
ToLatestUserUsageConditionsWindow::_SetWorkerThreadLocked(thread_id thread)
{
	BAutolock locker(&fLock);
	_SetWorkerThread(thread);
}


/*!	This method is called on the main thread in order to initiate the background
	processing to obtain the user usage conditions data.  It will take
	responsibility for coordinating the creation of the thread and starting the
	thread etc...
*/

void
ToLatestUserUsageConditionsWindow::_FetchData()
{
	{
		BAutolock locker(&fLock);
		if (-1 != fWorkerThread) {
			debugger("illegal state - attempt to fetch, but thread in "
				"progress");
		}
	}

	thread_id thread = spawn_thread(&_FetchDataThreadEntry,
		"Fetch usage conditions data", B_NORMAL_PRIORITY, this);
	if (thread >= 0) {
		fWorkerIndicator->Start();
		_SetWorkerThreadLocked(thread);
	} else {
		debugger("unable to start a thread to fetch the user usage "
			"conditions.");
	}
}


/*!	This method is called from the thread; it is
	the entry-point for the background processing to obtain the user usage
	conditions.
*/

/*static*/ int32
ToLatestUserUsageConditionsWindow::_FetchDataThreadEntry(void* data)
{
	ToLatestUserUsageConditionsWindow* win
		= reinterpret_cast<ToLatestUserUsageConditionsWindow*>(data);
	win->_FetchDataPerform();
	return 0;
}


/*!	This method will perform the task of obtaining data about the user usage
	conditions.
*/

void
ToLatestUserUsageConditionsWindow::_FetchDataPerform()
{
	UserUsageConditions conditions;
	WebAppInterface interface = fModel.GetWebAppInterface();

	if (interface.RetrieveUserUsageConditions("", conditions) == B_OK) {
		BMessage userUsageConditionsMessage;
		conditions.Archive(&userUsageConditionsMessage, true);
		BMessage dataMessage(MSG_USER_USAGE_CONDITIONS_DATA);
		dataMessage.AddMessage(KEY_USER_USAGE_CONDITIONS,
			&userUsageConditionsMessage);
		BMessenger(this).SendMessage(&dataMessage);
	} else {
		_NotifyFetchProblem();
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	}

	_SetWorkerThreadLocked(-1);
}


void
ToLatestUserUsageConditionsWindow::_NotifyFetchProblem()
{
	AppUtils::NotifySimpleError(
		B_TRANSLATE("Usage conditions download problem"),
		B_TRANSLATE("An error has arisen downloading the usage "
			"conditions. Check the log for details and try again. "
			ALERT_MSG_LOGS_USER_GUIDE));
}


void
ToLatestUserUsageConditionsWindow::_Agree()
{
	{
		BAutolock locker(&fLock);
		if (-1 != fWorkerThread) {
			debugger("illegal state - attempt to agree, but thread in "
				"progress");
		}
	}

	fMutableControlsEnabled = false;
	_EnableMutableControls();
	thread_id thread = spawn_thread(&_AgreeThreadEntry,
		"Agree usage conditions", B_NORMAL_PRIORITY, this);
	if (thread >= 0) {
		fWorkerIndicator->Start();
		_SetWorkerThreadLocked(thread);
	} else {
		debugger("unable to start a thread to fetch the user usage "
			"conditions.");
	}
}


/*static*/ int32
ToLatestUserUsageConditionsWindow::_AgreeThreadEntry(void* data)
{
	ToLatestUserUsageConditionsWindow* win
		= reinterpret_cast<ToLatestUserUsageConditionsWindow*>(data);
	win->_AgreePerform();
	return 0;
}


void
ToLatestUserUsageConditionsWindow::_AgreePerform()
{
	BMessenger messenger(this);
	BMessage responsePayload;
	WebAppInterface webApp = fModel.GetWebAppInterface();
	status_t result = webApp.AgreeUserUsageConditions(
		fUserUsageConditions.Code(), responsePayload);

	if (result != B_OK) {
		ServerHelper::NotifyTransportError(result);
		messenger.SendMessage(MSG_AGREE_FAILED);
	} else {
		int32 errorCode = WebAppInterface::ErrorCodeFromResponse(
			responsePayload);
		if (errorCode == ERROR_CODE_NONE) {
			AppUtils::NotifySimpleError(
				B_TRANSLATE("Usage conditions agreed"),
				B_TRANSLATE("The current usage conditions have been agreed "
					"to."));
			messenger.SendMessage(B_QUIT_REQUESTED);
		}
		else {
			AutoLocker<BLocker> locker(fModel.Lock());
			ServerHelper::NotifyServerJsonRpcError(responsePayload);
			messenger.SendMessage(MSG_AGREE_FAILED);
		}
	}

	_SetWorkerThreadLocked(-1);
}


void
ToLatestUserUsageConditionsWindow::_HandleAgreeFailed()
{
	fWorkerIndicator->Stop();
	fMutableControlsEnabled = true;
	_EnableMutableControls();
}


void
ToLatestUserUsageConditionsWindow::_DisplayData(
	const UserUsageConditions& userUsageConditions)
{
	fUserUsageConditions = userUsageConditions;
	fConfirmMinimumAgeCheckBox->SetLabel(
		LocaleUtils::CreateTranslatedIAmMinimumAgeSlug(
			fUserUsageConditions.MinimumAge()));
	fMutableControlsEnabled = true;
	_EnableMutableControls();
}


void
ToLatestUserUsageConditionsWindow::_HandleViewUserUsageConditions()
{
	if (!fUserUsageConditions.Code().IsEmpty()) {
		UserUsageConditionsWindow* window = new UserUsageConditionsWindow(
			fModel, fUserUsageConditions);
		window->Show();
	}
}

void
ToLatestUserUsageConditionsWindow::_HandleLogout()
{
	AutoLocker<BLocker> locker(fModel.Lock());
	fModel.SetNickname("");
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);
}


void
ToLatestUserUsageConditionsWindow::_HandleAgree()
{
	bool ageChecked = fConfirmMinimumAgeCheckBox->Value() == 1;
	bool conditionsChecked = fConfirmUserUsageConditionsCheckBox->Value() == 1;

	// precondition that the user has checked both of the checkboxes.
	if (!ageChecked || !conditionsChecked)
		debugger("the user has not agreed to the age and conditions");

	_Agree();
}