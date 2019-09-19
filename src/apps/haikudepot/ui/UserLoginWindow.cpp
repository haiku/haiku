/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserLoginWindow.h"

#include <algorithm>
#include <ctype.h>
#include <stdio.h>

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
#include <UnicodeChar.h>

#include "AppUtils.h"
#include "BitmapView.h"
#include "HaikuDepotConstants.h"
#include "LanguageMenuUtils.h"
#include "LinkView.h"
#include "Model.h"
#include "TabView.h"
#include "UserUsageConditions.h"
#include "UserUsageConditionsWindow.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserLoginWindow"

#define PLACEHOLDER_TEXT B_UTF8_ELLIPSIS

enum ActionTabs {
	TAB_LOGIN					= 0,
	TAB_CREATE_ACCOUNT			= 1
};

enum {
	MSG_SEND					= 'send',
	MSG_TAB_SELECTED			= 'tbsl',
	MSG_CAPTCHA_OBTAINED		= 'cpob',
	MSG_VALIDATE_FIELDS			= 'vldt'
};

/*! The creation of an account requires that some prerequisite data is first
    loaded in or may later need to be refreshed.  This enum controls what
    elements of the setup should be performed.
*/

enum CreateAccountSetupMask {
	CREATE_CAPTCHA					= 1 << 1,
	FETCH_USER_USAGE_CONDITIONS		= 1 << 2
};

/*! A background thread runs to gather data to use in the interface for creating
    a new user.  This structure is passed to the background thread.
*/

struct CreateAccountSetupThreadData {
	UserLoginWindow*	window;
	uint32				mask;
		// defines what setup steps are required
};


