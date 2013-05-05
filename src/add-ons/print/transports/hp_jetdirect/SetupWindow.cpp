

#include "SetupWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Button.h>
#include <Directory.h>
#include <NetEndpoint.h>
#include <Rect.h>
#include <String.h>
#include <TextControl.h>
#include <View.h>

#define	DLG_WIDTH		370
#define DLG_HEIGHT		100

#define BUTTON_WIDTH	70
#define BUTTON_HEIGHT	20

#define SERVER_H		10
#define SERVER_V		10
#define SERVER_WIDTH	(DLG_WIDTH - SERVER_H - SERVER_H)
#define SERVER_HEIGHT	20
#define SERVER_TEXT		"Printer address"

#define QUEUE_H			10
#define QUEUE_V			SERVER_V + SERVER_HEIGHT + 2
#define QUEUE_WIDTH		(DLG_WIDTH - QUEUE_H - QUEUE_H)
#define QUEUE_HEIGHT	20
#define QUEUE_TEXT		"Port"

#define OK_H			(DLG_WIDTH  - BUTTON_WIDTH  - 11)
#define OK_V			(DLG_HEIGHT - BUTTON_HEIGHT - 11)
#define OK_TEXT			"OK"

#define CANCEL_H		(OK_H - BUTTON_WIDTH - 12)
#define CANCEL_V		OK_V
#define CANCEL_TEXT		"Cancel"

const BRect SERVER_RECT(
	SERVER_H,
	SERVER_V,
	SERVER_H + SERVER_WIDTH,
	SERVER_V + SERVER_HEIGHT);

const BRect QUEUE_RECT(
	QUEUE_H,
	QUEUE_V,
	QUEUE_H + QUEUE_WIDTH,
	QUEUE_V + QUEUE_HEIGHT);

const BRect OK_RECT(
	OK_H,
	OK_V,
	OK_H + BUTTON_WIDTH,
	OK_V + BUTTON_HEIGHT);

const BRect CANCEL_RECT(
	CANCEL_H,
	CANCEL_V,
	CANCEL_H + BUTTON_WIDTH,
	CANCEL_V + BUTTON_HEIGHT);

enum MSGS {
	M_CANCEL = 1,
	M_OK
};


class SetupView : public BView {
public:
							SetupView(BRect, BDirectory* );
	virtual void 			AttachedToWindow();

		bool 				CheckSetup();
private:

		BTextControl*		fServerAddress;
		BTextControl*		fQueuePort;
		BDirectory*			fPrinterDirectory;
};


SetupView::SetupView(BRect frame, BDirectory* directory)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW),
	fPrinterDirectory(directory)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
SetupView::AttachedToWindow()
{
	float width = MAX(StringWidth(SERVER_TEXT), StringWidth(QUEUE_TEXT)) + 10;

	/* server name box */

	fServerAddress = new BTextControl(SERVER_RECT, "", SERVER_TEXT, "<printer's hostname or address>", NULL);
	AddChild(fServerAddress);
	fServerAddress->SetDivider(width);

	/* queue name box */

	fQueuePort = new BTextControl(QUEUE_RECT, "", QUEUE_TEXT, "9100", NULL);	// 9100 is default HP JetDirect port number
	AddChild(fQueuePort);
	fQueuePort->SetDivider(width);

	/* cancel */

	BButton* button = new BButton(CANCEL_RECT, "", CANCEL_TEXT, new BMessage(M_CANCEL));
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(M_OK));
	AddChild(button);
	button->MakeDefault(true);
}


bool
SetupView::CheckSetup()
{
	if (*fServerAddress->Text() && *fQueuePort->Text()) {
		BNetEndpoint* ep = new BNetEndpoint(SOCK_STREAM);
		if (ep->InitCheck() == B_NO_ERROR) {
			uint16 port = atoi(fQueuePort->Text());

			if (! port)
				port = 9100;

			if (ep->Connect(fServerAddress->Text(), port) != B_OK) {
				BString text;
				text << "Failed to connect to " << fServerAddress->Text() << ":" << (int) port << "!";
				BAlert* alert = new BAlert("", text.String(), "OK");
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
				return false;
			};

			char str[256];
			sprintf(str, "%s:%d", fServerAddress->Text(), port);
			fPrinterDirectory->WriteAttr("transport_address", B_STRING_TYPE,
				0, str, strlen(str) + 1);
			return true;
		};
	};

	BAlert* alert = new BAlert("", "Please input parameters.", "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
	return false;
}


// #pragma mark -


SetupWindow::SetupWindow(BDirectory* printerDirectory)
	: BWindow(BRect(100, 100, 100 + DLG_WIDTH, 100 + DLG_HEIGHT),
		"HP JetDirect Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
			| B_CLOSE_ON_ESCAPE)
{
	fResult = 0;

	Lock();
	SetupView* view = new SetupView(Bounds(), printerDirectory);
	AddChild(view);
	Unlock();

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
			Lock();
			success = ((SetupView*)ChildAt(0))->CheckSetup();
			Unlock();
			if (success) {
				fResult = B_NO_ERROR;
				release_sem(fExitSem);
			}
			break;

		case M_CANCEL:
			fResult = B_ERROR;
			release_sem(fExitSem);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


int
SetupWindow::Go()
{
	Show();
	acquire_sem(fExitSem);
	delete_sem(fExitSem);
	int value = fResult;
	Lock();
	Quit();
	return value;
}


