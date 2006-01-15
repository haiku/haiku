/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Phipps
 *              Jérôme Duval, jerome.duval@free.fr
 */

#include "PasswordWindow.h"
#include <stdio.h>
#include <unistd.h>
#include <Alert.h>
#include <Box.h>
#include <RadioButton.h>
#include <Screen.h>


PasswordWindow::PasswordWindow(ScreenSaverPrefs &prefs) 
	: BWindow(BRect(100,100,380,249),"",B_MODAL_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE),
	fPrefs(prefs)
{
	Setup();
	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint point;
	point.x = (screenFrame.Width() - Bounds().Width()) / 2;
	point.y = (screenFrame.Height() - Bounds().Height()) / 2;

	if (screenFrame.Contains(point))
		MoveTo(point);
}


void 
PasswordWindow::Setup() 
{
	BBox *owner=new BBox(Bounds(),"ownerView",B_FOLLOW_NONE,B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(owner);
	fUseNetwork=new BRadioButton(BRect(14,10,159,20),"useNetwork","Use Network password",new BMessage(kButton_changed),B_FOLLOW_NONE);
	owner->AddChild(fUseNetwork);
	fUseCustom=new BRadioButton(BRect(30,50,130,60),"fUseCustom","Use custom password",new BMessage(kButton_changed),B_FOLLOW_NONE);

	BBox *customBox=new BBox(BRect(9,30,269,105),"custBeBox",B_FOLLOW_NONE);
	customBox->SetLabel(fUseCustom);
	fPassword=new BTextControl(BRect(10,20,251,35),"pwdCntrl","Password:",NULL,B_FOLLOW_NONE);
	fConfirm=new BTextControl(BRect(10,45,251,60),"fConfirmCntrl","Confirm password:",NULL,B_FOLLOW_NONE);
	fPassword->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	float divider = be_plain_font->StringWidth("Confirm password:") + 5.0;
	fPassword->SetDivider(divider);
	fPassword->TextView()->HideTyping(true);
	fConfirm->SetAlignment(B_ALIGN_RIGHT,B_ALIGN_LEFT);
	fConfirm->SetDivider(divider);
	fConfirm->TextView()->HideTyping(true);
	customBox->AddChild(fPassword);
	customBox->AddChild(fConfirm);
	owner->AddChild(customBox);

	fDone = new BButton(BRect(194,118,269,129),"done","Done",new BMessage (kDone_clicked),B_FOLLOW_NONE);
	fCancel = new BButton(BRect(109,118,184,129),"cancel","Cancel",new BMessage (kCancel_clicked),B_FOLLOW_NONE);
	owner->AddChild(fDone);
	owner->AddChild(fCancel);
	fDone->MakeDefault(true);
	Update();
}


void 
PasswordWindow::Update() 
{
	if (fPrefs.IsNetworkPassword())
		fUseNetwork->SetValue(B_CONTROL_ON);
	else
		fUseCustom->SetValue(B_CONTROL_ON);
	bool useNetPassword=(fUseCustom->Value()>0);
	fConfirm->SetEnabled(useNetPassword);
	fPassword->SetEnabled(useNetPassword);
}


void 
PasswordWindow::MessageReceived(BMessage *message) 
{
	switch(message->what) {
	case kDone_clicked:
		fPrefs.SetLockMethod(fUseCustom->Value() ? "custom" : "network");
		if (fUseCustom->Value()) {
			if (strcmp(fPassword->Text(),fConfirm->Text())) {
				BAlert *alert=new BAlert("noMatch","Passwords don't match. Try again.","OK");
				alert->Go();
				break;
			} 
			fPrefs.SetPassword(crypt(fPassword->Text(), fPassword->Text()));
		} else {
			fPrefs.SetPassword("");
		}
		fPassword->SetText("");
		fConfirm->SetText("");
		Hide();
		break;
	case kCancel_clicked:
		fPassword->SetText("");
		fConfirm->SetText("");
		Hide();
		break;
	case kButton_changed:
		fPrefs.SetLockMethod(fUseCustom->Value()>0 ? "custom" : "network");
		Update();
		break;
	case kShow:
		Show();
		break;
	default:
		BWindow::MessageReceived(message);
		break;
 	}
}