UserLoginWindow::UserLoginWindow(BWindow* parent, BRect frame, Model& model)
	:
	BWindow(frame, B_TRANSLATE("Log in"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fPreferredLanguageCode(LANGUAGE_DEFAULT_CODE),
	fModel(model),
	fMode(NONE),
	fUserUsageConditions(NULL),
	fWorkerThread(-1)
{
	AddToSubset(parent);

	fUsernameField = new BTextControl(B_TRANSLATE("User name:"), "", NULL);
	fPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "", NULL);
	fPasswordField->TextView()->HideTyping(true);

	fNewUsernameField = new BTextControl(B_TRANSLATE("User name:"), "",
		NULL);
	fNewPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "",
		new BMessage(MSG_VALIDATE_FIELDS));
	fNewPasswordField->TextView()->HideTyping(true);
	fRepeatPasswordField = new BTextControl(B_TRANSLATE("Repeat pass phrase:"),
		"", new BMessage(MSG_VALIDATE_FIELDS));
	fRepeatPasswordField->TextView()->HideTyping(true);

	{
		AutoLocker<BLocker> locker(fModel.Lock());
		fPreferredLanguageCode = fModel.Language().PreferredLanguage().Code();
		// Construct languages popup
		BPopUpMenu* languagesMenu = new BPopUpMenu(B_TRANSLATE("Language"));
		fLanguageCodeField = new BMenuField("language",
			B_TRANSLATE("Preferred language:"), languagesMenu);

		LanguageMenuUtils::AddLanguagesToMenu(
			fModel.Language().SupportedLanguages(),
			languagesMenu);
		languagesMenu->SetTargetForItems(this);

		printf("using preferred language code [%s]\n",
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
	fNewUsernameField->SetModificationMessage(
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
		.AddTextControl(fUsernameField, 0, 0)
		.AddTextControl(fPasswordField, 0, 1)
		.AddGlue(0, 2)

		.SetInsets(B_USE_DEFAULT_SPACING)
	;
	fTabView->AddTab(loginCard);

	BGridView* createAccountCard = new BGridView(B_TRANSLATE("Create account"));
	BLayoutBuilder::Grid<>(createAccountCard)
		.AddTextControl(fNewUsernameField, 0, 0)
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
			_ValidateCreateAccountFields();
			break;

		case MSG_VIEW_LATEST_USER_USAGE_CONDITIONS:
			_ViewUserUsageConditions();
			break;

		case MSG_SEND:
			switch (fMode) {
				case LOGIN:
					_Login();
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

		case MSG_CAPTCHA_OBTAINED:
			if (fCaptchaImage.Get() != NULL) {
				fCaptchaView->SetBitmap(fCaptchaImage);
			} else {
				fCaptchaView->UnsetBitmap();
			}
			fCaptchaResultField->SetText("");
			break;

		case MSG_USER_USAGE_CONDITIONS_DATA:
			_SetUserUsageConditions(new UserUsageConditions(message));
			break;

		case MSG_LANGUAGE_SELECTED:
			message->FindString("code", &fPreferredLanguageCode);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
UserLoginWindow::SetOnSuccessMessage(
	const BMessenger& messenger, const BMessage& message)
{
	fOnSuccessTarget = messenger;
	fOnSuccessMessage = message;
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
			fUsernameField->MakeFocus();
			break;
		case CREATE_ACCOUNT:
			fTabView->Select(TAB_CREATE_ACCOUNT);
			fSendButton->SetLabel(B_TRANSLATE("Create account"));
			_CreateAccountSetupIfNecessary();
			fNewUsernameField->MakeFocus();
			_ValidateCreateAccountFields();
			break;
		default:
			break;
	}
}


static int32
count_digits(const BString& string)
{
	int32 digits = 0;
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		uint32 unicodeChar = BUnicodeChar::FromUTF8(&c);
		if (BUnicodeChar::IsDigit(unicodeChar))
			digits++;
	}
	return digits;
}


static int32
count_upper_case_letters(const BString& string)
{
	int32 upperCaseLetters = 0;
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		uint32 unicodeChar = BUnicodeChar::FromUTF8(&c);
		if (BUnicodeChar::IsUpper(unicodeChar))
			upperCaseLetters++;
	}
	return upperCaseLetters;
}


static bool
contains_any_whitespace(const BString& string)
{
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		if (isspace(c[i]))
			return true;
	}

	return false;
}


/*! This method will check that the inputs in the user interface are, as far as
    the desktop application can be sure, correct.
    \return true if the data in the form was valid.
*/

bool
UserLoginWindow::_ValidateCreateAccountFields(bool alertProblems)
{
	BString nickName(fNewUsernameField->Text());
	BString password1(fNewPasswordField->Text());
	BString password2(fRepeatPasswordField->Text());
	BString email(fEmailField->Text());
	BString captcha(fCaptchaResultField->Text());
	bool minimumAgeConfirmed = fConfirmMinimumAgeCheckBox->Value()
		== B_CONTROL_ON;
	bool userUsageConditionsConfirmed =
		fConfirmUserUsageConditionsCheckBox->Value() == B_CONTROL_ON;

	// TODO: Use the same validation as the web-serivce

	bool validEmail = email.IsEmpty() ||
		(B_ERROR != email.FindFirst("@") && !contains_any_whitespace(email));
	fEmailField->MarkAsInvalid(!validEmail);

	bool validUserName = nickName.Length() >= 3;
	fNewUsernameField->MarkAsInvalid(!validUserName);

	bool validPassword = password1.Length() >= 8
		&& count_digits(password1) >= 2
		&& count_upper_case_letters(password1) >= 2;
	fNewPasswordField->MarkAsInvalid(!validPassword);
	fRepeatPasswordField->MarkAsInvalid(password1 != password2);

	bool validCaptcha = captcha.Length() > 0;
	fCaptchaResultField->MarkAsInvalid(!validCaptcha);

	bool valid = validUserName && validPassword && password1 == password2
		&& validCaptcha && minimumAgeConfirmed && userUsageConditionsConfirmed
		&& validEmail;

	if (!valid && alertProblems) {
		BString message = B_TRANSLATE("There are problems in the form:\n\n");

		if (!validUserName) {
			message << B_TRANSLATE(
				"The user name needs to be at least "
				"3 letters long.") << "\n\n";
		}
		if (!validPassword) {
			message << B_TRANSLATE(
				"The password is too weak or invalid. "
				"Please use at least 8 characters with "
				"at least 2 numbers and 2 upper-case "
				"letters.") << "\n\n";
		}
		if (password1 != password2) {
			message << B_TRANSLATE(
				"The passwords do not match.") << "\n\n";
		}
		if (email.Length() == 0) {
			message << B_TRANSLATE(
				"If you do not provide an email address, "
				"you will not be able to reset your password "
				"if you forget it.") << "\n\n";
		}
		if (!validCaptcha) {
			message << B_TRANSLATE(
				"The captcha puzzle needs to be solved.") << "\n\n";
		}

		if (!minimumAgeConfirmed) {
			message << B_TRANSLATE(
				"The minimum age requirements must be met.") << "\n\n";
		}

		if (!userUsageConditionsConfirmed) {
			message << B_TRANSLATE(
				"The usage conditions must be agreed to.") << "\n\n";
		}

		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Input validation"),
			message,
			B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (alert != NULL)
			alert->Go();
	}

	return valid;
}


void
UserLoginWindow::_Login()
{
	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		return;

	thread_id thread = spawn_thread(&_AuthenticateThreadEntry,
		"Authenticator", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		_SetWorkerThread(thread);
}


void
UserLoginWindow::_CreateAccount()
{
	if (!_ValidateCreateAccountFields(true))
		return;

	BAutolock locker(&fLock);

	if (fWorkerThread >= 0)
		return;

	thread_id thread = spawn_thread(&_CreateAccountThreadEntry,
		"Account creator", B_NORMAL_PRIORITY, this);
	if (thread >= 0)
		_SetWorkerThread(thread);
}


void
UserLoginWindow::_CreateAccountSetupIfNecessary()
{
	uint32 setupMask = 0;
	if (fCaptchaToken.IsEmpty())
		setupMask |= CREATE_CAPTCHA;
	if (fUserUsageConditions == NULL)
		setupMask |= FETCH_USER_USAGE_CONDITIONS;
	_CreateAccountSetup(setupMask);
}


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

	if ((mask & CREATE_CAPTCHA) != 0) {
		fCaptchaToken = "";
		fCaptchaView->UnsetBitmap();
		fCaptchaImage.Unset();
	}

	if ((mask & FETCH_USER_USAGE_CONDITIONS) != 0) {
		fConfirmMinimumAgeCheckBox->SetLabel(PLACEHOLDER_TEXT);
		fConfirmMinimumAgeCheckBox->SetValue(0);
		fConfirmUserUsageConditionsCheckBox->SetValue(0);
	}

	Unlock();

	CreateAccountSetupThreadData* threadData = new CreateAccountSetupThreadData;
	threadData->window = this;
	threadData->mask = mask;

	thread_id thread = spawn_thread(&_CreateAccountSetupThreadEntry,
		"Create account setup", B_NORMAL_PRIORITY, threadData);
	if (thread >= 0)
		_SetWorkerThread(thread);
	else {
		debugger("unable to start a thread to gather data for creating an "
			"account");
	}
}


void
UserLoginWindow::_LoginSuccessful(const BString& message)
{
	// Clone these fields before the window goes away.
	// (This method is executd from another thread.)
	BMessenger onSuccessTarget(fOnSuccessTarget);
	BMessage onSuccessMessage(fOnSuccessMessage);

	BMessenger(this).SendMessage(B_QUIT_REQUESTED);

	BAlert* alert = new(std::nothrow) BAlert(
		B_TRANSLATE("Success"),
		message,
		B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();

	// Send the success message after the alert has been closed,
	// otherwise more windows will popup alongside the alert.
	if (onSuccessTarget.IsValid() && onSuccessMessage.what != 0)
		onSuccessTarget.SendMessage(&onSuccessMessage);
}


void
UserLoginWindow::_SetWorkerThread(thread_id thread)
{
	if (!Lock())
		return;

	bool enabled = thread < 0;

	fUsernameField->SetEnabled(enabled);
	fPasswordField->SetEnabled(enabled);
	fNewUsernameField->SetEnabled(enabled);
	fNewPasswordField->SetEnabled(enabled);
	fRepeatPasswordField->SetEnabled(enabled);
	fEmailField->SetEnabled(enabled);
	fLanguageCodeField->SetEnabled(enabled);
	fCaptchaResultField->SetEnabled(enabled);
	fConfirmMinimumAgeCheckBox->SetEnabled(enabled);
	fConfirmUserUsageConditionsCheckBox->SetEnabled(enabled);
	fUserUsageConditionsLink->SetEnabled(enabled);
	fSendButton->SetEnabled(enabled);

	if (thread >= 0) {
		fWorkerThread = thread;
		resume_thread(fWorkerThread);
	} else {
		fWorkerThread = -1;
	}

	Unlock();
}


void
UserLoginWindow::_CreateAccountUserUsageConditionsSetupThread()
{
	UserUsageConditions conditions;
	WebAppInterface interface = fModel.GetWebAppInterface();

	if (interface.RetrieveUserUsageConditions(NULL, conditions) == B_OK) {
		BMessage dataMessage(MSG_USER_USAGE_CONDITIONS_DATA);
		conditions.Archive(&dataMessage, true);
		BMessenger(this).SendMessage(&dataMessage);
	} else {
		AppUtils::NotifySimpleError(
			B_TRANSLATE("Usage conditions download problem"),
			B_TRANSLATE("An error has arisen downloading the usage "
				"conditions. Check the log for details and try again."));
		BMessenger(this).SendMessage(B_QUIT_REQUESTED);
	}
}


/*! This method is hit when the user usage conditions data arrives back from the
    server.  At this point some of the UI elements may need to be updated.
*/

void
UserLoginWindow::_SetUserUsageConditions(
	UserUsageConditions* userUsageConditions)
{
	fUserUsageConditions = userUsageConditions;
	BString minimumAgeString;
	minimumAgeString.SetToFormat("%" B_PRId8,
		fUserUsageConditions->MinimumAge());
	BString label = B_TRANSLATE(
		"I am %MinimumAgeYears% years of age or older");
	label.ReplaceAll("%MinimumAgeYears%", minimumAgeString);
	fConfirmMinimumAgeCheckBox->SetLabel(label);
}


int32
UserLoginWindow::_AuthenticateThreadEntry(void* data)
{
	UserLoginWindow* window = reinterpret_cast<UserLoginWindow*>(data);
	window->_AuthenticateThread();
	return 0;
}


void
UserLoginWindow::_AuthenticateThread()
{
	if (!Lock())
		return;

	BString nickName(fUsernameField->Text());
	BString passwordClear(fPasswordField->Text());

	Unlock();

	WebAppInterface interface;
	BMessage info;

	status_t status = interface.AuthenticateUser(
		nickName, passwordClear, info);

	BString error = B_TRANSLATE("Authentication failed. "
		"Connection to the service failed.");

	BMessage result;
	if (status == B_OK && info.FindMessage("result", &result) == B_OK) {
		BString token;
		if (result.FindString("token", &token) == B_OK && !token.IsEmpty()) {
			// We don't care for or store the token for now. The web-service
			// supports two methods of authorizing requests. One is via
			// Basic Authentication in the HTTP header, the other is via
			// Token Bearer. Since the connection is encrypted, it is hopefully
			// ok to send the password with each request instead of implementing
			// the Token Bearer. See section 5.1.2 in the haiku-depot-web
			// documentation.
			error = "";
			fModel.SetAuthorization(nickName, passwordClear, true);
		} else {
			error = B_TRANSLATE("Authentication failed. The user does "
				"not exist or the wrong password was supplied.");
		}
	}

	if (!error.IsEmpty()) {
		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Authentication failed"),
			error,
			B_TRANSLATE("Close"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (alert != NULL)
			alert->Go();

		_SetWorkerThread(-1);
	} else {
		_SetWorkerThread(-1);
		_LoginSuccessful(B_TRANSLATE("The authentication was successful."));
	}
}


int32
UserLoginWindow::_CreateAccountSetupThreadEntry(void* data)
{
	CreateAccountSetupThreadData* threadData =
		reinterpret_cast<CreateAccountSetupThreadData*>(data);
	if ((threadData->mask & CREATE_CAPTCHA) != 0)
		threadData->window->_CreateAccountCaptchaSetupThread();
	if ((threadData->mask & FETCH_USER_USAGE_CONDITIONS) != 0)
		threadData->window->_CreateAccountUserUsageConditionsSetupThread();
	threadData->window->_SetWorkerThread(-1);
	delete threadData;
	return 0;
}


void
UserLoginWindow::_CreateAccountCaptchaSetupThread()
{
	WebAppInterface interface;
	BMessage info;

	status_t status = interface.RequestCaptcha(info);

	BAutolock locker(&fLock);

	BMessage result;
	if (status == B_OK && info.FindMessage("result", &result) == B_OK) {
		result.FindString("token", &fCaptchaToken);
		BString imageDataBase64;
		if (result.FindString("pngImageDataBase64", &imageDataBase64) == B_OK) {
			ssize_t encodedSize = imageDataBase64.Length();
			ssize_t decodedSize = (encodedSize * 3 + 3) / 4;
			if (decodedSize > 0) {
				char* buffer = new char[decodedSize];
				decodedSize = decode_base64(buffer, imageDataBase64.String(),
					encodedSize);
				if (decodedSize > 0) {
					BMemoryIO memoryIO(buffer, (size_t)decodedSize);
					fCaptchaImage.SetTo(new(std::nothrow) SharedBitmap(
						memoryIO), true);
					BMessenger(this).SendMessage(MSG_CAPTCHA_OBTAINED);
				} else {
					fprintf(stderr, "Failed to decode captcha: %s\n",
						strerror(decodedSize));
				}
				delete[] buffer;
			}
		}
	} else {
		fprintf(stderr, "Failed to obtain captcha: %s\n", strerror(status));
	}
}


int32
UserLoginWindow::_CreateAccountThreadEntry(void* data)
{
	UserLoginWindow* window = reinterpret_cast<UserLoginWindow*>(data);
	window->_CreateAccountThread();
	return 0;
}


void
UserLoginWindow::_CreateAccountThread()
{
	if (!Lock())
		return;

	if (fUserUsageConditions == NULL)
		debugger("missing usage conditions when creating an account");

	if (fConfirmMinimumAgeCheckBox->Value() == 0
		|| fConfirmUserUsageConditionsCheckBox->Value() == 0) {
		debugger("expected that the minimum age and usage conditions are "
			"agreed to at this point");
	}

	BString nickName(fNewUsernameField->Text());
	BString passwordClear(fNewPasswordField->Text());
	BString email(fEmailField->Text());
	BString captchaToken(fCaptchaToken);
	BString captchaResponse(fCaptchaResultField->Text());
	BString languageCode(fPreferredLanguageCode);
	BString userUsageConditionsCode(fUserUsageConditions->Code());

	Unlock();

	WebAppInterface interface;
	BMessage info;

	status_t status = interface.CreateUser(
		nickName, passwordClear, email, captchaToken, captchaResponse,
		languageCode, userUsageConditionsCode, info);

	BAutolock locker(&fLock);

	BString error = B_TRANSLATE(
		"There was a puzzling response from the web service.");

	BMessage result;
	if (status == B_OK) {
		if (info.FindMessage("result", &result) == B_OK) {
			error = "";
		} else if (info.FindMessage("error", &result) == B_OK) {
			result.PrintToStream();
			BString message;
			if (result.FindString("message", &message) == B_OK) {
				if (message == "captchabadresponse") {
					error = B_TRANSLATE("You have not solved the captcha "
						"puzzle correctly.");
				} else if (message == "validationerror") {
					_CollectValidationFailures(result, error);
				} else {
					BString response = B_TRANSLATE("It responded with: %message%");
					response.ReplaceFirst("%message%", message);
					error << " " << response;
				}
			}
		}
	} else {
		error = B_TRANSLATE(
			"It was not possible to contact the web service.");
	}

	locker.Unlock();

	if (!error.IsEmpty()) {
		BAlert* alert = new(std::nothrow) BAlert(
			B_TRANSLATE("Failed to create account"),
			error,
			B_TRANSLATE("Close"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (alert != NULL)
			alert->Go();

		fprintf(stderr,
			B_TRANSLATE("Failed to create account: %s\n"), error.String());

		_SetWorkerThread(-1);

		// We need a new captcha, it can be used only once
		fCaptchaToken = "";
		_CreateAccountSetup(CREATE_CAPTCHA);
	} else {
		fModel.SetAuthorization(nickName, passwordClear, true);

		_SetWorkerThread(-1);
		_LoginSuccessful(B_TRANSLATE("Account created successfully. "
			"You can now rate packages and do other useful things."));
	}
}


void
UserLoginWindow::_CollectValidationFailures(const BMessage& result,
	BString& error) const
{
	error = B_TRANSLATE("There are problems with the data you entered:\n\n");

	bool found = false;

	BMessage data;
	BMessage failures;
	if (result.FindMessage("data", &data) == B_OK
		&& data.FindMessage("validationfailures", &failures) == B_OK) {
		int32 index = 0;
		while (true) {
			BString name;
			name << index++;
			BMessage failure;
			if (failures.FindMessage(name, &failure) != B_OK)
				break;

			BString property;
			BString message;
			if (failure.FindString("property", &property) == B_OK
				&& failure.FindString("message", &message) == B_OK) {
				found = true;
				if (property == "nickname" && message == "notunique") {
					error << B_TRANSLATE(
						"The username is already taken. "
						"Please choose another.");
				} else if (property == "passwordClear"
					&& message == "invalid") {
					error << B_TRANSLATE(
						"The password is too weak or invalid. "
						"Please use at least 8 characters with "
						"at least 2 numbers and 2 upper-case "
						"letters.");
				} else if (property == "email" && message == "malformed") {
					error << B_TRANSLATE(
						"The email address appears to be malformed.");
				} else {
					error << property << ": " << message;
				}
			}
		}
	}

	if (!found) {
		error << B_TRANSLATE("But none could be listed here, sorry.");
	}
}


/*! Opens a new window that shows the already downloaded user usage conditions.
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