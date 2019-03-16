/*
 * Copyright 2008-10, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <private/interface/AboutWindow.h>

#include "BluetoothMain.h"
#include "BluetoothWindow.h"
#include "defs.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"

BluetoothApplication::BluetoothApplication()
	:
	BApplication(BLUETOOTH_APP_SIGNATURE)
{
}


void
BluetoothApplication::ReadyToRun()
{
	if (!be_roster->IsRunning(BLUETOOTH_SIGNATURE)) {
		BAlert* alert = new BAlert("Services not running",
			B_TRANSLATE("The Bluetooth services are not currently running "
				"on this system."),
			B_TRANSLATE("Launch now"), B_TRANSLATE("Quit"), "",
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(1, B_ESCAPE);
		int32 choice = alert->Go();

		switch (choice) {
			case 0:
			{
				status_t error;
				error = be_roster->Launch(BLUETOOTH_SIGNATURE);
				printf("kMsgStartServices: %s\n", strerror(error));
				// TODO: This is temporal
				// BMessage handcheck: use the version of Launch()
				// that includes a BMessage	in that message include
				// a BMessenger to yourself and the BT server could
				// use that messenger to send back a reply indicating
				// when it's ready and you could just create window
				BMessageRunner::StartSending(be_app_messenger,
					new BMessage('Xtmp'), 2 * 1000000, 1);
				break;
			}
			case 1:
				PostMessage(B_QUIT_REQUESTED);
				break;
		}

		return;
	}

	PostMessage(new BMessage('Xtmp'));
}


void
BluetoothApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddToRemoteList:
			fWindow->PostMessage(message);
			break;

		case 'Xtmp':
			if (!be_roster->IsRunning(BLUETOOTH_SIGNATURE)) {
				// Give another chance
				BMessageRunner::StartSending(be_app_messenger,
					new BMessage('Xtmp'), 2 * 1000000, 1);
			} else {
				fWindow = new BluetoothWindow(BRect(100, 100, 750, 420));
				fWindow->Show();
			}
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
BluetoothApplication::AboutRequested()
{
	BAboutWindow* about = new BAboutWindow("Bluetooth", BLUETOOTH_APP_SIGNATURE);
	about->AddCopyright(2010, "Oliver Ruiz Dorantes");
	about->AddText(B_TRANSLATE(
		"With support of:\n"
		" - Mika Lindqvist\n"
		" - Adrien Destugues\n"
		" - Maksym Yevmenkin\n\n"
		"Thanks to the individuals who helped" B_UTF8_ELLIPSIS "\n\n"
		"Shipping/donating hardware:\n"
		" - Henry Jair Abril Florez (el Colombian)\n"
		"	& Stefanie Bartolich\n"
		" - Edwin Erik Amsler\n"
		" - Dennis d'Entremont\n"
		" - Luroh\n"
		" - Pieter Panman\n\n"
		"Economically:\n"
		" - Karl vom Dorff, Andrea Bernardi (OSDrawer),\n"
		" - Matt M, Doug F, Hubert H,\n"
		" - Sebastian B, Andrew M, Jared E,\n"
		" - Frederik H, Tom S, Ferry B,\n"
		" - Greg G, David F, Richard S, Martin W:\n\n"
		"With patches:\n"
		" - Michael Weirauch\n"
		" - Fredrik Ekdahl\n"
		" - Raynald Lesieur\n"
		" - Andreas Färber\n"
		" - Joerg Meyer\n"
		"Testing:\n"
		" - Petter H. Juliussen\n"
		"Who gave me all the knowledge:\n"
		" - the yellowTAB team"));
	about->Show();
}


int
main(int, char**)
{
	BluetoothApplication app;
	app.Run();

	return 0;
}
