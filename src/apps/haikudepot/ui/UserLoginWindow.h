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

#include "PackageInfo.h"


class BButton;
class BCheckBox;
class BMenuField;
class BTabView;
class BTextControl;
class BitmapView;
class LinkView;
class Model;
class UserUsageConditions;


class UserLoginWindow : public BWindow {
public:
								UserLoginWindow(BWindow* parent, BRect frame,
									Model& model);
	virtual						~UserLoginWindow();

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
			bool				_ValidateCreateAccountFields(
									bool alertProblems = false);
			void				_Login();
			void				_CreateAccount();
			void				_CreateAccountSetup(uint32 mask);
			void				_CreateAccountSetupIfNecessary();
			void				_LoginSuccessful(const BString& message);

			void				_SetWorkerThread(thread_id thread);

	static	int32				_AuthenticateThreadEntry(void* data);
			void				_AuthenticateThread();

	static	int32				_CreateAccountSetupThreadEntry(void* data);
			void				_CreateAccountCaptchaSetupThread();
			void				_CreateAccountUserUsageConditionsSetupThread();

			void				_SetUserUsageConditions(
									UserUsageConditions* userUsageConditions);

	static	int32				_CreateAccountThreadEntry(void* data);
			void				_CreateAccountThread();

			void				_CollectValidationFailures(
									const BMessage& result,
									BString& error) const;

			void				_ViewUserUsageConditions();

private:
			BMessenger			fOnSuccessTarget;
			BMessage			fOnSuccessMessage;

			BTabView*			fTabView;

			BTextControl*		fUsernameField;
			BTextControl*		fPasswordField;

			BTextControl*		fNewUsernameField;
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

			BString				fCaptchaToken;
			BitmapRef			fCaptchaImage;
			BString				fPreferredLanguageCode;

			Model&				fModel;

			Mode				fMode;

			UserUsageConditions*
								fUserUsageConditions;

			BLocker				fLock;
			thread_id			fWorkerThread;
};


#endif // USER_LOGIN_WINDOW_H
