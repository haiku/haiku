/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Julun, host.haiku@gmx.de
 *		Michael Phipps
 *		John Scipione, jscipione@gmail.com
 */


#include "PasswordWindow.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <RadioButton.h>
#include <Screen.h>
#include <Size.h>
#include <TextControl.h>

#include <ctype.h>

#include "ScreenSaverSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenSaver"


static const uint32 kPasswordTextWidth = 12;

static const uint32 kMsgDone = 'done';
static const uint32 kMsgPasswordTypeChanged = 'pwtp';


PasswordWindow::PasswordWindow(ScreenSaverSettings& settings) 
	:
	BWindow(BRect(100, 100, 300, 200), B_TRANSLATE("Password Window"),
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fSettings(settings)
{
	_Setup();
	Update();
}


void 
PasswordWindow::_Setup()
{
	float spacing = be_control_look->DefaultItemSpacing();

	BView* topView = new BView("topView", B_WILL_DRAW);
	topView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	topView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BBox* networkBox = new BBox("networkBox");
	networkBox->SetBorder(B_NO_BORDER);

	fUseNetwork = new BRadioButton("useNetwork",
		B_TRANSLATE("Use network password"),
		new BMessage(kMsgPasswordTypeChanged));
	networkBox->SetLabel(fUseNetwork);

	BBox* customBox = new BBox("customBox");

	fUseCustom = new BRadioButton("useCustom",
		B_TRANSLATE("Use custom password"),
		new BMessage(kMsgPasswordTypeChanged));
	customBox->SetLabel(fUseCustom);

	fPasswordControl = new BTextControl("passwordTextView",
		B_TRANSLATE("Password:"), B_EMPTY_STRING, NULL);
	fPasswordControl->TextView()->HideTyping(true);
	fPasswordControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	BLayoutItem* passwordTextView
		= fPasswordControl->CreateTextViewLayoutItem();
	passwordTextView->SetExplicitMinSize(BSize(spacing * kPasswordTextWidth,
		B_SIZE_UNSET));

	fConfirmControl = new BTextControl("confirmTextView", 
		B_TRANSLATE("Confirm password:"), B_EMPTY_STRING, NULL);
	fConfirmControl->SetExplicitMinSize(BSize(spacing * kPasswordTextWidth,
		B_SIZE_UNSET));
	fConfirmControl->TextView()->HideTyping(true);
	fConfirmControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	BLayoutItem* confirmTextView = fConfirmControl->CreateTextViewLayoutItem();
	confirmTextView->SetExplicitMinSize(BSize(spacing * kPasswordTextWidth,
		B_SIZE_UNSET));

	customBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_SMALL_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(fPasswordControl->CreateLabelLayoutItem(), 0, 0)
			.Add(passwordTextView, 1, 0)
			.Add(fConfirmControl->CreateLabelLayoutItem(), 0, 1)
			.Add(confirmTextView, 1, 1)
			.End()
		.View());

	BButton* doneButton = new BButton("done", B_TRANSLATE("Done"),
		new BMessage(kMsgDone));

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_CANCEL));

	BLayoutBuilder::Group<>(topView, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(networkBox)
		.Add(customBox)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(doneButton)
			.End()
		.End();

	doneButton->MakeDefault(true);

	SetLayout(new BGroupLayout(B_VERTICAL));
	GetLayout()->AddView(topView);
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


char*
PasswordWindow::_SanitizeSalt(const char* password)
{
	char* salt;

	uint8 length = strlen(password);

	if (length < 2)
		salt = new char[3];
	else
		salt = new char[length + 1];

	uint8 i = 0;
	uint8 j = 0;
	for (; i < length; i++) {
		if (isalnum(password[i]) || password[i] == '.' || password[i] == '/') {
			salt[j] = password[i];
			j++;
		}
	}

	/*
	 *	We need to pad the salt.
	 */ 
	while (j < 2) {
		salt[j] = '.';
		j++;
	}

	salt[j] = '\0';
	
	return salt;
}


void 
PasswordWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDone:
			fSettings.SetLockMethod(fUseCustom->Value() ? "custom" : "network");
			if (fUseCustom->Value()) {
				if (strcmp(fPasswordControl->Text(), fConfirmControl->Text())
						!= 0) {
					BAlert* alert = new BAlert("noMatch",
						B_TRANSLATE("Passwords don't match. Please try again."),
						B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
					break;
				}
				const char* salt = _SanitizeSalt(fPasswordControl->Text());
				fSettings.SetPassword(crypt(fPasswordControl->Text(), salt));
				delete[] salt;
			} else
				fSettings.SetPassword("");

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
 	}
}
