/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_LOGIN_WINDOW_H
#define USER_LOGIN_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "CreateUserDetail.h"
#include "PackageInfo.h"
#include "UserCredentials.h"
#include "ValidationFailure.h"


class BButton;
class BCheckBox;
class BMenuField;
class BTabView;
class BTextControl;
class BitmapView;
class Captcha;
class LinkView;
class Model;
class UserUsageConditions;


class UserLoginWindow : public BWindow {
public:
								UserLoginWindow(BWindow* parent, BRect frame,
									Model& model);
	virtual						~UserLoginWindow();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				SetOnSuccessMessage(
									const BMessenger& messenger,
									const BMessage& message);

private:

			enum Mode {
				NONE = 0,
				LOGIN,
				CREATE_ACCOUNT
			};

			void				_SetMode(Mode mode);
			void				_SetWorkerThread(thread_id thread);
			void				_SetWorkerThreadLocked(thread_id thread);

			void				_Authenticate();
			void				_Authenticate(
									const UserCredentials& credentials);
	static	int32				_AuthenticateThreadEntry(void* data);
			void				_AuthenticateThread(
									UserCredentials& credentials);
			void				_UnpackAuthenticationToken(
									BMessage& responsePayload, BString& token);
			void				_HandleAuthenticationFailed();
			void				_HandleAuthenticationSuccess(
									const UserCredentials & credentials);
			void				_HandleAuthenticationError();

			void				_CreateAccount();
			void				_AssembleCreateUserDetail(
									CreateUserDetail& detail);
			void				_ValidateCreateUserDetail(
									CreateUserDetail& detail,
									ValidationFailures& failures);
			void				_AlertCreateUserValidationFailure(
									const ValidationFailures& failures);
	static	BString				_CreateAlertTextFromValidationFailure(
									const BString& property,
									const BString& message);
			void				_MarkCreateUserInvalidFields();
			void				_MarkCreateUserInvalidFields(
									const ValidationFailures& failures);
	static	int32				_CreateAccountThreadEntry(void* data);
			void				_CreateAccountThread(CreateUserDetail* detail);
			void				_HandleCreateAccountSuccess(
									const UserCredentials& credentials);
			void				_HandleCreateAccountFailure(
									const ValidationFailures& failures);
			void				_HandleCreateAccountError();

			void				_CreateAccountSetup(uint32 mask);
			void				_CreateAccountSetupIfNecessary();
	static	int32				_CreateAccountSetupThreadEntry(void* data);
			status_t			_CreateAccountCaptchaSetupThread(
									Captcha& captcha);
			status_t			_CreateAccountUserUsageConditionsSetupThread(
									UserUsageConditions& userUsageConditions);
			status_t			_UnpackCaptcha(BMessage& responsePayload,
									Captcha& captcha);
			void				_HandleCreateAccountSetupSuccess(
									BMessage* message);

			void				_SetCaptcha(Captcha* captcha);
			void				_SetUserUsageConditions(
									UserUsageConditions* userUsageConditions);

			void				_CollectValidationFailures(
									const BMessage& result,
									BString& error) const;

			void				_ViewUserUsageConditions();

			void				_TakeUpCredentialsAndQuit(
									const UserCredentials& credentials);

			void				_EnableMutableControls(bool enabled);

	static	void				_ValidationFailuresToString(
									const ValidationFailures& failures,
									BString& output);

private:
			BMessenger			fOnSuccessTarget;
			BMessage			fOnSuccessMessage;

			BTabView*			fTabView;

			BTextControl*		fNicknameField;
			BTextControl*		fPasswordField;

			BTextControl*		fNewNicknameField;
			BTextControl*		fNewPasswordField;
			BTextControl*		fRepeatPasswordField;
			BTextControl*		fEmailField;
			BMenuField*			fLanguageCodeField;
			BitmapView*			fCaptchaView;
			BTextControl*		fCaptchaResultField;
			BCheckBox*			fConfirmMinimumAgeCheckBox;
			BCheckBox*			fConfirmUserUsageConditionsCheckBox;
			LinkView*			fUserUsageConditionsLink;

			BButton*			fSendButton;
			BButton*			fCancelButton;

			UserUsageConditions*
								fUserUsageConditions;
			Captcha*			fCaptcha;
			BString				fPreferredLanguageCode;

			Model&				fModel;

			Mode				fMode;

			BLocker				fLock;
			thread_id			fWorkerThread;
			bool				fQuitRequestedDuringWorkerThread;
};


#endif // USER_LOGIN_WINDOW_H
