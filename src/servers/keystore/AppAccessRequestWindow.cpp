/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AppAccessRequestWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NetworkDevice.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>
#include <View.h>

#include <new>


static const uint32 kMessageDisallow = 'btda';
static const uint32 kMessageOnce = 'btao';
static const uint32 kMessageAlways = 'btaa';


class AppAccessRequestView : public BView {
public:
	AppAccessRequestView(const char* keyringName, const char* signature,
		const char* path, const char* accessString, bool appIsNew,
		bool appWasUpdated)
		:
		BView("AppAccessRequestView", B_WILL_DRAW)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BGroupLayout* rootLayout = new(std::nothrow) BGroupLayout(B_VERTICAL);
		if (rootLayout == NULL)
			return;

		SetLayout(rootLayout);

		float inset = ceilf(be_plain_font->Size() * 0.7);
		rootLayout->SetInsets(inset, inset, inset, inset);
		rootLayout->SetSpacing(inset);

		BTextView* message = new(std::nothrow) BTextView("Message");
		if (message == NULL)
			return;

		BString details;
		details << "The application:\n"
			<< signature << " (" << path << ")\n\n";

		if (keyringName != NULL) {
			details << "requests access to keyring:\n"
				<< keyringName << "\n\n";
		}

		if (accessString != NULL) {
			details << "to perform the following action:\n"
				<< accessString << "\n\n";
		}

		if (appIsNew)
			details << "This application hasn't been granted access before.";
		else if (appWasUpdated) {
			details << "This application has been updated since it was last"
				<< " granted access.";
		} else {
			details << "This application doesn't yet have the required"
				" privileges.";
		}

		message->SetText(details);
		message->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
		message->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
		message->MakeEditable(false);
		message->MakeSelectable(false);
		message->SetWordWrap(true);

		message->SetExplicitMinSize(BSize(message->StringWidth(
				"01234567890123456789012345678901234567890123456789") + inset,
			B_SIZE_UNSET));

		BGroupView* buttons = new(std::nothrow) BGroupView(B_HORIZONTAL);
		if (buttons == NULL)
			return;

		fDisallowButton = new(std::nothrow) BButton("Disallow",
			new BMessage(kMessageDisallow));
		buttons->GroupLayout()->AddView(fDisallowButton);

		buttons->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

		fOnceButton = new(std::nothrow) BButton("Allow Once",
			new BMessage(kMessageOnce));
		buttons->GroupLayout()->AddView(fOnceButton);

		fAlwaysButton = new(std::nothrow) BButton("Allow Always",
			new BMessage(kMessageAlways));
		buttons->GroupLayout()->AddView(fAlwaysButton);

		rootLayout->AddView(message);
		rootLayout->AddView(buttons);
	}

	virtual void
	AttachedToWindow()
	{
		fDisallowButton->SetTarget(Window());
		fOnceButton->SetTarget(Window());
		fAlwaysButton->SetTarget(Window());

		// TODO: Decide for a sane default button (or none at all).
		//fButton->MakeDefault(true);
	}

private:
	BButton* fDisallowButton;
	BButton* fOnceButton;
	BButton* fAlwaysButton;
};


AppAccessRequestWindow::AppAccessRequestWindow(const char* keyringName,
	const char* signature, const char* path, const char* accessString,
	bool appIsNew, bool appWasUpdated)
	:
	BWindow(BRect(50, 50, 269, 302), "Application Keyring Access",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fRequestView(NULL),
	fDoneSem(-1),
	fResult(kMessageDisallow)
{
	fDoneSem = create_sem(0, "application keyring access dialog");
	if (fDoneSem < 0)
		return;

	BLayout* layout = new(std::nothrow) BGroupLayout(B_HORIZONTAL);
	if (layout == NULL)
		return;

	SetLayout(layout);

	fRequestView = new(std::nothrow) AppAccessRequestView(keyringName,
		signature, path, accessString, appIsNew, appWasUpdated);
	if (fRequestView == NULL)
		return;

	layout->AddView(fRequestView);
}


AppAccessRequestWindow::~AppAccessRequestWindow()
{
	if (fDoneSem >= 0)
		delete_sem(fDoneSem);
}


void
AppAccessRequestWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	int8 key;
	if (message->what == B_KEY_DOWN
		&& message->FindInt8("byte", 0, &key) == B_OK
		&& key == B_ESCAPE) {
		PostMessage(kMessageDisallow);
	}

	BWindow::DispatchMessage(message, handler);
}


void
AppAccessRequestWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageDisallow:
		case kMessageOnce:
		case kMessageAlways:
			fResult = message->what;
			release_sem(fDoneSem);
			return;
	}

	BWindow::MessageReceived(message);
}


status_t
AppAccessRequestWindow::RequestAppAccess(bool& allowAlways)
{
	CenterOnScreen();
	Show();

	while (acquire_sem(fDoneSem) == B_INTERRUPTED)
		;

	status_t result;
	switch (fResult) {
		default:
		case kMessageDisallow:
			result = B_NOT_ALLOWED;
			allowAlways = false;
			break;
		case kMessageOnce:
			result = B_OK;
			allowAlways = false;
			break;
		case kMessageAlways:
			result = B_OK;
			allowAlways = true;
			break;
	}

	LockLooper();
	Quit();
	return result;
}
