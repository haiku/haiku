/*
 * Copyright 2003-2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Philippe Houdoin
 */

#include "PasswordAlert.h"

#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Screen.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include "StripeView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PasswordAlert"


static const int kSemTimeOut = 50000;

static const int kWindowMinWidth = 310;


PasswordAlert::PasswordAlert(const char* title, const char* text)
	:
	BWindow(BRect(0, 0, 100, 100), title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fIcon(_IconSize(), B_RGBA32),
	fTextControl(NULL),
	fSemaphore(-1)
{
	BIconUtils::GetSystemIcon("dialog-warning", &fIcon);
	BStripeView* stripeView = new BStripeView(fIcon);

	fTextControl = new BTextControl("_password_", "", NULL, new BMessage('pass'));
	fTextControl->TextView()->HideTyping(true);

	BButton* cancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage('cncl'));
	BButton* okButton = new BButton(B_TRANSLATE("OK"), new BMessage('pass'));

	BTextView* textView = new BTextView("_password_text_");
	textView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	textView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetWordWrap(true);
	textView->SetText(text);
	textView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.Add(stripeView)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.SetInsets(B_USE_HALF_ITEM_INSETS)
			.Add(textView)
			.Add(fTextControl)
			.AddGroup(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
				.AddGlue()
				.Add(cancelButton)
				.Add(okButton);

	fTextControl->MakeFocus();
	SetDefaultButton(okButton);

	float fontFactor = be_plain_font->Size() / 11.0f;
	GetLayout()->SetExplicitMinSize(BSize(kWindowMinWidth * fontFactor, B_SIZE_UNSET));

	ResizeToPreferred();

	// Return early if we've already been moved...
	if (Frame().left != 0 && Frame().right != 0)
		return;

	// otherwise center ourselves on-top of parent window/screen
	BWindow* parent = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	const BRect frame = parent != NULL ? parent->Frame() : BScreen(this).Frame();

	CenterIn(frame);
}


status_t
PasswordAlert::Go(BString& password)
{
	fSemaphore = create_sem(0, "PasswordAlertSem");
	if (fSemaphore < B_OK) {
		Quit();
		return B_NO_MEMORY;
	}

	fStatus = B_CANCEL;

	// Get the originating window, if it exists
	BWindow* window
		= dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	Show();

	// Heavily modified from TextEntryAlert code; the original didn't let the
	// blocked window ever draw.
	if (window != NULL) {
		status_t status;
		for (;;) {
			do {
				status = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (status == B_INTERRUPTED);

			if (status == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fSemaphore) == B_INTERRUPTED) {
			;
		}
	}

	if (fStatus == B_OK)
		password = fTextControl->Text();

	if (Lock())
		Quit();

	return fStatus;
}


void
PasswordAlert::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'cncl':
		case 'pass':
		{
			fStatus = msg->what == 'cncl' ? B_CANCEL : B_OK;
			delete_sem(fSemaphore);
			fSemaphore = -1;
			break;
		}
		default:
			return BWindow::MessageReceived(msg);
	}
}


BRect
PasswordAlert::_IconSize()
{
	return BRect(BPoint(0, 0), be_control_look->ComposeIconSize(32));
}
