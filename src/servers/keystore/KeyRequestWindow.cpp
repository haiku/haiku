/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KeyRequestWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Key.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include <new>


static const uint32 kMessageCancel = 'btcl';
static const uint32 kMessageUnlock = 'btul';


class KeyRequestView : public BView {
public:
	KeyRequestView()
		:
		BView("KeyRequestView", B_WILL_DRAW),
		fPassword(NULL)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BGroupLayout* rootLayout = new(std::nothrow) BGroupLayout(B_VERTICAL);
		if (rootLayout == NULL)
			return;

		SetLayout(rootLayout);

		BGridView* controls = new(std::nothrow) BGridView();
		if (controls == NULL)
			return;

		BGridLayout* layout = controls->GridLayout();

		float inset = ceilf(be_plain_font->Size() * 0.7);
		rootLayout->SetInsets(inset, inset, inset, inset);
		rootLayout->SetSpacing(inset);
		layout->SetSpacing(inset, inset);

		BStringView* label = new(std::nothrow) BStringView("keyringLabel",
			"Keyring:");
		if (label == NULL)
			return;

		int32 row = 0;
		layout->AddView(label, 0, row);

		fKeyringName = new(std::nothrow) BStringView("keyringName", "");
		if (fKeyringName == NULL)
			return;

		layout->AddView(fKeyringName, 1, row++);

		fPassword = new(std::nothrow) BTextControl("Password:", "", NULL);
		if (fPassword == NULL)
			return;

		BLayoutItem* layoutItem = fPassword->CreateTextViewLayoutItem();
		layoutItem->SetExplicitMinSize(BSize(fPassword->StringWidth(
				"0123456789012345678901234567890123456789") + inset,
			B_SIZE_UNSET));

		layout->AddItem(fPassword->CreateLabelLayoutItem(), 0, row);
		layout->AddItem(layoutItem, 1, row++);

		BGroupView* buttons = new(std::nothrow) BGroupView(B_HORIZONTAL);
		if (buttons == NULL)
			return;

		fCancelButton = new(std::nothrow) BButton("Cancel",
			new BMessage(kMessageCancel));
		buttons->GroupLayout()->AddView(fCancelButton);

		buttons->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

		fUnlockButton = new(std::nothrow) BButton("Unlock",
			new BMessage(kMessageUnlock));
		buttons->GroupLayout()->AddView(fUnlockButton);

		BTextView* message = new(std::nothrow) BTextView("message");
		message->SetText("An application wants to access the keyring below, "
			"but it is locked with a passphrase. Please enter the passphrase "
			"to unlock the keyring.\n"
			"If you unlock the keyring, it stays unlocked until the system is "
			"shut down or the keyring is manually locked again.\n"
			"If you cancel this dialog the keyring will remain locked.");
		message->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
		message->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
		message->MakeEditable(false);
		message->MakeSelectable(false);
		message->SetWordWrap(true);

		rootLayout->AddView(message);
		rootLayout->AddView(controls);
		rootLayout->AddView(buttons);
	}

	virtual void
	AttachedToWindow()
	{
		fCancelButton->SetTarget(Window());
		fUnlockButton->SetTarget(Window());
		fUnlockButton->MakeDefault(true);
		fPassword->MakeFocus();
	}

	void
	SetUp(const BString& keyringName)
	{
		fKeyringName->SetText(keyringName);
	}

	status_t
	Complete(BMessage& keyMessage)
	{
		BPasswordKey password(fPassword->Text(), B_KEY_PURPOSE_KEYRING, "");
		return password.Flatten(keyMessage);
	}

private:
	BStringView* fKeyringName;
	BTextControl* fPassword;
	BButton* fCancelButton;
	BButton* fUnlockButton;
};


KeyRequestWindow::KeyRequestWindow()
	:
	BWindow(BRect(50, 50, 269, 302), "Unlock Keyring",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fRequestView(NULL),
	fDoneSem(-1),
	fResult(B_ERROR)
{
	fDoneSem = create_sem(0, "keyring unlock dialog");
	if (fDoneSem < 0)
		return;

	BLayout* layout = new(std::nothrow) BGroupLayout(B_HORIZONTAL);
	if (layout == NULL)
		return;

	SetLayout(layout);

	fRequestView = new(std::nothrow) KeyRequestView();
	if (fRequestView == NULL)
		return;

	layout->AddView(fRequestView);
}


KeyRequestWindow::~KeyRequestWindow()
{
	if (fDoneSem >= 0)
		delete_sem(fDoneSem);
}


void
KeyRequestWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	int8 key;
	if (message->what == B_KEY_DOWN
		&& message->FindInt8("byte", 0, &key) == B_OK
		&& key == B_ESCAPE) {
		PostMessage(kMessageCancel);
	}

	BWindow::DispatchMessage(message, handler);
}


void
KeyRequestWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageCancel:
		case kMessageUnlock:
			fResult = message->what == kMessageUnlock ? B_OK : B_CANCELED;
			release_sem(fDoneSem);
			return;
	}

	BWindow::MessageReceived(message);
}


status_t
KeyRequestWindow::RequestKey(const BString& keyringName, BMessage& keyMessage)
{
	fRequestView->SetUp(keyringName);

	CenterOnScreen();
	Show();

	while (acquire_sem(fDoneSem) == B_INTERRUPTED)
		;

	status_t result = fResult;
	if (result == B_OK)
		result = fRequestView->Complete(keyMessage);

	LockLooper();
	Quit();
	return result;
}
