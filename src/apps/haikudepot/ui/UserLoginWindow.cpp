/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserLoginWindow.h"

#include <algorithm>
#include <ctype.h>

#include <mail_encoding.h>

#include <Alert.h>
#include <Autolock.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "AppUtils.h"
#include "BitmapView.h"
#include "Captcha.h"
#include "HaikuDepotConstants.h"
#include "LanguageMenuUtils.h"
#include "LinkView.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "Model.h"
#include "ServerHelper.h"
#include "TabView.h"
#include "UserUsageConditions.h"
#include "UserUsageConditionsWindow.h"
#include "ValidationUtils.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserLoginWindow"

#define PLACEHOLDER_TEXT B_UTF8_ELLIPSIS

#define KEY_USER_CREDENTIALS			"userCredentials"
#define KEY_CAPTCHA_IMAGE				"captchaImage"
#define KEY_USER_USAGE_CONDITIONS		"userUsageConditions"
#define KEY_VALIDATION_FAILURES			"validationFailures"


enum ActionTabs {
	TAB_LOGIN							= 0,
	TAB_CREATE_ACCOUNT					= 1
};


enum {
	MSG_SEND							= 'send',
	MSG_TAB_SELECTED					= 'tbsl',
	MSG_CREATE_ACCOUNT_SETUP_SUCCESS	= 'cass',
	MSG_CREATE_ACCOUNT_SETUP_ERROR		= 'case',
	MSG_VALIDATE_FIELDS					= 'vldt',
	MSG_LOGIN_SUCCESS					= 'lsuc',
	MSG_LOGIN_FAILED					= 'lfai',
	MSG_LOGIN_ERROR						= 'lter',
	MSG_CREATE_ACCOUNT_SUCCESS			= 'csuc',
	MSG_CREATE_ACCOUNT_FAILED			= 'cfai',
	MSG_CREATE_ACCOUNT_ERROR			= 'cfae'
};


/*!	The creation of an account requires that some prerequisite data is first
	loaded in or may later need to be refreshed.  This enum controls what
	elements of the setup should be performed.
*/

enum CreateAccountSetupMask {
	CREATE_CAPTCHA						= 1 << 1,
	FETCH_USER_USAGE_CONDITIONS			= 1 << 2
};


/*!	To create a user, some details need to be provided.  Those details together
	with a pointer to the window structure are provided to the background thread
	using this struct.
*/

struct CreateAccountThreadData {
	UserLoginWindow*		window;
	CreateUserDetail*		detail;
};


/*!	A background thread runs to gather data to use in the interface for creating
	a new user.  This structure is passed to the background thread.
*/

struct CreateAccountSetupThreadData {
	UserLoginWindow*		window;
	uint32					mask;
		// defines what setup steps are required
};


/*!	A background thread runs to authenticate the user with the remote server
	system.  This structure provides the thread with the necessary data to
	perform this work.
*/

struct AuthenticateSetupThreadData {
	UserLoginWindow*		window;
	UserCredentials*		credentials;
};


