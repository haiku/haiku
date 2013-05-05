// Sun, 18 Jun 2000
// Y.Takagi

#include <Button.h>
#include <Rect.h>
#include <TextControl.h>
#include <View.h>
#include <Directory.h>
#include <Alert.h>

#include <algorithm>
#include <string.h>

#include "LpsClient.h"
#include "LprSetupDlg.h"
#include "LprDefs.h"
#include "DbgMsg.h"


#define	DLG_WIDTH		370
#define DLG_HEIGHT		100

#define BUTTON_WIDTH	70
#define BUTTON_HEIGHT	20

#define SERVER_H		10
#define SERVER_V		10
#define SERVER_WIDTH	(DLG_WIDTH - SERVER_H - SERVER_H)
#define SERVER_HEIGHT	20
#define SERVER_TEXT		"Server name"

#define QUEUE_H			10
#define QUEUE_V			SERVER_V + SERVER_HEIGHT + 2
#define QUEUE_WIDTH		(DLG_WIDTH - QUEUE_H - QUEUE_H)
#define QUEUE_HEIGHT	20
#define QUEUE_TEXT		"Queue name"

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


class LprSetupView : public BView {
public:
					LprSetupView(BRect, BDirectory *);
					~LprSetupView() {}
	virtual void	AttachedToWindow();
			bool	UpdateViewData();

private:
	BTextControl*	fServer;
	BTextControl*	fQueue;
	BDirectory*		fDir;
};


LprSetupView::LprSetupView(BRect frame, BDirectory *d)
	:
	BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), fDir(d)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
LprSetupView::AttachedToWindow()
{
	float width = max(StringWidth(SERVER_TEXT), StringWidth(QUEUE_TEXT)) + 10;

	/* server name box */

    // TODO remember previous value
	fServer = new BTextControl(SERVER_RECT, "", SERVER_TEXT, "192.168.0.0",
		NULL);
	AddChild(fServer);
	fServer->SetDivider(width);

	/* queue name box */

    // TODO remember previous value
	fQueue = new BTextControl(QUEUE_RECT, "", QUEUE_TEXT, "LPT1_PASSTHRU",
		NULL);
	AddChild(fQueue);
	fQueue->SetDivider(width);

	/* cancel */

	BButton *button = new BButton(CANCEL_RECT, "", CANCEL_TEXT,
		new BMessage(M_CANCEL));
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(M_OK));
	AddChild(button);
	button->MakeDefault(true);
}


bool
LprSetupView::UpdateViewData()
{
	if (*fServer->Text() && *fQueue->Text()) {

		try {
			LpsClient lpr(fServer->Text());
			lpr.connect();
		}

		catch (LPSException &err) {
			BAlert *alert = new BAlert("", err.what(), "OK");
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return false;
		}

		fDir->WriteAttr(LPR_SERVER_NAME, B_STRING_TYPE, 0, fServer->Text(),
			strlen(fServer->Text()) + 1);
		fDir->WriteAttr(LPR_QUEUE_NAME,  B_STRING_TYPE, 0, fQueue->Text(),
			strlen(fQueue->Text())  + 1);
		return true;
	}

	BAlert *alert = new BAlert("", "Please enter server address and printer"
		"queue name.", "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
	return false;
}


LprSetupDlg::LprSetupDlg(BDirectory *dir)
	:
	DialogWindow(BRect(100, 100, 100 + DLG_WIDTH, 100 + DLG_HEIGHT),
		"LPR Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
		| B_CLOSE_ON_ESCAPE)
{
	fSetupView = new LprSetupView(Bounds(), dir);
	AddChild(fSetupView);
}


void
LprSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_OK:
			if (fSetupView->UpdateViewData()) {
				SetResult(B_OK);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;

		case M_CANCEL:
			SetResult(B_ERROR);
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			DialogWindow::MessageReceived(msg);
	}
}
