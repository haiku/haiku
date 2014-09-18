/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_LOGIN_WINDOW_H
#define USER_LOGIN_WINDOW_H

#include <Window.h>

#include "PackageInfo.h"


class BButton;
class BTabView;
class BTextControl;
class BitmapView;


class UserLoginWindow : public BWindow {
public:
								UserLoginWindow(BWindow* parent, BRect frame);
	virtual						~UserLoginWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			enum Mode {
				NONE = 0,
				LOGIN,
				CREATE_ACCOUNT
			};

			void				_SetMode(Mode mode);
			void				_Login();
			void				_CreateAccount();
			void				_RequestCaptcha();

	static	int32				_RequestCaptchaThreadEntry(void* data);

private:
			BTabView*			fTabView;

			BTextControl*		fUsernameField;
			BTextControl*		fPasswordField;

			BTextControl*		fNewUsernameField;
			BTextControl*		fNewPasswordField;
			BTextControl*		fRepeatPasswordField;
			BTextControl*		fLanguageCodeField;
			BitmapView*			fCaptchaView;
			BTextControl*		fCaptchaResultField;

			BButton*			fSendButton;
			BButton*			fCancelButton;

			BString				fCaptchaToken;
			BitmapRef			fCaptchaImage;

			Mode				fMode;
};


#endif // USER_LOGIN_WINDOW_H