UserLoginWindow::UserLoginWindow(BWindow* parent, BRect frame, Model& model)
	:
	BWindow(frame, B_TRANSLATE("Log in"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE),
	fUserUsageConditions(NULL),
	fCaptcha(NULL),
	fPreferredLanguageCode(LANGUAGE_DEFAULT_CODE),
	fModel(model),
	fMode(NONE),
	fWorkerThread(-1),
	fQuitRequestedDuringWorkerThread(false)
{
	AddToSubset(parent);

	fNicknameField = new BTextControl(B_TRANSLATE("Nickname:"), "", NULL);
	fPasswordField = new BTextControl(B_TRANSLATE("Password:"), "", NULL);
	fPasswordField->TextView()->HideTyping(true);

	fNewNicknameField = new BTextControl(B_TRANSLATE("Nickname:"), "",
		NULL);
	fNewPasswordField = new BTextControl(B_TRANSLATE("Password:"), "",
		new BMessage(MSG_VALIDATE_FIELDS));
	fNewPasswordField->TextView()->HideTyping(true);
	fRepeatPasswordField = new BTextControl(B_TRANSLATE("Repeat password:"),
		"", new BMessage(MSG_VALIDATE_FIELDS));
	fRepeatPasswordField->TextView()->HideTyping(true);

	{
		AutoLocker<BLocker> locker(fModel.Lock());
		fPreferredLanguageCode = fModel.Language()->PreferredLanguage()->Code();
		// Construct languages popup
		BPopUpMenu* languagesMenu = new BPopUpMenu(B_TRANSLATE("Language"));
		fLanguageCodeField = new BMenuField("language",
			B_TRANSLATE("Preferred language:"), languagesMenu);

		LanguageMenuUtils::AddLanguagesToMenu(
			fModel.Language(), languagesMenu);
		languagesMenu->SetTargetForItems(this);

		HDINFO("using preferred language code [%s]",
			fPreferredLanguageCode.String());
		LanguageMenuUtils::MarkLanguageInMenu(fPreferredLanguageCode,
			languagesMenu);
	}

	fEmailField = new BTextControl(B_TRANSLATE("Email address:"), "", NULL);
	fCaptchaView = new BitmapView("captcha view");
	fCaptchaResultField = new BTextControl("", "", NULL);
	fConfirmMinimumAgeCheckBox = new BCheckBox("confirm minimum age",
		PLACEHOLDER_TEXT,
			// is filled in when the user usage conditions data is available
		NULL);
	fConfirmMinimumAgeCheckBox->SetEnabled(false);
	fConfirmUserUsageConditionsCheckBox = new BCheckBox(
		"confirm usage conditions",
		B_TRANSLATE("I agree to the usage conditions"),
		NULL);
	fUserUsageConditionsLink = new LinkView("usage conditions view",
		B_TRANSLATE("View the usage conditions"),
		new BMessage(MSG_VIEW_LATEST_USER_USAGE_CONDITIONS));
	fUserUsageConditionsLink->SetTarget(this);

	// Setup modification messages on all text fields to trigger validation
	// of input
	fNewNicknameField->SetModificationMessage(
		new BMessage(MSG_VALIDATE_FIELDS));
	fNewPasswordField->SetModificationMessage(
		new BMessage(MSG_VALIDATE_FIELDS));
	fRepeatPasswordField->SetModificationMessage(
		new BMessage(MSG_VALIDATE_FIELDS));
	fEmailField->SetModificationMessage(
		new BMessage(MSG_VALIDATE_FIELDS));
	fCaptchaResultField->SetModificationMessage(
		new BMessage(MSG_VALIDATE_FIELDS));
	fTabView = new TabView(BMessenger(this),
		BMessage(MSG_TAB_SELECTED));

	BGridView* loginCard = new BGridView(B_TRANSLATE("Log in"));
	BLayoutBuilder::Grid<>(loginCard)
		.AddTextControl(fNicknameField, 0, 0)
		.AddTextControl(fPasswordField, 0, 1)
		.AddGlue(0, 2)

		.SetInsets(B_USE_DEFAULT_SPACING)
	;
	fTabView->AddTab(loginCard);

	BGridView* createAccountCard = new BGridView(B_TRANSLATE("Create account"));
	BLayoutBuilder::Grid<>(createAccountCard)
		.AddTextControl(fNewNicknameField, 0, 0)
		.AddTextControl(fNewPasswordField, 0, 1)
		.AddTextControl(fRepeatPasswordField, 0, 2)
		.AddTextControl(fEmailField, 0, 3)
		.AddMenuField(fLanguageCodeField, 0, 4)
		.Add(fCaptchaView, 0, 5)
		.Add(fCaptchaResultField, 1, 5)
		.Add(fConfirmMinimumAgeCheckBox, 1, 6)
		.Add(fConfirmUserUsageConditionsCheckBox, 1, 7)
		.Add(fUserUsageConditionsLink, 1, 8)
		.SetInsets(B_USE_DEFAULT_SPACING)
	;
	fTabView->AddTab(createAccountCard);

	fSendButton = new BButton("send", B_TRANSLATE("Log in"),
		new BMessage(MSG_SEND));
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	// Build layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fTabView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fSendButton)
		.End()
		.SetInsets(B_USE_WINDOW_INSETS)
	;

	SetDefaultButton(fSendButton);

	_SetMode(LOGIN);

	CenterIn(parent->Frame());
}


UserLoginWindow::~UserLoginWindow()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		wait_for_thread(fWorkerThread, NULL);
}


void
UserLoginWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_VALIDATE_FIELDS:
			_MarkCreateUserInvalidFields();
			break;

		case MSG_VIEW_LATEST_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions();
			break;

		case MSG_SEND:
			switch (fMode) {
				case LOGIN:
					_Authenticate();
					break;
				case CREATE_ACCOUNT:
					_CreateAccount();
					break;
				default:
					break;
			}
			break;

		case MSG_TAB_SELECTED:
		{
			int32 tabIndex;
			if (message->FindInt32("tab index", &tabIndex) == B_OK) {
				switch (tabIndex) {
					case TAB_LOGIN:
						_SetMode(LOGIN);
						break;
					case TAB_CREATE_ACCOUNT:
						_SetMode(CREATE_ACCOUNT);
						break;
					default:
						break;
				}
			}
			break;
		}

		case MSG_CREATE_ACCOUNT_SETUP_ERROR:
			HDERROR("failed to setup for account setup - window must quit");
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
			break;

		case MSG_CREATE_ACCOUNT_SETUP_SUCCESS:
			_HandleCreateAccountSetupSuccess(message);
			break;

		case MSG_LANGUAGE_SELECTED:
			message->FindString("code", &fPreferredLanguageCode);
			break;

		case MSG_LOGIN_ERROR:
			_HandleAuthenticationError();
			break;

		case MSG_LOGIN_FAILED:
			_HandleAuthenticationFailed();
			break;

		case MSG_LOGIN_SUCCESS:
		{
			BMessage credentialsMessage;
			if (message->FindMessage(KEY_USER_CREDENTIALS,
					&credentialsMessage) != B_OK) {
				debugger("expected key in internal message not found");
			}

			_HandleAuthenticationSuccess(
				UserCredentials(&credentialsMessage));
			break;
		}
		case MSG_CREATE_ACCOUNT_SUCCESS:
		{
			BMessage credentialsMessage;
			if (message->FindMessage(KEY_USER_CREDENTIALS,
					&credentialsMessage) != B_OK) {
				debugger("expected key in internal message not found");
			}

			_HandleCreateAccountSuccess(
				UserCredentials(&credentialsMessage));
			break;
		}
		case MSG_CREATE_ACCOUNT_FAILED:
		{
			BMessage validationFailuresMessage;
			if (message->FindMessage(KEY_VALIDATION_FAILURES,
					&validationFailuresMessage) != B_OK) {
				debugger("expected key in internal message not found");
			}
			ValidationFailures validationFailures(&validationFailuresMessage);
			_HandleCreateAccountFailure(validationFailures);
			break;
		}
		case MSG_CREATE_ACCOUNT_ERROR:
			_HandleCreateAccountError();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
