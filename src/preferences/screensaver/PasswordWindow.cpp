/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Julun <host.haiku@gmx.de>
 */

#include "PasswordWindow.h"


#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>


const uint32 kMsgDone = 'done';
const uint32 kMsgPasswordTypeChanged = 'pwtp';


PasswordWindow::PasswordWindow(ScreenSaverSettings& settings) 
	: BWindow(BRect(100, 100, 380, 249), "Password Window", B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
	fSettings(settings)
{
	_Setup();
	Update();

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
	BRect bounds = Bounds();
	BView* topView = new BView(bounds, "topView", B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	bounds.InsetBy(10.0, 10.0);
	fUseNetwork = new BRadioButton(bounds, "useNetwork", "Use Network password",
		new BMessage(kMsgPasswordTypeChanged), B_FOLLOW_NONE);
	topView->AddChild(fUseNetwork);
	fUseNetwork->ResizeToPreferred();
	fUseNetwork->MoveBy(10.0, 0.0);

	bounds.OffsetBy(0.0, fUseNetwork->Bounds().Height());
	BBox *customBox = new BBox(bounds, "customBox", B_FOLLOW_NONE);
	topView->AddChild(customBox);

	fUseCustom = new BRadioButton(BRect(), "useCustom", "Use custom password",
		new BMessage(kMsgPasswordTypeChanged), B_FOLLOW_NONE);
	customBox->SetLabel(fUseCustom);
	fUseCustom->ResizeToPreferred();

	fPasswordControl = new BTextControl(bounds, "passwordControl", "Password:", 
		NULL, B_FOLLOW_NONE);
	customBox->AddChild(fPasswordControl);
	fPasswordControl->ResizeToPreferred();
	fPasswordControl->TextView()->HideTyping(true);
	fPasswordControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	
	bounds.OffsetBy(0.0, fPasswordControl->Bounds().Height() + 5.0);
	fConfirmControl = new BTextControl(bounds, "confirmControl", 
		"Confirm password:", "VeryLongPasswordPossible", B_FOLLOW_NONE);
	customBox->AddChild(fConfirmControl);
	fConfirmControl->TextView()->HideTyping(true);
	fConfirmControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	float width, height;
	fConfirmControl->GetPreferredSize(&width, &height);
	fPasswordControl->ResizeTo(width, height);
	
	float divider = be_plain_font->StringWidth("Confirm password:") + 5.0;
	fConfirmControl->SetDivider(divider);
	fPasswordControl->SetDivider(divider);
	
	customBox->ResizeTo(fConfirmControl->Frame().right + 10.0,
		fConfirmControl->Frame().bottom + 10.0);
	
	BButton* button = new BButton(BRect(), "done", "Done", new BMessage(kMsgDone));
	topView->AddChild(button);
	button->ResizeToPreferred();

	BRect frame = customBox->Frame();
	button->MoveTo(frame.right - button->Bounds().Width(), frame.bottom + 10.0);

	frame = button->Frame();
	button->MakeDefault(true);

	button = new BButton(frame, "cancel", "Cancel", new BMessage(B_CANCEL));
	topView->AddChild(button);
	button->ResizeToPreferred();
	button->MoveBy(-(button->Bounds().Width() + 10.0), 0.0);

	ResizeTo(customBox->Frame().right + 10.0, frame.bottom + 10.0);
}


void 
PasswordWindow::Update() 
{
	if (fSettings.IsNetworkPassword())
		fUseNetwork->SetValue(B_CONTROL_ON);
	else
		fUseCustom->SetValue(B_CONTROL_ON);

	bool useNetPassword = (fUseCustom->Value() > 0);
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
				if (strcmp(fPasswordControl->Text(), fConfirmControl->Text())) {
					BAlert *alert = new BAlert("noMatch",
						"Passwords don't match. Try again.","OK");
					alert->Go();
					break;
				} 
				fSettings.SetPassword(crypt(fPasswordControl->Text(), 
					fPasswordControl->Text()));
			} else {
				fSettings.SetPassword("");
			}
			fPasswordControl->SetText("");
			fConfirmControl->SetText("");
			fSettings.Save();
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
