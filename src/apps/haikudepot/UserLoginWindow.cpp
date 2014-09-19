/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserLoginWindow.h"

#include <algorithm>
#include <stdio.h>

#include <mail_encoding.h>

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <TabView.h>
#include <TextControl.h>

#include "BitmapView.h"
#include "Model.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserLoginWindow"


enum {
	MSG_SEND					= 'send',
	MSG_TAB_SELECTED			= 'tbsl',
	MSG_CAPTCHA_OBTAINED		= 'cpob',
};


class TabView : public BTabView {
public:
	TabView(const BMessenger& target, const BMessage& message)
		:
		BTabView("tab view", B_WIDTH_FROM_WIDEST),
		fTarget(target),
		fMessage(message)
	{
	}

	virtual void Select(int32 tabIndex)
	{
		BTabView::Select(tabIndex);

		BMessage message(fMessage);
		message.AddInt32("tab index", tabIndex);
		fTarget.SendMessage(&message);
	}

private:
	BMessenger	fTarget;
	BMessage	fMessage;
};


UserLoginWindow::UserLoginWindow(BWindow* parent, BRect frame, Model& model)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Log in"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fMode(NONE),
	fRequestCaptchaThread(-1)
{
	AddToSubset(parent);

	fUsernameField = new BTextControl(B_TRANSLATE("User name:"), "", NULL);
	fPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "", NULL);

	fNewUsernameField = new BTextControl(B_TRANSLATE("User name:"), "", NULL);
	fNewPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "",
		NULL);
	fRepeatPasswordField = new BTextControl(B_TRANSLATE("Repeat pass phrase:"),
		"", NULL);
	fLanguageCodeField = new BTextControl(B_TRANSLATE("Language code:"),
		model.PreferredLanguage(), NULL);
	fEmailField = new BTextControl(B_TRANSLATE("Email address:"), "", NULL);
	fCaptchaView = new BitmapView("captcha view");
	fCaptchaResultField = new BTextControl("", "", NULL);

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
		.AddTextControl(fLanguageCodeField, 0, 4)
		.Add(fCaptchaView, 0, 5)
		.Add(fCaptchaResultField, 1, 5)

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
		.SetInsets(B_USE_DEFAULT_SPACING)
	;

	SetDefaultButton(fSendButton);
	
	// TODO: Set initial mode based on whether there is account info
	_SetMode(LOGIN);

	CenterIn(parent->Frame());
}


UserLoginWindow::~UserLoginWindow()
{
	BAutolock locker(&fLock);
	
	if (fRequestCaptchaThread >= 0)
		wait_for_thread(fRequestCaptchaThread, NULL);
}


void
UserLoginWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
					case 0:
						_SetMode(LOGIN);
						break;
					case 1:
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
				fCaptchaView->SetBitmap(
					fCaptchaImage->Bitmap(SharedBitmap::SIZE_ANY));
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
UserLoginWindow::_SetMode(Mode mode)
{
	if (fMode == mode)
		return;

	fMode = mode;

	switch (fMode) {
		case LOGIN:
			fTabView->Select((int32)0);
			fSendButton->SetLabel(B_TRANSLATE("Log in"));
			break;
		case CREATE_ACCOUNT:
			fTabView->Select(1);
			fSendButton->SetLabel(B_TRANSLATE("Create account"));
			if (fCaptchaToken.IsEmpty())
				_RequestCaptcha();
			break;
		default:
			break;
	}
}


void
UserLoginWindow::_Login()
{
	// TODO: Implement...
	BAlert* alert = new BAlert(B_TRANSLATE("Not implemented"),
		B_TRANSLATE("Sorry, while the web application would already support "
		"logging in, HaikuDepot was not yet updated to use "
		"this functionality."),
		B_TRANSLATE("Bummer"));
	alert->Go(NULL);

	PostMessage(B_QUIT_REQUESTED);
}


void
UserLoginWindow::_CreateAccount()
{
	// TODO: Implement...
	BAlert* alert = new BAlert(B_TRANSLATE("Not implemented"),
		B_TRANSLATE("Sorry, while the web application would already support "
		"creating accounts remotely, HaikuDepot was not yet updated to use "
		"this functionality."),
		B_TRANSLATE("Bummer"));
	alert->Go(NULL);

	PostMessage(B_QUIT_REQUESTED);
}


void
UserLoginWindow::_RequestCaptcha()
{
	BAutolock locker(&fLock);
	
	if (fRequestCaptchaThread >= 0)
		return;

	fRequestCaptchaThread = spawn_thread(&_RequestCaptchaThreadEntry,
		"Captcha requester", B_NORMAL_PRIORITY, this);
	if (fRequestCaptchaThread >= 0)
		resume_thread(fRequestCaptchaThread);

}


int32
UserLoginWindow::_RequestCaptchaThreadEntry(void* data)
{
	UserLoginWindow* window = reinterpret_cast<UserLoginWindow*>(data);
	window->_RequestCaptchaThread();
	return 0;
}


void
UserLoginWindow::_RequestCaptchaThread()
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

	fRequestCaptchaThread = -1;
}