UserLoginWindow::QuitRequested()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0) {
		HDDEBUG("quit requested while worker thread is operating -- will "
			"try again once the worker thread has completed");
		fQuitRequestedDuringWorkerThread = true;
		return false;
	}

	return true;
}


void
UserLoginWindow::SetOnSuccessMessage(
	const BMessenger& messenger, const BMessage& message)
{
	fOnSuccessTarget = messenger;
	fOnSuccessMessage = message;
}


void
UserLoginWindow::_EnableMutableControls(bool enabled)
{
	fNicknameField->SetEnabled(enabled);
	fPasswordField->SetEnabled(enabled);
	fNewNicknameField->SetEnabled(enabled);
	fNewPasswordField->SetEnabled(enabled);
	fRepeatPasswordField->SetEnabled(enabled);
	fEmailField->SetEnabled(enabled);
	fLanguageCodeField->SetEnabled(enabled);
	fCaptchaResultField->SetEnabled(enabled);
	fConfirmMinimumAgeCheckBox->SetEnabled(enabled);
	fConfirmUserUsageConditionsCheckBox->SetEnabled(enabled);
	fUserUsageConditionsLink->SetEnabled(enabled);
	fSendButton->SetEnabled(enabled);
}


void
UserLoginWindow::_SetMode(Mode mode)
{
	if (fMode == mode)
		return;

	fMode = mode;

	switch (fMode) {
		case LOGIN:
			fTabView->Select(TAB_LOGIN);
			fSendButton->SetLabel(B_TRANSLATE("Log in"));
			fNicknameField->MakeFocus();
			break;
		case CREATE_ACCOUNT:
			fTabView->Select(TAB_CREATE_ACCOUNT);
			fSendButton->SetLabel(B_TRANSLATE("Create account"));
			_CreateAccountSetupIfNecessary();
			fNewNicknameField->MakeFocus();
			_MarkCreateUserInvalidFields();
			break;
		default:
			break;
	}
}


void
UserLoginWindow::_SetWorkerThreadLocked(thread_id thread)
{
	BAutolock locker(&fLock);
	_SetWorkerThread(thread);
}


void
UserLoginWindow::_SetWorkerThread(thread_id thread)
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


// #pragma mark - Authentication


void
UserLoginWindow::_Authenticate()
{
	_Authenticate(UserCredentials(
		fNicknameField->Text(), fPasswordField->Text()));
}


void
UserLoginWindow::_Authenticate(const UserCredentials& credentials)
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		return;

	_EnableMutableControls(false);
	AuthenticateSetupThreadData* threadData = new AuthenticateSetupThreadData();
		// this will be owned and deleted by the thread
	threadData->window = this;
	threadData->credentials = new UserCredentials(credentials);

	thread_id thread = spawn_thread(&_AuthenticateThreadEntry,
		"Authentication", B_NORMAL_PRIORITY, threadData);
	if (thread >= 0)
		_SetWorkerThread(thread);
}


/*static*/ int32
UserLoginWindow::_AuthenticateThreadEntry(void* data)
{
	AuthenticateSetupThreadData* threadData
		= static_cast<AuthenticateSetupThreadData*>(data);
	threadData->window->_AuthenticateThread(*(threadData->credentials));
	threadData->window->_SetWorkerThreadLocked(-1);
	delete threadData->credentials;
	delete threadData;
	return 0;
}


void
UserLoginWindow::_AuthenticateThread(UserCredentials& userCredentials)
{
	BMessage responsePayload;
	WebAppInterface interface = fModel.GetWebAppInterface();
	status_t status = interface.AuthenticateUser(
		userCredentials.Nickname(), userCredentials.PasswordClear(),
		responsePayload);
	BString token;

	if (status == B_OK) {
		int32 errorCode = interface.ErrorCodeFromResponse(responsePayload);

		if (errorCode == ERROR_CODE_NONE)
			_UnpackAuthenticationToken(responsePayload, token);
		else {
			ServerHelper::NotifyServerJsonRpcError(responsePayload);
			BMessenger(this).SendMessage(MSG_LOGIN_ERROR);
			return;
				// early exit
		}
	}

	if (status == B_OK) {
		userCredentials.SetIsSuccessful(!token.IsEmpty());

		if (Logger::IsDebugEnabled()) {
			if (token.IsEmpty()) {
				HDINFO("authentication failed");
			}
			else {
				HDINFO("authentication successful");
			}
		}

		BMessenger messenger(this);

		if (userCredentials.IsSuccessful()) {
			BMessage message(MSG_LOGIN_SUCCESS);
			BMessage credentialsMessage;
			status = userCredentials.Archive(&credentialsMessage);
			if (status == B_OK)
				status = message.AddMessage(KEY_USER_CREDENTIALS, &credentialsMessage);
			if (status == B_OK)
				messenger.SendMessage(&message);
		} else {
			BMessage message(MSG_LOGIN_FAILED);
			messenger.SendMessage(&message);
		}
	} else {
		ServerHelper::NotifyTransportError(status);
		BMessenger(this).SendMessage(MSG_LOGIN_ERROR);
	}
}


