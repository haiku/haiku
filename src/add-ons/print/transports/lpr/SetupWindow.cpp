/*
 * Copyright 2026, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Philippe Houdoin
 */

#include "SetupWindow.h"

#include <algorithm>
#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>

#include "DbgMsg.h"
#include "LprDefs.h"
#include "LpsClient.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LPRTransport::SetupWindow"

const char* kOKLabel = B_TRANSLATE("OK");
const char* kCancelLabel = B_TRANSLATE("Cancel");

enum MSGS {
	M_CANCEL = 1,
	M_OK
};


SetupWindow::SetupWindow(BDirectory* printerDirectory)
	:
	BWindow(BRect(0, 0, 400, 100), B_TRANSLATE("LPR Setup"), B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE |  B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fResult(B_ERROR),
	fPrinterDirectory(printerDirectory)
{
	fServerAddress = new BTextControl("", B_TRANSLATE("Server address:"),
		B_TRANSLATE("<LPD server's hostname or address>"), NULL);
	fServerAddress->SetToolTip(
		B_TRANSLATE_COMMENT("Enter the LPD server hostname or address", "tooltip"));

	fQueueName = new BTextControl("", B_TRANSLATE("Queue name:"), "LPT1_PASSTHRU", NULL);

	BButton* cancelButton = new BButton("", kCancelLabel, new BMessage(M_CANCEL));
	BButton* okButton = new BButton("", kOKLabel, new BMessage(M_OK));
	okButton->MakeDefault(true);

	BLayoutBuilder::Grid<>(this, B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fServerAddress->CreateLabelLayoutItem(), 0, 0)
		.Add(fServerAddress->CreateTextViewLayoutItem(), 1, 0, 2)
		.Add(fQueueName->CreateLabelLayoutItem(), 0, 1)
		.Add(fQueueName->CreateTextViewLayoutItem(), 1, 1, 2)
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
	if (*fServerAddress->Text() && *fQueueName->Text()) {
		// check this address + port endpoint is reachable
		try {
			LpsClient lpr(fServerAddress->Text());
			lpr.connect();
		} catch (LPSException& err) {
			BString text = B_TRANSLATE("Failed to connect to %address% %queue%: %error% !");
			text.ReplaceAll("%address%", fServerAddress->Text());
			text.ReplaceAll("%queue%", fQueueName->Text());
			text.ReplaceAll("%error%", err.what());

			BAlert* alert = new BAlert("", text.String(), kOKLabel);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}

		fPrinterDirectory->WriteAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, fServerAddress->Text(),
			strlen(fServerAddress->Text()) + 1);
		fPrinterDirectory->WriteAttr(LPR_QUEUE_NAME, B_STRING_TYPE, 0, fQueueName->Text(),
			strlen(fQueueName->Text()) + 1);
		return true;
	}

	BAlert* alert = new BAlert("", B_TRANSLATE("Please input required parameters."), kOKLabel);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
	return false;
}
