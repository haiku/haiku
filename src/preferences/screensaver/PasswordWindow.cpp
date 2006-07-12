/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "PasswordWindow.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>

#include <stdio.h>
#include <unistd.h>


const uint32 kMsgDone = 'done';
const uint32 kMsgPasswordTypeChanged = 'pwtp';


PasswordWindow::PasswordWindow(ScreenSaverSettings& settings) 
	: BWindow(BRect(100, 100, 380, 249), "Password Window", B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fSettings(settings)
{
	_Setup();

	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint point;
	point.x = (screenFrame.Width() - Bounds().Width()) / 2;
	point.y = (screenFrame.Height() - Bounds().Height()) / 2;

	if (screenFrame.Contains(point))
		MoveTo(point);
}


void 
PasswordWindow::_Setup()
{
	BView* topView = new BView(Bounds(), "mainView", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	fUseNetwork = new BRadioButton(BRect(14,10,62+be_plain_font->StringWidth("Use Network password"),20), "useNetwork",
		"Use Network password", new BMessage(kMsgPasswordTypeChanged), B_FOLLOW_NONE);
	topView->AddChild(fUseNetwork);
	fUseCustom = new BRadioButton(BRect(30,50,36+be_plain_font->StringWidth("Use custom password"),60), "fUseCustom", 
		"Use custom password", new BMessage(kMsgPasswordTypeChanged), B_FOLLOW_NONE);

	BBox *customBox = new BBox(BRect(9,30,269,105), "custBeBox", B_FOLLOW_NONE);
	customBox->SetLabel(fUseCustom);
	fPasswordControl = new BTextControl(BRect(10,20,251,35), "pwdCntrl", "Password:", NULL, B_FOLLOW_NONE);
	fConfirmControl = new BTextControl(BRect(10,45,251,60), "fConfirmCntrl", "Confirm password:", NULL, B_FOLLOW_NONE);
	fPasswordControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	float divider = be_plain_font->StringWidth("Confirm password:") + 5.0;
	fPasswordControl->SetDivider(divider);
	fPasswordControl->TextView()->HideTyping(true);
	fConfirmControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fConfirmControl->SetDivider(divider);
	fConfirmControl->TextView()->HideTyping(true);
	customBox->AddChild(fPasswordControl);
	customBox->AddChild(fConfirmControl);
	topView->AddChild(customBox);

	BButton* button = new BButton(BRect(194,118,269,129), "done", "Done", new BMessage(kMsgDone), B_FOLLOW_NONE);
	topView->AddChild(button);
	button->MakeDefault(true);

	button = new BButton(BRect(109,118,184,129), "cancel", "Cancel", new BMessage(B_CANCEL), B_FOLLOW_NONE);
	topView->AddChild(button);

	Update();
}


void 
PasswordWindow::Update() 
{
	if (fSettings.IsNetworkPassword())
		fUseNetwork->SetValue(B_CONTROL_ON);
	else
		fUseCustom->SetValue(B_CONTROL_ON);
	bool useNetPassword=(fUseCustom->Value()>0);
	fConfirmControl->SetEnabled(useNetPassword);
	fPasswordControl->SetEnabled(useNetPassword);
}


void 
PasswordWindow::MessageReceived(BMessage *message) 
{
	switch (message->what) {
		case kMsgDone:
			fSettings.SetLockMethod(fUseCustom->Value() ? "custom" : "network");
			if (fUseCustom->Value()) {
				if (strcmp(fPasswordControl->Text(),fConfirmControl->Text())) {
					BAlert *alert=new BAlert("noMatch","Passwords don't match. Try again.","OK");
					alert->Go();
					break;
				} 
				fSettings.SetPassword(crypt(fPasswordControl->Text(), fPasswordControl->Text()));
			} else {
				fSettings.SetPassword("");
			}
			fPasswordControl->SetText("");
			fConfirmControl->SetText("");
			Hide();
			break;

	case B_CANCEL:
		fPasswordControl->SetText("");
		fConfirmControl->SetText("");
		Hide();
		break;

	case kMsgPasswordTypeChanged:
		fSettings.SetLockMethod(fUseCustom->Value() > 0 ? "custom" : "network");
		Update();
		break;

	default:
		BWindow::MessageReceived(message);
		break;
 	}
}