void
UserLoginWindow::_UnpackAuthenticationToken(BMessage& responsePayload,
	BString& token)
{
	BMessage resultPayload;
	if (responsePayload.FindMessage("result", &resultPayload) == B_OK) {
		resultPayload.FindString("token", &token);
			// We don't care for or store the token for now. The web-service
			// supports two methods of authorizing requests. One is via
			// Basic Authentication in the HTTP header, the other is via
			// Token Bearer. Since the connection is encrypted, it is hopefully
			// ok to send the password with each request instead of implementing
			// the Token Bearer. See section 5.1.2 in the haiku-depot-web
			// documentation.
	}
}


/*!	This method gets hit when an error occurs while authenticating; something
	like a network error.  Because of the large number of possible errors, the
	reporting of the error is handled separately from this method.  This method
	only needs to take responsibility for returning the GUI and state of the
	window to a situation where the user can try again.
*/

void
UserLoginWindow::_HandleAuthenticationError()
{
	_EnableMutableControls(true);
}


void
UserLoginWindow::_HandleAuthenticationFailed()
{
	AppUtils::NotifySimpleError(
		B_TRANSLATE("Authentication failed"),
		B_TRANSLATE("The user does not exist or the wrong password was"
			" supplied. Check your credentials and try again.")
	);
	fPasswordField->SetText("");
	_EnableMutableControls(true);
}


/*!	This is called when the user has successfully authenticated with the remote
	HaikuDepotServer system; this handles the take-up of the data and closing
	the window etc...
*/

void
UserLoginWindow::_HandleAuthenticationSuccess(
	const UserCredentials& credentials)
{
	BString message = B_TRANSLATE("You have successfully authenticated as user "
		"%Nickname%.");
	message.ReplaceAll("%Nickname%", credentials.Nickname());

	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("Success"), message, B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();

	_TakeUpCredentialsAndQuit(credentials);
}


/*!	This method will fire any configured target + message, will set the
	authentication details (credentials) into the system so that further API
	calls etc... will be from this user and will quit the window.
*/

void
UserLoginWindow::_TakeUpCredentialsAndQuit(const UserCredentials& credentials)
{
	{
		AutoLocker<BLocker> locker(fModel.Lock());
		fModel.SetAuthorization(credentials.Nickname(),
			credentials.PasswordClear(), true);
	}

	// Clone these fields before the window goes away.
	BMessenger onSuccessTarget(fOnSuccessTarget);
	BMessage onSuccessMessage(fOnSuccessMessage);

	BMessenger(this).SendMessage(B_QUIT_REQUESTED);

	// Send the success message after the alert has been closed,
	// otherwise more windows will popup alongside the alert.
	if (onSuccessTarget.IsValid() && onSuccessMessage.what != 0)
		onSuccessTarget.SendMessage(&onSuccessMessage);
}


// #pragma mark - Create Account Setup


/*!	This method will trigger the process of gathering the data from the server
	that is necessary for setting up an account.  It will only gather that data
	that it does not already have to avoid extra work.
*/

void
UserLoginWindow::_CreateAccountSetupIfNecessary()
{
	uint32 setupMask = 0;
	if (fCaptcha == NULL)
		setupMask |= CREATE_CAPTCHA;
	if (fUserUsageConditions == NULL)
		setupMask |= FETCH_USER_USAGE_CONDITIONS;
	_CreateAccountSetup(setupMask);
}


/*!	Fetches the data required for creating an account.
	\param mask describes what data is required to be fetched.
*/

void
UserLoginWindow::_CreateAccountSetup(uint32 mask)
{
	if (mask == 0)
		return;

	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		return;

	if (!Lock())
		debugger("unable to lock the user login window");

	_EnableMutableControls(false);

	if ((mask & CREATE_CAPTCHA) != 0)
		_SetCaptcha(NULL);
	if ((mask & FETCH_USER_USAGE_CONDITIONS) != 0)
		_SetUserUsageConditions(NULL);

	Unlock();

	CreateAccountSetupThreadData* threadData = new CreateAccountSetupThreadData;
	threadData->window = this;
	threadData->mask = mask;

	thread_id thread = spawn_thread(&_CreateAccountSetupThreadEntry,
		"Create account setup", B_NORMAL_PRIORITY, threadData);
	if (thread >= 0)
		_SetWorkerThreadLocked(thread);
	else {
		debugger("unable to start a thread to gather data for creating an "
			"account");
	}
}


