/*
 * Copyright 2001-2026, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Philippe Houdoin
 */

#include "SetupWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <Socket.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HPJetDirectTransport::SetupWindow"

const char* kOKLabel = B_TRANSLATE("OK");
const char* kCancelLabel = B_TRANSLATE("Cancel");

enum MSGS {
	M_CANCEL = 1,
	M_OK
};


SetupWindow::SetupWindow(BDirectory* printerDirectory)
	:
	BWindow(BRect(0, 0, 400, 100), B_TRANSLATE("HP JetDirect Setup"), B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE |  B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fResult(B_ERROR),
	fPrinterDirectory(printerDirectory)
{
	fServerAddress = new BTextControl("", B_TRANSLATE("Printer address:"),
		B_TRANSLATE("<printer's hostname or address>"), NULL);
	fServerAddress->SetToolTip(
		B_TRANSLATE_COMMENT("Enter the printer's hostname or address", "tooltip"));

	fQueuePort = new BTextControl("", B_TRANSLATE("Port:"), "9100", NULL);
	fQueuePort->TextView()->SetMaxBytes(5);
	fQueuePort->SetExplicitSize(fQueuePort->MinSize());

	for (uint32 c = 0; c < 256; c++) {
		if (c < '0' || c > '9')
			fQueuePort->TextView()->DisallowChar(c);
	}

	BButton* cancelButton = new BButton("", kCancelLabel, new BMessage(M_CANCEL));
	BButton* okButton = new BButton("", kOKLabel, new BMessage(M_OK));
	okButton->MakeDefault(true);

	BLayoutBuilder::Grid<>(this, B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fServerAddress->CreateLabelLayoutItem(), 0, 0)
		.Add(fServerAddress->CreateTextViewLayoutItem(), 1, 0, 2)
		.Add(fQueuePort->CreateLabelLayoutItem(), 0, 1)
		.Add(fQueuePort->CreateTextViewLayoutItem(), 1, 1)
		.Add(cancelButton, 1, 3)
		.Add(okButton, 2, 3);

	CenterOnScreen();

	fExitSem = create_sem(0, "SetupWindowSem");
}


bool
SetupWindow::QuitRequested()
{
	fResult = B_ERROR;
	release_sem(fExitSem);
	return true;
}


void
SetupWindow::MessageReceived(BMessage* msg)
{
	bool success;

	switch (msg->what) {
		case M_OK:
			success = _CheckSetup();
			if (success) {
				fResult = B_NO_ERROR;
				delete_sem(fExitSem);
			}
			break;

		case M_CANCEL:
			fResult = B_ERROR;
			delete_sem(fExitSem);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


status_t
SetupWindow::Go()
{
	fServerAddress->MakeFocus(true);
	fServerAddress->TextView()->SelectAll();

	Show();
	acquire_sem(fExitSem);
	status_t status = fResult;
	if (Lock())
		Quit();
	return status;
}


// #pragma mark -


bool
SetupWindow::_CheckSetup()
{
	if (*fServerAddress->Text() && *fQueuePort->Text()) {
		// check this address + port endpoint is reachable
		uint16 port = atoi(fQueuePort->Text());

		if (!port)
			port = 9100;

		BNetworkAddress endpointAddress(fServerAddress->Text(), port);
		BSocket* socket = new(std::nothrow) BSocket();
		if (socket != NULL) {
			// 1s should be far enough
			status_t status = socket->Connect(endpointAddress, 1000000LL);
			delete socket;

			if (status == B_OK) {
				char str[256];
				sprintf(str, "%s:%d", fServerAddress->Text(), port);
				fPrinterDirectory->WriteAttr("transport_address", B_STRING_TYPE, 0, str,
					strlen(str) + 1);
				return true;
			}

			BString portNumber;
			portNumber << (int)port;

			BString text = B_TRANSLATE("Failed to connect to %address%:%port% !");
			text.ReplaceAll("%address%", fServerAddress->Text());
			text.ReplaceAll("%port%", portNumber.String());

			BAlert* alert = new BAlert("", text.String(), kOKLabel);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}
	}

	BAlert* alert = new BAlert("", B_TRANSLATE("Please input required parameters."), kOKLabel);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
	return false;
}
