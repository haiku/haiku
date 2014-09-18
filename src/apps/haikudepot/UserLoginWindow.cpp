/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UserLoginWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <TabView.h>
#include <TextControl.h>

#include "BitmapView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UserLoginWindow"


enum {
	MSG_SEND					= 'send',
	MSG_TAB_SELECTED			= 'tbsl'
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


UserLoginWindow::UserLoginWindow(BWindow* parent, BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Log in"),
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE),
	fMode(NONE)
{
	AddToSubset(parent);

	fUsernameField = new BTextControl(B_TRANSLATE("User name:"), "", NULL);
	fPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "", NULL);

	fNewUsernameField = new BTextControl(B_TRANSLATE("User name:"), "", NULL);
	fNewPasswordField = new BTextControl(B_TRANSLATE("Pass phrase:"), "",
		NULL);
	fRepeatPasswordField = new BTextControl(B_TRANSLATE("Repeat pass phrase:"),
		"", NULL);
	fLanguageCodeField = new BTextControl(B_TRANSLATE("Language code:"), "",
		NULL);
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
		.AddTextControl(fLanguageCodeField, 0, 3)
		.Add(fCaptchaView, 0, 4)
		.Add(fCaptchaResultField, 1, 4)

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
}


int32
UserLoginWindow::_RequestCaptchaThreadEntry(void* data)
{
	// TODO: Use WebInterface to 
	return 0;
}