int32
UserLoginWindow::_CreateAccountSetupThreadEntry(void* data)
{
	CreateAccountSetupThreadData* threadData =
		static_cast<CreateAccountSetupThreadData*>(data);
	BMessenger messenger(threadData->window);
	status_t result = B_OK;
	Captcha captcha;
	UserUsageConditions userUsageConditions;
	bool shouldCreateCaptcha = (threadData->mask & CREATE_CAPTCHA) != 0;
	bool shouldFetchUserUsageConditions
		= (threadData->mask & FETCH_USER_USAGE_CONDITIONS) != 0;

	if (result == B_OK && shouldCreateCaptcha)
		result = threadData->window->_CreateAccountCaptchaSetupThread(captcha);
	if (result == B_OK && shouldFetchUserUsageConditions) {
		result = threadData->window
			->_CreateAccountUserUsageConditionsSetupThread(userUsageConditions);
	}

	if (result == B_OK) {
		BMessage message(MSG_CREATE_ACCOUNT_SETUP_SUCCESS);
		if (result == B_OK && shouldCreateCaptcha) {
			BMessage captchaMessage;
			result = captcha.Archive(&captchaMessage);
			if (result == B_OK)
				result = message.AddMessage(KEY_CAPTCHA_IMAGE, &captchaMessage);
		}
		if (result == B_OK && shouldFetchUserUsageConditions) {
			BMessage userUsageConditionsMessage;
			result = userUsageConditions.Archive(&userUsageConditionsMessage);
			if (result == B_OK) {
				result = message.AddMessage(KEY_USER_USAGE_CONDITIONS,
					&userUsageConditionsMessage);
			}
		}
		if (result == B_OK) {
			HDDEBUG("successfully completed collection of create account "
				"data from the server in background thread");
			messenger.SendMessage(&message);
		} else {
			debugger("unable to configure the "
				"'MSG_CREATE_ACCOUNT_SETUP_SUCCESS' message.");
		}
	}

	if (result != B_OK) {
		// any error messages / alerts should have already been handled by this
		// point.
		messenger.SendMessage(MSG_CREATE_ACCOUNT_SETUP_ERROR);
	}

	threadData->window->_SetWorkerThreadLocked(-1);
	delete threadData;
	return 0;
}


status_t
UserLoginWindow::_CreateAccountUserUsageConditionsSetupThread(
	UserUsageConditions& userUsageConditions)
{
	WebAppInterface interface = fModel.GetWebAppInterface();
	status_t result = interface.RetrieveUserUsageConditions(
		NULL, userUsageConditions);

	if (result != B_OK) {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("Usage conditions download problem"),
			B_TRANSLATE("An error has arisen downloading the usage "
				"conditions required to create a new user. Check the log for "
				"details and try again. "
				ALERT_MSG_LOGS_USER_GUIDE));
	}

	return result;
}


status_t
UserLoginWindow::_CreateAccountCaptchaSetupThread(Captcha& captcha)
{
	WebAppInterface interface = fModel.GetWebAppInterface();
	BMessage responsePayload;

	status_t status = interface.RequestCaptcha(responsePayload);

// check for transport related errors.

	if (status != B_OK) {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("Captcha error"),
			B_TRANSLATE("It was not possible to communicate with the server to "
				"obtain a captcha image required to create a new user."));
	}

// check for server-generated errors.

	if (status == B_OK) {
		if (interface.ErrorCodeFromResponse(responsePayload)
				!= ERROR_CODE_NONE) {
			ServerHelper::AlertTransportError(&responsePayload);
			status = B_ERROR;
		}
	}

// now parse the response from the server and extract the captcha data.

	if (status == B_OK) {
		status = _UnpackCaptcha(responsePayload, captcha);
		if (status != B_OK) {
			AppUtils::NotifySimpleError(
				B_TRANSLATE("Captcha error"),
				B_TRANSLATE("It was not possible to extract necessary captcha "
					"information from the data sent back from the server."));
		}
	}

	return status;
}


/*!	Takes the data returned to the client after it was requested from the
	server and extracts from it the captcha image.
*/

status_t
UserLoginWindow::_UnpackCaptcha(BMessage& responsePayload, Captcha& captcha)
{
	status_t result = B_OK;

	BMessage resultMessage;
	if (result == B_OK)
		result = responsePayload.FindMessage("result", &resultMessage);
	BString token;
	if (result == B_OK)
		result = resultMessage.FindString("token", &token);
	BString pngImageDataBase64;
	if (result == B_OK)
		result = resultMessage.FindString("pngImageDataBase64", &pngImageDataBase64);

	ssize_t encodedSize;
	ssize_t decodedSize;
	if (result == B_OK) {
		encodedSize = pngImageDataBase64.Length();
		decodedSize = (encodedSize * 3 + 3) / 4;
		if (decodedSize <= 0)
			result = B_ERROR;
	}
	else
		HDERROR("obtained a captcha with no image data");

	char* buffer = NULL;
	if (result == B_OK) {
		buffer = new char[decodedSize];
		decodedSize = decode_base64(buffer, pngImageDataBase64.String(),
			encodedSize);
		if (decodedSize <= 0)
			result = B_ERROR;

		if (result == B_OK) {
			captcha.SetToken(token);
			captcha.SetPngImageData(buffer, decodedSize);
		}
		delete[] buffer;

		HDDEBUG("did obtain a captcha image of size %" B_PRIuSIZE " bytes",
			decodedSize);
	}

	return result;
}


