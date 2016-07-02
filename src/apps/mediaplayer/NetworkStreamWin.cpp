/*
 * Copyright 2016 Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */


#include "NetworkStreamWin.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>

#include "MainApp.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-NetworkStream"


enum  {
	M_OPEN_URL = 0,
	M_CANCEL
};


NetworkStreamWin::NetworkStreamWin(BMessenger target)
	:
	BWindow(BRect(0, 0, 300, 100), "Open Network Stream",
		B_MODAL_WINDOW, B_NOT_RESIZABLE),
	fTarget(target)
{
	fTextControl = new BTextControl("InputControl",
		"Insert URL", NULL, NULL);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0, 0, 0, 0)
		.Add(fTextControl)
			.AddGroup(B_HORIZONTAL)
				.Add(new BButton("Ok", new BMessage(M_OPEN_URL)))
				.Add(new BButton("Cancel", new BMessage(M_CANCEL)))
			.End()
		.End();

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
			BUrl url(fTextControl->Text());
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

		default:
			BWindow::MessageReceived(message);
	}
}
