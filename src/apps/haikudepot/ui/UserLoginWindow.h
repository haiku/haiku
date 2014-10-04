/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_LOGIN_WINDOW_H
#define USER_LOGIN_WINDOW_H

#include <Locker.h>
#include <Messenger.h>
#include <Window.h>

#include "PackageInfo.h"


class BButton;
class BMenuField;
class BTabView;
class BTextControl;
class BitmapView;
class Model;


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
			void				_RequestCaptcha();
			void				_LoginSuccessful(const BString& message);

			void				_SetWorkerThread(thread_id thread);

	static	int32				_AuthenticateThreadEntry(void* data);
			void				_AuthenticateThread();

	static	int32				_RequestCaptchaThreadEntry(void* data);
			void				_RequestCaptchaThread();

	static	int32				_CreateAccountThreadEntry(void* data);
			void				_CreateAccountThread();

			void				_CollectValidationFailures(
									const BMessage& result,
									BString& error) const;

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

			BButton*			fSendButton;
			BButton*			fCancelButton;

			BString				fCaptchaToken;
			BitmapRef			fCaptchaImage;
			BString				fPreferredLanguage;

			Model&				fModel;

			Mode				fMode;

			BLocker				fLock;
			thread_id			fWorkerThread;
};


#endif // USER_LOGIN_WINDOW_H