void
UserLoginWindow::_HandleCreateAccountSetupSuccess(BMessage* message)
{
	HDDEBUG("handling account setup success");
	BMessage captchaMessage;
	BMessage userUsageConditionsMessage;

	if (message->FindMessage(KEY_CAPTCHA_IMAGE, &captchaMessage) == B_OK)
		_SetCaptcha(new Captcha(&captchaMessage));

	if (message->FindMessage(KEY_USER_USAGE_CONDITIONS,
			&userUsageConditionsMessage) == B_OK) {
		_SetUserUsageConditions(
			new UserUsageConditions(&userUsageConditionsMessage));
	}

	_EnableMutableControls(true);
}


void
UserLoginWindow::_SetCaptcha(Captcha* captcha)
{
	HDDEBUG("setting captcha");
	if (fCaptcha != NULL)
		delete fCaptcha;
	fCaptcha = captcha;

	if (fCaptcha == NULL)
		fCaptchaView->UnsetBitmap();
	else {
		off_t size;
		fCaptcha->PngImageData()->GetSize(&size);
		SharedBitmap* captchaImage
			= new SharedBitmap(*(fCaptcha->PngImageData()));
		fCaptchaView->SetBitmap(captchaImage);
	}
	fCaptchaResultField->SetText("");
}


/*!	This method is hit when the user usage conditions data arrives back from the
	server.  At this point some of the UI elements may need to be updated.
*/

void
UserLoginWindow::_SetUserUsageConditions(
	UserUsageConditions* userUsageConditions)
{
	HDDEBUG("setting user usage conditions");
	if (fUserUsageConditions != NULL)
		delete fUserUsageConditions;
	fUserUsageConditions = userUsageConditions;

	if (fUserUsageConditions != NULL) {
		fConfirmMinimumAgeCheckBox->SetLabel(
    		LocaleUtils::CreateTranslatedIAmMinimumAgeSlug(
    			fUserUsageConditions->MinimumAge()));
	} else {
		fConfirmMinimumAgeCheckBox->SetLabel(PLACEHOLDER_TEXT);
		fConfirmMinimumAgeCheckBox->SetValue(0);
		fConfirmUserUsageConditionsCheckBox->SetValue(0);
	}
}


// #pragma mark - Create Account


void
UserLoginWindow::_CreateAccount()
{
	BAutolock locker(&fLock);

	if (fCaptcha == NULL)
		debugger("missing captcha when assembling create user details");
	if (fUserUsageConditions == NULL)
		debugger("missing user usage conditions when assembling create user "
			"details");

	if (fWorkerThread >= 0)
		return;

	CreateUserDetail* detail = new CreateUserDetail();
	ValidationFailures validationFailures;

	_AssembleCreateUserDetail(*detail);
	_ValidateCreateUserDetail(*detail, validationFailures);
	_MarkCreateUserInvalidFields(validationFailures);
	_AlertCreateUserValidationFailure(validationFailures);

	if (validationFailures.IsEmpty()) {
		CreateAccountThreadData* data = new CreateAccountThreadData();
		data->window = this;
		data->detail = detail;

		thread_id thread = spawn_thread(&_CreateAccountThreadEntry,
			"Account creator", B_NORMAL_PRIORITY, data);
		if (thread >= 0)
			_SetWorkerThread(thread);
	}
}


/*!	Take the data from the user interface and put it into a model object to be
	used as the input for the validation and communication with the backend
	application server (HDS).
*/

void
UserLoginWindow::_AssembleCreateUserDetail(CreateUserDetail& detail)
{
	detail.SetNickname(fNewNicknameField->Text());
	detail.SetPasswordClear(fNewPasswordField->Text());
	detail.SetIsPasswordRepeated(strlen(fRepeatPasswordField->Text()) > 0
		&& strcmp(fNewPasswordField->Text(),
			fRepeatPasswordField->Text()) == 0);
	detail.SetEmail(fEmailField->Text());

	if (fCaptcha != NULL)
		detail.SetCaptchaToken(fCaptcha->Token());

	detail.SetCaptchaResponse(fCaptchaResultField->Text());
	detail.SetLanguageCode(fPreferredLanguageCode);

	if ( fUserUsageConditions != NULL
		&& fConfirmMinimumAgeCheckBox->Value() == 1
		&& fConfirmUserUsageConditionsCheckBox->Value() == 1) {
		detail.SetAgreedToUserUsageConditionsCode(fUserUsageConditions->Code());
	}
}


/*!	This method will check the data supplied in the detail and will relay any
	validation or data problems into the supplied ValidationFailures object.
*/

