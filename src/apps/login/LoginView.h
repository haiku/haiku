/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOGINVIEW_H_
#define _LOGINVIEW_H_

#include <Button.h>
#include <CheckBox.h>
#include <View.h>
#include <ListItem.h>
#include <ListView.h>
#include <StringView.h>
#include <TextControl.h>

const uint32 kUserSelected = 'usel';
const uint32 kUserInvoked = 'uinv';
const uint32 kLoginEdited = 'logc';
const uint32 kPasswordEdited = 'pasc';
const uint32 kHidePassword = 'hidp';
const uint32 kAddNextUser = 'adnu';
const uint32 kSetProgress = 'setp';

class LoginView : public BView {
public:
				LoginView(BRect frame);
	virtual		~LoginView();
	void		AttachedToWindow();
	void		MessageReceived(BMessage *message);
	void		Pulse();

private:
	void		AddNextUser();
	void		EnableControls(bool enable);

	BListView*		fUserList;
	BTextControl*	fLoginControl;
	BTextControl*	fPasswordControl;
	BCheckBox*		fHidePasswordCheckBox;
	BButton*		fHaltButton;
	BButton*		fRebootButton;
	BButton*		fLoginButton;
	BStringView*	fInfoView;
};


#endif	// _LOGINVIEW_H_
