/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */


#include "NetworkStreamWin.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <LayoutBuilder.h>

#include "MainApp.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-NetworkStream"


enum  {
	M_OPEN_URL = 'NSOP',
	M_CANCEL = 'NSCN'
};


NetworkStreamWin::NetworkStreamWin(BMessenger target)
	:
	BWindow(BRect(0, 0, 300, 100), B_TRANSLATE("Open network stream"),
		B_MODAL_WINDOW, B_NOT_RESIZABLE),
	fTarget(target)
{
	fTextControl = new BTextControl("InputControl",
		B_TRANSLATE("Stream URL:"), NULL, NULL);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_HALF_ITEM_INSETS)
		.Add(fTextControl)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(new BButton(B_TRANSLATE("OK"), new BMessage(M_OPEN_URL)))
			.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(M_CANCEL)))
		.End()
	.End();

	_LookIntoClipboardForUrl();

	CenterOnScreen();
}


NetworkStreamWin::~NetworkStreamWin()
{
}


void
NetworkStreamWin::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case M_OPEN_URL:
		{
			BUrl url(fTextControl->Text(), true);
			if (!url.IsValid()) {
				BAlert* alert = new BAlert(B_TRANSLATE("Bad URL"),
					B_TRANSLATE("Invalid URL inserted!"),
						B_TRANSLATE("OK"));
					alert->Go();
				return;
			}

			BMessage archivedUrl;
			url.Archive(&archivedUrl);

			BMessage msg(M_URL_RECEIVED);
			msg.AddMessage("mediaplayer:url", &archivedUrl);
			fTarget.SendMessage(&msg);

			Quit();
			break;
		}

		case M_CANCEL:
			Quit();
			break;

		case B_KEY_DOWN:
		{
			int8 key;
			if (message->FindInt8("byte", &key) == B_OK
					&& key == B_ENTER) {
				PostMessage(M_OPEN_URL);
				break;
			}
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
NetworkStreamWin::WindowActivated(bool active)
{
	if (active)
		_LookIntoClipboardForUrl();
}


void
NetworkStreamWin::_LookIntoClipboardForUrl()
{
	// Don't do anything if we already have something
	// set to avoid annoying the user.
	if (fTextControl->TextView()->TextLength() > 0)
		return;

	// Let's see if we already have an url in the clipboard
	// in that case avoid the user to paste it.
	if (be_clipboard->Lock()) {
		char* text = NULL;
		ssize_t textLen = 0;

		BMessage* data = be_clipboard->Data();
		if (data->FindData("text/plain", B_MIME_TYPE,
				(const void **)&text, &textLen) == B_OK) {

			// NOTE: This is done because urls copied
			// from WebPositive have the mime string at
			// the end.
			// TODO: Looks like a problem in Web+, should
			// be fixed.
			text[textLen] = '\0';

			// Before to set the text let's see if it's really
			// a valid URL.
			BUrl url(text, true);
			if (url.IsValid())
				fTextControl->SetText(text);
		}

		be_clipboard->Unlock();
	}
}