void
UserLoginWindow::_ValidateCreateUserDetail(
	CreateUserDetail& detail, ValidationFailures& failures)
{
	if (!ValidationUtils::IsValidEmail(detail.Email()))
		failures.AddFailure("email", "malformed");

	if (detail.Nickname().IsEmpty())
		failures.AddFailure("nickname", "required");
	else {
		if (!ValidationUtils::IsValidNickname(detail.Nickname()))
			failures.AddFailure("nickname", "malformed");
	}

	if (detail.PasswordClear().IsEmpty())
		failures.AddFailure("passwordClear", "required");
	else {
		if (!ValidationUtils::IsValidPasswordClear(detail.PasswordClear()))
			failures.AddFailure("passwordClear", "invalid");
	}

	if (!detail.IsPasswordRepeated())
		failures.AddFailure("repeatPasswordClear", "repeat");

	if (detail.AgreedToUserUsageConditionsCode().IsEmpty())
		failures.AddFailure("agreedToUserUsageConditionsCode", "required");

	if (detail.CaptchaResponse().IsEmpty())
		failures.AddFailure("captchaResponse", "required");
}


void
UserLoginWindow::_MarkCreateUserInvalidFields()
{
	CreateUserDetail detail;
	ValidationFailures failures;
	_AssembleCreateUserDetail(detail);
	_ValidateCreateUserDetail(detail, failures);
	_MarkCreateUserInvalidFields(failures);
}


void
UserLoginWindow::_MarkCreateUserInvalidFields(
	const ValidationFailures& failures)
{
	fNewNicknameField->MarkAsInvalid(failures.Contains("nickname"));
	fNewPasswordField->MarkAsInvalid(failures.Contains("passwordClear"));
	fRepeatPasswordField->MarkAsInvalid(failures.Contains("repeatPasswordClear"));
	fEmailField->MarkAsInvalid(failures.Contains("email"));
	fCaptchaResultField->MarkAsInvalid(failures.Contains("captchaResponse"));
}


void
UserLoginWindow::_AlertCreateUserValidationFailure(
	const ValidationFailures& failures)
{
	if (!failures.IsEmpty()) {
		BString alertMessage = B_TRANSLATE("There are problems in the supplied "
			"data:");
		alertMessage << "\n\n";

		for (int32 i = 0; i < failures.CountFailures(); i++) {
			ValidationFailure* failure = failures.FailureAtIndex(i);
			BStringList messages = failure->Messages();

			for (int32 j = 0; j < messages.CountStrings(); j++) {
				alertMessage << _CreateAlertTextFromValidationFailure(
					failure->Property(), messages.StringAt(j));
				alertMessage << '\n';
			}
		}

		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Input validation"),
			alertMessage,
			B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (alert != NULL)
			alert->Go();
	}
}


/*!	This method produces a debug string for a set of validation failures.
 */

/*static*/ void
UserLoginWindow::_ValidationFailuresToString(const ValidationFailures& failures,
	BString& output)
{
	for (int32 i = 0; i < failures.CountFailures(); i++) {
		ValidationFailure* failure = failures.FailureAtIndex(i);
		BStringList messages = failure->Messages();
		for (int32 j = 0; j < messages.CountStrings(); j++)
		{
			if (0 != j || 0 != i)
				output << ", ";
			output << failure->Property();
			output << ":";
			output << messages.StringAt(j);
		}
	}
}


/*static*/ BString
UserLoginWindow::_CreateAlertTextFromValidationFailure(
	const BString& property, const BString& message)
{
	if (property == "email" && message == "malformed")
		return B_TRANSLATE("The email is malformed.");

	if (property == "nickname" && message == "notunique") {
		return B_TRANSLATE("The nickname must be unique, but the supplied "
			"nickname is already taken. Choose a different nickname.");
	}

	if (property == "nickname" && message == "required")
		return B_TRANSLATE("The nickname is required.");

	if (property == "nickname" && message == "malformed") {
		return B_TRANSLATE("The nickname is malformed. The nickname may only "
			"contain digits and lower case latin characters. The nickname "
			"must be between four and sixteen characters in length.");
	}

	if (property == "passwordClear" && message == "required")
		return B_TRANSLATE("A password is required.");

	if (property == "passwordClear" && message == "invalid") {
		return B_TRANSLATE("The password must be at least eight characters "
			"long, consist of at least two digits and one upper case "
			"character.");
	}

	if (property == "passwordClearRepeated" && message == "required") {
		return B_TRANSLATE("The password must be repeated in order to reduce "
			"the chance of entering the password incorrectly.");
	}

	if (property == "passwordClearRepeated" && message == "repeat")
		return B_TRANSLATE("The password has been incorrectly repeated.");

	if (property == "agreedToUserUsageConditionsCode"
			&& message == "required") {
		return B_TRANSLATE("The usage agreement must be agreed to and a "
			"confirmation should be made that the person creating the user "
			"meets the minimum age requirement.");
	}

	if (property == "captchaResponse" && message == "required") {
		return B_TRANSLATE("A response to the captcha question must be "
			"provided.");
	}

	if (property == "captchaResponse" && message == "captchabadresponse") {
		return B_TRANSLATE("The supplied response to the captcha is "
			"incorrect. A new captcha will be generated; try again.");
	}

	BString result = B_TRANSLATE("An unexpected error '%Message%' has arisen "
		"with property '%Property%'");
	result.ReplaceAll("%Message%", message);
	result.ReplaceAll("%Property%", property);
	return result;
}


/*!	This is the entry-point for the thread that will process the data to create
	the new account.
*/

int32
UserLoginWindow::_CreateAccountThreadEntry(void* data)
{
	CreateAccountThreadData* threadData =
		static_cast<CreateAccountThreadData*>(data);
	threadData->window->_CreateAccountThread(threadData->detail);
	threadData->window->_SetWorkerThreadLocked(-1);
	if (NULL != threadData->detail)
		delete threadData->detail;
	return 0;
}


/*!	This method runs in a background thread run and makes the necessary calls
	to the application server to actually create the user.
*/

void
UserLoginWindow::_CreateAccountThread(CreateUserDetail* detail)
{
	WebAppInterface interface = fModel.GetWebAppInterface();
	BMessage responsePayload;
	BMessenger messenger(this);

	status_t status = interface.CreateUser(
		detail->Nickname(),
		detail->PasswordClear(),
		detail->Email(),
		detail->CaptchaToken(),
		detail->CaptchaResponse(),
		detail->LanguageCode(),
		detail->AgreedToUserUsageConditionsCode(),
		responsePayload);

	BString error = B_TRANSLATE(
		"There was a puzzling response from the web service.");

	if (status == B_OK) {
		int32 errorCode = interface.ErrorCodeFromResponse(responsePayload);

		switch (errorCode) {
			case ERROR_CODE_NONE:
			{
				BMessage userCredentialsMessage;
				UserCredentials userCredentials(detail->Nickname(),
					detail->PasswordClear());
				userCredentials.Archive(&userCredentialsMessage);
				BMessage message(MSG_CREATE_ACCOUNT_SUCCESS);
				message.AddMessage(KEY_USER_CREDENTIALS,
					&userCredentialsMessage);
				messenger.SendMessage(&message);
				break;
			}
			case ERROR_CODE_CAPTCHABADRESPONSE:
			{
				ValidationFailures validationFailures;
				validationFailures.AddFailure("captchaResponse", "captchabadresponse");
				BMessage validationFailuresMessage;
				validationFailures.Archive(&validationFailuresMessage);
				BMessage message(MSG_CREATE_ACCOUNT_FAILED);
				message.AddMessage(KEY_VALIDATION_FAILURES,
					&validationFailuresMessage);
				messenger.SendMessage(&message);
				break;
			}
			case ERROR_CODE_VALIDATION:
			{
				ValidationFailures validationFailures;
				ServerHelper::GetFailuresFromJsonRpcError(validationFailures,
					responsePayload);
				if (Logger::IsDebugEnabled()) {
					BString debugString;
					_ValidationFailuresToString(validationFailures,
						debugString);
					HDDEBUG("create account validation issues; %s",
						debugString.String());
				}
				BMessage validationFailuresMessage;
				validationFailures.Archive(&validationFailuresMessage);
				BMessage message(MSG_CREATE_ACCOUNT_FAILED);
				message.AddMessage(KEY_VALIDATION_FAILURES,
					&validationFailuresMessage);
				messenger.SendMessage(&message);
				break;
			}
			default:
				ServerHelper::NotifyServerJsonRpcError(responsePayload);
				messenger.SendMessage(MSG_CREATE_ACCOUNT_ERROR);
				break;
		}
	} else {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("User creation error"),
			B_TRANSLATE("It was not possible to create the new user."));
		messenger.SendMessage(MSG_CREATE_ACCOUNT_ERROR);
	}
}


void
UserLoginWindow::_HandleCreateAccountSuccess(
	const UserCredentials& credentials)
{
	BString message = B_TRANSLATE("The user %Nickname% has been successfully "
		"created in the HaikuDepotServer system. You can administer your user "
		"details by using the web interface. You are now logged-in as this "
		"new user.");
	message.ReplaceAll("%Nickname%", credentials.Nickname());

	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("User Created"), message, B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();

	_TakeUpCredentialsAndQuit(credentials);
}


void
UserLoginWindow::_HandleCreateAccountFailure(const ValidationFailures& failures)
{
	_MarkCreateUserInvalidFields(failures);
	_AlertCreateUserValidationFailure(failures);
	_EnableMutableControls(true);

	// if an attempt was made to the server then the captcha would have been
	// used up and a new captcha is required.
	_CreateAccountSetup(CREATE_CAPTCHA);
}


/*!	Handles the main UI-thread processing for the situation where there was an
	unexpected error when creating the account.  Note that any error messages
	presented to the user are expected to be prepared and initiated from the
	background thread creating the account.
*/

void
UserLoginWindow::_HandleCreateAccountError()
{
	_EnableMutableControls(true);
}


/*!	Opens a new window that shows the already downloaded user usage conditions.
*/

void
UserLoginWindow::_ViewUserUsageConditions()
{
	if (fUserUsageConditions == NULL)
		debugger("the usage conditions should be set");
	UserUsageConditionsWindow* window = new UserUsageConditionsWindow(
		fModel, *fUserUsageConditions);
	window->Show();
}
